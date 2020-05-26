/*
 * Обертка интерфейса работы с часами и таймером
 *
 * * */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "scheduler.h"


typedef void (*timer_callback_t)();

void alltime_init();

//void reset_timer();

void start_timer(uint32_t time_ms, priority_t priority, callback_t callback, void* params);