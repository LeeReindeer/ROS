#ifndef __ROS_TIMER_H__
#define __ROS_TIMER_H__

#include "ros.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ros_timer {
  ROS_TCB *blocked_tcb;
  uint32_t ticks;
  struct ros_timer *next_timer;
} ROS_TIMER;

void ros_check_timer();
// add the timer to timer queue
status_t ros_register_timer(ROS_TIMER *timer);
status_t ros_delay(uint32_t ticks);
void ros_set_sys_tick(uint32_t ticks);
uint32_t ros_get_sys_tick();
// transfer system tick to the time in the real world in 24-hours format
void ros_set_time(uint8_t hour, uint8_t minute, uint8_t second);
char* ros_get_time();

#ifdef __cplusplus
}
#endif

#endif  //__ROS_H__