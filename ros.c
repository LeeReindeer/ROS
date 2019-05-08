// A preemptive priority scheduler
#include "ros.h"

/*private fields and functions*/

// +1 for idle task
static ROS_TCB TCBS[MAX_TASK_SIZE + 1];
static uint8_t next_tcb_id = 0;

static int ros_int_cnt = 0;
static uint32_t ros_sys_ticks = 0;
static uint8_t idle_task_stack[ROS_IDLE_STACK_SIZE];
// current running task
static ROS_TCB *current_tcb = NULL;

bool ROS_STARTED = false;

ROS_TCB *tcb_ready_list = NULL;

bool ros_init() {
  CRITICAL_STORE;
  CRITICAL_START();
  ros_init_timer();
  status_t ok = ros_create_task(ros_idle_task, MIN_TASK_PRIORITY,
                                &idle_task_stack[ROS_IDLE_STACK_SIZE - 1]]);
  ROS_STARTED = ok == ROS_OK;
  CRITICAL_END();
  return ROS_STARTED;
}

/**
 * @retval current tcb if NOT in interrupt service routine
 */
ROS_TCB *ros_current_tcb() {
  if (ros_int_cnt == 0) return current_tcb;
  return NULL;
}

// create a task, valid it then add it to the ready list
status_t ros_create_task(task_func task_f, uint8_t priority, void *stack_top) {
  CRITICAL_STORE;
  ROS_TCB *tcb;
  if (next_tcb_id > MAX_TASK_SIZE) {
    return ROS_ERR_TASK_OF;
  }
  tcb = &TCBS[next_tcb_id++];
  tcb->priority = priority;
  tcb->next_tcb = NULL;
  tcb->status = TASK_READY;
  tcb->task_entry = task_f;

  // Initial task context(pc, called-registers, SREG), and set current stack
  // pointer to tcb
  ros_task_context_init(tcb, task_f, stack_top);

  CRITICAL_START();
  // todo enqueue tcb
  CRITICAL_END();
}

// select the max priority task in the ready list, then swap in it and swap out
// current_tcb and set the current_tcb
void ros_schedule() {}

void ros_int_enter() { ros_int_cnt++; }

void ros_sys_tick() {
  if (ROS_STARTED) {
    ros_sys_ticks++;
  } else {
    // #warning "ROS not started, please call ros_init() first."
  }
}

void ros_int_exit() { ros_int_cnt - ; }