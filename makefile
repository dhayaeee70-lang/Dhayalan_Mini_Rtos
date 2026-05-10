CROSS_COMPILE = aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
AS = $(CROSS_COMPILE)gcc

CFLAGS = -Wall -O2 -ffreestanding -nostdlib -nostartfiles -mgeneral-regs-only -Isrc/include
LDFLAGS = -T linker.ld -nostdlib

# Source files
BOOT_SRCS = src/boot/startup.s src/boot/vectors.s src/boot/mmu.s
KERNEL_SRCS = src/kernel/main.c src/kernel/scheduler.c src/kernel/mutex.c src/kernel/mmu.c
DRIVER_SRCS = src/drivers/uart.c src/drivers/gic.c src/drivers/timer.c
IRQ_SRCS = src/interrupt/irq_handler.c

# Object files
OBJS = build/startup.o build/vectors.o build/mmu_boot.o \
       build/main.o build/scheduler.o build/mutex.o build/mmu.o \
       build/uart.o build/gic.o build/timer.o \
       build/irq_handler.o

TARGET = build/kernel8.elf

.PHONY: all clean run debug

all: $(TARGET)

# Boot assembly files
build/startup.o: src/boot/startup.s
	@mkdir -p build
	@echo "Assembling $<"
	$(AS) $(CFLAGS) -c $< -o $@

build/vectors.o: src/boot/vectors.s
	@mkdir -p build
	@echo "Assembling $<"
	$(AS) $(CFLAGS) -c $< -o $@

build/mmu_boot.o: src/boot/mmu.s
	@mkdir -p build
	@echo "Assembling $<"
	$(AS) $(CFLAGS) -c $< -o $@

# Kernel C files
build/main.o: src/kernel/main.c
	@mkdir -p build
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

build/scheduler.o: src/kernel/scheduler.c
	@mkdir -p build
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

build/mutex.o: src/kernel/mutex.c
	@mkdir -p build
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

build/mmu.o: src/kernel/mmu.c
	@mkdir -p build
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Driver C files
build/uart.o: src/drivers/uart.c
	@mkdir -p build
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

build/gic.o: src/drivers/gic.c
	@mkdir -p build
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

build/timer.o: src/drivers/timer.c
	@mkdir -p build
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Interrupt handler
build/irq_handler.o: src/interrupt/irq_handler.c
	@mkdir -p build
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(TARGET): $(OBJS)
	@mkdir -p build
	@echo "Linking $(TARGET)"
	$(LD) $(LDFLAGS) $(OBJS) -o $(TARGET)
	@echo "Build complete!"

.PHONY: all clean run debug

# ... rest of makefile ...

run: 
	@echo "Starting QEMU with kernel: $(TARGET)"
	@test -f $(TARGET) || (echo "Error: $(TARGET) not found. Run 'make' first." && exit 1)
	qemu-system-aarch64 -M virt,secure=on,gic-version=3 -cpu cortex-a57 -nographic -kernel $(TARGET)

run-debug: $(TARGET)
	@echo "Starting QEMU in debug mode..."
	qemu-system-aarch64 -M virt,secure=on,gic-version=3 -cpu cortex-a57 -nographic -kernel $(TARGET) -d int,cpu_reset

clean:
	rm -rf build/
	@echo "Clean complete"

debug:
	@echo "Current directory: $(shell pwd)"
	@echo "Object files needed: $(OBJS)"
	@echo ""
	@echo "Source files found:"
	@ls -la src/boot/*.s
	@ls -la src/kernel/*.c
	@ls -la src/drivers/*.c
	@ls -la src/interrupt/*.c
