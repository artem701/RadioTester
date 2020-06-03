
#include "alltime.h"

#include "nordic_common.h"
#include "nrf_error.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_timer.h"
#include "nrf_timer.h"
#include "nrf.h"
#include "app_timer.h"


void alltime_init()
{
  uint32_t err_code = nrf_drv_clock_init();
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);
  
  // start low frequency crystal oscillator for app_timer
  NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
  NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_LFCLKSTART    = 1;

  // start 64 MHz crystal oscillator.
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART    = 1;

  // wait for external oscillators to start up
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0 || NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
  {
    
  }

  nrf_drv_clock_lfclk_request(NULL);
}
