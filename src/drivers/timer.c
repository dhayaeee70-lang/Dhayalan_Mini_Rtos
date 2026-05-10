#include "timer.h"

void timer_init(void) {
    unsigned long cntfrq;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    
    // Set timer to fire every 100ms (10 ticks per second)
    asm volatile ("msr cntv_tval_el0, %0" : : "r" (cntfrq / 10));
    
    // Enable timer
    asm volatile ("msr cntv_ctl_el0, %0" : : "r" (1));
}

void timer_reset(void) {
    unsigned long cntfrq;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    asm volatile ("msr cntv_tval_el0, %0" : : "r" (cntfrq / 10));
}

unsigned long timer_get_frequency(void) {
    unsigned long cntfrq;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    return cntfrq;
}
