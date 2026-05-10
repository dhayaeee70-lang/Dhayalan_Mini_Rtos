#include "../drivers/uart.h"
#include "../drivers/gic.h"
#include "../drivers/timer.h"
#include "scheduler.h"
#include "mmu.h"

static void enable_interrupts(void) {
    /* Unmask all exceptions (Debug, SError, IRQ, FIQ) */
    asm volatile ("msr daifclr, #15");
}

void main(void) {
    /* Print banner */
    uart_puts("\n");
    uart_puts("=====================================\n");
    uart_puts("  ARM Bare-Metal OS - v1.0\n");
    uart_puts("  Priority Preemptive Scheduler\n");
    uart_puts("=====================================\n\n");

    uart_puts("[Main] Initializing subsystems...\n");

    /* --- MMU setup (must be before anything that needs caches) --- */
    uart_puts("[MMU] Setting up page tables...\n");
    mmu_setup_pagetable();
    uart_puts("[MMU] Enabling MMU...\n");
    mmu_enable();
    uart_puts("[MMU] MMU enabled. Running with virtual memory.\n");

    scheduler_init();
    gic_init();
    timer_init();

    uart_puts("[Main] Enabling interrupts...\n");
    enable_interrupts();

    uart_puts("[Main] System ready. Starting scheduler...\n\n");

    /* Start the scheduler (never returns) */
    scheduler_start();

    while(1);
}
