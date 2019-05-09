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
 * @brief The idle task takes advantage of atmega328p's sleep mode, sleep when
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

  // enable interrupt after context switching finished.
  sei();

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
  // the function pointer is uint16_t in avr
  *stack_top-- = (uint8_t)((uint16_t)task_shell & 0xFF);         // the LSB
  *stack_top-- = (uint8_t)(((uint16_t)task_shell >> 8) & 0xFF);  // THE MSB
  // Make space for R2-R17, R28-R29
  *stack_top-- = 0x00; // R2
  *stack_top-- = 0x00; // R3
  *stack_top-- = 0x00; // R4
  *stack_top-- = 0x00; // R5
  *stack_top-- = 0x00; // R6
  *stack_top-- = 0x00; // R7
  *stack_top-- = 0x00; // R8
  *stack_top-- = 0x00; // R9
  *stack_top-- = 0x00; // R10
  *stack_top-- = 0x00; // R11
  *stack_top-- = 0x00; // R12
  *stack_top-- = 0x00; // R13
  *stack_top-- = 0x00; // R14
  *stack_top-- = 0x00; // R15
  *stack_top-- = 0x00; // R16
  *stack_top-- = 0x00; // R17
  *stack_top-- = 0x00; // R28
  *stack_top-- = 0x00; // R29
  save the SREG contents to save the interrupt flag
  // *stack_top-- = 0x80;  // sreg, enable interrupt
  tcb_ptr->sp = stack_top;
}


/**
 * Specific context switch routine for avr.
 * 
 * This function do actual context switch, and called from ros_switch_context_shell().
 * Interrupt should always be disabled when this function is called.
 * 
 * In this routine, the assembly code is in 4 steps:
 * 1. Save current  context (push registers R2-R17, R28-R29)
 * 2. update current task's stack pointer (save the stack pointer to task->sp)
 * 3. change the stack pointer to next task's stack pointer
 * 4. restore the next task's context (now we're at the new task' stask, pop to registers)
 * 
 * We just save and restore registers R2-R17, R28-R29, 
 * because whether context switch is called from ISR or
 * task voluntarily yield() to the scheduler, 
 * the gcc compiler or the ISR will do save other registers.
 * https://gcc.gnu.org/wiki/avr-gcc#Call-Saved_Registers
 * 
 * @param  *old_tcb: the tcb will swap out
 * @param  *new_tcb: the tcb will swap in
 */
void ros_switch_context(ROS_TCB *old_tcb, ROS_TCB *new_tcb) {
  // The assembly code is in intel style, source is always on the right
  asm volatile(
      "push r2\n\t"
      "push r3\n\t"
      "push r4\n\t"
      "push r5\n\t"
      "push r6\n\t"
      "push r7\n\t"
      "push r8\n\t"
      "push r9\n\t"
      "push r10\n\t"
      "push r11\n\t"
      "push r12\n\t"
      "push r13\n\t"
      "push r14\n\t"
      "push r15\n\t"
      "push r16\n\t"
      "push r17\n\t"
      "push r28\n\t"
      "push r29\n\t"
      "mov r28, %A[_old_tcb_]\n\t" // move old tcb(LSB) to Y-regs
      "mov r29, %B[_old_tcb_]\n\t" // MSB
      "sbiw r28:r29, 0\n\t"        // subract 0 from r29:r28, we need this to set SREG-Z if result is zero
      "breq restore\n\t"           // if old_tcb is NULL, jump to restore
      "in r16, %[_SPL_]\n\t"       // get stack pointer to r17:r16
      "in r17, %[_SPH_]\n\t"
      "st Y, r16\n\t"              // set old_tcb->sp to stack pointer
      "std Y+1, r17\n\t"           // because sp is the first member of the TCB struct
      "restore:"
      "mov r28, %A[_new_tcb_]\n\t"
      "mov r29, %B[_new_tcb_]\n\t"
      "ld r28, Y\n\t"              //load new_tcb->sp to r17:r16
      "ldd r29, Y+1\n\t"
      "out %[_SPL_], r16\n\t"      //change the stack pointer to new_tcb->sp
      "out %[_SPH_], r17\n\t"
      "pop r29\n\t"                // restore new_tcb's context
      "pop r28\n\t"
      "pop r17\n\t"
      "pop r16\n\t"
      "pop r15\n\t"
      "pop r14\n\t"
      "pop r13\n\t"
      "pop r12\n\t"
      "pop r11\n\t"
      "pop r10\n\t"
      "pop r9\n\t"
      "pop r8\n\t"
      "pop r7\n\t"
      "pop r6\n\t"
      "pop r5\n\t"
      "pop r4\n\t"
      "pop r3\n\t"
      "pop r2\n\t"
      "ret"
      "" ::
      [_SPL_] "i" _SFR_IO_ADDR(SPL),
      [_SPH_] "i" _SFR_IO_ADDR(SPH),
      [_old_tcb_] "r"(old_tcb),
      [_new_tcb_] "r" (new_tcb)
  );
}

// interrupt every SYS_TICK to re-schedule tasks
ISR(TIMER1_COMPA_vect) {
  ros_int_enter();
  ros_sys_tick();
  // exit ISR, ready to call scheduler
  ros_int_exit();
}
#endif  //__LEERPORT_H__