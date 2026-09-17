#include "NRF24L01.h"
#include "spi.h"
#include "board_config.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_utils.h"
#include <stdio.h>

#define NRF24_CMD_R_REGISTER            (0x00U)
#define NRF24_CMD_W_REGISTER            (0x20U)
#define NRF24_CMD_R_RX_PAYLOAD          (0x61U)
#define NRF24_CMD_FLUSH_RX              (0xE2U)
#define NRF24_CMD_NOP                   (0xFFU)

#define NRF24_REG_CONFIG                (0x00U)
#define NRF24_REG_EN_AA                 (0x01U)
#define NRF24_REG_EN_RXADDR             (0x02U)
#define NRF24_REG_SETUP_AW              (0x03U)
#define NRF24_REG_RF_CH                 (0x05U)
#define NRF24_REG_RF_SETUP              (0x06U)
#define NRF24_REG_STATUS                (0x07U)
#define NRF24_REG_RX_ADDR_P0            (0x0AU)
#define NRF24_REG_RX_PW_P0              (0x11U)
#define NRF24_REG_FIFO_STATUS           (0x17U)

#define NRF24_CONFIG_EN_CRC             (1U << 3)
#define NRF24_CONFIG_CRCO               (1U << 2)
#define NRF24_CONFIG_PWR_UP             (1U << 1)
#define NRF24_CONFIG_PRIM_RX            (1U << 0)

#define NRF24_STATUS_RX_DR              (1U << 6)
#define NRF24_STATUS_TX_DS              (1U << 5)
#define NRF24_STATUS_MAX_RT             (1U << 4)

#define NRF24_FIFO_STATUS_RX_EMPTY      (1U << 0)

#define NRF24_DEFAULT_CHANNEL           (76U)
#define NRF24_ADDR_WIDTH                (5U)

static const uint8_t s_default_addr[NRF24_ADDR_WIDTH] = { 'R', 'O', 'V', '0', '1' };

static inline void nrf24_csn_high(void)
{
    LL_GPIO_SetOutputPin(GPIOA, (1U << BOARD_NRF24_CSN_PIN));
}

static inline void nrf24_csn_low(void)
{
    LL_GPIO_ResetOutputPin(GPIOA, (1U << BOARD_NRF24_CSN_PIN));
}

static inline void nrf24_ce_high(void)
{
    LL_GPIO_SetOutputPin(GPIOA, (1U << BOARD_NRF24_CE_PIN));
}

static inline void nrf24_ce_low(void)
{
    LL_GPIO_ResetOutputPin(GPIOA, (1U << BOARD_NRF24_CE_PIN));
}

static uint8_t nrf24_read_reg(uint8_t reg)
{
    nrf24_csn_low();
    spi_bus_transfer_byte(NRF24_CMD_R_REGISTER | (reg & 0x1FU));
    uint8_t val = spi_bus_transfer_byte(NRF24_CMD_NOP);
    nrf24_csn_high();
    return val;
}

static void nrf24_write_reg(uint8_t reg, uint8_t val)
{
    nrf24_csn_low();
    spi_bus_transfer_byte(NRF24_CMD_W_REGISTER | (reg & 0x1FU));
    spi_bus_transfer_byte(val);
    nrf24_csn_high();
}

static void nrf24_read_reg_buf(uint8_t reg, uint8_t *buf, uint8_t len)
{
    nrf24_csn_low();
    spi_bus_transfer_byte(NRF24_CMD_R_REGISTER | (reg & 0x1FU));
    spi_bus_transfer(NULL, buf, len);
    nrf24_csn_high();
}

static void nrf24_write_reg_buf(uint8_t reg, const uint8_t *buf, uint8_t len)
{
    nrf24_csn_low();
    spi_bus_transfer_byte(NRF24_CMD_W_REGISTER | (reg & 0x1FU));
    spi_bus_write(buf, len);
    nrf24_csn_high();
}

static void nrf24_send_cmd(uint8_t cmd)
{
    nrf24_csn_low();
    spi_bus_transfer_byte(cmd);
    nrf24_csn_high();
}

