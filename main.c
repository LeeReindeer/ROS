/**
 * Blink example in ROS, DO NOT use any function in Arduino.h
 */
#include <avr/io.h>
#include "ros.h"

// include for simavr
// #include "avr_mcu_section.h"
// AVR_MCU(F_CPU, "atmega328p");
ROS_TCB task1;
ROS_TCB task2;
uint8_t task1_stack[ROS_DEFAULT_STACK_SIZE];
uint8_t task2_stack[ROS_DEFAULT_STACK_SIZE];

#define LED1 13
#define LED2 12
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))

#define TASK1_PRIORITY 1
#define TASK2_PRIORITY 0  // max priority

void t1() {
  while (1) {
    // set LED1 high
    bitSet(PORTB, 5);
    ros_delay(200);
    bitClear(PORTB, 5);
    ros_delay(200);
  }
}

void t2() {
  while (1) {
    bitSet(PORTB, 4);
    // delay a second
    ros_delay(100);
    bitClear(PORTB, 4);
    ros_delay(100);
  }
}

void setup() {
  // set LED 13 and LED 12 as output
  bitSet(DDRB, 5);
  bitSet(DDRB, 4);
  bool os_started = ros_init();
  if (os_started) {
    ros_create_task(&task1, t1, TASK1_PRIORITY, task1_stack, ROS_DEFAULT_STACK_SIZE);
    ros_create_task(&task2, t2, TASK2_PRIORITY, task2_stack, ROS_DEFAULT_STACK_SIZE);
    ros_schedule();
  }
}

void loop() {
  // nothing
}

int main() {
  setup();
  // never call loop
  loop();
  return 0;
}
