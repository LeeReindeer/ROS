// A preemptive priority scheduler
#include "ros.h"

/*private fields and functions*/

static uint8_t next_tcb_id = 0;

static int ros_int_cnt = 0;
static uint32_t ros_sys_ticks = 0;
static ROS_TCB idle_tcb;
static uint8_t idle_task_stack[ROS_IDLE_STACK_SIZE];
// current running task
static ROS_TCB *current_tcb = NULL;

static ros_switch_context_shell(ROS_TCB *old_tcb, ROS_TCB *new_tcb);

/**
 * @brief  Warpper function of context switch. It will be called by schduler,set
 * the current_tcb
 * @param  *old_tcb: tcb need to swap out, it always pointer to current_tcb
 * @param  *new_tcb: tcb need to swap in, current_tcb will be set to new_tcb
 */
static ros_switch_context_shell(ROS_TCB *old_tcb, ROS_TCB *new_tcb) {
  // diable self-preemption
  if (old_tcb != new_tcb) {
    current_tcb = new_tcb;  // we don't need to update current_tcb in asm code
    // the old_tcb is previous current_tcb
    ros_switch_context(old_tcb, new_tcb);
  }
}

bool ROS_STARTED = false;

/**
 * The tcb list is a single linked list, and it's ordered by priority.
 * The head of list always has the highest priority, there's already a idel task
 * whil os started, so when schedule just take the head of list, but the enqueue
 * and dequeue operations both take O(N) time complexity.
 */
ROS_TCB *tcb_ready_list = NULL;

bool ros_init() {
  CRITICAL_STORE;
  CRITICAL_START();
  ros_init_timer();
  status_t ok =
      ros_create_task(&idle_tcb, ros_idle_task, MIN_TASK_PRIORITY,
                      STACK_POINT(idle_task_stack, ROS_IDLE_STACK_SIZE));
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

/**
 * @brief create a task, valid it then add it to the ready list
 * @param  *tcb: the caller provides the tcb storage
 * @param  task_f: task function entry point
 * @param  priority: task priotity, 0(max) to 255(min)
 * @param  *stack_top: caller provides the stack storage
 * @retval ROS_OK Success
 * @retval ROS_ERR_PARAM Bad param
 */
status_t ros_create_task(ROS_TCB *tcb, task_func task_f, uint8_t priority,
                         void *stack_top) {
  CRITICAL_STORE;
  if (tcb == NULL || task_f == NULL || stack_top == NULL) {
    return ROS_ERR_PARAM;
  }
  // if (next_tcb_id > MAX_TASK_SIZE) {
  // return ROS_ERR_TASK_OF;
  // }
  // tcb = &TCBS[next_tcb_id++];
  tcb->priority = priority;
  tcb->next_tcb = NULL;
  tcb->status = TASK_READY;
  tcb->task_entry = task_f;

  // Initial task context(pc, called-registers, SREG), and set current stack
  // pointer to tcb
  ros_task_context_init(tcb, task_f, stack_top);

  // critical block: disable interrupt
  CRITICAL_START();
  // todo enqueue tcb
  CRITICAL_END();
  if (ROS_STARTED && ros_current_tcb()) ros_schedule();
  return ROS_OK;
}

// select the max priority task in the ready list, then swap in it and swap out
// current_tcb and set the current_tcb
void ros_schedule() {
  // no schedule and context switch util the very end of ISR
  if (ros_int_cnt != 0 || !ROS_STARTED) return;
  CRITICAL_STORE;
  ROS_TCB *new_tcb = NULL;
  CRITICAL_START();
  // if current task is NULL or suspend or terminated, a new task will swap in
  // unconditionally
  if (current_tcb == NULL || current_tcb->status == TASK_BLOCKED ||
      current_tcb->status == TASK_TERMINATED) {
    // task with any priority(0~255) can be swap in
    new_tcb = ros_tcb_dequeue(MIN_TASK_PRIORITY);
    ros_switch_context_shell(current_tcb, new_tcb);
  } else {
    // remove terminated task
    // fixme what for suspended task?
    do {
      new_tcb = ros_tcb_dequeue(current_tcb->priority);
    } while (new_tcb && new_tcb->status == TASK_TERMINATED);
    if (new_tcb) {
      // new_tcb->status = TASK_RUNNING;
      ros_switch_context_shell(current_tcb, new_tcb);
    }
  }
  CRITICAL_END();
}

void ros_tcb_enqueue(ROS_TCB *tcb) {
  if (tcb == NULL) return;
  ROS_TCB *prev_ptr, *next_ptr;
  prev_ptr = next_ptr = tcb_ready_list;
  do {
    // Insert when:
    // next == NULL
    // next tcb's priority is lower than than the one we enqueuing
    // same priority task will do round-bobin
    if ((next_ptr == NULL) || (next_ptr->priority > tcb->priority)) {
      // list is empty or insert to head
      if (next_ptr == ready_list) {
        ready_list = tcb;
        tcb->next = next_ptr;  // next_ptr maybe NULL
      } else {                 // insert between tow tcb or tail
        tcb->next = next_ptr;  // next_ptr maybe NUL
        prev_ptr->next = tcb;
      }
      break;
    } else {
      prev_ptr = next_ptr;
      next_ptr = next_ptr->next;
    }
  } while (prev_ptr != NULL);
}

void ros_tcb_dequeue(uint8_t lowest_priority) {
  if (tcb_ready_list == NULL || tcb_ready_list->priority > lowest_priority) {
    return NULL;
  } else {
    ROS_TCB *tcb = tcb_ready_list;
    tcb_ready_list = tcb_ready_list->next;
    if (tcb_ready_list) {
      // make return tcb isolated
      tcb->next = NULL;
    }
    return tcb;
  }
}

void ros_int_enter() { ros_int_cnt++; }

void ros_sys_tick() {
  if (ROS_STARTED) {
    ros_sys_ticks++;
  } else {
    // #warning "ROS not started, please call ros_init() first."
  }
}

void ros_int_exit() { ros_int_cnt - ; }