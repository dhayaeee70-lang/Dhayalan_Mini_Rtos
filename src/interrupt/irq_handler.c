#include "irq_handler.h"
#include "../drivers/uart.h"
#include "../drivers/gic.h"
#include "../drivers/timer.h"
#include "../kernel/scheduler.h"

void handle_sync(void) {
    uart_puts("\n[SYNC] Synchronous Exception!\n");
    while(1);
}

unsigned long handle_irq(unsigned long old_sp) {
    // Acknowledge interrupt
    unsigned int iar = gic_acknowledge_irq();
    unsigned int irq_id = iar & 0xFFFFFF;
    
    if (irq_id == TIMER_IRQ_ID) {
        uart_puts("\n>>> [IRQ] TIMER TICK! <<<\n");
        
        // Perform task switch
        unsigned long new_sp = scheduler_switch_task(old_sp);
        
        // Reset timer for next tick
        timer_reset();
        
        // End of interrupt
        gic_end_of_irq(iar);
        
        return new_sp;
    } else {
        uart_puts("\n>>> [IRQ] Unknown Interrupt ID: ");
        // Could add number printing here
        uart_puts(" <<<\n");
        
        gic_end_of_irq(iar);
        return old_sp;
    }
}