#pragma once
#include "tasks_common.h"
#include <cstdlib>

/**
 * @brief FreeRTOS task responsible for reading control packets from the queue,
 *        calculating skid-steer wheel speeds, applying PWM/direction to motor drivers,
 *        and executing active braking and failsafe stops.
 * @param pvParameters Unused task parameters pointer.
 */
void motorTask(void* pvParameters);
