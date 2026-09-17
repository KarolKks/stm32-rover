#include "main.h"
#include "NRF24L01.h"
#include "uart.h"
#include "stm32g4xx_ll_utils.h"
#include <stdio.h>

int main(void)
{
    LL_Init1msTick(SystemCoreClock);

    if (uart_init() != UART_OK) {
        while (1) {
        }
    }

    uart_send_string("       Radio Rover Receiver\r\n");

    if (nrf24_init_rx() != NRF24_OK) {
        uart_send_string("WARNING: NRF24L01+ not responding on SPI bus!\r\n");
        uart_send_string("Verify pin connections:\r\n");
        uart_send_string("  PA5  -> SCK\r\n");
        uart_send_string("  PA6  -> MISO\r\n");
        uart_send_string("  PA7  -> MOSI\r\n");
        uart_send_string("  PA10 -> CSN\r\n");
        uart_send_string("  PA9  -> CE\r\n");
        uart_send_string("  PA8  -> IRQ\r\n");
        uart_send_string("  3V3  -> VCC\r\n");
        uart_send_string("  GND  -> GND\r\n");
    } else {
        uart_send_string("OK: NRF24L01+ configured as receiver.\r\n");
        nrf24_print_details();
    }

    rover_packet_t packet = {0};
    uint32_t time_without_packet_ms = 500U;
    bool failsafe_active = true;

    while (1)
    {
        if (nrf24_is_data_available()) {
            if (nrf24_read_payload(&packet, sizeof(packet)) == NRF24_OK) {
                time_without_packet_ms = 0U;
                failsafe_active = false;

                // Apply packet.throttle and packet.steering to the motor driver here.
                printf("RX Seq: %3u | Steering:%4d%% | Throttle:%4d%% | BTN:%s\r\n",
                       packet.sequence,
                       (int)packet.steering,
                       (int)packet.throttle,
                       packet.button ? "PRESSED" : "RELEASED");
            }
        } else {
            if (time_without_packet_ms < 500U) {
                ++time_without_packet_ms;
            }

            if (time_without_packet_ms >= 500U && !failsafe_active) {
                failsafe_active = true;
                uart_send_string("FAILSAFE: radio signal lost, stop motors\r\n");
                // Stop all motors here when the motor driver is added.
            }
        }

        LL_mDelay(1);
    }
}
