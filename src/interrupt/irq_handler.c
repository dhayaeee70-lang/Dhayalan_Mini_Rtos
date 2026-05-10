#include "irq_handler.h"
#include "../drivers/uart.h"
#include "../drivers/gic.h"
#include "../drivers/timer.h"
#include "../kernel/scheduler.h"

/* handle_svc — called from vectors.s curr_el_spx_sync (SVC #0)
 * Performs an immediate voluntary context switch (scheduler_yield).
 * Does NOT acknowledge a GIC interrupt — SVC is a software trap.  */
unsigned long handle_svc(unsigned long old_sp) {
    scheduler_tick();                        /* still count the tick */
    return scheduler_switch_task(old_sp);    /* pick next READY task */
}

unsigned long handle_irq(unsigned long old_sp) {
    // Acknowledge interrupt
    unsigned int iar = gic_acknowledge_irq();
    unsigned int irq_id = iar & 0xFFFFFF;
    
    if (irq_id == TIMER_IRQ_ID) {
        /* wakeup sleeping tasks if tick expired */
        scheduler_tick();
        
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
