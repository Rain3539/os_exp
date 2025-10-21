# RISC-V操作系统 Makefile
# 标准构建配置文件

# 工具链配置
CROSS_COMPILE = riscv64-unknown-elf-
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJDUMP = $(CROSS_COMPILE)objdump
OBJCOPY = $(CROSS_COMPILE)objcopy
NM = $(CROSS_COMPILE)nm

# 编译选项
CFLAGS = -Wall  -O -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib
CFLAGS += -mno-relax -fno-stack-protector -fno-pie -no-pie

ASFLAGS = -gdwarf-2

# 链接选项
LDFLAGS = -z max-page-size=4096

# 目标文件
OBJS = \
kernel/boot/entry.o \
kernel/boot/start.o \
kernel/main.o \
kernel/utils/printf.o \
kernel/utils/uart.o \
kernel/utils/console.o \
kernel/mm/kalloc.o \
kernel/utils/string.o \
kernel/mm/vm.o \
kernel/trap/trap.o \
kernel/trap/kernelvec.o \
kernel/trap/syscall.o \
kernel/proc/proc.o \
kernel/proc/swtch.o 

# 默认目标
all: kernel.elf

# 编译汇编文件
kernel/boot/entry.o: kernel/boot/entry.S
	$(CC) $(CFLAGS) -c $< -o $@

kernel/trap/kernelvec.o: kernel/trap/kernelvec.S
	$(CC) $(CFLAGS) -c $< -o $@

# 编译C文件
kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 链接生成内核镜像
kernel.elf: $(OBJS) kernel/kernel.ld
	$(LD) $(LDFLAGS) -T kernel/kernel.ld -o $@ $(OBJS)

# 生成反汇编文件用于调试
kernel.asm: kernel.elf
	$(OBJDUMP) -S $< > $@

# 生成符号表
kernel.sym: kernel.elf
	$(NM) $< | sort > $@

# 编译上下文切换汇编文件（新增）
kernel/proc/swtch.o: kernel/proc/swtch.S
	$(CC) $(CFLAGS) -c $< -o $@

# 验证内存布局
check-layout: kernel.elf
	@echo "=== 内存段布局 ==="
	$(OBJDUMP) -h $<
	@echo ""
	@echo "=== 关键符号地址 ==="
	$(NM) $< | grep -E "(start|end|text|bss|etext|edata|scheduler|procinit)"

# 验证进程管理符号（新增）
check-proc: kernel.elf
	@echo "=== 进程管理相关符号 ==="
	$(NM) $< | grep -E "(proc|scheduler|swtch|yield|allocproc|myproc)"

# 在QEMU中运行（需要安装qemu-system-riscv64）
run: kernel.elf
	qemu-system-riscv64 -machine virt -bios none -kernel $< -m 128M -smp 1 -nographic

# 调试模式运行
debug: kernel.elf
	@echo "=== 启动QEMU调试模式 ==="
	@echo "QEMU将在端口1234等待GDB连接..."
	@echo "请在另一个终端运行: make gdb"
	@echo "或手动运行: gdb-multiarch -ex 'target remote :1234' -ex 'symbol-file kernel.elf' kernel.elf"
	qemu-system-riscv64 -machine virt -bios none -kernel $< -m 128M -smp 1 -nographic -s -S

# GDB调试连接
gdb: kernel.elf
	@echo "=== 连接到QEMU调试会话 ==="
	gdb-multiarch -ex "target remote :1234" -ex "symbol-file kernel.elf" kernel.elf

# 一键调试（同时启动QEMU和GDB）
debug-all: kernel.elf
	@echo "=== 启动完整调试环境 ==="
	@echo "将在后台启动QEMU，然后启动GDB..."
	@(qemu-system-riscv64 -machine virt -bios none -kernel $< -m 128M -smp 1 -nographic -s -S &) && sleep 2 && gdb-multiarch -ex "target remote :1234" -ex "symbol-file kernel.elf" kernel.elf

# 清理
clean:
	rm -f kernel/boot/*.o kernel/boot/*.d
	rm -f kernel/mm/*.o kernel/mm/*.d
	rm -f kernel/trap/*.o kernel/trap/*.d
	rm -f kernel/proc/*.o kernel/proc/*.d    # 新增
	rm -f kernel/utils/*.o kernel/utils/*.d
	rm -f kernel/*.o kernel/*.d
	rm -f kernel.elf kernel.asm kernel.sym

# 完整构建和验证
test: clean all check-layout check-proc kernel.asm kernel.sym 
	@echo ""
	@echo "=== 构建完成! ==="
	@echo "进程管理模块已集成"              # 新增提示
	@echo "运行: make run"
	@echo "调试: make debug (然后在另一终端运行 make gdb)"
.PHONY: all clean run debug debug-all gdb test check-layout

# 包含依赖文件
-include kernel/*/*.d 