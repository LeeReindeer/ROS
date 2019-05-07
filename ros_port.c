#ifndef __LEERPORT_H__
#define __LEERPORT_H__

#include "ros_port.h"
#include <avr/sleep.h>
#include "ros.h"
/*specific port file for Arduino Uno */

static void init_timer1() {
  // Set prescaler 256
  TCCR1B = _BV(CS12) | _BV(WGM12);
  // For Arduino Uno CPU 16000000 HZ, so the OCR1A should count from 0 to 624
  // x * 1/16M * 256 = 10 ms = 0.01 s
  // x = 16 M / 100 / 256 = 625
  OCR1A = (F_CPU / 256 / ROS_SYS_TICK) - 1;
  // enable compare match 1A interrupt
  TIMSK1 = _BV(OCIE1A);
}

void ros_init_timer() { init_timer1(); }

void ros_idle_task() {
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  while (1) {
    sleep_mode();
  }
}

void ros_task_context_init(ROS_TCB tcb_ptr, task_func task_f, void *stack_top) {
}

void ros_switch_context(/*ROS_TCB *new_tcb*/) {}

// interrupt every SYS_TICK to re-schedule tasks
ISR(TIMER2_COMPA_vect) {
  ros_int_enter();
  ros_sys_tick();
  // exit ISR, ready to call scheduler
  ros_int_exit();
}
#endif  //__LEERPORT_H__