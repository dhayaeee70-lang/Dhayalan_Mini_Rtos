================================================================================
  Dhayalan Mini RTOS
  ARM64 Bare-Metal OS with 3-Task Round-Robin Scheduler
================================================================================

OVERVIEW
--------
Dhayalan Mini RTOS is a bare-metal operating system kernel written in C and
ARM64 assembly, targeting the QEMU "virt" machine. It demonstrates a preemptive
round-robin scheduler driven by the ARM virtual timer (IRQ 27) and the GICv3
interrupt controller. Three tasks run concurrently, each printing to the UART
console, and the CPU switches between them every 0.5 seconds.

FEATURES
--------
  - ARM64 (AArch64) bare-metal boot from EL3 down to EL1
  - Preemptive round-robin scheduler (3 tasks, timer-driven)
  - Full CPU context save/restore on every task switch (34 registers)
  - GICv3 interrupt controller driver
  - ARM virtual timer driver (fires every 0.5 s)
  - PL011 UART driver for serial output
  - Custom linker script and Makefile for cross-compilation
  - Runs on QEMU virt machine (no real hardware required)

REPOSITORY STRUCTURE
--------------------
  Dhayalan_Mini_Rtos/
  ├── src/
  │   ├── boot/
  │   │   ├── startup.s          Entry point; EL3 → EL1 transition, stack &
  │   │   │                      vector-table setup, calls main()
  │   │   └── vectors.s          ARM64 exception vector table; full context
  │   │                          save/restore on IRQ, calls handle_irq()
  │   ├── drivers/
  │   │   ├── uart.c / uart.h    PL011 UART driver (base 0x09000000)
  │   │   ├── gic.c  / gic.h     GICv3 interrupt controller driver
  │   │   └── timer.c/ timer.h   ARM virtual timer driver (IRQ 27, 0.5 s tick)
  │   ├── include/
  │   │   └── types.h            TCB (Task Control Block) type definition
  │   ├── interrupt/
  │   │   ├── irq_handler.c      IRQ dispatcher: timer tick → task switch
  │   │   └── irq_handler.h      Declares handle_irq() and handle_sync()
  │   └── kernel/
  │       ├── main.c             System entry; initialises subsystems, starts
  │       │                      scheduler
  │       ├── scheduler.c        Round-robin scheduler; task stack init
  │       └── scheduler.h        Scheduler public API
  ├── linker.ld                  Linker script (load address 0x40000000,
  │                              16 KB stack)
  └── makefile                   Cross-compilation build rules

SOURCE FILE DETAILS
-------------------

src/boot/startup.s
  - First code to execute (_start, placed in .text.boot)
  - Sets up EL3 stack pointer to _stack_top
  - Configures SCR_EL3 = 0x431 (NS bit + RW bit for AArch64)
  - Unlocks GICv3 system registers from EL3 (ICC_SRE_EL3)
  - Configures GICD_CTLR (0x08000000) to enable interrupt groups
  - Wakes GIC Redistributor for core 0 (GICR_WAKER at 0x080A0014)
  - Sets virtual timer IRQ 27 to Group 1 Non-Secure (GICR_IGROUPR0)
  - Drops to EL1 via SPSR_EL3 / ELR_EL3 / ERET
  - Sets EL1 stack 8 KB below EL3 stack
  - Loads vector table address into VBAR_EL1
  - Branches to main()

src/boot/vectors.s
  - ARM64 vector table aligned to 2 KB (.align 11)
  - SPx Synchronous exception  → branches to handle_sync()
  - SPx IRQ (hardware interrupt) handler:
      1. Allocates 272 bytes on the stack
      2. Saves all general-purpose registers x0–x30
      3. Saves ELR_EL1 (program counter) and SPSR_EL1 (CPU state)
      4. Passes old SP to handle_irq(); receives new SP in return
      5. Switches stack pointer to the new task's stack
      6. Restores the new task's registers, ELR, SPSR
      7. ERET — resumes the new task

src/drivers/uart.h / uart.c
  - UART_DATA_REG : 0x09000000  (write a character here)
  - UART_FLAG_REG : 0x09000018  (bit 5 = TX FIFO full)
  - uart_putc(char c)  : waits until TX FIFO has space, then writes c
  - uart_puts(char *s) : calls uart_putc() for every character in s

src/drivers/gic.h / gic.c
  - GICR_SGI_BASE : 0x080B0000
  - gic_init()
      * Enables GICv3 system register access (ICC_SRE_EL1)
      * Sets priority mask to 0xFF (ICC_PMR_EL1) — all priorities allowed
      * Enables interrupt group 1 (ICC_IGRPEN1_EL1)
      * Enables IRQ 27 (virtual timer) in GICR_ISENABLER0
  - gic_acknowledge_irq() : reads ICC_IAR1_EL1, returns IAR value
  - gic_end_of_irq(iar)   : writes IAR to ICC_EOIR1_EL1

src/drivers/timer.h / timer.c
  - TIMER_IRQ_ID : 27  (ARM virtual timer)
  - timer_init()
      * Reads system counter frequency from CNTFRQ_EL0
      * Programs CNTV_TVAL_EL0 = cntfrq / 2  (fires every 0.5 seconds)
      * Enables the virtual timer via CNTV_CTL_EL0 = 1
  - timer_reset()         : reloads CNTV_TVAL_EL0 for the next tick
  - timer_get_frequency() : returns CNTFRQ_EL0

src/include/types.h
  - Defines the Task Control Block (TCB):
      typedef struct { unsigned long sp; } TCB;
    sp holds the saved stack pointer of a suspended task.

