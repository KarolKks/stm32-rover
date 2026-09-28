#pragma once
#include "stm32g4xx_ll_spi.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_bus.h"
#include <span>
#include <cstdint>
#include <cstddef>

/**
 * @brief Status codes returned by SPI bus functions.
 */
enum class SpiStatus : uint8_t {
    Ok = 0,     /*!< Operation completed successfully */
    ErrInit,    /*!< Hardware initialization failed */
    ErrTimeout, /*!< Operation timed out waiting for hardware flag */
    ErrParam    /*!< Invalid parameter or empty buffer */
};

/**
 * @brief Hardware SPI bus driver for STM32G4.
 */
class SpiBus {
public:
    static inline GPIO_TypeDef* const DefaultPort = GPIOA;
    static constexpr uint32_t DefaultSckPin   = LL_GPIO_PIN_5;
    static constexpr uint32_t DefaultMisoPin  = LL_GPIO_PIN_6;
    static constexpr uint32_t DefaultMosiPin  = LL_GPIO_PIN_7;
    static constexpr uint32_t DefaultAf       = LL_GPIO_AF_5;

    /**
     * @brief Constructs an SpiBus instance.
     * @param instance Pointer to the STM32 SPI peripheral instance (e.g., SPI1).
     */
    explicit SpiBus(SPI_TypeDef* instance = SPI1)
        : m_instance(instance), m_initialized(false) {}

    /**
     * @brief Initializes GPIO pins and the SPI peripheral in Master mode (Mode 0, 8-bit, full-duplex).
     * @return SpiStatus::Ok on success, or SpiStatus::ErrInit on failure.
     */
    [[nodiscard]] SpiStatus init();

    /**
     * @brief Transmits and receives a single byte over SPI (full-duplex, blocking).
     * @param txData Byte to transmit.
     * @return Byte received from the slave device during transmission.
     */
    uint8_t transferByte(uint8_t txData);

    /**
     * @brief Transmits and receives buffers of equal length simultaneously (blocking).
     * @param txBuf Source span containing data bytes to transmit.
     * @param rxBuf Destination span to store received bytes. Must match txBuf length.
     * @return SpiStatus::Ok on success, SpiStatus::ErrParam on size mismatch/empty, or SpiStatus::ErrTimeout.
     */
    [[nodiscard]] SpiStatus transfer(std::span<const uint8_t> txBuf, std::span<uint8_t> rxBuf);

    /**
     * @brief Transmits a buffer of bytes while discarding incoming data (blocking).
     * @param txBuf Span containing data bytes to transmit.
     * @return SpiStatus::Ok on success, SpiStatus::ErrParam if empty, or SpiStatus::ErrTimeout.
     */
    [[nodiscard]] SpiStatus write(std::span<const uint8_t> txBuf);

    /**
     * @brief Receives a buffer of bytes by transmitting dummy bytes (0xFF) (blocking).
     * @param rxBuf Span to store received bytes.
     * @return SpiStatus::Ok on success, SpiStatus::ErrParam if empty, or SpiStatus::ErrTimeout.
     */
    [[nodiscard]] SpiStatus read(std::span<uint8_t> rxBuf);

    /**
     * @brief Checks if the SPI peripheral is initialized.
     * @return true if initialized, false otherwise.
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief Gets the underlying hardware SPI instance pointer.
     * @return Pointer to SPI_TypeDef.
     */
    [[nodiscard]] SPI_TypeDef* getInstance() const { return m_instance; }

private:
    SPI_TypeDef* m_instance;
    bool m_initialized;

    static constexpr uint32_t TimeoutCycles = 100000U;
    static constexpr uint8_t DummyByte = 0xFFU;
};
