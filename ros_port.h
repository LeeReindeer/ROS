#ifndef __ROS_PORT_H__
#define __ROS_PORT_H__

// Uncomment when use ros as an arduino library
/*
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif  // ARDUINO
*/

/**
 * include with avr-libc for uintX_t and bool
 */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Because the interrupt flag is stored in SREG, so we save the old SREG, then
 * disable interrupt to do critical codes. After calling CRITICAL_END(), the
 * interrupt flag restored either enable or diable.
 */
#define CRITICAL_STORE uint8_t sreg
#define CRITICAL_START() \
  sreg = SREG;           \
  cli();
#define CRITICAL_END() SREG = sreg;

#ifndef F_CPU
// Atmega328p CPU frequency is 16MHZ
#define F_CPU 16000000UL
#endif

// System ticks pre-second, 100 means 1/100s(10ms) one tick
#define ROS_SYS_TICK 100

#define ROS_IDLE_STACK_SIZE 64
#define ROS_DEFAULT_STACK_SIZE 128
#define ROS_MIN_STACK_SIZE 32

#ifdef __cplusplus
}
#endif

#endif  //__ROS_PORT_H__