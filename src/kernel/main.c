#include "../drivers/uart.h"
#include "../drivers/gic.h"
#include "../drivers/timer.h"
#include "scheduler.h"

static void enable_interrupts(void) {
    // Unmask all exceptions (Debug, SError, IRQ, FIQ)
    asm volatile ("msr daifclr, #15");
}

void main(void) {
    // Print banner
    uart_puts("\n");
    uart_puts("=====================================\n");
    uart_puts("  ARM Bare-Metal OS - v1.0\n");
    uart_puts("  3-Task Round-Robin Scheduler\n");
    uart_puts("=====================================\n\n");
    
    // Initialize all subsystems
    uart_puts("[Main] Initializing subsystems...\n");
    
    scheduler_init();
    gic_init();
    timer_init();
    
    uart_puts("[Main] Enabling interrupts...\n");
    enable_interrupts();
    
    uart_puts("[Main] System ready. Starting scheduler...\n\n");
    
    // Start the scheduler (never returns)
    scheduler_start();
    
    // Should never reach here
    while(1);
}