nrf24_status_t nrf24_init_rx(void)
{
    // Enable GPIOA clock for control pins (PA8=IRQ, PA9=CE, PA10=CSN)
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

    // Configure PA9 (CE) and PA10 (CSN) as push-pull outputs
    LL_GPIO_InitTypeDef gpio_init;
    LL_GPIO_StructInit(&gpio_init);
    gpio_init.Pin        = (1U << BOARD_NRF24_CE_PIN) | (1U << BOARD_NRF24_CSN_PIN);
    gpio_init.Mode       = LL_GPIO_MODE_OUTPUT;
    gpio_init.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &gpio_init);

    // Configure PA8 (IRQ) as input with pull-up (ready for EXTI or polling)
    gpio_init.Pin        = (1U << BOARD_NRF24_IRQ_PIN);
    gpio_init.Mode       = LL_GPIO_MODE_INPUT;
    gpio_init.Pull       = LL_GPIO_PULL_UP;
    LL_GPIO_Init(GPIOA, &gpio_init);

    // Initial pin states: CE low (standby), CSN high (SPI unselected)
    nrf24_ce_low();
    nrf24_csn_high();

    // Initialize underlying SPI1 master bus
    if (spi_bus_init() != SPI_OK) {
        return NRF24_ERR_NOT_FOUND;
    }

    // Allow the radio module to complete its power-on reset.
    LL_mDelay(100);

    // Verify radio presence by writing and reading back address width register
    nrf24_write_reg(NRF24_REG_SETUP_AW, 0x03U);
    if (nrf24_read_reg(NRF24_REG_SETUP_AW) != 0x03U) {
        return NRF24_ERR_NOT_FOUND;
    }

    // Set default RF channel (2476 MHz)
    nrf24_set_channel(NRF24_DEFAULT_CHANNEL);

    // Set maximum RF output power (0 dBm for auto-ACK response) and 1 Mbps air rate
    nrf24_set_pa_level(NRF24_PA_MAX);
    nrf24_set_data_rate(NRF24_RATE_1MBPS);

    // Enable auto-acknowledgement on pipe 0
    nrf24_write_reg(NRF24_REG_EN_AA, 0x01U);

    // Enable RX address on pipe 0
    nrf24_write_reg(NRF24_REG_EN_RXADDR, 0x01U);

    // Set fixed payload size on pipe 0
    nrf24_write_reg(NRF24_REG_RX_PW_P0, (uint8_t)sizeof(rover_packet_t));

    // Set default receive address on pipe 0
    nrf24_set_rx_address(s_default_addr);

    // Clear all pending interrupt flags in status register
    nrf24_write_reg(NRF24_REG_STATUS, NRF24_STATUS_RX_DR | NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT);

    // Flush RX FIFO to start in a clean state
    nrf24_flush_rx();

    // Power up directly in RX mode with 2-byte CRC enabled
    nrf24_write_reg(NRF24_REG_CONFIG,
                    NRF24_CONFIG_EN_CRC | NRF24_CONFIG_CRCO |
                    NRF24_CONFIG_PWR_UP | NRF24_CONFIG_PRIM_RX);

    // Allow oscillator to stabilize in Standby-I mode before enabling receiver
    LL_mDelay(2);

    // Pull CE high to enter RX listening mode
    nrf24_ce_high();
    LL_mDelay(1);

    return NRF24_OK;
}

bool nrf24_is_connected(void)
{
    nrf24_write_reg(NRF24_REG_SETUP_AW, 0x03U);
    return (nrf24_read_reg(NRF24_REG_SETUP_AW) == 0x03U);
}

void nrf24_set_rx_address(const uint8_t *addr)
{
    if (addr == NULL) {
        return;
    }

    nrf24_write_reg_buf(NRF24_REG_RX_ADDR_P0, addr, NRF24_ADDR_WIDTH);
}

void nrf24_set_channel(uint8_t channel)
{
    if (channel > 125U) {
        channel = 125U;
    }
    nrf24_write_reg(NRF24_REG_RF_CH, channel);
}

void nrf24_set_pa_level(nrf24_pa_level_t level)
{
    uint8_t setup = nrf24_read_reg(NRF24_REG_RF_SETUP) & 0xF9U;

    switch (level) {
        case NRF24_PA_MIN:
            setup |= (0x00U << 1);
            break;
        case NRF24_PA_LOW:
            setup |= (0x01U << 1);
            break;
        case NRF24_PA_HIGH:
            setup |= (0x02U << 1);
            break;
        case NRF24_PA_MAX:
        default:
            setup |= (0x03U << 1);
            break;
    }

    nrf24_write_reg(NRF24_REG_RF_SETUP, setup);
}

