#include "L298N.h"

bool L298N::init()
{
    // Configure all 4 direction pins as Push-Pull outputs
    uint32_t dirPinMask = m_in1 | m_in2 | m_in3 | m_in4;
    if (!Bsp::Gpio::initOutput(m_dirPort, dirPinMask)) {
        return false;
    }

    // Configure the PWM pin for this driver
    if (m_pwmTimer.configureChannel(m_pwmChannel, m_pwmPort, m_pwmPin, m_pwmAf) != PwmStatus::Ok) {
        return false;
    }

    // Ensure motor outputs are turned off initially
    stop();

    m_initialized = true;
    return true;
}

void L298N::setSpeed(int8_t speedPercent)
{
    if (speedPercent == 0) {
        stop();
        return;
    }

    if (speedPercent > 0) {
        if (speedPercent > 100) {
            speedPercent = 100;
        }

        // Forward direction: IN1=1, IN2=0 (front motor) & IN3=1, IN4=0 (rear motor)
        LL_GPIO_SetOutputPin(m_dirPort, m_in1 | m_in3);
        LL_GPIO_ResetOutputPin(m_dirPort, m_in2 | m_in4);

        m_pwmTimer.setDutyCycle(m_pwmChannel, static_cast<float>(speedPercent));
    } else {
        if (speedPercent < -100) {
            speedPercent = -100;
        }

        // Reverse direction: IN1=0, IN2=1 (front motor) & IN3=0, IN4=1 (rear motor)
        LL_GPIO_ResetOutputPin(m_dirPort, m_in1 | m_in3);
        LL_GPIO_SetOutputPin(m_dirPort, m_in2 | m_in4);

        m_pwmTimer.setDutyCycle(m_pwmChannel, static_cast<float>(-speedPercent));
    }
}

void L298N::stop()
{
    // Coast / Free-wheeling: 0% PWM and all direction pins LOW
    m_pwmTimer.setDutyCycle(m_pwmChannel, 0.0f);
    LL_GPIO_ResetOutputPin(m_dirPort, m_in1 | m_in2 | m_in3 | m_in4);
}

void L298N::brake()
{
    // Active electrical braking: 100% PWM and all direction pins HIGH (shorting motor coils)
    m_pwmTimer.setDutyCycle(m_pwmChannel, 100.0f);
    LL_GPIO_SetOutputPin(m_dirPort, m_in1 | m_in2 | m_in3 | m_in4);
}
