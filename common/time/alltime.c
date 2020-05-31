
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

static callback_t timer_callback[6];
static priority_t callback_priority[6];
static void*      callback_params[6];

static bool busy[6];

static void timer_evt_handler(nrf_timer_event_t event_type, void* p_context)
{
  switch(event_type)
  {
    case NRF_TIMER_EVENT_COMPARE0:
      // add the requested callback to the schedule
      scheduler_add(timer_callback[0], callback_priority[0], callback_params[0]);
      busy[0] = false;
      break;
    case NRF_TIMER_EVENT_COMPARE1:
      // add the requested callback to the schedule
      scheduler_add(timer_callback[1], callback_priority[1], callback_params[1]);
      busy[1] = false;
      break;
    case NRF_TIMER_EVENT_COMPARE2:
      // add the requested callback to the schedule
      scheduler_add(timer_callback[2], callback_priority[2], callback_params[2]);
      busy[2] = false;
      break;
    case NRF_TIMER_EVENT_COMPARE3:
      // add the requested callback to the schedule
      scheduler_add(timer_callback[3], callback_priority[3], callback_params[3]);
      busy[3] = false;
      break;
    case NRF_TIMER_EVENT_COMPARE4:
      // add the requested callback to the schedule
      scheduler_add(timer_callback[4], callback_priority[4], callback_params[4]);
      busy[4] = false;
      break;
    case NRF_TIMER_EVENT_COMPARE5:
      // add the requested callback to the schedule
      scheduler_add(timer_callback[5], callback_priority[5], callback_params[5]);
      busy[5] = false;
      break;
  }
}

static void timer_init()
{
  // transmitter's tasks timer initialization
  nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
  uint32_t err_code = nrf_drv_timer_init(&TX_TIMER, &timer_cfg, timer_evt_handler);
  APP_ERROR_CHECK(err_code);
  
  nrf_drv_timer_enable(&TX_TIMER);
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

  for (int i = 0; i < 6; ++i)
    busy[i] = false;

  timer_init();
}

static bool get_free_cc(uint8_t* result)
{
  for (int i = 0; i < 6; ++i)
    if (!busy[i])
    {
      *result = i;
      return true;
    }
  return false;
}

bool start_timer(uint32_t time_ms, priority_t priority, callback_t callback, void* params)
{
  uint8_t cc;
  if (!get_free_cc(&cc))
    return false;
  
  busy[cc] = true;
  uint32_t time_ticks = nrf_drv_timer_ms_to_ticks(&TX_TIMER, time_ms);

  switch(cc)
  {
    case 0:
      nrf_drv_timer_extended_compare(&TX_TIMER, NRF_TIMER_CC_CHANNEL0, time_ticks, 
        NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK | NRF_TIMER_SHORT_COMPARE0_STOP_MASK, true);
      break;
    case 1:
      nrf_drv_timer_extended_compare(&TX_TIMER, NRF_TIMER_CC_CHANNEL1, time_ticks, 
        NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK | NRF_TIMER_SHORT_COMPARE1_STOP_MASK, true);
      break;
    case 2:
      nrf_drv_timer_extended_compare(&TX_TIMER, NRF_TIMER_CC_CHANNEL2, time_ticks, 
        NRF_TIMER_SHORT_COMPARE2_CLEAR_MASK | NRF_TIMER_SHORT_COMPARE2_STOP_MASK, true);
      break;
    case 3:
      nrf_drv_timer_extended_compare(&TX_TIMER, NRF_TIMER_CC_CHANNEL3, time_ticks, 
        NRF_TIMER_SHORT_COMPARE3_CLEAR_MASK | NRF_TIMER_SHORT_COMPARE3_STOP_MASK, true);
      break;
    case 4:
      nrf_drv_timer_extended_compare(&TX_TIMER, NRF_TIMER_CC_CHANNEL4, time_ticks, 
        NRF_TIMER_SHORT_COMPARE4_CLEAR_MASK | NRF_TIMER_SHORT_COMPARE4_STOP_MASK, true);
      break;
    case 5:
      nrf_drv_timer_extended_compare(&TX_TIMER, NRF_TIMER_CC_CHANNEL5, time_ticks, 
        NRF_TIMER_SHORT_COMPARE5_CLEAR_MASK | NRF_TIMER_SHORT_COMPARE5_STOP_MASK, true);
      break;
  }

  timer_callback[cc] = callback;
  callback_priority[cc] = priority;
  callback_params[cc] = params;
  
  //NRF_TIMER0->TASKS_START = 1;
  
  return true;
}