.section ".text"
.global mmu_enable

/* ---------------------------------------------------------------
 * mmu_enable
 *   Called from main.c AFTER mmu_setup_pagetable().
 *   Configures ARM64 MMU system registers and turns on the MMU.
 *
 *   Steps:
 *   1. MAIR_EL1  — define memory attribute types
 *   2. TCR_EL1   — translation control (VA size, granule, cacheability)
 *   3. TTBR0_EL1 — point to our L0 page table
 *   4. SCTLR_EL1 — set bit 0 to enable MMU
 *   5. ISB        — flush pipeline so next fetch uses new translation
 * --------------------------------------------------------------- */
mmu_enable:
    /* Save link register — we call mmu_get_l0_table() which clobbers x30 */
    stp x29, x30, [sp, #-16]!
    mov x29, sp

    /* ---- Step 1: MAIR_EL1 ----
     * Define two memory attribute types:
     *   Attr0 (index 0) = 0xFF = Normal memory, Write-Back Cacheable
     *   Attr1 (index 1) = 0x00 = Device memory, nGnRnE (strongly ordered)
     *
     * MAIR_EL1 layout: [63:56]=Attr7 ... [15:8]=Attr1 [7:0]=Attr0
     * We set: Attr0=0xFF, Attr1=0x00 → value = 0x000000000000FF
     */
    ldr  x0, =0x00FF
    msr  mair_el1, x0

    /* ---- Step 2: TCR_EL1 ----
     * Configure the translation system:
     *   T0SZ = 16  → TTBR0 covers 2^(64-16) = 48-bit VA space (bits[5:0])
     *              → 4-level translation: L0→L1→L2→L3 (matches our page table)
     *   IRGN0 = 01 → Inner Write-Back cacheable (bits[9:8])
     *   ORGN0 = 01 → Outer Write-Back cacheable (bits[11:10])
     *   SH0   = 11 → Inner Shareable (bits[13:12])
     *   TG0   = 00 → 4KB granule (bits[15:14])
     *   IPS   = 001→ 36-bit physical address size (bits[34:32])
     *
     * Value: 0x0000000100003510
     *   0x10 = T0SZ=16 (48-bit VA)
     *   0x3500 = SH0=11, ORGN0=01, IRGN0=01
     *   0x100000000 = IPS=001 (36-bit PA, covers 0x40000000 RAM)
     */
    ldr  x0, =0x0000000100003510
    msr  tcr_el1, x0
    isb

    /* ---- Step 3: TTBR0_EL1 ----
     * Point to our L0 page table.
     * mmu_get_l0_table() returns the address of page_table_l0[].
     */
    bl   mmu_get_l0_table    /* x0 = address of L0 page table */
    msr  ttbr0_el1, x0
    isb

    /* ---- Step 4 + 5: Enable MMU via SCTLR_EL1 ----
     * SCTLR_EL1 bit 0 = M (MMU enable)
     * SCTLR_EL1 bit 2 = C (data cache enable)
     * SCTLR_EL1 bit 12 = I (instruction cache enable)
     *
     * We enable MMU + both caches for performance.
     * Identity mapping ensures PC remains valid after MMU turns on.
     */
    mrs  x0, sctlr_el1
    orr  x0, x0, #(1 << 0)    /* M: enable MMU */
    orr  x0, x0, #(1 << 2)    /* C: enable D-cache */
    orr  x0, x0, #(1 << 12)   /* I: enable I-cache */
    msr  sctlr_el1, x0
    isb                         /* flush pipeline — next fetch is translated */

    /* Restore link register and return */
    ldp x29, x30, [sp], #16
    ret
