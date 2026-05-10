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

void mmu_setup_pagetable(void)
{
    /* --- L0 entry 0: points to L1 table ---
     * L0[0] covers VA 0x0000_0000 – 0x3FFF_FFFF (first 256GB slot)
     * Bits [47:12] = physical address of L1 table
     * Bits [1:0]   = 0b11 (valid table entry)
     */
    page_table_l0[0] = ((unsigned long)page_table_l1)
                       | PTE_VALID
                       | PTE_TABLE;

    /* --- L1 entry 0: 1GB block for device memory (GIC + UART) ---
     * Covers PA 0x0000_0000 – 0x3FFF_FFFF
     * Block entry: bit[1] = 0 (omit PTE_TABLE)
     */
    page_table_l1[0] = (0x00000000UL)
                       | PTE_VALID
                       | PTE_AF
                       | PTE_AP_RW
                       | ATTR_DEVICE;

    /* --- L1 entry 1: 1GB block for normal RAM ---
     * Covers PA 0x4000_0000 – 0x7FFF_FFFF
     */
    page_table_l1[1] = (0x40000000UL)
                       | PTE_VALID
                       | PTE_AF
                       | PTE_SH_INNER
                       | PTE_AP_RW
                       | ATTR_NORMAL;
}
