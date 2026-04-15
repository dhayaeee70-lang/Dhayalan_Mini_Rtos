.section ".text"
.align 11
.global vectors

vectors:
    /* --- 0x000: SP0 --- */
    .align 7
    b .
    .align 7
    b .
    .align 7
    b .
    .align 7
    b .

    /* --- 0x200: SPx (Synchronous / SVC) --- */
    .align 7
    curr_el_spx_sync:
        b handle_sync
        
    /* --- 0x280: SPx (Hardware Interrupts / Timer) --- */
    .align 7
    curr_el_spx_irq:
        /* 1. SAVE THE INTERRUPTED TASK'S STATE */
        /* Make room for 34 registers (x0-x30, PC, PSTATE) = 272 bytes */
        sub sp, sp, #272
        
        stp x0, x1, [sp, #0]
        stp x2, x3, [sp, #16]
        stp x4, x5, [sp, #32]
        stp x6, x7, [sp, #48]
        stp x8, x9, [sp, #64]
        stp x10, x11, [sp, #80]
        stp x12, x13, [sp, #96]
        stp x14, x15, [sp, #112]
        stp x16, x17, [sp, #128]
        stp x18, x19, [sp, #144]
        stp x20, x21, [sp, #160]
        stp x22, x23, [sp, #176]
        stp x24, x25, [sp, #192]
        stp x26, x27, [sp, #208]
        stp x28, x29, [sp, #224]
        
        /* Save Link Register (x30). Second slot is empty for 16-byte alignment */
        str x30, [sp, #240]

        /* Save Program Counter (ELR) and CPU Status (SPSR) */
        mrs x0, elr_el1
        mrs x1, spsr_el1
        stp x0, x1, [sp, #256]

        /* 2. CALL THE C SCHEDULER */
        mov x0, sp                 /* Arg 1 (x0) = The old stack pointer */
        bl handle_irq              /* C function returns the new stack pointer in x0 */
        
        mov sp, x0                 /* SWITCH THE CPU TO THE NEW TASK'S STACK! */

        /* 3. RESTORE THE NEW TASK'S STATE */
        ldp x0, x1, [sp, #256]
        msr elr_el1, x0            /* Restore the Program Counter */
        msr spsr_el1, x1           /* Restore the CPU Status */

        ldr x30, [sp, #240]
        
        ldp x28, x29, [sp, #224]
        ldp x26, x27, [sp, #208]
        ldp x24, x25, [sp, #192]
        ldp x22, x23, [sp, #176]
        ldp x20, x21, [sp, #160]
        ldp x18, x19, [sp, #144]
        ldp x16, x17, [sp, #128]
        ldp x14, x15, [sp, #112]
        ldp x12, x13, [sp, #96]
        ldp x10, x11, [sp, #80]
        ldp x8, x9, [sp, #64]
        ldp x6, x7, [sp, #48]
        ldp x4, x5, [sp, #32]
        ldp x2, x3, [sp, #16]
        ldp x0, x1, [sp, #0]
        
        add sp, sp, #272           /* Free the stack space */
        eret                       /* Jump into the new task! */
        
    .align 7
    b . /* FIQ */
    .align 7
    b . /* SError */

    /* --- Lower ELs --- */
    .align 7
    .rept 8
    b .
    .align 7
    .endr