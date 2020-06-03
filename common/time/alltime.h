/*
 * Обертка интерфейса работы с часами и таймером
 *
 * * */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "nordic_common.h"
#include "nrf_error.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_timer.h"
#include "nrf_timer.h"
#include "nrf.h"
#include "app_timer.h"

typedef void (*timer_callback_t)();

void alltime_init();

//void reset_timer();