void nrf24_set_data_rate(nrf24_data_rate_t rate)
{
    uint8_t setup = nrf24_read_reg(NRF24_REG_RF_SETUP) & ~((1U << 5) | (1U << 3));

    switch (rate) {
        case NRF24_RATE_250KBPS:
            setup |= (1U << 5);
            break;
        case NRF24_RATE_2MBPS:
            setup |= (1U << 3);
            break;
        case NRF24_RATE_1MBPS:
        default:
            break;
    }

    nrf24_write_reg(NRF24_REG_RF_SETUP, setup);
}

bool nrf24_is_data_available(void)
{
    return (nrf24_read_reg(NRF24_REG_FIFO_STATUS) & NRF24_FIFO_STATUS_RX_EMPTY) == 0U;
}

nrf24_status_t nrf24_read_payload(void *data, uint8_t length)
{
    if (data == NULL || length == 0U || length > 32U) {
        return NRF24_ERR_PARAM;
    }

    nrf24_csn_low();
    spi_bus_transfer_byte(NRF24_CMD_R_RX_PAYLOAD);
    spi_bus_transfer(NULL, (uint8_t *)data, length);
    nrf24_csn_high();

    nrf24_clear_rx_flag();
    return NRF24_OK;
}

void nrf24_clear_rx_flag(void)
{
    nrf24_write_reg(NRF24_REG_STATUS, NRF24_STATUS_RX_DR);
}

void nrf24_flush_rx(void)
{
    nrf24_send_cmd(NRF24_CMD_FLUSH_RX);
}

void nrf24_print_details(void)
{
    uint8_t cfg         = nrf24_read_reg(NRF24_REG_CONFIG);
    uint8_t status      = nrf24_read_reg(NRF24_REG_STATUS);
    uint8_t fifo_status = nrf24_read_reg(NRF24_REG_FIFO_STATUS);
    uint8_t rf_ch       = nrf24_read_reg(NRF24_REG_RF_CH);
    uint8_t setup       = nrf24_read_reg(NRF24_REG_RF_SETUP);
    uint8_t rx_pw_p0    = nrf24_read_reg(NRF24_REG_RX_PW_P0);

    uint8_t rx_addr[NRF24_ADDR_WIDTH] = {0};
    nrf24_read_reg_buf(NRF24_REG_RX_ADDR_P0, rx_addr, NRF24_ADDR_WIDTH);

    printf("\r\n--- NRF24L01+ Receiver Configuration ---\r\n");
    printf("CONFIG:      0x%02X (PWR_UP:%u, PRIM_RX:%u, CRC:%u)\r\n",
           cfg,
           (cfg & NRF24_CONFIG_PWR_UP) ? 1U : 0U,
           (cfg & NRF24_CONFIG_PRIM_RX) ? 1U : 0U,
           (cfg & NRF24_CONFIG_EN_CRC) ? 1U : 0U);
    printf("STATUS:      0x%02X (RX_DR:%u)\r\n",
           status, (status & NRF24_STATUS_RX_DR) ? 1U : 0U);
    printf("FIFO_STATUS: 0x%02X (RX_EMPTY:%u)\r\n",
           fifo_status, (fifo_status & NRF24_FIFO_STATUS_RX_EMPTY) ? 1U : 0U);
    printf("RF_CH:       %u (Frequency: %u MHz)\r\n", rf_ch, 2400U + rf_ch);
    printf("RF_SETUP:    0x%02X\r\n", setup);
    printf("RX_PW_P0:    %u bytes\r\n", rx_pw_p0);
    printf("RX_ADDR_P0:  %c%c%c%c%c (0x%02X 0x%02X 0x%02X 0x%02X 0x%02X)\r\n",
           rx_addr[0], rx_addr[1], rx_addr[2], rx_addr[3], rx_addr[4],
           rx_addr[0], rx_addr[1], rx_addr[2], rx_addr[3], rx_addr[4]);
    printf("----------------------------------------\r\n");
}
