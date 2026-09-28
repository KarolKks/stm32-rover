#pragma once
#include "spi.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_utils.h"
#include <cstdio>
#include <span>
#include <array>
#include <cstdint>
#include <cstddef>

/**
 * @file NRF24L01.h
 * @brief Modern C++ driver for NRF24L01+ 2.4GHz transceiver using STM32 LL.
 */

/**
 * @brief Status codes returned by NRF24L01 functions.
 */
enum class Nrf24Status : uint8_t {
    Ok = 0,         /*!< Operation completed successfully */
    ErrNotFound,    /*!< Device not responding or communication failed */
    ErrParam        /*!< Invalid parameter provided */
};

/**
 * @brief Output power levels for NRF24L01 (used during auto-ACK response or transmission).
 */
enum class Nrf24PaLevel : uint8_t {
    Min = 0,   /*!< -18 dBm */
    Low,       /*!< -12 dBm */
    High,      /*!<  -6 dBm */
    Max        /*!<   0 dBm */
};

/**
 * @brief Air data rates for NRF24L01.
 */
enum class Nrf24DataRate : uint8_t {
    Rate1Mbps = 0,  /*!< 1 Mbps air data rate */
    Rate2Mbps,      /*!< 2 Mbps air data rate */
    Rate250Kbps     /*!< 250 kbps air data rate */
};

/**
 * @brief Structure representing the control packet sent to the rover.
 */
struct __attribute__((packed)) RoverPacket {
    int8_t   steering;  /*!< Steering command: -100 (full left) to +100 (full right) */
    int8_t   throttle;  /*!< Throttle command: -100 (full reverse) to +100 (full forward) */
    uint8_t  button;    /*!< Stick button state: 1 if pressed, 0 if released */
    uint8_t  sequence;  /*!< Incremental packet counter for diagnostics */
};

/**
 * @brief Modern C++ driver for NRF24L01+ 2.4GHz transceiver using STM32 LL.
 */
class Nrf24L01 {
public:
    static constexpr size_t AddressWidth = 5;
    using Address = std::array<uint8_t, AddressWidth>;

    static inline GPIO_TypeDef* const DefaultCePort  = GPIOA;
    static constexpr uint32_t         DefaultCePin   = LL_GPIO_PIN_9;

    static inline GPIO_TypeDef* const DefaultCsnPort = GPIOA;
    static constexpr uint32_t         DefaultCsnPin  = LL_GPIO_PIN_10;

    static inline GPIO_TypeDef* const DefaultIrqPort = GPIOA;
    static constexpr uint32_t         DefaultIrqPin  = LL_GPIO_PIN_8;

    /**
     * @brief Constructs an Nrf24L01 driver instance.
     * @param spi Reference to the initialized or to-be-initialized SpiBus.
     * @param cePort GPIO port for Chip Enable pin (default: GPIOA).
     * @param cePin GPIO pin mask for Chip Enable pin (default: LL_GPIO_PIN_9).
     * @param csnPort GPIO port for Chip Select Not pin (default: GPIOA).
     * @param csnPin GPIO pin mask for Chip Select Not pin (default: LL_GPIO_PIN_10).
     * @param irqPort GPIO port for Interrupt Request pin (default: GPIOA).
     * @param irqPin GPIO pin mask for Interrupt Request pin (default: LL_GPIO_PIN_8).
     */
    Nrf24L01(SpiBus& spi,
             GPIO_TypeDef* cePort  = DefaultCePort,  uint32_t cePin  = DefaultCePin,
             GPIO_TypeDef* csnPort = DefaultCsnPort, uint32_t csnPin = DefaultCsnPin,
             GPIO_TypeDef* irqPort = DefaultIrqPort, uint32_t irqPin = DefaultIrqPin)
        : m_spi(spi),
          m_cePort(cePort),   m_cePin(cePin),
          m_csnPort(csnPort), m_csnPin(csnPin),
          m_irqPort(irqPort), m_irqPin(irqPin) {}

    /**
     * @brief Initializes GPIO pins, SPI bus, and configures the radio in PRX (Receiver) mode.
     * @return Nrf24Status::Ok on success, or Nrf24Status::ErrNotFound if the chip is not detected.
     */
    [[nodiscard]] Nrf24Status initRx();

    /**
     * @brief Tests SPI communication with the module by writing and verifying a test register.
     * @return true if the module responds correctly, false otherwise.
     */
    [[nodiscard]] bool isConnected();

    /**
     * @brief Configures the receive pipe 0 address.
     * @param addr 5-byte address span.
     */
    void setRxAddress(std::span<const uint8_t, AddressWidth> addr);

    /**
     * @brief Sets the RF channel frequency.
     * @param channel RF channel (0 to 125, corresponding to 2400 to 2525 MHz).
     */
    void setChannel(uint8_t channel);

    /**
     * @brief Sets the RF power amplifier level.
     * @param level Power amplifier output level (Min, Low, High, Max).
     */
    void setPaLevel(Nrf24PaLevel level);

    /**
     * @brief Sets the air data rate.
     * @param rate Air data rate (250kbps, 1Mbps, 2Mbps).
     */
    void setDataRate(Nrf24DataRate rate);

    /**
     * @brief Checks if there is any payload data available in the RX FIFO.
     * @return true if data is available to read, false if RX FIFO is empty.
     */
    [[nodiscard]] bool isDataAvailable();

    /**
     * @brief Reads a payload from the RX FIFO and clears the RX_DR flag.
     * @param data Destination span to store the received payload bytes (max 32 bytes).
     * @return Nrf24Status::Ok on success, or Nrf24Status::ErrParam if buffer is empty/too large.
     */
    [[nodiscard]] Nrf24Status readPayload(std::span<uint8_t> data);

    /**
     * @brief Clears the RX Data Ready (RX_DR) interrupt flag in the STATUS register.
     */
    void clearRxFlag();

    /**
     * @brief Flushes the RX FIFO buffer.
     */
    void flushRx();

    /**
     * @brief Prints the current configuration and register states to standard output (printf).
     */
    void printDetails();

private:
    void csnHigh() const { LL_GPIO_SetOutputPin(m_csnPort, m_csnPin); }
    void csnLow()  const { LL_GPIO_ResetOutputPin(m_csnPort, m_csnPin); }
    void ceHigh()  const { LL_GPIO_SetOutputPin(m_cePort, m_cePin); }
    void ceLow()   const { LL_GPIO_ResetOutputPin(m_cePort, m_cePin); }

    uint8_t readReg(uint8_t reg);
    void writeReg(uint8_t reg, uint8_t val);
    void readRegBuf(uint8_t reg, std::span<uint8_t> buf);
    void writeRegBuf(uint8_t reg, std::span<const uint8_t> buf);
    void sendCmd(uint8_t cmd);

    SpiBus& m_spi;
    GPIO_TypeDef* m_cePort;
    uint32_t      m_cePin;
    GPIO_TypeDef* m_csnPort;
    uint32_t      m_csnPin;
    GPIO_TypeDef* m_irqPort;
    uint32_t      m_irqPin;
};
