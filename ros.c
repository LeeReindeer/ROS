// A preemptive priority scheduler
#include "ros.h"

/*private fields and functions*/

static int ros_int_cnt = 0;
static ROS_TCB idle_tcb;
static uint8_t idle_task_stack[ROS_IDLE_STACK_SIZE];
// current running task
static ROS_TCB *current_tcb = NULL;

static void ros_switch_context_shell(ROS_TCB *old_tcb, ROS_TCB *new_tcb);

/*Global fields*/

bool ROS_STARTED = false;

/**
 * The tcb list is a single linked list, and it's ordered by priority.
 * The head of list always has the highest priority, there's already a idel task
 * whil os started, so when schedule just take the head of list, but the enqueue
 * operations take O(N) time complexity, the dequeue is O(1).
 */
ROS_TCB *tcb_ready_list = NULL;

/**
 * @brief  Warpper function of context switch. It will be called by schduler,set
 * the current_tcb
 * @param  *old_tcb: tcb need to swap out, it always pointer to current_tcb,
 * nullable
 * @param  *new_tcb: tcb need to swap in, current_tcb will be set to new_tcb,
 * nonull
 */
static void ros_switch_context_shell(ROS_TCB *old_tcb, ROS_TCB *new_tcb) {
  // diable self-preemption
  if (old_tcb != new_tcb) {
    current_tcb = new_tcb;  // we don't need to update current_tcb in asm code
    // the old_tcb is previous current_tcb
    ros_switch_context(old_tcb, new_tcb);
  }
}

/**
 * @brief  Start the os:
 * 1. init the system timer, start ticking
 * 2. add a idle task into the list
 * @retval ture if os started
 */
bool ros_init() {
  CRITICAL_STORE;
  CRITICAL_START();
  ros_init_timer();
  status_t ok =
      ros_create_task(&idle_tcb, ros_idle_task, MIN_TASK_PRIORITY, idle_task_stack, ROS_IDLE_STACK_SIZE);
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
                         stack_t *stack, int stack_size) {
  CRITICAL_STORE;
  if (tcb == NULL || task_f == NULL || stack == NULL || stack_size < ROS_MIN_STACK_SIZE) {
    return ROS_ERR_PARAM;
  }
  void *stack_top = STACK_POINT(stack, stack_size);
  tcb->priority = priority;
  tcb->next_tcb = NULL;
  tcb->status = TASK_READY;
  tcb->task_entry = task_f;

  // Initial task context(pc, calle-used registers), and set current stack
  // pointer to tcb
  ros_task_context_init(tcb, task_f, stack_top);

  // critical block: disable interrupt
  CRITICAL_START();
  ros_tcb_enqueue(tcb);
  CRITICAL_END();
  if (ROS_STARTED && ros_current_tcb()) ros_schedule();
  return ROS_OK;
}

/**
 * OS core scheduler implementation.
 * The scheduler will be called only in follwing 3 places:
 * - timer ISR calls scheduler
 * - task voluntarily ros_delay() to the scheduler
 * - the task run to compeletion, delete this task, call the scheduler to pick
 * the next task
 *
 * The schduler is preemptive priority-based with round-robin.The round-robin is
 * only preformed for the task with same priority.We allow swap in the task,
 * when its priority is NO LESS than current task.
 *
 * While scheduler is based on the list operations: enqueue and dequeue, so the
 * scheduler tasks O(N) timer complexity.
 */
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
    if (new_tcb) {
      // Do not enqueue curren_tcb here, when the task is blocked, it is added
      // to timer_queue, it will enqueue when the ticks due.
      ros_switch_context_shell(current_tcb, new_tcb);
    } else {
      // but you can't block the idle task
      if (current_tcb == &idle_tcb) current_tcb->status = TASK_READY;
    }
  } else {
    // remove terminated task
    do {
      new_tcb = ros_tcb_dequeue(current_tcb->priority);
    } while (new_tcb && new_tcb->status == TASK_TERMINATED);
    if (new_tcb) {
      ros_tcb_enqueue(current_tcb);
      ros_switch_context_shell(current_tcb, new_tcb);
    }
  }
  CRITICAL_END();
}

/**
 * @brief enqueue tcb to list order by priority. Do round-robin when priority is
 * same.
 * @param  *tcb: the tcb to insert
 */
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
      if (next_ptr == tcb_ready_list) {
        tcb_ready_list = tcb;
        tcb->next_tcb = next_ptr;  // next_ptr maybe NULL
      } else {                     // insert between tow tcb or tail
        tcb->next_tcb = next_ptr;  // next_ptr maybe NUL
        prev_ptr->next_tcb = tcb;
      }
      break;
    } else {
      prev_ptr = next_ptr;
      next_ptr = next_ptr->next_tcb;
    }
  } while (prev_ptr != NULL);
}

/**
 * @brief  dequeue a tcb to swap in, requeir its priority no lower than
 * lowest_priority Because the list ordered by priority, we just check the head,
 * if the head is lower than lowest_priority, return NULL. use
 * ros_tcb_dequeue(MIN_TASK_PRIORITY) to dequeue head unconditionally
 * @param lowest_priority: the lowest priority of dequeue tcb or NULL if no such
 * tcb
 */
ROS_TCB *ros_tcb_dequeue(uint8_t lowest_priority) {
  if (tcb_ready_list == NULL || tcb_ready_list->priority > lowest_priority) {
    return NULL;
  } else {
    ROS_TCB *tcb = tcb_ready_list;
    tcb_ready_list = tcb_ready_list->next_tcb;
    if (tcb_ready_list) {
      // make return tcb isolated
      tcb->next_tcb = NULL;
    }
    return tcb;
  }
}

void ros_int_enter() { ros_int_cnt++; }

void ros_int_exit() { 
  ros_int_cnt--;
  ros_schedule();
}