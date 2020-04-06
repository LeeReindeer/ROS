# ROS

![LOGO](art/ros-logo.webp)

> ROS -> LEERTOS -> LEER'S RTOS

RTOS implementation. Get more information on [my blog](https://leer.moe/2019/05/12/ros/).

## Porting

Implement following four function in your own porting file:

```c
void ros_init_timer();
void ros_idle_task();
void ros_task_context_init(ROS_TCB *tcb_ptr, task_func task_f, void *stack_top);
void ros_switch_context(ROS_TCB *old_tcb, ROS_TCB *new_tcb);
```
