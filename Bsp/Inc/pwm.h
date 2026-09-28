#pragma once
#include "stm32g4xx_ll_tim.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_gpio.h"
#include <cstdint>

enum class PwmStatus : uint8_t {
    Ok = 0,
    ErrInit,
    ErrParam
};

enum class PwmChannel : uint32_t {
    Ch1 = LL_TIM_CHANNEL_CH1,
    Ch2 = LL_TIM_CHANNEL_CH2,
    Ch3 = LL_TIM_CHANNEL_CH3,
    Ch4 = LL_TIM_CHANNEL_CH4
};

/**
 * @brief Hardware PWM Timer Driver for STM32G4.
 */
class PwmTimer {
public:
    static constexpr uint32_t DefaultFrequencyHz = 1000U; // 1 kHz PWM for L298N DC motors

    explicit PwmTimer(TIM_TypeDef* instance = TIM3)
        : m_instance(instance), m_frequencyHz(0), m_arr(0), m_initialized(false) {}

    /**
     * @brief Initializes the timer base for PWM generation.
     * @param frequencyHz Desired PWM frequency in Hz (default: 1000 Hz).
     */
    [[nodiscard]] PwmStatus init(uint32_t frequencyHz = DefaultFrequencyHz);

    /**
     * @brief Configures a GPIO pin as a PWM output channel.
     * @param channel Timer channel (Ch1..Ch4).
     * @param port GPIO port (e.g. GPIOB).
     * @param pin GPIO pin mask (e.g. LL_GPIO_PIN_1).
     * @param af Alternate function number (e.g. LL_GPIO_AF_2).
     */
    [[nodiscard]] PwmStatus configureChannel(PwmChannel channel,
                                              GPIO_TypeDef* port,
                                              uint32_t pin,
                                              uint32_t af);

    /**
     * @brief Sets duty cycle in percent (0.0% to 100.0%).
     */
    void setDutyCycle(PwmChannel channel, float percent);

    /**
     * @brief Sets raw duty cycle counter value (0 to ARR).
     */
    void setDutyCycleRaw(PwmChannel channel, uint32_t compareValue);

    /**
     * @brief Starts PWM timer counter.
     */
    void start();

    /**
     * @brief Stops PWM timer counter.
     */
    void stop();

    [[nodiscard]] bool isInitialized() const { return m_initialized; }
    [[nodiscard]] uint32_t getArr() const { return m_arr; }
    [[nodiscard]] TIM_TypeDef* getInstance() const { return m_instance; }

private:
    TIM_TypeDef* m_instance;
    uint32_t     m_frequencyHz;
    uint32_t     m_arr;
    bool         m_initialized;
};
