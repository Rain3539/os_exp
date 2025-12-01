
// trap.c - 完整的中断/陷阱处理实现

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "printf.h"
#include "proc.h"
#include "interrupt.h"

volatile uint64 ticks = 0;

// 定时器测试相关状态
volatile int interrupt_test_mode = 0;    // 是否在测试模式下计数
volatile int interrupt_test_count = 0;   // 测试期间被中断处理器增加的计数

// 中断处理开销统计
volatile uint64 intr_cycles_sum_g = 0;
volatile uint64 intr_cycles_min_g = (uint64)-1;
volatile uint64 intr_cycles_max_g = 0;
volatile uint64 intr_cycles_count_g = 0;

// 获取当前时间（CLINT mtime）
uint64 get_time(void) {
    volatile uint64 *mtime = (volatile uint64*)CLINT_MTIME;
    return *mtime;
}

// 设置下次定时器比较值
static void set_next_timer(void) {
    uint64 hart = r_mhartid();
    volatile uint64 *mtime = (volatile uint64*)CLINT_MTIME;
    volatile uint64 *mtimecmp = (volatile uint64*)CLINT_MTIMECMP(hart);
    
    // 设置下一次中断在1000000个周期后
    uint64 next = *mtime + 1000000;
    *mtimecmp = next;
    
    // 确保写入完成
    asm volatile("fence iorw, iorw" ::: "memory");
}

// 定时器中断处理函数
void timer_interrupt_handler(void) {
    ticks++;
    
    // 如果处于测试模式，则增加测试计数
    if (interrupt_test_mode) {
        interrupt_test_count++;
    }
    
    // 安排下一次中断
    set_next_timer();
    
    // 请求调度
    need_resched = 1;
}

// 初始化陷阱系统
void trap_init(void) {
    ticks = 0;
    interrupt_test_mode = 0;
    interrupt_test_count = 0;
    
    // 设置陷阱向量为kernelvec
    w_mtvec((uint64)kernelvec);
    
    // 启用机器模式中断
    w_mie(r_mie() | MIE_MTIE);
    
    // 启用全局中断
    intr_on();
    
    // 设置第一次定时器中断
    set_next_timer();
    
    printf("[trap_init] 中断系统初始化完成\n");
    printf("[trap_init] mtvec=0x%lx, mie=0x%lx, mstatus=0x%lx\n", 
           r_mtvec(), r_mie(), r_mstatus());
}

// 简化的内核态中断处理入口
void kerneltrap(void) {
    uint64 mcause = r_mcause();
    uint64 is_interrupt = (mcause >> 63) & 1ULL;
    uint64 cause = mcause & 0x7FFFFFFFFFFFFFFFULL;
    uint64 epc = r_mepc();  // 读取当前 epc

    if (is_interrupt) {
        // 机器模式中断
        switch (cause) {
            case 7: { // Machine timer interrupt
                // 记录中断处理时间
                uint64 t0 = get_time();
                timer_interrupt_handler();
                uint64 t1 = get_time();
                uint64 delta = (t1 > t0) ? (t1 - t0) : 0;
                
                // 更新全局统计
                intr_cycles_sum_g += delta;
                if (delta < intr_cycles_min_g) intr_cycles_min_g = delta;
                if (delta > intr_cycles_max_g) intr_cycles_max_g = delta;
                intr_cycles_count_g++;
                break;
            }
            default:
                printf("[kerneltrap] 未知中断: cause=%lu\n", cause);
                break;
        }
    } else {
        // 同步异常 - 构造一个临时 trapframe 并交给统一的 handle_exception 处理
        struct trapframe tf;
        // 只需填充我们会使用的字段（epc），其余保持未初始化/零值
        tf.epc = epc;

        printf("[kerneltrap] 处理异常: cause=%lu, epc=0x%lx\n", cause, epc);

        // 使用统一的异常处理入口
        handle_exception(&tf);

        // 如果 handle_exception 没有 panic，应保证 mepc 已被更新或按策略处理
        uint64 new_epc = r_mepc();
        printf("[kerneltrap] 异常处理完成: epc 从 0x%lx 更新为 0x%lx\n", epc, new_epc);
    }
}

