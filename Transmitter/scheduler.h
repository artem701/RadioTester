
#ifndef SCHEDULER_H
#define SCHEDULER_H

typedef enum {
  HIGHEST,
  PRIORITY_0 = HIGHEST,
  PRIORITY_1,
  PRIORITY_2,
  PRIORITY_3,
  PRIORITY_4,
  PRIORITY_5,
  PRIORITY_6,
  PRIORITY_7,
  LOWEST = PRIORITY_7
} priority_t;

// void* is a pointer to the params structure
typedef void (*callback_t)(void*);

typedef struct schedule_node_t
{
  struct schedule_node_t* next;
  priority_t       priority;
  callback_t       callback;
  void*            params;
} schedule_node_t;

typedef struct 
{
  schedule_node_t* head;
} schedule_t;

// call once before any usage, don't call twice
void scheduler_init();

// add a function to the queue
void scheduler_add(callback_t callback, priority_t priority, void* params);

// run all the functions, which are scheduled
void scheduler_process();


// default priorities for basic transmitter's tasks

#define SEND_DATA_PRIORITY PRIORITY_1
#define SPI_MSG_PRIORITY   PRIORITY_2
#define CLI_PRIORITY       PRIORITY_5
#define TRACING_PRIORITY   LOWEST

#endif
