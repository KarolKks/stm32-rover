#include "main.h"
#include "tasks_common.h"
#include "radio_task.h"
#include "telemetry_task.h"
#include "motor_task.h"

// Global hardware instances
static SpiBus   g_spi(SPI1);
static PwmTimer g_pwmTimer(TIM3);

Uart     g_uart(USART2);
Nrf24L01 g_radio(g_spi);

// Right side motor driver: PWM on PB1 (TIM3_CH4), Direction pins on PC6, PC7 (front) and PC8, PC9 (rear)
L298N g_rightMotors(g_pwmTimer, PwmChannel::Ch4,
                    GPIOB, LL_GPIO_PIN_1, LL_GPIO_AF_2,
                    GPIOC, LL_GPIO_PIN_6, LL_GPIO_PIN_7, LL_GPIO_PIN_8, LL_GPIO_PIN_9);

// Shared FreeRTOS queue holding the latest received RoverPacket
QueueHandle_t g_roverPacketQueue = nullptr;

int main()
{
    // Initialize SysTick timebase for pre-scheduler LL_mDelay()
    LL_Init1msTick(SystemCoreClock);

    // Cortex-M4 FreeRTOS port requires 4 bits of preemption priority (PRIGROUP = 3)
    NVIC_SetPriorityGrouping(3U);

    // Initialize UART for telemetry and debug output
    if (g_uart.init() != UartStatus::Ok) {
        while (true) {
        }
    }

    g_uart.sendString("\r\n========================================\r\n");
    g_uart.sendString("   Radio Rover Controller (FreeRTOS)    \r\n");
    g_uart.sendString("========================================\r\n");

    // Initialize NRF24L01+ in Receiver (PRX) mode
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

    // Initialize hardware PWM timer at 1 kHz (optimal for L298N DC motors)
    if (g_pwmTimer.init(1000) != PwmStatus::Ok) {
        g_uart.sendString("ERROR: Failed to initialize PWM timer!\r\n");
    }

    // Initialize right side motor driver GPIOs and PWM channel
    if (!g_rightMotors.init()) {
        g_uart.sendString("ERROR: Failed to initialize right motor driver!\r\n");
    } else {
        g_uart.sendString("OK: Right motor driver initialized (PB1 PWM, PC6..PC9 Dir).\r\n");
    }

    // Start PWM timer counter
    g_pwmTimer.start();

    // Create a 1-element queue for the latest RoverPacket
    g_roverPacketQueue = xQueueCreate(1, sizeof(RoverPacket));
    if (g_roverPacketQueue == nullptr) {
        g_uart.sendString("ERROR: Failed to allocate FreeRTOS queue!\r\n");
        while (true) {
        }
    }

    // Create FreeRTOS tasks
    BaseType_t radioTaskCreated     = xTaskCreate(radioTask, "RadioTask", 256, nullptr, 2, nullptr);
    BaseType_t telemetryTaskCreated = xTaskCreate(telemetryTask, "TelemetryTask", 256, nullptr, 1, nullptr);
    BaseType_t motorTaskCreated     = xTaskCreate(motorTask, "MotorTask", 256, nullptr, 1, nullptr);

    if (radioTaskCreated != pdPASS || telemetryTaskCreated != pdPASS || motorTaskCreated != pdPASS) {
        g_uart.sendString("ERROR: Failed to create FreeRTOS tasks!\r\n");
        while (true) {
        }
    }

    g_uart.sendString("Starting FreeRTOS scheduler...\r\n");

    // Start FreeRTOS scheduler (transfers execution control to tasks)
    vTaskStartScheduler();

    // Execution should never reach here unless heap memory was exhausted
    while (true) {
    }
}
