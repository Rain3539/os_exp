# Makefile for the minimal RISC-V OS

# 1. 工具链定义
TOOLCHAIN_PREFIX := riscv64-unknown-elf-
CC := $(TOOLCHAIN_PREFIX)gcc
LD := $(TOOLCHAIN_PREFIX)ld
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP := $(TOOLCHAIN_PREFIX)objdump

# 2. 编译和链接标志
# 新增 -Iinclude 以指定头文件搜索路径
CFLAGS := -march=rv64gc -mabi=lp64d -ffreestanding -nostdlib -O2 -g -Wall -Wextra -mcmodel=medany -Iinclude
LDFLAGS := -T kernel/kernel.ld

# 3. 源文件和目标文件
# 新增 printf.o 和 console.o
OBJS := \
    kernel/entry.o \
    kernel/main.o \
    kernel/uart.o \
    kernel/printf.o \
    kernel/console.o

# 默认目标
all: kernel.elf

# 4. 链接规则
kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJS)
	@echo "LD   kernel.elf"

# 5. 编译规则
kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "AS   $<"

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC   $<"

# 6. QEMU运行和调试
qemu: all
	@qemu-system-riscv64 \
		-machine virt \
		-nographic \
		-bios none \
		-kernel kernel.elf

# 7. 清理规则
clean:
	@rm -f kernel.elf kernel/*.o *~

.PHONY: all qemu clean