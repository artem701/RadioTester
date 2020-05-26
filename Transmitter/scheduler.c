
#include "scheduler.h"

#include "nrf_gpio.h"

#include <stdlib.h>

static schedule_t schedule;

void scheduler_init()
{
  schedule.head = NULL;
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

void scheduler_run()
{
  while(1)
  {
    while(schedule.head == NULL)
      __WFE();

    while(schedule.head != NULL)
    {
      // call the function with given params, then move to the next one
      schedule_node_t* next = schedule.head->next;
      schedule.head->callback(schedule.head->params);
      free(schedule.head);
      schedule.head = next;
    }
  }
}