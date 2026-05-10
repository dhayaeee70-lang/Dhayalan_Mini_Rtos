#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../include/types.h"

#define NUM_TASKS       3
#define TASK_STACK_SIZE 2048

/* ---------------------------------------------------------------
 * Task lifecycle
 * --------------------------------------------------------------- */
int task_create(const char *name, void (*func)(void), unsigned int priority);

/* ---------------------------------------------------------------
 * Scheduler core
 * --------------------------------------------------------------- */
void          scheduler_init(void);
unsigned long scheduler_switch_task(unsigned long old_sp);
void          scheduler_start(void);

/* Called from timer IRQ every tick */
void scheduler_tick(void);

/* ---------------------------------------------------------------
 * Scheduler accessors — used by mutex.c
 * --------------------------------------------------------------- */

/* Returns the index of the currently running task */
int scheduler_get_current_task(void);

/* Set a task's state (READY / BLOCKED / SLEEPING / RUNNING) */
void scheduler_set_task_state(int id, State_t state);

/* Temporarily change a task's priority (for priority inheritance) */
void scheduler_set_task_priority(int id, unsigned long prio);

/* Read a task's current priority */
unsigned long scheduler_get_task_priority(int id);

/* Restore a task's priority back to its base_priority (undo inheritance) */
void scheduler_restore_priority(int id);

/* Voluntary yield: current task gives up CPU immediately.
 * Implemented as an ARM64 SVC #0 — triggers the SVC exception
 * which calls scheduler_switch_task() just like a timer IRQ. */
void scheduler_yield(void);

#endif /* SCHEDULER_H */
