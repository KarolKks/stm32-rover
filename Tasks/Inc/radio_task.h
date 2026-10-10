#pragma once
#include "tasks_common.h"
#include <span>

/**
 * @brief FreeRTOS task responsible for polling NRF24L01 radio and pushing packets to the queue.
 * @param pvParameters Unused task parameters pointer.
 */
void radioTask(void* pvParameters);
