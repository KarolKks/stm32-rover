#pragma once
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_bus.h"
#include <cstdint>

/**
 * @file gpio.h
 * @brief BSP module for configuring and managing GPIO pins on STM32G4.
 */

namespace Bsp::Gpio {

/**
 * @brief Enables the peripheral clock for the specified GPIO port.
 * @param port Pointer to GPIO port (e.g. GPIOA, GPIOB, GPIOC).
 * @return true if clock was successfully enabled, false if port is invalid.
 */
bool enablePortClock(GPIO_TypeDef* port);

/**
 * @brief Configures one or more pins on a port as digital Push-Pull outputs.
 *        Automatically enables the GPIO port clock if not already enabled.
 * @param port Pointer to GPIO port (e.g. GPIOC).
 * @param pinMask Bitmask of pins to configure (e.g. LL_GPIO_PIN_6 | LL_GPIO_PIN_7).
 * @param speed GPIO output speed (default: LL_GPIO_SPEED_FREQ_HIGH).
 * @return true on success, false on failure.
 */
bool initOutput(GPIO_TypeDef* port, uint32_t pinMask, uint32_t speed = LL_GPIO_SPEED_FREQ_HIGH);

} // namespace Bsp::Gpio
