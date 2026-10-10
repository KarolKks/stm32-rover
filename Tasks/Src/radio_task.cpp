#include "radio_task.h"

void radioTask(void* pvParameters)
{
    (void)pvParameters;
    RoverPacket packet{};

    while (true)
    {
        if (g_radio.isDataAvailable()) {
            std::span<uint8_t> payloadSpan{reinterpret_cast<uint8_t*>(&packet), sizeof(packet)};
            if (g_radio.readPayload(payloadSpan) == Nrf24Status::Ok) {
                // Overwrite the queue so the latest control packet is always immediately available
                xQueueOverwrite(g_roverPacketQueue, &packet);
            }
        }

        // Yield CPU for 5 ms to allow other tasks to run
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
