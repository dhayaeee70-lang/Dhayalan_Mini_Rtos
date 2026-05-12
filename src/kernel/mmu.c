#include "mmu.h"

static unsigned long page_table_l0[512] __attribute__((aligned(4096)));
static unsigned long page_table_l1[512] __attribute__((aligned(4096)));

/* ARM64 Page Table Entry attribute bits */
#define PTE_VALID       (1UL << 0)   /* entry is valid */
#define PTE_TABLE       (1UL << 1)   /* 1=table/page, 0=block */
#define PTE_AF          (1UL << 10)  /* Access Flag — must be set */
#define PTE_SH_INNER    (3UL << 8)   /* Inner shareable */
#define PTE_AP_RW       (0UL << 6)   /* kernel read/write */

/* AttrIdx — index into MAIR_EL1 */
#define ATTR_NORMAL     (0UL << 2)   /* index 0 = normal memory */
#define ATTR_DEVICE     (1UL << 2)   /* index 1 = device memory */

/* Expose page table base address for mmu.s */
unsigned long *mmu_get_l0_table(void)
{
    return page_table_l0;
}

/* ---------------------------------------------------------------
 * mmu_setup_pagetable
 *   Identity map (VA == PA) for kernel + MMIO using 1GB block entries.
 *
 *   Memory map (QEMU virt):
 *     0x00000000 – 0x3FFFFFFF  device memory (GIC @ 0x08000000, UART @ 0x09000000)
 *     0x40000000 – 0x7FFFFFFF  normal RAM    (kernel loads at 0x40000000)
 *
 *   Page table structure:
 *     L0[0] → L1 table
 *     L1[0] → 1GB block @ 0x00000000 (device, nGnRnE)
 *     L1[1] → 1GB block @ 0x40000000 (normal, WB cacheable)
 * --------------------------------------------------------------- */
void mmu_setup_pagetable(void)
{
    /* L0[0]: table entry pointing to L1 */
    page_table_l0[0] = ((unsigned long)page_table_l1)
                       | PTE_VALID
                       | PTE_TABLE;

    /* L1[0]: 1GB block — device memory (GIC + UART) */
    page_table_l1[0] = (0x00000000UL)
                       | PTE_VALID
                       | PTE_AF
                       | PTE_AP_RW
                       | ATTR_DEVICE;

    /* L1[1]: 1GB block — normal RAM */
    page_table_l1[1] = (0x40000000UL)
                       | PTE_VALID
                       | PTE_AF
                       | PTE_SH_INNER
                       | PTE_AP_RW
                       | ATTR_NORMAL;
}
