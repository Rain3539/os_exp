# 工具链配置
CROSS_COMPILE = riscv64-unknown-elf-
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJDUMP = $(CROSS_COMPILE)objdump
OBJCOPY = $(CROSS_COMPILE)objcopy
NM = $(CROSS_COMPILE)nm

# 编译选项
CFLAGS = -Wall -O -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib
CFLAGS += -mno-relax -fno-stack-protector -fno-pie -no-pie

ASFLAGS = -gdwarf-2

# 链接选项
LDFLAGS = -z max-page-size=4096

# 目标文件
# 按模块组织，便于维护
OBJS = \
	kernel/boot/entry.o \
	kernel/boot/start.o \
	kernel/main.o \
	kernel/utils/printf.o \
	kernel/utils/uart.o \
	kernel/utils/console.o \
	kernel/utils/string.o \
	kernel/mm/kalloc.o \
	kernel/mm/vm.o \
	kernel/trap/trap.o \
	kernel/trap/kernelvec.o \
	kernel/syscall/syscall.o \
	kernel/syscall/sysproc.o \
	kernel/syscall/sysfile.o \
	kernel/proc/proc.o \
	kernel/proc/swtch.o \
	kernel/fs/bio.o \
	kernel/fs/log.o \
	kernel/fs/fs.o \
	kernel/fs/file.o

# QEMU配置
QEMU = qemu-system-riscv64
QEMUOPTS = -machine virt -bios none -kernel kernel.elf -m 128M -smp 1 -nographic

# 默认目标 - 编译内核
all: kernel.elf
	@echo ""
	@echo "======================================"
	@echo "  编译完成 - 优先级调度版本"
	@echo "======================================"
	@echo ""
	@echo "运行命令: make run 或 make qemu"
	@echo ""

# 完整构建（包含文件系统镜像）
full: kernel.elf fs.img

# 编译汇编文件
kernel/boot/entry.o: kernel/boot/entry.S
	@echo "编译 $<..."
	$(CC) $(CFLAGS) -c $< -o $@

kernel/trap/kernelvec.o: kernel/trap/kernelvec.S
	@echo "编译 $<..."
	$(CC) $(CFLAGS) -c $< -o $@

kernel/proc/swtch.o: kernel/proc/swtch.S
	@echo "编译 $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# 编译C文件
kernel/%.o: kernel/%.c
	@echo "编译 $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# 链接生成内核镜像
kernel.elf: $(OBJS) kernel/kernel.ld
	@echo "链接内核..."
	$(LD) $(LDFLAGS) -T kernel/kernel.ld -o $@ $(OBJS)
	@echo "内核编译完成!"

# 生成反汇编文件用于调试
kernel.asm: kernel.elf
	@echo "生成反汇编文件..."
	$(OBJDUMP) -S $< > $@

# 生成符号表
kernel.sym: kernel.elf
	@echo "生成符号表..."
	$(NM) $< | sort > $@

# 创建文件系统镜像
fs.img: mkfs README.md
	@echo "创建文件系统镜像..."
	./mkfs fs.img README.md
	@echo "文件系统镜像创建完成!"

# 编译mkfs工具
mkfs: kernel/mkfs.c kernel/fs/fs.h
	@echo "编译mkfs工具..."
	gcc -Werror -Wall -I. -Ikernel -o mkfs kernel/mkfs.c
	@echo "mkfs工具编译完成!"

# 验证内存布局
check-layout: kernel.elf
	@echo "=== 内存段布局 ==="
	$(OBJDUMP) -h $<
	@echo ""
	@echo "=== 关键符号地址 ==="
	$(NM) $< | grep -E "(start|end|text|bss|etext|edata|scheduler|procinit)"

# 验证进程管理符号（优先级调度）
check-proc: kernel.elf
	@echo "=== 进程管理相关符号 ==="
	$(NM) $< | grep -E "(proc|scheduler|swtch|yield|allocproc|myproc|priority|aging)"

# 验证系统调用符号
check-syscall: kernel.elf
	@echo "=== 系统调用相关符号 ==="
	$(NM) $< | grep -E "(syscall|sys_|handle_syscall|setpriority|getpriority)"

# 验证文件系统符号
check-fs: kernel.elf
	@echo "=== 文件系统相关符号 ==="
	$(NM) $< | grep -E "(inode|file|bread|bwrite|log|balloc|bfree)"

# 在QEMU中运行
run: kernel.elf
	@echo "======================================"
	@echo "  启动QEMU - 优先级调度测试"
	@echo "======================================"
	@echo ""
	$(QEMU) $(QEMUOPTS)

# qemu别名
qemu: run

# 带文件系统镜像运行
run-fs: kernel.elf fs.img
	@echo "======================================"
	@echo "  启动QEMU（带文件系统镜像）"
	@echo "======================================"
	@echo ""
	$(QEMU) $(QEMUOPTS) \
		-drive file=fs.img,if=none,format=raw,id=x0 \
		-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

# 调试模式运行
debug: kernel.elf
	@echo "======================================"
	@echo "  启动QEMU调试模式"
	@echo "======================================"
	@echo ""
	@echo "QEMU将在端口1234等待GDB连接..."
	@echo "请在另一个终端运行: make gdb"
	@echo ""
	$(QEMU) $(QEMUOPTS) -s -S

