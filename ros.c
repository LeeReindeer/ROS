// A preemptive priority scheduler
#include "ros.h"

static int ros_int_cnt = 0;
static uint32_t ros_sys_ticks = 0;

bool ROS_STARTED = false;

status_t ros_init() { ROS_STARTED = true; }

void ros_int_enter() { ros_int_cnt++; }

void ros_sys_tick() {
  if (ROS_STARTED) {
    ros_sys_ticks++;
  } else {
#warning "ROS not started, please call ros_init() first."
  }
}

void ros_int_exit() { ros_int_cnt - ; }