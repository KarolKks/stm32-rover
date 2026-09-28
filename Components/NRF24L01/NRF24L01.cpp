#include "NRF24L01.h"

namespace {
    constexpr uint8_t CmdRRegister   = 0x00U;
    constexpr uint8_t CmdWRegister   = 0x20U;
    constexpr uint8_t CmdRRxPayload  = 0x61U;
    constexpr uint8_t CmdFlushRx     = 0xE2U;
    constexpr uint8_t CmdNop         = 0xFFU;

    constexpr uint8_t RegConfig      = 0x00U;
    constexpr uint8_t RegEnAa        = 0x01U;
    constexpr uint8_t RegEnRxAddr    = 0x02U;
    constexpr uint8_t RegSetupAw     = 0x03U;
    constexpr uint8_t RegRfCh        = 0x05U;
    constexpr uint8_t RegRfSetup     = 0x06U;
    constexpr uint8_t RegStatus      = 0x07U;
    constexpr uint8_t RegRxAddrP0    = 0x0AU;
    constexpr uint8_t RegRxPwP0      = 0x11U;
    constexpr uint8_t RegFifoStatus  = 0x17U;

    constexpr uint8_t ConfigEnCrc    = (1U << 3);
    constexpr uint8_t ConfigCrco     = (1U << 2);
    constexpr uint8_t ConfigPwrUp    = (1U << 1);
    constexpr uint8_t ConfigPrimRx   = (1U << 0);

    constexpr uint8_t StatusRxDr     = (1U << 6);
    constexpr uint8_t StatusTxDs     = (1U << 5);
    constexpr uint8_t StatusMaxRt    = (1U << 4);

    constexpr uint8_t FifoStatusRxEmpty = (1U << 0);

    constexpr uint8_t DefaultChannel = 76U;
    constexpr std::array<uint8_t, Nrf24L01::AddressWidth> DefaultAddr = { 'R', 'O', 'V', '0', '1' };
}

uint8_t Nrf24L01::readReg(uint8_t reg)
{
    csnLow();
    m_spi.transferByte(CmdRRegister | (reg & 0x1FU));
    uint8_t val = m_spi.transferByte(CmdNop);
    csnHigh();
    return val;
}

void Nrf24L01::writeReg(uint8_t reg, uint8_t val)
{
    csnLow();
    m_spi.transferByte(CmdWRegister | (reg & 0x1FU));
    m_spi.transferByte(val);
    csnHigh();
}

void Nrf24L01::readRegBuf(uint8_t reg, std::span<uint8_t> buf)
{
    csnLow();
    m_spi.transferByte(CmdRRegister | (reg & 0x1FU));
    (void)m_spi.read(buf);
    csnHigh();
}

void Nrf24L01::writeRegBuf(uint8_t reg, std::span<const uint8_t> buf)
{
    csnLow();
    m_spi.transferByte(CmdWRegister | (reg & 0x1FU));
    (void)m_spi.write(buf);
    csnHigh();
}

void Nrf24L01::sendCmd(uint8_t cmd)
{
    csnLow();
    m_spi.transferByte(cmd);
    csnHigh();
}

Nrf24Status Nrf24L01::initRx()
{
    // Enable GPIO clocks for control pins
    if (m_cePort == GPIOA || m_csnPort == GPIOA || m_irqPort == GPIOA) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    }
    if (m_cePort == GPIOB || m_csnPort == GPIOB || m_irqPort == GPIOB) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    }
    if (m_cePort == GPIOC || m_csnPort == GPIOC || m_irqPort == GPIOC) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    }

    // Configure CE as push-pull output
    LL_GPIO_InitTypeDef gpio_init;
    LL_GPIO_StructInit(&gpio_init);
    gpio_init.Pin        = m_cePin;
    gpio_init.Mode       = LL_GPIO_MODE_OUTPUT;
    gpio_init.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(m_cePort, &gpio_init);

    // Configure CSN as push-pull output
    gpio_init.Pin        = m_csnPin;
    LL_GPIO_Init(m_csnPort, &gpio_init);

    // Configure IRQ as input with pull-up
    gpio_init.Pin        = m_irqPin;
    gpio_init.Mode       = LL_GPIO_MODE_INPUT;
    gpio_init.Pull       = LL_GPIO_PULL_UP;
    LL_GPIO_Init(m_irqPort, &gpio_init);

    // Initial pin states: CE low (standby), CSN high (SPI unselected)
    ceLow();
    csnHigh();

    // Initialize underlying SPI bus
    if (m_spi.init() != SpiStatus::Ok) {
        return Nrf24Status::ErrNotFound;
    }

    // Allow the radio module to complete its power-on reset
    LL_mDelay(100);

    // Verify radio presence by writing and reading back address width register
    writeReg(RegSetupAw, 0x03U);
    if (readReg(RegSetupAw) != 0x03U) {
        return Nrf24Status::ErrNotFound;
    }

    // Set default RF channel (2476 MHz)
    setChannel(DefaultChannel);

    // Set maximum RF output power (0 dBm for auto-ACK response) and 1 Mbps air rate
    setPaLevel(Nrf24PaLevel::Max);
    setDataRate(Nrf24DataRate::Rate1Mbps);

    // Enable auto-acknowledgement on pipe 0
    writeReg(RegEnAa, 0x01U);

    // Enable RX address on pipe 0
    writeReg(RegEnRxAddr, 0x01U);

    // Set fixed payload size on pipe 0
    writeReg(RegRxPwP0, static_cast<uint8_t>(sizeof(RoverPacket)));

    // Set default receive address on pipe 0
    setRxAddress(DefaultAddr);

    // Clear all pending interrupt flags in status register
    writeReg(RegStatus, StatusRxDr | StatusTxDs | StatusMaxRt);

    // Flush RX FIFO to start in a clean state
    flushRx();

    // Power up directly in RX mode with 2-byte CRC enabled
    writeReg(RegConfig, ConfigEnCrc | ConfigCrco | ConfigPwrUp | ConfigPrimRx);

    // Allow oscillator to stabilize in Standby-I mode before enabling receiver
    LL_mDelay(2);

    // Pull CE high to enter RX listening mode
    ceHigh();
    LL_mDelay(1);

    return Nrf24Status::Ok;
}

