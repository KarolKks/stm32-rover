#include "motor_task.h"

void motorTask(void* pvParameters)
{
    (void)pvParameters;
    RoverPacket packet{};
    constexpr int8_t Deadzone = 5; // 5% joystick deadzone

    while (true)
    {
        // Wait up to 500 ms for the latest packet from RadioTask
        if (xQueueReceive(g_roverPacketQueue, &packet, pdMS_TO_TICKS(500)) == pdPASS) {
            // 1. Emergency Handbrake: Button pressed
            if (packet.button == 1) {
                g_rightMotors.brake();
                continue;
            }

            // 2. Skid-steer kinematics: Right side speed = throttle - steering
            int16_t rightSpeed = static_cast<int16_t>(packet.throttle) - static_cast<int16_t>(packet.steering);

            // 3. Deadzone filter
            if (std::abs(static_cast<int>(packet.throttle)) < Deadzone &&
                std::abs(static_cast<int>(packet.steering)) < Deadzone) {
                g_rightMotors.stop();
            } else {
                // Clamp speed to valid [-100, +100] range
                if (rightSpeed > 100) {
                    rightSpeed = 100;
                } else if (rightSpeed < -100) {
                    rightSpeed = -100;
                }

                g_rightMotors.setSpeed(static_cast<int8_t>(rightSpeed));
            }
        } else {
            // 4. Timeout (Signal Lost): Failsafe stop
            g_rightMotors.stop();
        }
    }
}
