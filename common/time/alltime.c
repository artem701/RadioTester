
#include "alltime.h"

#include "nordic_common.h"
#include "nrf_error.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_timer.h"
#include "nrf_timer.h"
#include "nrf.h"
#include "app_timer.h"

#include "scheduler.h"

const static nrf_drv_timer_t TX_TIMER = NRF_DRV_TIMER_INSTANCE(0);

static callback_t timer_callback = NULL;
static priority_t callback_priority;
static void*      callback_params;

static bool busy = false;

static void timer_evt_handler(nrf_timer_event_t event_type, void* p_context)
{
  switch(event_type)
  {
    case NRF_TIMER_EVENT_COMPARE0:
      // add the requested callback to the schedule
      scheduler_add(timer_callback, callback_priority, callback_params);
      NRF_TIMER0->TASKS_STOP = 1;
      busy = false;
      break;
  }
}

void alltime_init()
{
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

  uint32_t err_code = nrf_drv_clock_init();
  APP_ERROR_CHECK(err_code);
  nrf_drv_clock_lfclk_request(NULL);

  err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);

  // transmitter's tasks timer initialization
  nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
  err_code = nrf_drv_timer_init(&TX_TIMER, &timer_cfg, timer_evt_handler);
  APP_ERROR_CHECK(err_code);
  
  nrf_drv_timer_enable(&TX_TIMER);
}

bool timer_is_busy()
{
  return busy;
}

bool start_timer(uint32_t time_ms, priority_t priority, callback_t callback, void* params)
{
  if (busy)
    return false;

  uint32_t time_ticks = nrf_drv_timer_ms_to_ticks(&TX_TIMER, time_ms);
  nrf_drv_timer_extended_compare(&TX_TIMER, NRF_TIMER_CC_CHANNEL0, time_ticks, 
    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK | NRF_TIMER_SHORT_COMPARE0_STOP_MASK, true);

  timer_callback = callback;
  callback_priority = priority;
  callback_params = params;
  
  NRF_TIMER0->TASKS_START = 1;
  
  busy = true;
  return true;
}