// --------- 新增：统一异常处理与 helper ---------
// Helper prototypes
static void handle_syscall(struct trapframe *tf);
static void handle_instruction_page_fault(struct trapframe *tf);
static void handle_load_page_fault(struct trapframe *tf);
static void handle_store_page_fault(struct trapframe *tf);

// 注意：此处使用 r_mcause()（机器态）来获取 cause 编号
void handle_exception(struct trapframe *tf) {
    uint64 mcause = r_mcause();
    uint64 is_interrupt = (mcause >> 63) & 1ULL;
    uint64 cause = mcause & 0x7FFFFFFFFFFFFFFFULL;

    if (is_interrupt) {
        // 不在此函数处理硬中断
        panic("handle_exception invoked for interrupt");
    }

    switch (cause) {
    case 0: // Instruction address misaligned
        printf("[handle_exception] 指令地址未对齐异常\n");
        w_mepc(tf->epc + 4);
        break;
    case 1: // Instruction access fault
        printf("[handle_exception] 指令访问故障\n");
        w_mepc(tf->epc + 4);
        break;
    case 2: // Illegal instruction
        printf("[handle_exception] 非法指令异常\n");
        w_mepc(tf->epc + 4);
        break;
    case 3: // Breakpoint
        printf("[handle_exception] 断点异常 - 跳过 ebreak 指令\n");
        w_mepc(tf->epc + 4);
        break;
    case 5: // Load address misaligned
        printf("[handle_exception] 加载地址未对齐异常\n");
        w_mepc(tf->epc + 4);
        break;
    case 7: // Store address misaligned
        printf("[handle_exception] 存储地址未对齐异常\n");
        w_mepc(tf->epc + 4);
        break;
    case 8: // Environment call from U-mode
    case 9: // Environment call from S-mode
    case 11: // Environment call from M-mode
        printf("[handle_exception] 环境调用异常\n");
        handle_syscall(tf);
        break;
    case 12: // 指令页故障
        handle_instruction_page_fault(tf);
        break;
    case 13: // 加载页故障
        handle_load_page_fault(tf);
        break;
    case 15: // 存储页故障
        handle_store_page_fault(tf);
        break;
    default:
        panic("Unknown exception");
    }
}

// 系统调用处理：调用系统调用分发器
static void handle_syscall(struct trapframe *tf) {
    struct proc *p = myproc();
    if (!p || !p->trapframe) {
        printf("[handle_syscall] 无当前进程或trapframe\n");
        w_mepc(r_mepc() + 4);
        return;
    }
    
    // 将trapframe传递给syscall
    // syscall会从trapframe中提取参数并执行系统调用
    syscall();
    
    // 跳过ecall指令
    w_mepc(r_mepc() + 4);
}

// 页错误处理：目前保守处理为打印信息并 panic，
// 因为仓库中未实现 vmfault（链接会失败）。
static void handle_instruction_page_fault(struct trapframe *tf) {
    uint64 stval = r_stval();
    printf("[handle_instruction_page_fault] instruction page fault stval=0x%lx\n", stval);
    panic("instruction page fault");
}

static void handle_load_page_fault(struct trapframe *tf) {
    uint64 stval = r_stval();
    printf("[handle_load_page_fault] load page fault stval=0x%lx\n", stval);
    panic("load page fault");
}

static void handle_store_page_fault(struct trapframe *tf) {
    uint64 stval = r_stval();
    printf("[handle_store_page_fault] store page fault stval=0x%lx\n", stval);
    panic("store page fault");
}

// 中断功能测试：等待并计数真实的时钟中断
void test_timer_interrupt(void) {
    printf("=== 测试定时器中断 ===\n");

    // 记录中断前的时间
    uint64 start_time = get_time();
    interrupt_test_count = 0;
    interrupt_test_mode = 1;

    printf("等待5次定时器中断...\n");
    
    // 等待若干次中断
    int target_count = interrupt_test_count + 5;
    while (interrupt_test_count < target_count) {
        // 简单延时，避免忙等待消耗过多CPU
        for (volatile int i = 0; i < 10000; i++);
    }

    interrupt_test_mode = 0;
    uint64 end_time = get_time();
    
    printf("定时器测试完成: %d 次中断，耗时 %lu 周期\n",
           interrupt_test_count, end_time - start_time);
    
    // 验证测试结果
    if (interrupt_test_count >= 5) {
        printf("定时器中断测试通过\n");
    } else {
        printf("定时器中断测试失败\n");
    }
}

