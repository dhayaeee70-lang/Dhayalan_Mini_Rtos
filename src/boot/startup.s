.section ".text.boot"       /* Tells the linker to put this code at the very beginning of the binary */
.global _start              /* Makes the label '_start' visible to the linker */

_start:
    /* --- 1. Setup EL3 Stack --- */
    ldr x0, =_stack_top     /* Load the 64-bit memory address labeled '_stack_top' into register x0 */
    mov sp, x0              /* Copy the value in x0 into 'sp' (the Stack Pointer register) */

    /* --- 2. Configure SCR_EL3 (Secure Configuration Register) --- */
    ldr x0, =0x431          /* Load the hex value 0x431 into x0. This configures Security and Bit-width */
    msr scr_el3, x0         /* Write the value from x0 into the special SCR_EL3 system register */

    /* --- 3. Configure GICv3 (Interrupt Controller) from Secure World --- */
    
    /* Enable System Register Access for GICv3 (ICC_SRE_EL3) */
    mov x0, #15             /* Move the decimal number 15 (binary 1111) into x0 */
    msr icc_sre_el3, x0     /* Write x0 into ICC_SRE_EL3 to unlock GIC system registers */
    isb                     /* Instruction Synchronization Barrier: Forces the CPU to wait until the unlock is finished */

    /* Configure GIC Distributor (GICD_CTLR) */
    ldr x0, =0x08000000     /* Load the base memory address of the GIC Distributor into x0 */
    mov w1, #0x37           /* Move hex 0x37 into the 32-bit register w1 (Enables interrupt groups) */
    str w1, [x0]            /* Store the 32-bit value in w1 into the memory address pointed to by x0 */

    /* Configure Redistributor for Core 0 (GICR_WAKER) */
    ldr x0, =0x080A0014     /* Load the memory address of the Redistributor Wake register into x0 */
    str wzr, [x0]           /* Store 'wzr' (a special register that always holds 0) into that memory address to wake it up */

    /* Set Virtual Timer (IRQ 27) to Group 1 Non-Secure */
    ldr x0, =0x080B0080     /* Load the memory address of GICR_IGROUPR0 into x0 */
    ldr w1, [x0]            /* Load the 32-bit value currently at that memory address into w1 */
    orr w1, w1, #(1 << 27)  /* Perform a Bitwise OR on w1 with a 1 shifted left by 27 (sets bit 27 to 1) */
    str w1, [x0]            /* Store the modified w1 back into the memory address */

    /* --- 4. Drop to EL1 (Kernel Mode) --- */
    mov x0, #0x3c5          /* Move hex 0x3c5 into x0. This is the code for 'EL1 mode with interrupts masked' */
    msr spsr_el3, x0        /* Write x0 into SPSR_EL3 (Saved Program Status Register) */
    
    adr x0, el1_entry       /* Get the memory address of the 'el1_entry' label below and put it in x0 */
    msr elr_el3, x0         /* Write x0 into ELR_EL3 (Exception Link Register). This is where the CPU will 'return' to */
    
    eret                    /* Exception Return: The CPU looks at SPSR_EL3 and ELR_EL3, changes privilege to EL1, and jumps to el1_entry */

el1_entry:
    /* --- We are now in EL1 --- */
    /* Set EL1 Stack */
    ldr x0, =_stack_top     /* Load the stack top address into x0 again */
    sub x0, x0, #0x2000     /* Subtract 0x2000 (8KB) from x0 so EL1 gets its own isolated stack space */
    mov sp, x0              /* Set the EL1 Stack Pointer to this new address */

    /* Set Vector Table */
    ldr x0, =vectors        /* Load the memory address of the 'vectors' table (from vectors.s) into x0 */
    msr vbar_el1, x0        /* Write x0 into VBAR_EL1 (Vector Base Address Register). Now the CPU knows where to go on an interrupt! */

    bl main                 /* Branch with Link: Call the C function 'main'. The 'Link' saves the return address just in case. */

halt:
    wfe                     /* Wait For Event: Puts the CPU to sleep to save power */
    b halt                  /* Branch (jump) back to the 'halt' label to create an infinite loop */