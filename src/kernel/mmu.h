#ifndef MMU_H
#define MMU_H

/* Set up identity-mapped page tables (VA == PA) for kernel + MMIO */
void mmu_setup_pagetable(void);

/* Returns pointer to L0 page table (used by mmu_enable in assembly) */
unsigned long *mmu_get_l0_table(void);

/* Enable the MMU — implemented in mmu.s */
void mmu_enable(void);

#endif /* MMU_H */