src/interrupt/irq_handler.h / irq_handler.c
  - handle_sync()
      * Called on a synchronous exception; prints an error message and halts.
  - handle_irq(unsigned long old_sp) → unsigned long new_sp
      * Acknowledges the interrupt (gic_acknowledge_irq)
      * If IRQ ID == TIMER_IRQ_ID (27):
          - Calls scheduler_switch_task(old_sp) to get the new task's SP
          - Resets the timer (timer_reset)
          - Signals end-of-interrupt (gic_end_of_irq)
          - Returns new_sp so vectors.s can switch stacks
      * Unknown IRQ: prints a warning, signals EOI, returns old_sp unchanged

src/kernel/scheduler.h
  - NUM_TASKS       : 3
  - TASK_STACK_SIZE : 2048 bytes
  - scheduler_init(void)
  - scheduler_switch_task(unsigned long old_sp) → unsigned long new_sp
  - scheduler_start(void)

src/kernel/scheduler.c
  - Static task functions: task1(), task2(), task3()
      Each loops forever, printing "taskN running...\n" then busy-waiting
      4 000 000 NOP cycles before printing again.
  - init_task_stack(stack_top, task_func, task_num)
      Initialises a task's stack frame:
        * Allocates 34 slots (x0–x30 = 0, ELR = task_func, SPSR = 0x345)
        * Stores the resulting SP in tasks[task_num].sp
  - scheduler_init()
      * Task 1 uses the main stack (already set up by startup.s)
      * Calls init_task_stack() for task 2 (task_2_stack) and task 3
        (task_3_stack); both stacks are 16-byte aligned
  - scheduler_switch_task(old_sp)
      * Saves old_sp into tasks[current_task].sp
      * Advances current_task = (current_task + 1) % NUM_TASKS
      * Returns tasks[current_task].sp
  - scheduler_start()
      * Prints "[Scheduler] Starting Task 1..." and calls task1()

src/kernel/main.c
  - enable_interrupts() : executes "msr daifclr, #15" to unmask all
    exceptions (Debug, SError, IRQ, FIQ)
  - main()
      1. Prints startup banner:
           ARM Bare-Metal OS - v1.0
           3-Task Round-Robin Scheduler
      2. Calls scheduler_init(), gic_init(), timer_init()
      3. Calls enable_interrupts()
      4. Calls scheduler_start()  ← never returns

linker.ld
  - Entry point : _start
  - Load address : 0x40000000
  - Sections     : .text (boot code first), .data, .bss
  - Stack        : 16 KB reserved above .bss; _stack_top marks the top

makefile
  - Cross-compiler : aarch64-linux-gnu-gcc / aarch64-linux-gnu-ld
  - CFLAGS         : -Wall -O2 -ffreestanding -nostdlib -nostartfiles
  - Output         : build/kernel8.elf

BUILD REQUIREMENTS
------------------
  1. aarch64-linux-gnu cross-compiler toolchain
       Ubuntu/Debian: sudo apt install gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu
  2. QEMU with AArch64 support
       Ubuntu/Debian: sudo apt install qemu-system-aarch64

BUILD & RUN
-----------
  Build:
    make

  Run in QEMU:
    make run
    (equivalent to: qemu-system-aarch64 -M virt,secure=on,gic-version=3
                     -cpu cortex-a57 -nographic -kernel build/kernel8.elf)

  Run with debug output (interrupts + CPU resets):
    make run-debug

  Clean build artefacts:
    make clean

EXPECTED OUTPUT
---------------
  When running, the UART console should display something like:

    ======================================
      ARM Bare-Metal OS - v1.0
      3-Task Round-Robin Scheduler
    ======================================

    [Main] Initializing subsystems...
    [Scheduler] Initializing tasks...
    [Scheduler] Tasks initialized.
    [Main] Enabling interrupts...
    [Main] System ready. Starting scheduler...

    [Scheduler] Starting Task 1...
    task1 running...
    task1 running...

    >>> [IRQ] TIMER TICK! <<<
    task2 running...
    task2 running...

    >>> [IRQ] TIMER TICK! <<<
    task3 running...
    ...

  The scheduler switches tasks every 0.5 seconds on each timer tick.

HOW IT WORKS — TASK SWITCH FLOW
---------------------------------
  1. ARM virtual timer fires → CPU jumps to curr_el_spx_irq in vectors.s
  2. vectors.s saves all 34 registers of the interrupted task onto its stack
  3. handle_irq() is called with the old stack pointer
  4. handle_irq() calls scheduler_switch_task() which:
       - Saves old_sp into the current task's TCB
       - Increments current_task (mod 3)
       - Returns the new task's saved SP from its TCB
  5. handle_irq() resets the timer and signals EOI to the GIC
  6. vectors.s switches the CPU stack pointer to the new task's stack
  7. vectors.s restores the new task's registers from its stack
  8. ERET resumes the new task exactly where it was interrupted

HARDWARE / EMULATION TARGET
-----------------------------
  Machine  : QEMU virt (secure=on, gic-version=3)
  CPU      : Cortex-A57 (AArch64)
  UART     : PL011 at 0x09000000
  GIC      : GICv3 — Distributor 0x08000000, Redistributor 0x080A0000
  Timer    : ARM virtual timer (CNTV), IRQ 27

================================================================================
  Author : dhayaeee70-lang
  Repo   : https://github.com/dhayaeee70-lang/Dhayalan_Mini_Rtos
================================================================================
