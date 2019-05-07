#ifndef __LEERPORT_H__
#define __LEERPORT_H__

#include <avr/interrupt.h>
/*specific port file for Arduino Uno */

// interrupt every SYS_TICK to re-schedule tasks
ISR(TIMER2_COMPA_vect) {
}

#endif //__LEERPORT_H__