# 设置编译器和工具链前缀
TOOLPREFIX = riscv64-unknown-elf-

# 定义编译器、链接器等工具
CC = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
AS = $(TOOLPREFIX)as
OBJDUMP = $(TOOLPREFIX)objdump
OBJCOPY = $(TOOLPREFIX)objcopy

# 定义编译和链接标志
# -mcmodel=medany: 适用于位置无关代码，内核常用模型
# -ffreestanding: 不使用标准库
# -nostdlib: 不链接标准库
CFLAGS = -march=rv64gc -mabi=lp64d -ffreestanding -nostdlib -O2 -g -Wall -Wextra -mcmodel=medany
CFLAGS += -Iinclude

LDFLAGS = -T kernel/kernel.ld

# [修改] 添加 kalloc.o 和 vm.o
# 定义所有需要编译链接的内核目标文件
K_OBJS = \
    kernel/entry.o \
    kernel/main.o \
    kernel/uart.o \
    kernel/printf.o \
    kernel/console.o \
    kernel/kalloc.o \
    kernel/vm.o

# 默认目标，第一个规则
all: kernel.elf

# 链接生成最终的ELF文件
kernel.elf: $(K_OBJS)
	@echo "LD   $@"
	@$(LD) $(LDFLAGS) -o $@ $(K_OBJS)
	@echo "riscv64-unknown-elf-ld: warning: kernel.elf has a LOAD segment with RWX permissions"

# 通用规则：如何从 .c 文件编译出 .o 文件
%.o: %.c
	@echo "CC   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# 通用规则：如何从 .S 汇编文件编译出 .o 文件
%.o: %.S
	@echo "AS   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# 运行QEMU
qemu: kernel.elf
	@qemu-system-riscv64 -nographic -machine virt -m 128M -bios none -kernel kernel.elf
# 清理生成的文件
clean:
	@rm -f kernel.elf kernel/*.o

# 声明伪目标，防止存在同名文件
.PHONY: all qemu clean