#include "spi.h"

SpiStatus SpiBus::init()
{
    if (m_initialized) {
        return SpiStatus::Ok;
    }

    if (m_instance == SPI1) {
        // Enable GPIO clock for SPI1 pins
        if (DefaultPort == GPIOA) {
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
        } else if (DefaultPort == GPIOB) {
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
        }

        // Configure SCK and MOSI as alternate function push-pull
        LL_GPIO_InitTypeDef gpio_init;
        LL_GPIO_StructInit(&gpio_init);
        gpio_init.Pin        = DefaultSckPin | DefaultMosiPin;
        gpio_init.Mode       = LL_GPIO_MODE_ALTERNATE;
        gpio_init.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        gpio_init.Pull       = LL_GPIO_PULL_NO;
        gpio_init.Alternate  = DefaultAf;

        if (LL_GPIO_Init(DefaultPort, &gpio_init) != SUCCESS) {
            return SpiStatus::ErrInit;
        }

        // Configure MISO as alternate function with pull-up
        gpio_init.Pin        = DefaultMisoPin;
        gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        gpio_init.Pull       = LL_GPIO_PULL_UP;
        gpio_init.Alternate  = DefaultAf;

        if (LL_GPIO_Init(DefaultPort, &gpio_init) != SUCCESS) {
            return SpiStatus::ErrInit;
        }

        // Enable SPI1 peripheral clock on APB2 bus
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);
    } else {
        return SpiStatus::ErrInit;
    }

    // Ensure SPI is disabled before configuring
    LL_SPI_Disable(m_instance);

    // Configure SPI parameters: Master, Mode 0 (CPOL=0, CPHA=0), 8-bit, Software NSS
    LL_SPI_InitTypeDef spi_cfg;
    LL_SPI_StructInit(&spi_cfg);
    spi_cfg.TransferDirection = LL_SPI_FULL_DUPLEX;
    spi_cfg.Mode              = LL_SPI_MODE_MASTER;
    spi_cfg.DataWidth         = LL_SPI_DATAWIDTH_8BIT;
    spi_cfg.ClockPolarity     = LL_SPI_POLARITY_LOW;
    spi_cfg.ClockPhase        = LL_SPI_PHASE_1EDGE;
    spi_cfg.NSS               = LL_SPI_NSS_SOFT;
    spi_cfg.BaudRate          = LL_SPI_BAUDRATEPRESCALER_DIV32;
    spi_cfg.BitOrder          = LL_SPI_MSB_FIRST;
    spi_cfg.CRCCalculation    = LL_SPI_CRCCALCULATION_DISABLE;

    if (LL_SPI_Init(m_instance, &spi_cfg) != SUCCESS) {
        return SpiStatus::ErrInit;
    }

    // Set RX FIFO threshold to 8-bit
    LL_SPI_SetRxFIFOThreshold(m_instance, LL_SPI_RX_FIFO_TH_QUARTER);

    // Enable SPI peripheral
    LL_SPI_Enable(m_instance);

    m_initialized = true;
    return SpiStatus::Ok;
}

uint8_t SpiBus::transferByte(uint8_t txData)
{
    // Clear any pending overrun condition before starting transfer
    if (LL_SPI_IsActiveFlag_OVR(m_instance)) {
        LL_SPI_ClearFlag_OVR(m_instance);
    }

    uint32_t timeout = TimeoutCycles;

    // Wait until transmit FIFO has space
    while (!LL_SPI_IsActiveFlag_TXE(m_instance) && (--timeout > 0U)) {
    }

    if (timeout == 0U) {
        return DummyByte;
    }

    // Send 8-bit byte to SPI data register
    LL_SPI_TransmitData8(m_instance, txData);

    // Wait until receive FIFO contains data
    timeout = TimeoutCycles;
    while (!LL_SPI_IsActiveFlag_RXNE(m_instance) && (--timeout > 0U)) {
    }

    if (timeout == 0U) {
        return DummyByte;
    }

    return LL_SPI_ReceiveData8(m_instance);
}

SpiStatus SpiBus::transfer(std::span<const uint8_t> txBuf, std::span<uint8_t> rxBuf)
{
    const size_t length = !txBuf.empty() ? txBuf.size() : rxBuf.size();
    if (length == 0) {
        return SpiStatus::Ok;
    }

    for (size_t i = 0; i < length; ++i) {
        uint8_t tx = (i < txBuf.size()) ? txBuf[i] : DummyByte;
        uint8_t rx = transferByte(tx);

        if (i < rxBuf.size()) {
            rxBuf[i] = rx;
        }
    }

    // Wait until transmission completes and bus becomes idle
    uint32_t timeout = TimeoutCycles;
    while (LL_SPI_IsActiveFlag_BSY(m_instance) && (--timeout > 0U)) {
    }

    if (timeout == 0U) {
        return SpiStatus::ErrTimeout;
    }

    return SpiStatus::Ok;
}

SpiStatus SpiBus::write(std::span<const uint8_t> txBuf)
{
    if (txBuf.empty()) {
        return SpiStatus::Ok;
    }
    return transfer(txBuf, {});
}

SpiStatus SpiBus::read(std::span<uint8_t> rxBuf)
{
    if (rxBuf.empty()) {
        return SpiStatus::Ok;
    }
    return transfer({}, rxBuf);
}
