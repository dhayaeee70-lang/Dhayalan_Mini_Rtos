#ifndef GIC_H
#define GIC_H

#define GICR_SGI_BASE 0x080B0000

void gic_init(void);
unsigned int gic_acknowledge_irq(void);
void gic_end_of_irq(unsigned int iar);

#endif