#ifndef __ROS_PORT_H__
#define __ROS_PORT_H__

/**
 * include with avr-libc for uintX_t and bool
 */
#include <stdbool.h>
#include <stdint.h>
/**
 * Because the interrupt flag is stored in SREG, so we save the old SREG, then
 * disable interrupt to do critical codes. After calling CRITICAL_END(), the
 * interrupt flag restored either enable or diable.
 */
#define CRITICAL_STORE uint8_t sreg
#define CRITICAL_START() \
  sreg = SREG;           \
  cli();
#define CRITICAL_END() SREG = sre;

#define F_CPU 16000000
// #define

#endif  //__ROS_PORT_H__