# 调试模式运行（带文件系统）
debug-fs: kernel.elf fs.img
	@echo "======================================"
	@echo "  启动QEMU调试模式（带文件系统）"
	@echo "======================================"
	@echo ""
	@echo "QEMU将在端口1234等待GDB连接..."
	@echo "请在另一个终端运行: make gdb"
	@echo ""
	$(QEMU) $(QEMUOPTS) \
		-drive file=fs.img,if=none,format=raw,id=x0 \
		-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 \
		-s -S

# GDB调试连接
gdb: kernel.elf
	@echo "======================================"
	@echo "  连接到QEMU调试会话"
	@echo "======================================"
	@echo ""
	gdb-multiarch -ex "target remote :1234" -ex "symbol-file kernel.elf" kernel.elf

# 清理
clean:
	@echo "清理编译文件..."
	rm -f kernel/boot/*.o kernel/boot/*.d
	rm -f kernel/mm/*.o kernel/mm/*.d
	rm -f kernel/trap/*.o kernel/trap/*.d
	rm -f kernel/syscall/*.o kernel/syscall/*.d
	rm -f kernel/proc/*.o kernel/proc/*.d
	rm -f kernel/fs/*.o kernel/fs/*.d
	rm -f kernel/utils/*.o kernel/utils/*.d
	rm -f kernel/*.o kernel/*.d
	rm -f kernel.elf kernel.asm kernel.sym
	@echo "清理完成!"

# 完全清理（包括文件系统镜像）
distclean: clean
	@echo "清理所有生成文件..."
	rm -f mkfs fs.img
	@echo "完全清理完成!"

# 完整构建和验证
test: clean all check-layout check-proc check-syscall kernel.asm kernel.sym 
	@echo ""
	@echo "======================================"
	@echo "  构建和验证完成!"
	@echo "======================================"
	@echo ""
	@echo "优先级调度功能已集成"
	@echo "系统调用模块已重组"
	@echo ""
	@echo "可用命令:"
	@echo "  make run          - 运行内核"
	@echo "  make qemu         - 运行内核（别名）"
	@echo "  make debug        - 调试模式运行"
	@echo "  make check-proc   - 检查进程和调度符号"
	@echo "  make check-syscall- 检查系统调用符号"
	@echo ""

# 显示文件结构
show-structure:
	@echo "======================================"
	@echo "  当前文件结构"
	@echo "======================================"
	@echo ""
	@echo "kernel/"
	@echo "├── boot/          - 启动代码"
	@echo "├── mm/            - 内存管理"
	@echo "├── trap/          - 陷阱处理"
	@echo "├── syscall/       - 系统调用（新）"
	@echo "│   ├── syscall.c  - 系统调用分发器"
	@echo "│   ├── sysproc.c  - 进程相关系统调用"
	@echo "│   └── sysfile.c  - 文件相关系统调用"
	@echo "├── proc/          - 进程管理（优先级调度）"
	@echo "├── fs/            - 文件系统"
	@echo "├── utils/         - 工具函数"
	@echo "├── main.c         - 主函数"
	@echo "└── def.h          - 全局定义"
	@echo ""

# 显示帮助
help:
	@echo "======================================================"
	@echo "  RISC-V OS 实验5 - 优先级调度 Makefile 帮助"
	@echo "======================================================"
	@echo ""
	@echo "编译目标:"
	@echo "  make              - 编译内核（默认）"
	@echo "  make kernel.elf   - 只编译内核"
	@echo "  make full         - 编译内核 + 创建文件系统镜像"
	@echo "  make fs.img       - 只创建文件系统镜像"
	@echo "  make mkfs         - 编译mkfs工具"
	@echo ""
	@echo "运行目标:"
	@echo "  make run          - 运行内核（优先级调度测试）"
	@echo "  make qemu         - 运行内核（别名）"
	@echo "  make run-fs       - 运行内核（带文件系统镜像）"
	@echo ""
	@echo "调试目标:"
	@echo "  make debug        - 启动QEMU等待GDB连接"
	@echo "  make debug-fs     - 启动QEMU（带fs.img）等待GDB"
	@echo "  make gdb          - 连接到QEMU进行调试"
	@echo ""
	@echo "验证目标:"
	@echo "  make check-layout - 检查内存布局"
	@echo "  make check-proc   - 检查进程和调度符号"
	@echo "  make check-syscall- 检查系统调用符号"
	@echo "  make check-fs     - 检查文件系统符号"
	@echo "  make test         - 完整构建和验证"
	@echo ""
	@echo "信息目标:"
	@echo "  make show-structure - 显示文件结构"
	@echo "  make help           - 显示此帮助信息"
	@echo ""
	@echo "清理目标:"
	@echo "  make clean        - 清理编译文件"
	@echo "  make distclean    - 完全清理（包括fs.img）"
	@echo ""
	@echo "辅助文件:"
	@echo "  make kernel.asm   - 生成反汇编文件"
	@echo "  make kernel.sym   - 生成符号表"
	@echo ""
	@echo "推荐使用流程:"
	@echo "  1. make               - 编译内核"
	@echo "  2. make run           - 测试优先级调度"
	@echo "  3. make check-proc    - 验证调度相关符号"
	@echo "  4. make check-syscall - 验证系统调用"
	@echo ""
	@echo "新功能特性:"
	@echo "  ✓ 优先级调度算法（0-10级）"
	@echo "  ✓ Aging机制防止饥饿"
	@echo "  ✓ 系统调用模块重组"
	@echo "  ✓ setpriority/getpriority系统调用"
	@echo ""

.PHONY: all full clean distclean run qemu run-fs debug debug-fs gdb test \
        check-layout check-proc check-syscall check-fs show-structure help

# 包含依赖文件
-include kernel/*/*.d