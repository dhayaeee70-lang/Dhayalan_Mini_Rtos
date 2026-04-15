#ifndef IRQ_HANDLER_H
#define IRQ_HANDLER_H

// Main IRQ handler called from vectors.s
unsigned long handle_irq(unsigned long old_sp);

// Exception handlers
void handle_sync(void);

#endif