
#include "scheduler.h"

#include "alltime.h"

#include "nrf_gpio.h"

#include <stdlib.h>

static schedule_t schedule;

static void scheduler_timer_init();

void scheduler_init()
{
  schedule.head = NULL;
  void scheduler_timer_init();
}

static void add_after(schedule_node_t* base, schedule_node_t* next)
{
  next->next = base->next;
  base->next = next;
}

void scheduler_add(callback_t callback, priority_t priority, void* params)
{
  schedule_node_t* node = (schedule_node_t*)malloc(sizeof(schedule_node_t));
  node->callback = callback;
  node->priority = priority;
  node->params   = params;
  node->next     = NULL;
  
  if (schedule.head == NULL)
  {
    schedule.head = node;
  }
  else if (schedule.head->priority > node->priority)
  {
    node->next = schedule.head;
    schedule.head = node;
  }else
  {
    // skip everything with same or higher priority
    schedule_node_t* i = schedule.head;
    while (i->next != NULL && i->priority <= node->priority)
      i = i->next;

    add_after(i, node);
  }
}

void scheduler_process()
{
  while(schedule.head != NULL)
  {
    // call the function with given params, then move to the next one
    schedule_node_t* next = schedule.head->next;
    schedule.head->callback(schedule.head->params);
    free(schedule.head);
    schedule.head = next;
  }
}


// Timer implementation

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

static void scheduler_timer_init()
{
  
  // transmitter's tasks timer initialization
  nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
  uint32_t err_code = nrf_drv_timer_init(&TX_TIMER, &timer_cfg, timer_evt_handler);
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

