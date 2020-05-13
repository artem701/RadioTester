#include "nrf_drv_spis.h"
#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "boards.h"
#include "app_error.h"
#include <string.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define SPIS_INSTANCE  0 /**< SPIS instance index. */

/* Должно быть реализовано пользователем */
void spis_event_handler(nrf_drv_spis_event_t event);

void allspis_init();

void allspis_transfer(uint8_t* tx, uint8_t len_tx, uint8_t* rx, uint8_t len_rx);
