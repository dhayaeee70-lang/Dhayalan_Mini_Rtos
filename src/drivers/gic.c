#include "gic.h"

void gic_init(void) {
    // Enable System Register Access for GICv3
    asm volatile ("msr icc_sre_el1, %0" : : "r" (1)); 
    asm volatile ("isb");
    
    // Set priority mask
    asm volatile ("msr icc_pmr_el1, %0" : : "r" (0xFF));
    
    // Enable interrupt group 1
    asm volatile ("msr icc_igrpen1_el1, %0" : : "r" (1));
    
    // Enable IRQ 27 (virtual timer)
    *(volatile unsigned int *)(GICR_SGI_BASE + 0x0100) = (1 << 27);
}

unsigned int gic_acknowledge_irq(void) {
    unsigned int iar;
    asm volatile ("mrs %0, icc_iar1_el1" : "=r" (iar));
    return iar;
}

void gic_end_of_irq(unsigned int iar) {
    asm volatile ("msr icc_eoir1_el1, %0" : : "r" (iar));
}