#ifndef __ROS_H__
#define __ROS_H__

#include "ros.h"
#include "ros_port.h"
#include "ros_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  TASK_READY = 0,
  // TASK_RUNNING,
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

#define STACK_POINT(A, SIZE) (&A[SIZE - 1])

// #define TRUE 1
// #define FALSE 0
#define MIN_TASK_PRIORITY 255
#define MAX_TASK_PRIORITY 0

/* Error codes */
typedef uint8_t status_t;
#define ROS_OK 0U
#define ROS_ERROR 1U
#define ROS_ERR_PARAM 200U
#define ROS_ERR_CONTEXT 201U
#define ROS_ERR_TIMER 201U

/*OS core functions: scheduler, context init, context switch and system tick*/

bool ros_init();
ROS_TCB *ros_current_tcb();
status_t ros_create_task(ROS_TCB *tcb, task_func task, uint8_t priority,
                         void *stack_top);
void ros_schedule();

// list operations
void ros_tcb_enqueue(ROS_TCB *tcb);
void ros_tcb_dequeue(int lowest_priority);

// call the following three functions from ISR
void ros_int_enter();
// define in ros_timer.c
extern void ros_sys_tick();
void ros_int_exit();

/**
 * Provide functions to schedule task periodly
 * We just wrap the task_func with ros_delay() in following format:
 * void task_wrapper() {
 *  ros_delay(delay);
 *  while (1) {
 *    tcb->task_entry();
 *    ros_delay(period);
 *  }
 * }
 * So your task function MUST be a run to compeletion task.
 * //TODO but i do not know how to wrap
 * /
// status_t ros_create_period_task(ROS_TCB *tcb, task_func task, uint8_t
priority, void *stack_top, uint32_t period, uint32_t delay);

/* Global values and functions*/

extern bool ROS_STARTED;
extern ROS_TCB *tcb_ready_list;

// define in ros_port.c for porting
extern void ros_init_timer();
extern void ros_idle_task();
extern void ros_task_context_init(ROS_TCB *tcb_ptr, task_func task_f,
                                  void *stack_top);
extern void ros_switch_context(ROS_TCB *old_tcb, ROS_TCB *new_tcb);

#ifdef __cplusplus
}
#endif

#endif  //__ROS_H__