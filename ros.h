#ifndef __ROS_H__
#define __ROS_H__

#define ARDUINO 100  // include arduino files when i'm coding...
// #define SUPPRESS  // for suppress error in vs code, undefine when compile
#ifndef SUPPRESS
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif  // ARDUINO
#endif  // SUPPRESS

#include "ros.h"
#include "ros_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  TASK_READY = 0,
  TASK_RUNNING,
  TASK_BLOCKED,
  TASK_TERMINATED
} Task_Status;

/**
 * Task implementation function:
 * void task1(void *param) {
 *    for(;;) {
 *      // task code here
 *    }
 *    // should never return, once return the task will be deleted from the
 * ready list
 * }
 */
typedef void (*task_func)();

/**
 * Define the task control block using doubly linked list
 * while there is always a idle task in the list with priority 255.
 */
typedef struct ros_tcb {
  void *sp;
  Task_Status status;
  uint8_t priority;  // 0~255
  task_func task_entry;
  // ROS_TCB *prev_tcb; //need doubly-list?
  struct ros_tcb *next_tcb;
} ROS_TCB;

// #define TRUE 1
// #define FALSE 0
#define MIN_TASK_PRIORITY 255
#define MAX_TASK_PRIORITY 0

/* Error codes */
typedef uint8_t status_t;
#define ROS_OK 0U
#define ROS_ERROR 1U
#define ROS_ERR_PARAM 200U
// task size > MAX_TASK_SIZE
#define ROS_ERR_TASK_OF 201U

/*OS core functions: scheduler, context init, context switch and system tick*/

// init the os, add a idle task into the list, init the system timer tick
bool ros_init();
// create a task, valid it then add it to the ready list
status_t ros_create_task(task_func task, uint8_t priority, void *stack_top);
// select the max priority task in the ready list, then swap in it and swap out
// current_tcb and set the current_tcb
void ros_schedule();

// call the following three functions from ISR
void ros_int_enter();
void ros_sys_tick();
void ros_int_exit();

/* Global values and functions*/

extern bool ROS_STARTED;
extern ROS_TCB *tcb_ready_list;

// define in ros_port.c for porting
extern void ros_init_timer();
extern void ros_idle_task();
extern void ros_task_context_init(ROS_TCB tcb_ptr, task_func task_f,
                                  void *stack_top);
extern void ros_switch_context();

#ifdef __cplusplus
}
#endif
#endif  //__ROS_H__