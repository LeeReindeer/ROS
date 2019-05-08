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

/**
 * @brief The idle tasks advantage of atmega328p's sleep mode, sleep when
 * there is no task to run
 */
void ros_idle_task() {
  set_sleep_mode(SLEEP_MODE_IDLE);
  while (1) {
    sleep_mode();
  }
}

/**
 * @brief Wrapper of task function, which can pass param to task(for furture
 * usage), and terminated it and re-schedule when a task run to compeletion
 */
static void task_shell() {
  ROS_TCB *cur_tcb = ros_current_tcb();
  if (cur_tcb && cur_tcb->task_entry) {
    cur_tcb->task_entry();
  }
  // when the task terminated(task return), remove it from ready list and
  // re-schedule
  cur_tcb->status = TASK_TERMINATED;
  ros_schedule();
}

void ros_task_context_init(ROS_TCB *tcb_ptr, task_func task_f,
                           void *stack_top) {
  // pc
  // *stack_top-- = task_f & 0xFF;
  //*stack_top-- = task_f >> 8;  // MSB
  *stack_top-- = (uint8_t)((uint16_t)task_shell & 0xFF);
  *stack_top-- = (uint8_t)(((uint16_t)task_shell >> 8) & 0xFF);
  // 32 registers
  for (int i = 0; i < 32; i++) {
    *stack_top-- = 0x00;  // initial value of registers
  }
  // save the SREG contents to save the interrupt flag
  *stack_top-- = 0x80;  // sreg, enable interrupt
  tcb_ptr->sp = stack_top;
}

void ros_switch_context(/*ROS_TCB *new_tcb*/) {}

// interrupt every SYS_TICK to re-schedule tasks
ISR(TIMER1_COMPA_vect) {
  ros_int_enter();
  ros_sys_tick();
  // exit ISR, ready to call scheduler
  ros_int_exit();
}
#endif  //__LEERPORT_H__