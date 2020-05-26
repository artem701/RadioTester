
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

void scheduler_init();
void scheduler_add(callback_t callback, priority_t priority, void* params);
void scheduler_run();