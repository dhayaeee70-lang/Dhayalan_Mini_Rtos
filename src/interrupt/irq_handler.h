#ifndef IRQ_HANDLER_H
#define IRQ_HANDLER_H

/* Called from vectors.s curr_el_spx_irq (timer IRQ) */
unsigned long handle_irq(unsigned long old_sp);

/* Called from vectors.s curr_el_spx_sync (SVC #0 — scheduler_yield) */
unsigned long handle_svc(unsigned long old_sp);

#endif /* IRQ_HANDLER_H */
