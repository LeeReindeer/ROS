# ROS

![](https://img.shields.io/badge/language-C-green.svg)
![](https://img.shields.io/badge/category-learning-blue.svg)
[![](https://img.shields.io/badge/blog-@LeeReindeer-red.svg)](https://leer.moe)

![LOGO](art/ros-logo.webp)

> ROS -> LEERTOS -> LEER'S RTOS

RTOS implementation, a task scheduler run on atmega328p. Get more information on [my chinese blog](https://leer.moe/2019/05/12/ros/)(中文).

## How to use

### Build

To manually build ROS, the following avr and Arduino tool chains are utilized: avr-gcc, avr-size, avr-objcopy, and avrdude. The correct paths of these tool chains should be properly configured within the Makefile.

The default target "all" will build $(BUILD_DIR)/$(TARGET).hex, which depends on $(BUILD_DIR)/$(TARGET).elf, which in turn depends on OBJS. Therefore, the final build order is:  OBJS -> $(BUILD_DIR)/$(TARGET).elf -> $(BUILD_DIR)/$(TARGET).hex  The "Upload" target will use the avrdude tool to upload the built hex file to the AVR development board, in this case the Arduino Uno with atmega328p. The MCU variable can also be customized to build and upload for different development boards.

Then you can simply execute the command `make all` to both build and upload the code.

### Example

The following code is an example of using ROS to blink two LEDs at different frequencies:

```c
/**
 * Blink example in ROS, DO NOT use any function in Arduino.h
 */
#include <avr/io.h>
#include "ros.h"

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

int main() {
  setup();
  return 0;
}
```

The two tasks run as follows:  
1. After the system initializes and creates the task, Task1, Task2 and the default task are all in the ready queue, the highest priority Task2 is selected to run, LED2 is lit, and then the ros_delay (100) is encountered, and Task2 will be blocked for 1 second. 
2. Task1 is run, LED 1 is lit, and then call ros_delay (200), which will block for two seconds. 
3. Both tasks are blocked and the default task runs. 
4. After one second, Task 2 is woken up, continues to run, turns off LED2, and then Task 2 will be blocked for another 1 second. 
5. After two seconds, Task1 and Task2 are woken up, and Task2 with higher priority runs, lights LED2, and blocks the call ros_delay (100). 6. Task2 is run, LED 1 is turned off, and the call ros_delay (200) is blocked. 

The two tasks continue in this order, with the effect that LDE1 lights up and off every 2 seconds, while LED2 lights up and off every 1 second. Of course, this is just a simple basic example

## Porting to other boards

Implement following 4 functions in your own porting file:

```c
void ros_init_timer();
void ros_idle_task();
void ros_task_context_init(ROS_TCB *tcb_ptr, task_func task_f, void *stack_top);
void ros_switch_context(ROS_TCB *old_tcb, ROS_TCB *new_tcb);
```

## Related Project

- [s_task - full platform multi-task library for C](https://github.com/LeeReindeer/ROS/issues/1)
