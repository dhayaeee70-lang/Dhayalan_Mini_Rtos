#include "scheduler.h"
#include "../drivers/uart.h"

/* ---------------------------------------------------------------
 * Static task table and stack pool
 * --------------------------------------------------------------- */
static TCB tasks[NUM_TASKS];
static unsigned char task_stacks[NUM_TASKS][TASK_STACK_SIZE] __attribute__((aligned(16)));

static int task_count   = 0;   /* number of tasks created so far  */
static int current_task = 0;   /* index of currently running task */

/* ---------------------------------------------------------------
 * init_task_stack
 *   Lay down a fake exception-return frame so the CPU can
 *   "return" into task_func on the first context switch.
 *
 *   Frame layout (34 × 8-byte slots, grows downward):
 *     sp[0..30]  = x0..x30  (general-purpose registers)
 *     sp[31]     = (padding / x31 placeholder)
 *     sp[32]     = ELR_EL1  (entry point — where CPU jumps on eret)
 *     sp[33]     = SPSR_EL1 (processor state: EL1, IRQs enabled)
 * --------------------------------------------------------------- */
static void init_task_stack(unsigned long *stack_top,
                             void (*task_func)(void),
                             int task_num)
{
    unsigned long *sp = stack_top;
    sp -= 34;

    for (int i = 0; i < 31; i++) sp[i] = 0;            /* x0–x30 = 0  */
    sp[32] = (unsigned long)task_func;                 /* ELR_EL1     */
    sp[33] = 0x345;                                    /* SPSR_EL1    */

    tasks[task_num].sp = (unsigned long)sp;
}

/* ---------------------------------------------------------------
 * task_create
 *   Register a new task. Returns task id (0-based) or -1 if full.
 * --------------------------------------------------------------- */
int task_create(const char *name, void (*func)(void), unsigned int priority)
{
    if (task_count >= NUM_TASKS)
        return -1;

    int id = task_count++;   /* post-increment: id = old value, then count++ */

    unsigned long *stack_top = (unsigned long *)&task_stacks[id][TASK_STACK_SIZE];

    init_task_stack(stack_top, func, id);

    tasks[id].priority      = priority;
    tasks[id].base_priority = priority;   /* save original for inheritance restore */
    tasks[id].state         = READY;
    tasks[id].name          = name;
    tasks[id].sleep_ticks   = 0;
    tasks[id].deadline      = 0;

    return id;
}

/* ---------------------------------------------------------------
 * Task functions (defined here for now; move to main.c later)
 * --------------------------------------------------------------- */
static void task1(void) {
    while (1) {
        uart_puts("task1 (prio 0 - HIGH) running\n");
        task_sleep(5);    /* 5 ticks × 100ms = 0.5s */
    }
}

static void task2(void) {
    while (1) {
        uart_puts("task2 (prio 1 - MED)  running\n");
        task_sleep(10);   /* 10 ticks × 100ms = 1.0s */
    }
}

static void task3(void) {
    while (1) {
        uart_puts("task3 (prio 2 - LOW)  running\n");
        task_sleep(20);   /* 20 ticks × 100ms = 2.0s */
    }
}

/* ---------------------------------------------------------------
 * scheduler_init
 *   Create the initial tasks via task_create().
 * --------------------------------------------------------------- */
void scheduler_init(void)
{
    uart_puts("[Scheduler] Initializing tasks...\n");

    task_create("task1", task1, 0);   /* highest priority */
    task_create("task2", task2, 1);
    task_create("task3", task3, 2);   /* lowest priority  */

    /* Mark the first task as RUNNING (it will be started by scheduler_start) */
    tasks[0].state = RUNNING;
    current_task   = 0;

    uart_puts("[Scheduler] Tasks initialized.\n");
}

/* ---------------------------------------------------------------
 * scheduler_switch_task  (Priority Round-Robin)
 *
 *   Called from the timer IRQ handler with the old stack pointer.
 *   Returns the new stack pointer to restore.
 *
 *   Algorithm:
 *   1. Save old_sp into current task's TCB.
 *   2. If current task was RUNNING, demote it back to READY
 *      (it was preempted — it's still alive, just not running).
 *   3. Scan all tasks starting AFTER current_task (round-robin
 *      tie-break) to find the READY task with the lowest priority
 *      number (= highest urgency).
 *   4. Mark winner as RUNNING, update current_task, return its sp.
 * --------------------------------------------------------------- */
