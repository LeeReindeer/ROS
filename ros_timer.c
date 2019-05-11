#include "ros_timer.h"
#include <stdio.h>

static ROS_TIMER *timer_queue;
static uint32_t ros_sys_ticks = 0;
static char day_time[9];  // format: 00:00:00

static void wakeup_task(ROS_TCB *tcb) {
  CRITICAL_STORE;
  CRITICAL_START();
  tcb->status = TASK_READY;
  ros_tcb_enqueue(tcb);
  CRITICAL_END();
}

// dec the ticks of every timer in timer_queue, wake up task if ticks == 0
void ros_check_timer() {
  ROS_TIMER *prev, *next;
  prev = next = timer_queue;
  // timer list to wakeup
  ROS_TIMER *wakeup_head = NULL, *cur_wakeup = NULL;
  // Remove timer which's ticks = 0 from timer queue, and add to wakeup list
  while (next) {
    // use to update next
    ROS_TIMER *saved_next = next->next_timer;
    if ((--next->ticks) == 0) {
      if (next == timer_queue) {
        timer_queue = next->next_timer;
      } else {
        // remove a mid or tail timer
        prev->next_timer = next->next_timer;
      }
      // make this timer isolate
      next->next_timer = NULL;
      if (wakeup_head == NULL) {
        cur_wakeup = wakeup_head = next;
      } else {
        cur_wakeup->next_timer = next;
        cur_wakeup = cur_wakeup->next_timer;
      }
    } else {
      // Use previous timer to remove timer
      prev = next;
    }
    next = saved_next;
  }

  // walk through wakeup list
  while (wakeup_head) {
    wakeup_task(wakeup_head->blocked_tcb);
    wakeup_head = wakeup_head->next_timer;
  }
}

// delay current tcb
status_t ros_delay(uint32_t ticks) {
  ROS_TIMER timer;
  ROS_TCB *cur_tcb;
  uint8_t status;
  CRITICAL_STORE;
  cur_tcb = ros_current_tcb();
  if (ticks == 0) {
    status = ROS_ERR_PARAM;
  } else if (cur_tcb == NULL) {
    status = ROS_ERR_CONTEXT;
  } else {
    CRITICAL_START();
    cur_tcb->status = TASK_BLOCKED;
    timer.ticks = ticks;
    timer.blocked_tcb = cur_tcb;
    if (ros_register_timer(&timer) != ROS_OK) {
      status = ROS_ERR_TIMER;
      CRITICAL_END();
    } else {
      status = ROS_OK;
      CRITICAL_END();
      // call scheduler to swap out current task
      ros_schedule();
    }
  }
  return status;
}

// insert timer to the timer queue head
status_t ros_register_timer(ROS_TIMER *timer) {
  if (timer == NULL || timer->blocked_tcb == NULL) {
    return ROS_ERR_PARAM;
  }
  CRITICAL_STORE;
  CRITICAL_START();
  if (timer_queue == NULL) {
    timer_queue = timer;
    timer_queue->next_timer = NULL;
  } else {
    timer->next_timer = timer_queue;
    timer_queue = timer;
  }
  CRITICAL_END();
  return ROS_OK;
}

void ros_sys_tick() {
  if (ROS_STARTED) {
    ros_sys_ticks++;
    // check for any delay task is due
    ros_check_timer();
  } else {
    // #warning "ROS not started, please call ros_init() first."
  }
}

void ros_set_sys_tick(uint32_t ticks) { ros_sys_ticks = ticks; }

uint32_t ros_get_sys_tick() { return ros_sys_ticks; }

status_t ros_set_time(uint8_t hour, uint8_t minute, uint8_t second) {
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 ||
      second > 59) {
    return ROS_ERR_PARAM;
  }
  ros_sys_ticks = (hour * 3600U + minute * 60U + second) * 100U;
  return ROS_OK;
}

char *ros_get_time() {
  uint32_t cur_second = ros_sys_ticks % 8640000U / 100U;
  uint8_t hour = 0, minute = 0, second = 0;
  hour = cur_second / 3600U;
  cur_second %= 3600U;
  minute = cur_second / 60U;
  cur_second %= 60U;
  second = cur_second;
  sprintf(day_time, "%02d:%02d:%02d", hour, minute, second);
  return day_time;
}