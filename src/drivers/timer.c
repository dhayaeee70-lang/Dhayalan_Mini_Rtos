#include "timer.h"

void timer_init(void) {
    unsigned long cntfrq;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    
    // Set timer to fire every 0.5 seconds
    asm volatile ("msr cntv_tval_el0, %0" : : "r" (cntfrq / 2));
    
    // Enable timer
    asm volatile ("msr cntv_ctl_el0, %0" : : "r" (1));
}

void timer_reset(void) {
    unsigned long cntfrq;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    asm volatile ("msr cntv_tval_el0, %0" : : "r" (cntfrq / 2));
}

unsigned long timer_get_frequency(void) {
    unsigned long cntfrq;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    return cntfrq;
}