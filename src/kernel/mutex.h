#ifndef MUTEX_H
#define MUTEX_H

#include "scheduler.h"   /* needs TCB, task state constants */

typedef struct {
    int locked;       /* 0 = free, 1 = held */
    int owner_id;     /* task index holding the mutex (-1 = none) */
    int waiter_id;    /* task index blocked waiting (-1 = none) */
} Mutex_t;

/* Initialize a mutex to unlocked state */
void mutex_init(Mutex_t *m);

/* Lock: blocks caller if mutex held; applies priority inheritance */
void mutex_lock(Mutex_t *m);

/* Unlock: releases mutex; restores priority; unblocks waiter */
void mutex_unlock(Mutex_t *m);

#endif /* MUTEX_H */
