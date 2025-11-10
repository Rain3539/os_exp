#include "../mm/memlayout.h"
#include "../def.h"
#include "../proc/proc.h"

// 全局变量定义
volatile int global_interrupt_count = 0;

// 外部声明
extern void kernelvec();
extern void handle_syscall(struct trapframe *tf);  // 在syscall/syscall.c中实现

// 设置异常和陷阱处理
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

// 时钟中断处理
void timer_interrupt(void) {
    // 递增全局中断计数器
    global_interrupt_count++;
    
    // 设置下次中断时间
    sbi_set_timer(1000000);
    
    // 触发任务调度（时间片用完）
    struct proc *p = myproc();
    if(p && p->state == RUNNING) {
        yield();
    }
}

// 内核陷阱处理
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
        printf("[INTERRUPT] Software interrupt\n");
        break;
      case CAUSE_EXTERNAL_INTERRUPT:
        printf("[INTERRUPT] External interrupt\n");
        break;
      default:
        printf("[INTERRUPT] Unknown interrupt: %d\n", interrupt_cause);
        panic("Unknown interrupt");
    }
  } else {
    // 处理异常
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
    uint64 stval = r_stval();
    
    switch (cause) {
        case CAUSE_INSTRUCTION_MISALIGNED:
            printf("[EXCEPTION] Instruction address misaligned\n");
            panic("Instruction misaligned");
            break;
            
        case CAUSE_INSTRUCTION_ACCESS_FAULT:
            printf("[EXCEPTION] Instruction access fault\n");
            panic("Instruction access fault");
            break;
            
        case CAUSE_ILLEGAL_INSTRUCTION:
            printf("[EXCEPTION] Illegal instruction at 0x%lx\n", tf->epc);
            panic("Illegal instruction");
            break;
            
        case CAUSE_BREAKPOINT:
            printf("[EXCEPTION] Breakpoint at 0x%lx\n", tf->epc);
            tf->epc += 2;  // 跳过ebreak指令
            break;
            
        case CAUSE_LOAD_MISALIGNED:
            printf("[EXCEPTION] Load address misaligned: addr=0x%lx\n", stval);
            panic("Load misaligned");
            break;
            
        case CAUSE_LOAD_ACCESS_FAULT:
            printf("[EXCEPTION] Load access fault: addr=0x%lx\n", stval);
            panic("Load access fault");
            break;
            
        case CAUSE_STORE_MISALIGNED:
            printf("[EXCEPTION] Store address misaligned: addr=0x%lx\n", stval);
            panic("Store misaligned");
            break;
            
        case CAUSE_STORE_ACCESS_FAULT:
            printf("[EXCEPTION] Store access fault: addr=0x%lx\n", stval);
            panic("Store access fault");
            break;
            
        case CAUSE_USER_ECALL:
        case CAUSE_SUPERVISOR_ECALL:
            // 系统调用处理（在syscall/syscall.c中实现）
            handle_syscall(tf);
            break;
            
        case CAUSE_INSTRUCTION_PAGE_FAULT:
            printf("[EXCEPTION] Instruction page fault: addr=0x%lx\n", stval);
            handle_instruction_page_fault(tf);
            break;
            
        case CAUSE_LOAD_PAGE_FAULT:
            printf("[EXCEPTION] Load page fault: addr=0x%lx\n", stval);
            handle_load_page_fault(tf);
            break;
            
        case CAUSE_STORE_PAGE_FAULT:
            printf("[EXCEPTION] Store page fault: addr=0x%lx\n", stval);
            handle_store_page_fault(tf);
            break;
            
        default:
            printf("[EXCEPTION] Unknown exception: cause=%ld\n", cause);
            panic("Unknown exception");
    }
}

// 页故障处理（简化实现）
void handle_instruction_page_fault(struct trapframe *tf) {
    printf("[PAGE FAULT] Instruction page fault not implemented\n");
    panic("Instruction page fault");
}

void handle_load_page_fault(struct trapframe *tf) {
    printf("[PAGE FAULT] Load page fault not implemented\n");
    panic("Load page fault");
}

void handle_store_page_fault(struct trapframe *tf) {
    printf("[PAGE FAULT] Store page fault not implemented\n");
    panic("Store page fault");
}

// ========== 测试函数 ==========

void test_timer_interrupt(void) {
    printf("\n=== Testing Timer Interrupt ===\n");
    uint64 start_time = get_time();
    global_interrupt_count = 0;
    
    while (global_interrupt_count < 5) {
        printf("Waiting for interrupt %d...\n", global_interrupt_count + 1);
        for (volatile int i = 0; i < 10000000; i++);
    }
    
    uint64 end_time = get_time();
    printf("Timer test completed: %d interrupts in %ld cycles\n",
           global_interrupt_count, end_time - start_time);
}

void test_breakpoint(void) {
    printf("\n=== Testing Breakpoint Exception ===\n");
    printf("Before breakpoint\n");
    asm volatile("ebreak");
    printf("After breakpoint - SUCCESS!\n");
}

void test_syscall(void) {
    printf("\n=== Testing System Calls ===\n");
    uint64 result;
    
    // 测试getpid
    asm volatile(
        "li a7, 2\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(result)
        :
        : "memory"
    );
    printf("SYS_GETPID returned: %ld\n", result);
}