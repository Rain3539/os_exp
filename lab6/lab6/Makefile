CC = riscv64-unknown-elf-gcc
LD = riscv64-unknown-elf-ld
OBJCOPY = riscv64-unknown-elf-objcopy
OBJDUMP = riscv64-unknown-elf-objdump

CFLAGS = -nostdlib -fno-builtin -mcmodel=medany -Iinclude -Wall -O2 -ffreestanding
LDFLAGS = -T kernel/kernel.ld -nostdlib -static

OBJS = kernel/entry.o kernel/start.o kernel/uart.o kernel/printf.o kernel/console.o kernel/spinlock.o kernel/simplified.o kernel/pmm.o kernel/vm.o kernel/trap.o kernel/kernelvec.o kernel/sched.o kernel/swtch.o kernel/test_proc.o kernel/syscall.o kernel/sysproc.o kernel/sysfile.o kernel/usys.o kernel/test_syscall.o

all: kernel.elf

kernel.elf: $(OBJS) kernel/kernel.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "Built kernel.elf successfully"

kernel/entry.o: kernel/entry.S
	$(CC) $(CFLAGS) -c -o $@ $<

kernel/kernelvec.o: kernel/kernelvec.S
	$(CC) $(CFLAGS) -c -o $@ $<

kernel/trap.o: kernel/trap.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel/sched.o: kernel/sched.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel/%.o: kernel/%.c include/types.h
	$(CC) $(CFLAGS) -c -o $@ $<

kernel/usys.o: kernel/usys.S
	$(CC) $(CFLAGS) -c -o $@ $<

disasm: kernel.elf
	$(OBJDUMP) -S kernel.elf > kernel.disasm

clean:
	rm -f kernel.elf $(OBJS) kernel.disasm

qemu: kernel.elf
	qemu-system-riscv64 -machine virt -bios none -kernel kernel.elf -nographic

.PHONY: all clean qemu disasm
