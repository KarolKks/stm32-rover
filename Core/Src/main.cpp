#include "main.h"

static SpiBus g_spi(SPI1);
static Uart g_uart(USART2);
static Nrf24L01 g_radio(g_spi);

int main()
{
    LL_Init1msTick(SystemCoreClock);

    if (g_uart.init() != UartStatus::Ok) {
        while (true) {
        }
    }

    g_uart.sendString("       Radio Rover Receiver (C++)\r\n");

    if (g_radio.initRx() != Nrf24Status::Ok) {
        g_uart.sendString("WARNING: NRF24L01+ not responding on SPI bus!\r\n");
        g_uart.sendString("Verify pin connections:\r\n");
        g_uart.sendString("  PA5  -> SCK\r\n");
        g_uart.sendString("  PA6  -> MISO\r\n");
        g_uart.sendString("  PA7  -> MOSI\r\n");
        g_uart.sendString("  PA10 -> CSN\r\n");
        g_uart.sendString("  PA9  -> CE\r\n");
        g_uart.sendString("  PA8  -> IRQ\r\n");
        g_uart.sendString("  3V3  -> VCC\r\n");
        g_uart.sendString("  GND  -> GND\r\n");
    } else {
        g_uart.sendString("OK: NRF24L01+ configured as receiver.\r\n");
        g_radio.printDetails();
    }

    RoverPacket packet{};
    uint32_t timeWithoutPacketMs = 500U;
    bool failsafeActive = true;

    while (true)
    {
        if (g_radio.isDataAvailable()) {
            std::span<uint8_t> payloadSpan{reinterpret_cast<uint8_t*>(&packet), sizeof(packet)};
            if (g_radio.readPayload(payloadSpan) == Nrf24Status::Ok) {
                timeWithoutPacketMs = 0U;
                failsafeActive = false;

                // Apply packet.throttle and packet.steering to the motor driver here.
                std::printf("RX Seq: %3u | Steering:%4d%% | Throttle:%4d%% | BTN:%s\r\n",
                            packet.sequence,
                            static_cast<int>(packet.steering),
                            static_cast<int>(packet.throttle),
                            packet.button ? "PRESSED" : "RELEASED");
            }
        } else {
            if (timeWithoutPacketMs < 500U) {
                ++timeWithoutPacketMs;
            }

            if (timeWithoutPacketMs >= 500U && !failsafeActive) {
                failsafeActive = true;
                g_uart.sendString("FAILSAFE: radio signal lost, stop motors\r\n");
                // Stop all motors here when the motor driver is added.
            }
        }

        LL_mDelay(1);
    }
}
