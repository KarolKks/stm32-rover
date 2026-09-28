#pragma once
#include "stm32g4xx_ll_usart.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_rcc.h"
#include <cstdio>
#include <string_view>
#include <cstdint>

/**
 * @brief Status codes returned by UART functions.
 */
enum class UartStatus : uint8_t {
    Ok = 0,     /*!< Operation completed successfully */
    ErrInit,    /*!< Peripheral initialization failed */
    ErrTimeout  /*!< Transmission timed out */
};

/**
 * @brief Hardware UART driver for STM32G4.
 */
class Uart {
public:
    static constexpr uint32_t DefaultBaudrate = 115200U;
    static inline GPIO_TypeDef* const DefaultTxPort = GPIOA;
    static constexpr uint32_t DefaultTxPin   = LL_GPIO_PIN_2;
    static constexpr uint32_t DefaultAf      = LL_GPIO_AF_7;

    /**
     * @brief Constructs a Uart instance.
     * @param instance Pointer to the STM32 USART peripheral instance (e.g., USART2).
     */
    explicit Uart(USART_TypeDef* instance = USART2)
        : m_instance(instance), m_initialized(false) {}

    /**
     * @brief Initializes GPIO TX pin and USART peripheral with 8N1 configuration.
     * @param baudrate Desired baud rate in bits per second (default: 115200).
     * @return UartStatus::Ok on success, or UartStatus::ErrInit on failure.
     */
    [[nodiscard]] UartStatus init(uint32_t baudrate = DefaultBaudrate);

    /**
     * @brief Transmits a single character (blocking).
     * @param c Character to transmit.
     * @return true on success, false on timeout.
     */
    bool sendChar(char c);

    /**
     * @brief Transmits a string view (blocking).
     * @param str String view to transmit.
     */
    void sendString(std::string_view str);

    /**
     * @brief Formats and transmits a signed 32-bit integer as ASCII text.
     * @param num Integer number to transmit.
     */
    void sendInt(int32_t num);

    /**
     * @brief Formats and transmits a floating point number as ASCII text.
     * @param num Float value to transmit.
     * @param decimals Number of decimal digits to display (default: 2).
     */
    void sendFloat(float num, uint8_t decimals = 2);

    /**
     * @brief Redirects standard output (stdout / printf) to this UART instance.
     */
    void setAsDebugOutput();

    /**
     * @brief Checks if the UART peripheral is initialized.
     * @return true if initialized, false otherwise.
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief Gets the underlying hardware USART instance pointer.
     * @return Pointer to USART_TypeDef.
     */
    [[nodiscard]] USART_TypeDef* getInstance() const { return m_instance; }

private:
    USART_TypeDef* m_instance;
    bool m_initialized;

    static constexpr uint32_t TimeoutCycles = 50000U;
};
