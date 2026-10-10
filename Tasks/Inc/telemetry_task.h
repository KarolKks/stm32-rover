#pragma once
#include "tasks_common.h"
#include <cstdio>

/**
 * @brief FreeRTOS task responsible for receiving packets, printing diagnostic telemetry,
 *        and handling failsafe timeouts.
 * @param pvParameters Unused task parameters pointer.
 */
void telemetryTask(void* pvParameters);