// 改进的异常处理测试：使用更安全的方法
void test_exception_handling(void) {
    printf("=== 测试异常处理 ===\n");

    // 使用内联汇编确保精确控制指令流
    printf("触发: ebreak (断点)\n");
    asm volatile(
        "ebreak\n"
        "j 1f\n"        // 跳转到标签1，确保继续执行
        "1:\n"
    );
    printf("成功从ebreak异常返回\n");

    printf("触发: ecall (环境调用)\n");
    asm volatile(
        "ecall\n"
        "j 1f\n"        // 跳转到标签1，确保继续执行  
        "1:\n"
    );
    printf("成功从ecall异常返回\n");

    printf("触发: 非法指令\n");
    asm volatile(
        ".word 0x00000000\n"  // 非法指令
        "j 1f\n"              // 跳转到标签1，确保继续执行
        "1:\n"
    );
    printf("成功从非法指令异常返回\n");

    // 新增测试：未对齐加载（可能触发指令 13/15 或地址未对齐异常）
    printf("触发: 未对齐加载（load misaligned）\n");
    asm volatile(
        "li t0, 1\n"      // 地址 1，通常未对齐
        "lw t1, 0(t0)\n"  // 从地址1加载（未对齐）
        "j 1f\n"
        "1:\n"
    );
    printf("未对齐加载测试返回（如果未触发异常则继续）\n");

    // 新增测试：未对齐存储
    printf("触发: 未对齐存储（store misaligned）\n");
    asm volatile(
        "li t0, 1\n"
        "sw t1, 0(t0)\n"  // 对地址1执行存储（未对齐）
        "j 1f\n"
        "1:\n"
    );
    printf("未对齐存储测试返回（如果未触发异常则继续）\n");

    // 新增测试：空指针加载（访问非法地址）
    printf("触发: 空指针加载（load from 0x0）\n");
    asm volatile(
        "li t0, 0\n"
        "lw t1, 0(t0)\n"
        "j 1f\n"
        "1:\n"
    );
    printf("空指针加载测试返回（如果未触发异常则继续）\n");

    // 新增测试：除零（如果目标支持 div 指令并触发异常）
    printf("触发: 除零测试（div by zero）\n");
    asm volatile(
        "li t0, 42\n"
        "li t1, 0\n"
        "div t2, t0, t1\n"  // 除以0，行为实现定义；某些实现会发生异常
        "j 1f\n"
        "1:\n"
    );
    printf("除零测试返回（如果未触发异常则继续）\n");

    printf("异常处理测试完成\n");
}

// 性能测试：测量中断处理开销
void test_interrupt_overhead(void) {
    const uint64 target = 10; // 等待10次中断以获得统计
    printf("=== 测试中断处理开销: 等待 %lu 次定时器中断 ===\n", target);

    // 清除统计
    intr_cycles_sum_g = 0;
    intr_cycles_min_g = (uint64)-1;
    intr_cycles_max_g = 0;
    intr_cycles_count_g = 0;

    // 等待统计达到 target
    uint64 start_cycles = get_time();
    while (intr_cycles_count_g < target) {
        // 等待下一次中断到达
        for (volatile int i = 0; i < 10000; i++);
    }
    uint64 end_cycles = get_time();

    uint64 sum = intr_cycles_sum_g;
    uint64 cnt = intr_cycles_count_g;
    uint64 avg = cnt ? (sum / cnt) : 0;
    
    printf("中断处理开销统计:\n");
    printf("  中断次数: %lu\n", cnt);
    printf("  总耗时: %lu 周期\n", end_cycles - start_cycles);
    printf("  平均处理时间: %lu 周期\n", avg);
    printf("  最小处理时间: %lu 周期\n", intr_cycles_min_g);
    printf("  最大处理时间: %lu 周期\n", intr_cycles_max_g);
    
    if (cnt >= target) {
        printf("中断性能测试完成\n");
    } else {
        printf("中断性能测试未达到目标次数\n");
    }
}
