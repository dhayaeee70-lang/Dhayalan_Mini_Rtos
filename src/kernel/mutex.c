#include "mutex.h"
#include "../drivers/uart.h"

/* ---------------------------------------------------------------
 * mutex_init
 *   Must be called before first use of a mutex.
 *   Sets it to the unlocked state with no owner or waiter.
 * --------------------------------------------------------------- */
void mutex_init(Mutex_t *m)
{
    m->locked    = 0;
    m->owner_id  = -1;
    m->waiter_id = -1;
}

/* ---------------------------------------------------------------
 * mutex_lock  — with Priority Inheritance Protocol
 *
 *   Case A: mutex is FREE
 *     → take it immediately, record owner, return.
 *
 *   Case B: mutex is HELD by another task
 *     → Priority Inheritance:
 *        If the owner's current priority is LOWER (higher number)
 *        than ours, bump the owner's priority up to ours so it
 *        can finish the critical section without being preempted
 *        by medium-priority tasks.
 *     → Record ourselves as the waiter.
 *     → Block ourselves (BLOCKED state).
 *     → Yield CPU — scheduler picks the owner (now boosted) to run.
 *
 *   When we eventually wake up (mutex_unlock set us READY and
 *   gave us the mutex), we simply return.
 * --------------------------------------------------------------- */
void mutex_lock(Mutex_t *m)
{
    int caller = scheduler_get_current_task();

    if (!m->locked) {
        /* ---- Case A: mutex is free — take it ---- */
        m->locked   = 1;
        m->owner_id = caller;
        /* waiter_id stays -1 */
        return;
    }

    /* ---- Case B: mutex is held by someone else ---- */

    /* Safety: don't allow the same task to lock twice (deadlock) */
    if (m->owner_id == caller) {
        uart_puts("[MUTEX] ERROR: recursive lock detected!\n");
        return;
    }

    int owner = m->owner_id;

    /* --- Priority Inheritance ---
     * Get the TCB priority of owner via scheduler accessor.
     * We stored priority as unsigned long; compare numerically.
     * Lower number = higher urgency.
     * If owner's priority number > caller's priority number,
     * owner is lower-urgency → boost it to caller's level.
     *
     * We read owner priority indirectly: we set it via
     * scheduler_set_task_priority() and trust the scheduler
     * already stored it. We need to READ it too — add a getter,
     * or simply compare: if owner_priority > caller_priority,
     * boost. We use a simple approach: always set owner priority
     * to caller's priority if caller has higher urgency.
     * The base_priority in TCB ensures we can restore it later.
     *
     * Since we don't have a getter yet, we use the fact that
     * scheduler_set_task_priority() is a setter. We pass the
     * caller's priority as the new owner priority — this is safe
     * because mutex_unlock() restores base_priority.
     *
     * For a real RTOS you'd expose a getter; here we keep it
     * simple and always apply the boost.
     */
    /* Boost owner's priority to caller's priority if caller is more urgent.
     * Lower number = higher urgency (0 = highest).
     * Only boost if owner is currently lower-urgency than caller.          */
    unsigned long caller_prio = scheduler_get_task_priority(caller);
    unsigned long owner_prio  = scheduler_get_task_priority(owner);

    uart_puts("[MUTEX] Priority inheritance: boosting owner\n");
    if (owner_prio > caller_prio)
        scheduler_set_task_priority(owner, caller_prio);

    /* Record us as the waiter */
    m->waiter_id = caller;

    /* Block ourselves — we cannot run until mutex is released */
    scheduler_set_task_state(caller, BLOCKED);

    /* Yield CPU immediately — owner (now boosted) will be picked */
    scheduler_yield();

    /* ---- We resume here after mutex_unlock() wakes us ----
     * At this point:
     *   - mutex is ours (mutex_unlock transferred ownership to us)
     *   - our state is RUNNING again
     *   - owner's priority has been restored to base_priority
     */
}

/* ---------------------------------------------------------------
 * mutex_unlock  — release the mutex and restore priorities
 *
 *   Steps:
 *   1. Restore owner's priority to base_priority (undo inheritance).
 *   2. If there is a waiter:
 *        a. Transfer mutex ownership to the waiter.
 *        b. Wake the waiter (set state → READY).
 *        c. Clear waiter_id.
 *   3. If no waiter: just unlock (locked=0, owner_id=-1).
 * --------------------------------------------------------------- */
void mutex_unlock(Mutex_t *m)
{
    int owner = m->owner_id;

    if (owner == -1) {
        uart_puts("[MUTEX] ERROR: unlock called on free mutex!\n");
        return;
    }

    /* Step 1: Restore owner's priority to its original base value.
     * We need the base_priority — expose it via a new accessor, or
     * for simplicity restore to a known value. In a full system:
     *   scheduler_restore_base_priority(owner);
     * Here we call set_priority with the base value. Since we don't
     * have a getter for base_priority, we use a workaround:
     * scheduler_restore_priority() is the clean solution.
     * For now, we demonstrate the concept — the key line is:
     */
    scheduler_restore_priority(owner);
    uart_puts("[MUTEX] Restoring owner priority\n");

    /* Step 2: Transfer to waiter or just free */
    if (m->waiter_id != -1) {
        int waiter = m->waiter_id;

        /* Transfer ownership */
        m->owner_id  = waiter;
        m->waiter_id = -1;
        /* mutex stays locked=1, new owner is waiter */

        /* Wake the waiter — scheduler will pick it up */
        scheduler_set_task_state(waiter, READY);

        uart_puts("[MUTEX] Transferred to waiter, waiter is now READY\n");
    } else {
        /* No one waiting — just release */
        m->locked   = 0;
        m->owner_id = -1;
        uart_puts("[MUTEX] Released (no waiter)\n");
    }
}
