/*
 * Обертка интерфейса работы с часами и таймером
 *
 * * */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "scheduler.h"

void alltime_init();

// returns false, if timer was not started
bool start_timer(uint32_t time_ms, priority_t priority, callback_t callback, void* params);
//bool start_timer(uint8_t cc, uint32_t time_ms, priority_t priority, callback_t callback, void* params);