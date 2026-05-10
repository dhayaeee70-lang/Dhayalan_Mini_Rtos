#ifndef TYPES_H
#define TYPES_H

typedef enum
{
    READY,
    RUNNING,
    BLOCKED,
    SLEEPING
} State_t;

/* Task Control Block */
typedef struct {
    unsigned long sp;           /* Stack pointer — offset 0, ASM depends on this */
    unsigned long priority;     /* Current (possibly inherited) priority: 0 = highest */
    unsigned int  base_priority;/* Original priority before inheritance bump */
    State_t       state;        /* READY / RUNNING / BLOCKED / SLEEPING */
    unsigned long sleep_ticks;  /* Ticks remaining before wake (SLEEPING state) */
    unsigned long deadline;     /* Absolute deadline tick (for EDF later) */
    const char   *name;         /* Human-readable task name */
} TCB;

#endif /* TYPES_H */
