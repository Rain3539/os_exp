# Makefile for the minimal RISC-V OS

# 1. 工具链定义 (Toolchain Definition)
# 指定我们用于交叉编译的工具链前缀
TOOLCHAIN_PREFIX := riscv64-unknown-elf-
CC := $(TOOLCHAIN_PREFIX)gcc
LD := $(TOOLCHAIN_PREFIX)ld
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP := $(TOOLCHAIN_PREFIX)objdump

# 2. 编译和链接标志 (Compiler and Linker Flags)
# CFLAGS:
# -march=rv64gc: 目标架构为RISC-V 64位，包含通用扩展(G)和压缩指令(C)
# -mabi=lp64d:    使用64位长整型和指针，以及双精度浮点数的ABI
# -ffreestanding: 告诉编译器我们不依赖标准库，因为我们在裸机上运行
# -nostdlib:      链接时不使用标准库
# -O2:            二级优化
# -g:             生成调试信息，方便GDB调试
# -Wall -Wextra:  开启所有常用和额外的编译警告
# -mcmodel=medany: 中等代码模型，适用于地址在+/- 2GiB范围内的内核代码
CFLAGS := -march=rv64gc -mabi=lp64d -ffreestanding -nostdlib -O2 -g -Wall -Wextra -mcmodel=medany

# LDFLAGS:
# -T kernel/kernel.ld: 指定链接脚本的位置
LDFLAGS := -T kernel/kernel.ld

# 3. 源文件和目标文件 (Source and Object Files)
# 定义内核需要的所有对象文件
OBJS := \
    kernel/entry.o \
    kernel/main.o \
    kernel/uart.o

# 默认目标，当执行'make'时会构建kernel.elf
all: kernel.elf

# 4. 链接规则 (Linking Rule)
# 将所有.o文件链接成最终的ELF格式可执行文件
kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJS)
	@echo "LD   kernel.elf"

# 5. 编译规则 (Compilation Rules)
# 规则：如何从.S汇编文件生成.o对象文件
kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "AS   $<"

# 规则：如何从.c C源文件生成.o对象文件
kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC   $<"

# 6. QEMU运行和调试 (QEMU Execution and Debugging)
# 运行内核
# -machine virt:       模拟RISC-V的'virt'通用虚拟平台
# -nographic:          禁用图形界面，将所有输出重定向到终端
# -bios none:          不加载默认的BIOS(如OpenSBI)，直接从-kernel指定的地址开始执行
# -kernel kernel.elf:  将我们的内核加载到内存中
qemu: all
	@qemu-system-riscv64 \
		-machine virt \
		-nographic \
		-bios none \
		-kernel kernel.elf

# 7. 清理规则 (Clean Rule)
# 删除所有编译生成的文件
clean:
	@rm -f kernel.elf kernel/*.o *~

.PHONY: all qemu clean