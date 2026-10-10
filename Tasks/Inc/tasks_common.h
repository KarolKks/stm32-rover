#pragma once
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "NRF24L01.h"
#include "L298N.h"
#include "uart.h"

// Shared FreeRTOS queue holding the latest RoverPacket
extern QueueHandle_t g_roverPacketQueue;

// Shared hardware instances
extern Nrf24L01 g_radio;
extern Uart     g_uart;
extern L298N    g_rightMotors;
