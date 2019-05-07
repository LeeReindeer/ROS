#ifndef __ROS_H__
#define __ROS_H__

#define ARDUINO 100
// #define SUPPRESS  // for suppress error in vs code, undefine when compile
#ifndef SUPPRESS
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif  // ARDUINO
#endif  // SUPPRESS

#include "ros.h"

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct leer_tcb LEER_TCB;
/**
 * Define the task control block using doubly linked list
 * while there is always a idle task in the list with priority 255.
 */
struct leer_tcb {
  void *sp;
  uint8_t priority;  // 0~255
  task_func task_entry;
  LEER_TCB *prev_tcb;
  LEER_TCB *next_tcb;
};

// #define TRUE 1
// #define FALSE 0
#define MIN_TASK_PRIORITY 255
#define MAX_TASK_PRIORITY 0

/* Error codes */
typedef uint8_t status_t;
#define ROS_OK 0
#define ROS_ERROR 1

/* Global values and functions*/

extern LEER_TCB *tcb_ready_list;
// current running task
extern LEER_TCB *current_tcb;

// init the os, add a idle task into the list, init the system timer tick
status_t ros_init();
// create a task, valid it then add it to the ready list
status_t ros_create_task(uint8_t priority, task_func task, void *param);
// select the max priority task in the ready list, then swap in it and swap out
// current_tcb and set the current_tcb
void ros_switch_context();

void ros_int_enter();
void ros_sys_tick();
void ros_int_exit();

#ifdef __cplusplus
}
#endif
#endif  //__ROS_H__