unsigned long scheduler_switch_task(unsigned long old_sp)
{
    /* Step 1: save current stack pointer */
    tasks[current_task].sp = old_sp;

    /* Step 2: preempted task goes back to READY (unless BLOCKED/SLEEPING) */
    if (tasks[current_task].state == RUNNING)
        tasks[current_task].state = READY;

    /* Step 3: find highest-priority READY task
     *   - Scan starting from the task AFTER current (round-robin fairness)
     *   - Track best_priority (lowest number wins)
     *   - On a tie, the first one found in scan order wins
     */
    int   best_task     = -1;
    unsigned long best_priority = 0xFFFFFFFF;   /* worst possible */

    for (int offset = 1; offset <= task_count; offset++) {
        int i = (current_task + offset) % task_count;

        if (tasks[i].state == READY && tasks[i].priority < best_priority) {
            best_priority = tasks[i].priority;
            best_task     = i;
        }
    }

    /* Fallback: if nothing is READY, keep running current task */
    if (best_task == -1)
        best_task = current_task;

    /* Step 4: switch to winner */
    tasks[best_task].state = RUNNING;
    current_task           = best_task;

    return tasks[current_task].sp;
}

/* ---------------------------------------------------------------
 * scheduler_tick
 *   Called every timer IRQ. Decrements sleep counters and wakes
 *   tasks whose sleep has expired.
 * --------------------------------------------------------------- */
void scheduler_tick(void)
{
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].state == SLEEPING) {
            if (tasks[i].sleep_ticks > 0)
                tasks[i].sleep_ticks--;

            if (tasks[i].sleep_ticks == 0)
                tasks[i].state = READY;   /* wake up! */
        }
    }
}

/* ---------------------------------------------------------------
 * scheduler_start
 *   Jump into task1 to begin execution. The timer IRQ will
 *   preempt it and call scheduler_switch_task() from then on.
 * --------------------------------------------------------------- */
void scheduler_start(void)
{
    uart_puts("[Scheduler] Starting...\n");
    task1();
}

/* ---------------------------------------------------------------
 * Scheduler accessors (used by mutex.c)
 * --------------------------------------------------------------- */

int scheduler_get_current_task(void)
{
    return current_task;
}

void scheduler_set_task_state(int id, State_t state)
{
    if (id >= 0 && id < task_count)
        tasks[id].state = state;
}

void scheduler_set_task_priority(int id, unsigned long prio)
{
    if (id >= 0 && id < task_count)
        tasks[id].priority = prio;
}

unsigned long scheduler_get_task_priority(int id)
{
    if (id >= 0 && id < task_count)
        return tasks[id].priority;
    return 0xFFFFFFFF;   /* invalid id → return worst priority */
}

void scheduler_restore_priority(int id)
{
    /* Undo priority inheritance: put priority back to what it was
     * before any mutex boosting. base_priority is never changed
     * by the scheduler — only priority is temporarily bumped.    */
    if (id >= 0 && id < task_count)
        tasks[id].priority = tasks[id].base_priority;
}

/* ---------------------------------------------------------------
 * scheduler_yield
 *   Voluntary context switch — task gives up CPU right now.
 *   Uses ARM64 SVC #0 instruction which triggers the Synchronous
 *   exception handler (same save/restore path as timer IRQ).
 *
 *   NOTE: For this to work, vectors.s must route SVC #0 to
 *   handle_irq() just like the timer IRQ does.
 *   As a simple alternative we use a direct C call here since
 *   our vectors.s already handles the stack save/restore.
 * --------------------------------------------------------------- */
void scheduler_yield(void)
{
    /* Trigger SVC exception — vectors.s will save context and
     * call handle_irq(), which calls scheduler_switch_task().    */
    asm volatile("svc #0");
}

void task_sleep(unsigned long ticks)
{
    int current_task = scheduler_get_current_task();
    
    tasks[current_task].sleep_ticks = ticks;
    
    tasks[current_task].state = SLEEPING;
    
    scheduler_yield();
}
