#include "scheduler.h"
#include "../drivers/uart.h"

static TCB tasks[NUM_TASKS];
static unsigned char task_2_stack[TASK_STACK_SIZE] __attribute__((aligned(16)));
static unsigned char task_3_stack[TASK_STACK_SIZE] __attribute__((aligned(16)));
static int current_task = 0;

// Task functions
static void task1(void) {
    while(1) {
        uart_puts("task1 running...\n");
        for(int i=0; i<4000000; i++) asm volatile("nop");
    }
}

static void task2(void) {
    while(1) {
        uart_puts("task2 running...\n");
        for(int i=0; i<4000000; i++) asm volatile("nop");
    }
}

static void task3(void) {
    while(1) {
        uart_puts("task3 running...\n");
        for(int i=0; i<4000000; i++) asm volatile("nop");
    }
}

// Initialize a task's stack
static void init_task_stack(unsigned long *stack_top, void (*task_func)(void), int task_num) {
    unsigned long *sp = stack_top;
    sp -= 34;
    
    // Initialize saved registers
    for(int i=0; i<31; i++) sp[i] = 0;      // x0-x30
    sp[32] = (unsigned long)task_func;      // ELR (return address)
    sp[33] = 0x345;                         // SPSR_EL1 (processor state)
    
    tasks[task_num].sp = (unsigned long)sp;
}

void scheduler_init(void) {
    uart_puts("[Scheduler] Initializing tasks...\n");
    
    // Task 1 uses main stack (initialized by startup.s)
    // Only initialize task 2 and 3
    init_task_stack((unsigned long *)&task_2_stack[TASK_STACK_SIZE], task2, 1);
    init_task_stack((unsigned long *)&task_3_stack[TASK_STACK_SIZE], task3, 2);
    
    uart_puts("[Scheduler] Tasks initialized.\n");
}

unsigned long scheduler_switch_task(unsigned long old_sp) {
    // Save current task's stack pointer
    tasks[current_task].sp = old_sp;
    
    // Switch to next task (round-robin)
    current_task = (current_task + 1) % NUM_TASKS;
    
    // Return new task's stack pointer
    return tasks[current_task].sp;
}

void scheduler_start(void) {
    uart_puts("[Scheduler] Starting Task 1...\n");
    task1();
}