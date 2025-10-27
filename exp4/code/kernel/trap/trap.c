#include "../mm/memlayout.h"
#include "../def.h"


// 全局变量定义
volatile int global_interrupt_count = 0;
volatile uint64 total_interrupt_overhead_cycles = 0;
// 声明汇编函数



void yield(void){
    // printf("temp yield\n");
}

// 声明在 kernelvec.S 中定义的汇编代码入口点
void kernelvec();

// 初始化内核态的陷阱处理
// 每个hart（CPU核心）在启动时都需要执行一次
void
trapinithart(void)
{
  // 设置 stvec (Supervisor Trap Vector Base Address Register) 寄存器。
  // 这个寄存器指向当在 S 模式下发生中断或异常时，CPU应该跳转到的代码地址。
  // 这里我们让它指向 kernelvec 这个汇编函数。
  w_stvec((uint64)kernelvec);
  // 开启 S 模式下的中断
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
    uint64 start_time = get_time(); // 记录开始时间

// 任务3: 触发任务调度
// 这里的 yield() 是一个简化的调度触发点。
    yield();
// 递增全局中断计数器
    global_interrupt_count++;
// 任务5: 设置下一次中断时间
// 必须重新设置下一次时钟中断，否则它不会再次发生。
// 这创建了一个周期性的“时钟滴答”。
    sbi_set_timer(1000000);

// 记录结束时间并累加开销
    uint64 end_time = get_time();
    total_interrupt_overhead_cycles += (end_time - start_time);
}

void 
kerneltrap(struct trapframe *tf)
{
    uint64 sepc = r_sepc();        // 读取 sepc 即发生陷阱时的指令地址
    uint64 sstatus = r_sstatus();  // 读取 sstatus包含了陷阱发生时的CPU状态
    uint64 scause = r_scause();    // 读取 scause 用于判断陷阱的原因
    // 检查陷阱是否来自 S 模式。SSTATUS_SPP 位为1表示前一个模式是 S 模式。
  // 如果不是，说明内核代码出了严重问题。
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");

    // 检查中断是否已经被禁用。在处理一个陷阱时，中断应该是关闭的，以防重入。
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  // 检查是中断还是异常
  // scause 寄存器的最高位为1表示中断，为0表示异常
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
   // 处理异常
    // 将 sepc 保存到陷阱帧中，这样异常处理函数可以知道出错的指令地址
    tf->epc = sepc;
    handle_exception(tf);
    // 异常处理函数可能会修改epc（例如，系统调用后需要指向下一条指令），所以需要更新sepc
    sepc = tf->epc;
  }
// 恢复 sepc 和 sstatus 寄存器，为 sret 返回做准备
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
    
    // 注意：不要在这里调用 w_a0()，因为会在 kerneltrap 中恢复
    
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


//时钟中断测试函数
void test_timer_interrupt(void) {
      printf("Testing timer interrupt...\n");
      // 获取当前时间
      uint64 start_time = get_time();
      // 初始化全局中断计数器为0
      global_interrupt_count = 0;
      // 循环等待，直到全局中断计数器达到5
     // 这意味着等待5次时钟中断发生
      while (global_interrupt_count < 5) {
      printf("Waiting for interrupt %d...\n", global_interrupt_count + 1);

      // 执行一个忙等待（busy-wait）循环,'volatile' 关键字确保编译器不会优化掉这个循环
      for (volatile int i = 0; i < 10000000; i++);
      }
      // 获取当前时间作为结束时间
      uint64 end_time = get_time();
      printf("Timer test completed: %d interrupts in %d cycles\n",global_interrupt_count, end_time - start_time);
}

// 断点测试函数
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

//异常测试函数
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


// 性能测试：测量中断开销
void test_interrupt_overhead(void) {
    printf("\n=== Testing Interrupt Overhead ===\n");
  
    int num_interrupts_to_test = 10; // 测试10次中断以获取平均值
  
// 1. 重置计数器
    global_interrupt_count = 0;
    total_interrupt_overhead_cycles = 0;
    
// 2. 记录总开始时间
     uint64 start_time = get_time();
    
     printf("Waiting for %d timer interrupts...\n", num_interrupts_to_test);
     
// 3. 等待 N 次中断发生
     while (global_interrupt_count < num_interrupts_to_test) {
// 使用一个更长的忙等待循环来确保有足够的时间让中断发生
    for (volatile int i = 0; i < 20000000; i++); 
     }
     
// 4. 记录总结束时间
    uint64 end_time = get_time();
   
// 5. 抓取累加的开销时间 (volatile read)
    uint64 total_handler_time = total_interrupt_overhead_cycles;
    uint64 total_test_duration = end_time - start_time;
    
// 6. 计算并打印结果
     if (global_interrupt_count > 0) {
    uint64 avg_overhead = total_handler_time / global_interrupt_count;
     printf("Test complete for %d interrupts.\n", global_interrupt_count);
     printf("Total test duration: %d cycles\n", total_test_duration);
     printf("Total time spent in handler: %d cycles\n", total_handler_time);
     printf("Average interrupt handler overhead: %d cycles per interrupt\n", avg_overhead);
    } else {
     printf("Error: No interrupts were counted.\n");
     }
     printf("=== Interrupt Overhead Test Passed ===\n");
     
// 重置计数器，以免影响其他测试
   global_interrupt_count = 0;
    total_interrupt_overhead_cycles = 0;
}