bool Nrf24L01::isConnected()
{
    writeReg(RegSetupAw, 0x03U);
    return (readReg(RegSetupAw) == 0x03U);
}

void Nrf24L01::setRxAddress(std::span<const uint8_t, AddressWidth> addr)
{
    writeRegBuf(RegRxAddrP0, addr);
}

void Nrf24L01::setChannel(uint8_t channel)
{
    if (channel > 125U) {
        channel = 125U;
    }
    writeReg(RegRfCh, channel);
}

void Nrf24L01::setPaLevel(Nrf24PaLevel level)
{
    uint8_t setup = readReg(RegRfSetup) & 0xF9U;

    switch (level) {
        case Nrf24PaLevel::Min:
            setup |= (0x00U << 1);
            break;
        case Nrf24PaLevel::Low:
            setup |= (0x01U << 1);
            break;
        case Nrf24PaLevel::High:
            setup |= (0x02U << 1);
            break;
        case Nrf24PaLevel::Max:
        default:
            setup |= (0x03U << 1);
            break;
    }

    writeReg(RegRfSetup, setup);
}

void Nrf24L01::setDataRate(Nrf24DataRate rate)
{
    uint8_t setup = readReg(RegRfSetup) & ~((1U << 5) | (1U << 3));

    switch (rate) {
        case Nrf24DataRate::Rate250Kbps:
            setup |= (1U << 5);
            break;
        case Nrf24DataRate::Rate2Mbps:
            setup |= (1U << 3);
            break;
        case Nrf24DataRate::Rate1Mbps:
        default:
            break;
    }

    writeReg(RegRfSetup, setup);
}

bool Nrf24L01::isDataAvailable()
{
    return (readReg(RegFifoStatus) & FifoStatusRxEmpty) == 0U;
}

Nrf24Status Nrf24L01::readPayload(std::span<uint8_t> data)
{
    if (data.empty() || data.size() > 32U) {
        return Nrf24Status::ErrParam;
    }

    csnLow();
    m_spi.transferByte(CmdRRxPayload);
    (void)m_spi.read(data);
    csnHigh();

    clearRxFlag();
    return Nrf24Status::Ok;
}

void Nrf24L01::clearRxFlag()
{
    writeReg(RegStatus, StatusRxDr);
}

void Nrf24L01::flushRx()
{
    sendCmd(CmdFlushRx);
}

void Nrf24L01::printDetails()
{
    uint8_t cfg         = readReg(RegConfig);
    uint8_t status      = readReg(RegStatus);
    uint8_t fifo_status = readReg(RegFifoStatus);
    uint8_t rf_ch       = readReg(RegRfCh);
    uint8_t setup       = readReg(RegRfSetup);
    uint8_t rx_pw_p0    = readReg(RegRxPwP0);

    std::array<uint8_t, AddressWidth> rx_addr{};
    readRegBuf(RegRxAddrP0, rx_addr);

    std::printf("\r\n--- NRF24L01+ Receiver Configuration ---\r\n");
    std::printf("CONFIG:      0x%02X (PWR_UP:%u, PRIM_RX:%u, CRC:%u)\r\n",
           cfg,
           (cfg & ConfigPwrUp) ? 1U : 0U,
           (cfg & ConfigPrimRx) ? 1U : 0U,
           (cfg & ConfigEnCrc) ? 1U : 0U);
    std::printf("STATUS:      0x%02X (RX_DR:%u)\r\n",
           status, (status & StatusRxDr) ? 1U : 0U);
    std::printf("FIFO_STATUS: 0x%02X (RX_EMPTY:%u)\r\n",
           fifo_status, (fifo_status & FifoStatusRxEmpty) ? 1U : 0U);
    std::printf("RF_CH:       %u (Frequency: %u MHz)\r\n", rf_ch, 2400U + rf_ch);
    std::printf("RF_SETUP:    0x%02X\r\n", setup);
    std::printf("RX_PW_P0:    %u bytes\r\n", rx_pw_p0);
    std::printf("RX_ADDR_P0:  %c%c%c%c%c (0x%02X 0x%02X 0x%02X 0x%02X 0x%02X)\r\n",
           rx_addr[0], rx_addr[1], rx_addr[2], rx_addr[3], rx_addr[4],
           rx_addr[0], rx_addr[1], rx_addr[2], rx_addr[3], rx_addr[4]);
    std::printf("----------------------------------------\r\n");
}
