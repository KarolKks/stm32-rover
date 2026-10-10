#pragma once
#include "pwm.h"
#include "gpio.h"
#include <cstdint>

/**
 * @brief Driver for an L298N dual H-bridge motor driver module (controlling 2 DC motors).
 */
class L298N {
public:
    /**
     * @brief Constructs an L298N driver instance.
     * @param pwmTimer Reference to the hardware PWM timer instance.
     * @param pwmChannel Timer PWM channel used for motor speed control.
     * @param pwmPort GPIO port of the PWM output pin.
     * @param pwmPin GPIO pin mask for the PWM output.
     * @param pwmAf Alternate function index for the PWM pin.
     * @param dirPort GPIO port for direction control pins (IN1..IN4).
     * @param in1 Direction control pin for Motor A (Forward/Reverse terminal 1).
     * @param in2 Direction control pin for Motor A (Forward/Reverse terminal 2).
     * @param in3 Direction control pin for Motor B (Forward/Reverse terminal 1).
     * @param in4 Direction control pin for Motor B (Forward/Reverse terminal 2).
     */
    L298N(PwmTimer& pwmTimer, PwmChannel pwmChannel,
          GPIO_TypeDef* pwmPort, uint32_t pwmPin, uint32_t pwmAf,
          GPIO_TypeDef* dirPort,
          uint32_t in1, uint32_t in2, uint32_t in3, uint32_t in4)
        : m_pwmTimer(pwmTimer),
          m_pwmChannel(pwmChannel),
          m_pwmPort(pwmPort),
          m_pwmPin(pwmPin),
          m_pwmAf(pwmAf),
          m_dirPort(dirPort),
          m_in1(in1), m_in2(in2), m_in3(in3), m_in4(in4),
          m_initialized(false) {}

    /**
     * @brief Initializes GPIO direction pins and configures the timer PWM channel.
     * @return true on success, false on failure.
     */
    bool init();

    /**
     * @brief Sets speed and direction of the connected motors.
     * @param speedPercent Target speed from -100 (full reverse) to +100 (full forward), 0 = stop.
     */
    void setSpeed(int8_t speedPercent);

    /**
     * @brief Stops all motors (coast / free-wheeling, 0% PWM).
     */
    void stop();

    /**
     * @brief Actively brakes all motors (100% PWM, shorting motor terminals).
     */
    void brake();

    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    PwmTimer&     m_pwmTimer;
    PwmChannel    m_pwmChannel;
    GPIO_TypeDef* m_pwmPort;
    uint32_t      m_pwmPin;
    uint32_t      m_pwmAf;
    GPIO_TypeDef* m_dirPort;
    uint32_t      m_in1;
    uint32_t      m_in2;
    uint32_t      m_in3;
    uint32_t      m_in4;
    bool          m_initialized;
};
