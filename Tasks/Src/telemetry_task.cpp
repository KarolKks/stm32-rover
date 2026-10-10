#include "telemetry_task.h"

void telemetryTask(void* pvParameters)
{
    (void)pvParameters;
    RoverPacket packet{};
    bool failsafeActive = true;

    while (true)
    {
        // Use xQueuePeek so telemetry only reads the packet without consuming/removing it from MotorTask
        if (xQueuePeek(g_roverPacketQueue, &packet, pdMS_TO_TICKS(500)) == pdPASS) {
            failsafeActive = false;

            std::printf("RX Seq: %3u | Steering:%4d%% | Throttle:%4d%% | BTN:%s\r\n",
                        packet.sequence,
                        static_cast<int>(packet.steering),
                        static_cast<int>(packet.throttle),
                        packet.button ? "PRESSED" : "RELEASED");
        } else {
            // Timeout elapsed: signal lost
            if (!failsafeActive) {
                failsafeActive = true;
                g_uart.sendString("FAILSAFE: radio signal lost!\r\n");
            }
        }

        // Output telemetry at ~20 Hz (every 50 ms) to prevent spamming UART
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
