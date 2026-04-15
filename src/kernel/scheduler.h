#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../include/types.h"

#define NUM_TASKS 3
#define TASK_STACK_SIZE 2048

void scheduler_init(void);
unsigned long scheduler_switch_task(unsigned long old_sp);
void scheduler_start(void);

#endif