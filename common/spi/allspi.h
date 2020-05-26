/*
 * SPI driver wrapper
 *
 * * */

#include "nrf_drv_spi.h"
#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "app_error.h"
#include <string.h>

#define SPI_INSTANCE  0 /**< SPI instance index. */

typedef void (*spi_handler_t)(nrf_drv_spi_evt_t const * p_event, void* p_context);

void allspi_init(spi_handler_t spi_event_handler);

void allspi_transfer(uint8_t* tx, uint8_t len_tx, uint8_t* rx, uint8_t len_rx);
