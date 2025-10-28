#include "../mm/memlayout.h"
#include "../def.h"
#include "../proc/proc.h"

// 全局变量定义
volatile int global_interrupt_count = 0;

// in kernelvec.S, calls kerneltrap().
void kernelvec();

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
  intr_on();
}

// 设置下次时钟中断时间
void sbi_set_timer(uint64 time){
    w_stimecmp(r_time() + time);
}

// 获取当前时间
uint64 get_time(void){
    return r_time();
}

void timer_interrupt(void) {
    // 1. 更新系统时间
    // uint64 time=get_time();
    
    // 2. 递增全局中断计数器
    global_interrupt_count++;
    
    // 3. 设置下次中断时间
    sbi_set_timer(1000000);
    
    // 4. 触发任务调度（时间片用完）
    struct proc *p = myproc();
    if(p && p->state == RUNNING) {
        yield();  // 使用进程管理系统的yield
    }
}

void 
kerneltrap(struct trapframe *tf)
{
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  // 检查是中断还是异常
  if (scause & CAUSE_INTERRUPT_FLAG) {
    // 处理中断
    uint64 interrupt_cause = scause & ~CAUSE_INTERRUPT_FLAG;
    switch (interrupt_cause) {
      case CAUSE_TIMER_INTERRUPT:
        timer_interrupt();
        break;
      case CAUSE_SOFTWARE_INTERRUPT:
        printf("Software interrupt received\n");
        break;
      case CAUSE_EXTERNAL_INTERRUPT:
        printf("External interrupt received\n");
        break;
      default:
        printf("Unknown interrupt: %d\n", interrupt_cause);
        panic("Unknown interrupt");
    }
  } else {
    tf->epc = sepc;
    handle_exception(tf);
    sepc = tf->epc;
  }

  w_sepc(sepc);
  w_sstatus(sstatus);
}

// 异常处理主函数
void handle_exception(struct trapframe *tf) {
    uint64 cause = r_scause();
    uint64 stval = r_stval();  // 陷阱值寄存器
    
    switch (cause) {
        case CAUSE_INSTRUCTION_MISALIGNED://指令地址未对齐
            printf("Instruction address misaligned\n");
            panic("Instruction misaligned");
            break;
            
        case CAUSE_INSTRUCTION_ACCESS_FAULT://指令访问故障
            printf("Instruction access fault\n");
            panic("Instruction access fault");
            break;
            
        case CAUSE_ILLEGAL_INSTRUCTION://非法指令
            printf("Illegal instruction at 0x%x\n", tf->epc);
            panic("Illegal instruction");
            break;
            
        case CAUSE_BREAKPOINT://断点指令
            printf("Breakpoint at 0x%x\n", tf->epc);
            // 跳过断点指令 (ebreak是2字节指令)
            tf->epc += 2;
            printf("Breakpoint handled successfully!\n");
            break;
            
        case CAUSE_LOAD_MISALIGNED://加载地址未对齐
            printf("Load address misaligned: addr=0x%x\n", stval);
            panic("Load misaligned");
            break;
            
        case CAUSE_LOAD_ACCESS_FAULT://加载访问故障
            printf("Load access fault: addr=0x%x\n", stval);
            panic("Load access fault");
            break;
            
        case CAUSE_STORE_MISALIGNED://存储地址未对齐
            printf("Store address misaligned: addr=0x%x\n", stval);
            panic("Store misaligned");
            break;
            
        case CAUSE_STORE_ACCESS_FAULT://存储访问故障
            printf("Store access fault: addr=0x%x\n", stval);
            panic("Store access fault");
            break;
            
        case CAUSE_USER_ECALL://用户模式系统调用
            printf("User mode environment call\n");
            handle_syscall(tf);
            break;
            
        case CAUSE_SUPERVISOR_ECALL://特权模式系统调用
            printf("Supervisor mode environment call\n");
            handle_syscall(tf);
            break;
            
        case CAUSE_INSTRUCTION_PAGE_FAULT://指令页故障
            printf("Instruction page fault: addr=0x%x\n", stval);
            handle_instruction_page_fault(tf);
            break;
            
        case CAUSE_LOAD_PAGE_FAULT://加载页故障     
            printf("Load page fault: addr=0x%x\n", stval);
            handle_load_page_fault(tf);
            break;
            
        case CAUSE_STORE_PAGE_FAULT://存储页故障
            printf("Store page fault: addr=0x%x\n", stval);
            handle_store_page_fault(tf);
            break;
            
        default:
            printf("Unknown exception: cause=%d\n", cause);
            panic("Unknown exception");
    }
}

// 系统调用函数指针数组
static uint64 (*syscalls[])(void) = {
    [SYS_EXIT]   = sys_exit,
    [SYS_GETPID] = sys_getpid,
    [SYS_FORK]   = sys_fork,
    [SYS_WAIT]   = sys_wait,
    [SYS_READ]   = sys_read,
    [SYS_WRITE]  = sys_write,
    [SYS_OPEN]   = sys_open,
    [SYS_CLOSE]  = sys_close,
    [SYS_EXEC]   = sys_exec,
    [SYS_SBRK]   = sys_sbrk,
};

// 系统调用处理
void handle_syscall(struct trapframe *tf) {
    uint64 syscall_num = tf->a7;
    
    printf("System call: %d\n", syscall_num);
    
    // 检查系统调用号是否有效
    if(syscall_num > 0 && syscall_num < sizeof(syscalls)/sizeof(syscalls[0]) 
       && syscalls[syscall_num]) {
        // 调用对应的系统调用函数
        tf->a0 = syscalls[syscall_num]();
        printf("System call: %d, return value: %d\n", syscall_num, tf->a0);
    } else {
        printf("Unknown system call: %d\n", syscall_num);
        tf->a0 = -1;
        printf("Unknown syscall returned: %d (should be -1)\n", tf->a0);
    }
    
    // 系统调用完成后,EPC需要指向下一条指令
    tf->epc += 4;
}

// 指令页故障处理
void handle_instruction_page_fault(struct trapframe *tf) {
    printf("Instruction page fault handling not implemented\n");
    panic("Instruction page fault");
}

// 加载页故障处理
void handle_load_page_fault(struct trapframe *tf) {
    printf("Load page fault handling not implemented\n");
    panic("Load page fault");
}

// 存储页故障处理
void handle_store_page_fault(struct trapframe *tf) {
    printf("Store page fault handling not implemented\n");
    panic("Store page fault");
}

//测试函数
void test_timer_interrupt(void) {
    printf("Testing timer interrupt...\n");
    uint64 start_time = get_time();
    global_interrupt_count = 0;
    while (global_interrupt_count < 5) {
        printf("Waiting for interrupt %d...\n", global_interrupt_count + 1);
        for (volatile int i = 0; i < 10000000; i++);
    }
    
    uint64 end_time = get_time();
    printf("Timer test completed: %d interrupts in %d cycles\n",global_interrupt_count, end_time - start_time);
}

// 异常测试函数
void test_breakpoint(void) {
    printf("=== Testing Exception Handling ===\n");
    
    // 测试1：基本断点异常
    printf("\n--- Test 1: Basic Breakpoint ---\n");
    printf("Before breakpoint instruction\n");
    
    // 执行断点指令
    asm volatile("ebreak");
    
    // 这行代码只有在断点被正确处理并跳过时才会执行
    printf("After breakpoint instruction - SUCCESS!\n");
    
    // 测试2：连续断点
    printf("\n--- Test 2: Multiple Breakpoints ---\n");
    for (int i = 0; i < 3; i++) {
        printf("Breakpoint %d: ", i + 1);
        asm volatile("ebreak");
        printf("Handled successfully!\n");
    }
    
    printf("\n=== All Exception Tests Completed Successfully ===\n");
}

// 系统调用测试函数
void test_syscall(void) {
    printf("\n=== Testing System Call Handling ===\n");
    
    // 测试1: 测试 SYS_GETPID 系统调用
    printf("\n--- Test 1: SYS_GETPID ---\n");
    uint64 result;
    
    // 执行系统调用: li a7, 2; ecall
    asm volatile(
        "li a7, 2\n"        // 系统调用号 SYS_GETPID
        "ecall\n"           // 执行系统调用
        "mv %0, a0\n"
        : "=r"(result)
        :
        : "memory"
    );

    printf("SYS_GETPID returned: %d\n", result);
    
    
    // 测试2: 测试 SYS_EXIT 系统调用
    printf("\n--- Test 2: SYS_EXIT ---\n");
    asm volatile(
        "li a7, 1\n"        // 系统调用号 SYS_EXIT
        "li a0, 0\n"        // 退出码
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(result)
        :
        : "memory"
    );
    
    printf("SYS_EXIT returned: %d\n", result);
    printf("After SYS_EXIT (should not panic)\n");
    
    // 测试3: 测试 SYS_WRITE 系统调用
    printf("\n--- Test 3: SYS_WRITE ---\n");
    asm volatile(
        "li a7, 6\n"        // 系统调用号 SYS_WRITE
        "li a0, 1\n"        // 文件描述符 (stdout)
        "ecall\n"
        : "=r"(result)
        :
        : "memory"
    );
    printf("SYS_WRITE called\n");
    printf("SYS_WRITE returned: %d\n", result);
    
    // 测试4: 测试未知系统调用
    printf("\n--- Test 4: Unknown System Call ---\n");
    asm volatile(
        "li a7, 99\n"       // 未知系统调用号
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(result)
        :
        : "memory"
    );
    
    // 测试5: 测试所有已定义的系统调用
    printf("\n--- Test 5: All System Calls ---\n");
    for(int i = 1; i <= 10; i++) {
        asm volatile(
            "mv a7, %1\n"
            "ecall\n"
            "mv %0, a0\n"
            : "=r"(result)
            : "r"((uint64)i)
            : "memory"
        );
        printf("Syscall %d returned: %d\n", i, result);
    }
    
    printf("\n=== All System Call Tests Completed ===\n");
}

void test_exception(void){
    printf("\n=== Testing Exception Handling ===\n");
    
    // 测试2: 非法指令 (Illegal Instruction)
    printf("\n--- Test 2: Illegal Instruction ---\n");
    printf("This test will cause panic!\n");
    printf("Executing illegal instruction...\n");
    // 注意：这会导致panic，所以先注释掉
    // asm volatile(".word 0x00000000");  // 非法指令
    printf("Skipped: Would cause panic\n");
    
    // 测试3: 加载地址未对齐 (Load Misaligned)
    printf("\n--- Test 3: Load Address Misaligned ---\n");
    printf("This test will cause panic!\n");
    // 注意：这会导致panic，所以先注释掉
    // volatile uint64 *ptr = (uint64*)0x80000001;  // 未对齐的地址
    // uint64 value = *ptr;  // 尝试读取
    printf("Skipped: Would cause panic\n");
    
    // 测试4: 存储地址未对齐 (Store Misaligned)
    printf("\n--- Test 4: Store Address Misaligned ---\n");
    printf("This test will cause panic!\n");
    // 注意：这会导致panic，所以先注释掉
    // volatile uint64 *ptr = (uint64*)0x80000003;  // 未对齐的地址
    // *ptr = 0x1234;  // 尝试写入
    printf("Skipped: Would cause panic\n");
}