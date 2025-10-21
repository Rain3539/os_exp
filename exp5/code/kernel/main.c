#include "def.h"
#include "type.h"
#include "utils/console.h"
#include "proc/proc.h"

/* RISC-V操作系统主函数 - 实验5: 进程管理与调度 */

// 测试函数声明
void test_printf_basic();
void test_printf_edge_cases();
void console_demo(void);
void progress_bar_demo(void);
void test_physical_memory(void);
void test_pagetable(void);

// 进程测试函数
void test_process_creation(void);
void test_scheduler(void);
void test_priority_scheduling(void);
void test_synchronization(void);

// 测试任务函数
void simple_task(void);
void cpu_intensive_task(void);
void high_priority_task(void);
void medium_priority_task(void);
void low_priority_task(void);
void producer_task(void);
void consumer_task(void);

void system_shutdown(void);
void delay_seconds(int seconds);

void main(void) {
  printf("My RISC-V OS Starting...\r\n");
  printf("Lab 5: Process Management and Scheduling\r\n\r\n");

  // 初始化各个子系统
  printf("Initializing memory management...\r\n");
  kinit();
  
  printf("Initializing virtual memory...\r\n");
  kvminit();
  kvminithart();
  
  printf("Initializing trap handling...\r\n");
  trapinithart();
  
  printf("Initializing process management...\r\n");
  procinit();

  printf("\r\n=== System initialization complete! ===\r\n\r\n");

  // 基础测试（之前的实验）
  printf("=== Running Basic Tests ===\r\n");
  // test_printf_basic();
  // test_physical_memory();
  // test_pagetable();
  test_timer_interrupt();
  printf("Timer interrupt test passed!\r\n");
  test_breakpoint();
  printf("Breakpoint test passed!\r\n");
  test_syscall();
  printf("System call test passed!\r\n");
  
  printf("\r\n=== Running Process Management Tests ===\r\n\r\n");
  
  // 实验5测试
  test_process_creation();
  test_scheduler();
  test_priority_scheduling();
  test_synchronization();
  
  printf("\r\n=== All Tests Completed Successfully! ===\r\n");
  printf("Entering scheduler loop...\r\n\r\n");
  
  // 进入调度器（不返回）
  scheduler();
}

// ========== 进程测试函数实现 ==========

// 测试1: 进程创建测试
void test_process_creation(void) {
  printf("--- Test 1: Process Creation ---\r\n");
  
  // 创建简单进程
  int pid1 = create_process(simple_task, "task1", 2);
  printf("Created process: PID=%d, Name=task1, Priority=2\r\n", pid1);
  
  int pid2 = create_process(simple_task, "task2", 2);
  printf("Created process: PID=%d, Name=task2, Priority=2\r\n", pid2);
  
  int pid3 = create_process(simple_task, "task3", 3);
  printf("Created process: PID=%d, Name=task3, Priority=3\r\n", pid3);
  
  // 显示进程表
  debug_proc_table();
  
  // 等待一段时间让进程运行
  printf("Waiting for processes to run...\r\n");
  for(volatile int i = 0; i < 50000000; i++);
  
  printf("Process creation test completed!\r\n\r\n");
}

// 测试2: 调度器测试
void test_scheduler(void) {
  printf("--- Test 2: Scheduler Test ---\r\n");
  
  // 创建多个计算密集型进程
  printf("Creating CPU-intensive processes...\r\n");
  for(int i = 0; i < 3; i++) {
    char name[16];
    name[0] = 'c';
    name[1] = 'p';
    name[2] = 'u';
    name[3] = '0' + i;
    name[4] = 0;
    int pid = create_process(cpu_intensive_task, name, 2);
    printf("Created CPU-intensive process: PID=%d\r\n", pid);
  }
  
  // 显示进程表
  debug_proc_table();
  
  // 让进程运行一段时间
  printf("Scheduler running...\r\n");
  uint64 start_time = r_time();
  for(volatile int i = 0; i < 100000000; i++);
  uint64 end_time = r_time();
  
  printf("Scheduler test completed in %d cycles\r\n", end_time - start_time);
  printf("Interrupts occurred: %d\r\n\r\n", global_interrupt_count);
}

// 测试3: 优先级调度测试
void test_priority_scheduling(void) {
  printf("--- Test 3: Priority Scheduling Test ---\r\n");
  
  // 创建不同优先级的进程
  printf("Creating processes with different priorities...\r\n");
  
  int pid_low = create_process(low_priority_task, "low_prio", 4);
  printf("Created low priority process: PID=%d, Priority=4\r\n", pid_low);
  
  int pid_med = create_process(medium_priority_task, "med_prio", 2);
  printf("Created medium priority process: PID=%d, Priority=2\r\n", pid_med);
  
  int pid_high = create_process(high_priority_task, "high_prio", 0);
  printf("Created high priority process: PID=%d, Priority=0\r\n", pid_high);
  
  // 显示进程表
  debug_proc_table();
  
  // 观察调度行为
  printf("Observing priority scheduling...\r\n");
  printf("High priority task should run first!\r\n\r\n");
  
  for(volatile int i = 0; i < 100000000; i++);
  
  printf("Priority scheduling test completed!\r\n\r\n");
}

// 测试4: 同步机制测试
void test_synchronization(void) {
  printf("--- Test 4: Synchronization Test ---\r\n");
  printf("Testing sleep/wakeup mechanism...\r\n");
  
  // 创建生产者和消费者进程
  int pid_producer = create_process(producer_task, "producer", 2);
  printf("Created producer process: PID=%d\r\n", pid_producer);
  
  int pid_consumer = create_process(consumer_task, "consumer", 2);
  printf("Created consumer process: PID=%d\r\n", pid_consumer);
  
  debug_proc_table();
  
  // 让进程运行
  printf("Producer-Consumer running...\r\n");
  for(volatile int i = 0; i < 100000000; i++);
  
  printf("Synchronization test completed!\r\n\r\n");
}

// ========== 测试任务实现 ==========

// 简单任务
void simple_task(void) {
  struct proc *p = myproc();
  printf("[%s] Simple task started (PID=%d)\r\n", p->name, p->pid);
  
  for(int i = 0; i < 3; i++) {
    printf("[%s] Working... iteration %d\r\n", p->name, i+1);
    for(volatile int j = 0; j < 10000000; j++);
    yield();  // 主动让出CPU
  }
  
  printf("[%s] Task completed, exiting\r\n", p->name);
  exit(0);
}

// CPU密集型任务
void cpu_intensive_task(void) {
  struct proc *p = myproc();
  printf("[%s] CPU intensive task started (PID=%d)\r\n", p->name, p->pid);
  
  volatile uint64 sum = 0;
  for(int i = 0; i < 10; i++) {
    // 执行大量计算
    for(volatile uint64 j = 0; j < 5000000; j++) {
      sum += j;
    }
    printf("[%s] Computed iteration %d, sum=%d\r\n", p->name, i+1, sum);
  }
  
  printf("[%s] Task completed, exiting\r\n", p->name);
  exit(0);
}

// 高优先级任务
void high_priority_task(void) {
  struct proc *p = myproc();
  printf("[HIGH PRIORITY %s] Started (PID=%d, Priority=%d)\r\n", 
         p->name, p->pid, p->priority);
  
  for(int i = 0; i < 3; i++) {
    printf("[HIGH] Executing important task %d\r\n", i+1);
    for(volatile int j = 0; j < 10000000; j++);
  }
  
  printf("[HIGH PRIORITY %s] Completed\r\n", p->name);
  exit(0);
}

// 中等优先级任务
void medium_priority_task(void) {
  struct proc *p = myproc();
  printf("[MEDIUM PRIORITY %s] Started (PID=%d, Priority=%d)\r\n", 
         p->name, p->pid, p->priority);
  
  for(int i = 0; i < 3; i++) {
    printf("[MEDIUM] Executing task %d\r\n", i+1);
    for(volatile int j = 0; j < 10000000; j++);
    yield();
  }
  
  printf("[MEDIUM PRIORITY %s] Completed\r\n", p->name);
  exit(0);
}

// 低优先级任务
void low_priority_task(void) {
  struct proc *p = myproc();
  printf("[LOW PRIORITY %s] Started (PID=%d, Priority=%d)\r\n", 
         p->name, p->pid, p->priority);
  
  for(int i = 0; i < 3; i++) {
    printf("[LOW] Executing background task %d\r\n", i+1);
    for(volatile int j = 0; j < 10000000; j++);
    yield();
  }
  
  printf("[LOW PRIORITY %s] Completed\r\n", p->name);
  exit(0);
}

// 生产者任务
static volatile int buffer = 0;
static volatile int buffer_full = 0;
static void *buffer_chan = (void*)&buffer;

void producer_task(void) {
  struct proc *p = myproc();
  printf("[PRODUCER %s] Started (PID=%d)\r\n", p->name, p->pid);
  
  for(int i = 1; i <= 5; i++) {
    // 等待buffer为空
    while(buffer_full) {
      printf("[PRODUCER] Buffer full, sleeping...\r\n");
      sleep(buffer_chan);
    }
    
    // 生产数据
    buffer = i;
    buffer_full = 1;
    printf("[PRODUCER] Produced: %d\r\n", buffer);
    
    // 唤醒消费者
    wakeup(buffer_chan);
    
    for(volatile int j = 0; j < 10000000; j++);
  }
  
  printf("[PRODUCER] Completed\r\n");
  exit(0);
}

// 消费者任务
void consumer_task(void) {
  struct proc *p = myproc();
  printf("[CONSUMER %s] Started (PID=%d)\r\n", p->name, p->pid);
  
  for(int i = 0; i < 5; i++) {
    // 等待buffer有数据
    while(!buffer_full) {
      printf("[CONSUMER] Buffer empty, sleeping...\r\n");
      sleep(buffer_chan);
    }
    
    // 消费数据
    int data = buffer;
    buffer_full = 0;
    printf("[CONSUMER] Consumed: %d\r\n", data);
    
    // 唤醒生产者
    wakeup(buffer_chan);
    
    for(volatile int j = 0; j < 10000000; j++);
  }
  
  printf("[CONSUMER] Completed\r\n");
  exit(0);
}

// ========== 辅助函数 ==========

void delay_seconds(int seconds) {
  volatile int count = 0;
  int loops_per_second = 250000000;
  printf("Working...\r\n");
  for (int i = 0; i < seconds; i++) {
    for (int j = 0; j < loops_per_second; j++) {
      count++;
    }
    printf("%d...", seconds - i - 1);
  }
}

void system_shutdown(void) {
  printf("System shutting down...\n");
  volatile uint32 *test_finisher = (volatile uint32 *)0x100000;
  *test_finisher = 0x5555;
  asm volatile("wfi");
  while (1);
}

void test_printf_basic() {
  printf("Testing integer: %d\n", 42);
  printf("Testing negative: %d\n", -123);
  printf("Testing zero: %d\n", 0);
  printf("Testing hex: 0x%x\n", 0xABC);
  printf("Testing string: %s\n", "Hello");
  printf("Testing char: %c\n", 'X');
  printf("Testing percent: %%\n");
}

void test_printf_edge_cases() {
  printf("INT_MAX: %d\n", 2147483647);
  printf("INT_MIN: %d\n", -2147483648);
  printf("NULL string: %s\n", (char *)0);
  printf("Empty string: %s\n", "");
}

void console_demo(void) {
  clear_screen();
  goto_xy(20, 2);
  printf_color(ANSI_COLOR_CYAN, "=== RISC-V OS Console Demo ===");
  goto_xy(5, 4);
  printf_color(ANSI_COLOR_RED, "Red Text");
  goto_xy(5, 5);
  printf_color(ANSI_COLOR_GREEN, "Green Text");
  goto_xy(30, 4);
  printf("Cursor moved to (30,4)");
}

void progress_bar_demo(void) {
  clear_screen();
  goto_xy(10, 5);
  cons_puts("Progress Bar Demo:");
  for (int i = 0; i <= 20; i++) {
    goto_xy(10, 7);
    cons_puts("Progress: [");
    for (int j = 0; j < 20; j++) {
      if (j < i) {
        uart_putc('=');
      } else {
        uart_putc(' ');
      }
    }
    cons_puts("] ");
    printf("%d%%", (i * 100) / 20);
    volatile int delay = 0;
    for (int k = 0; k < 1000000; k++) {
      delay++;
    }
  }
  goto_xy(10, 9);
  printf_color(ANSI_COLOR_GREEN, "Complete!");
}

void test_physical_memory(void) {
  void *page1 = kalloc();
  void *page2 = kalloc();
  assert(page1 != page2);
  assert(((uint64)page1 & 0xFFF) == 0);
  *(int*)page1 = 0x12345678;
  assert(*(int*)page1 == 0x12345678);
  kfree(page1);
  void *page3 = kalloc();
  kfree(page2);
  kfree(page3);
  printf("Physical memory test passed!\n");
}

void test_pagetable(void) {
  pagetable_t pt = create_pagetable();
  uint64 va = 0x10000;
  uint64 pa = (uint64)kalloc();
  assert(mappages(pt, va, 4096, pa, PTE_R | PTE_W) == 0);
  uint64 *pte = (uint64 *)walk(pt, va, 0);
  assert(pte != 0 && (*pte & PTE_V));
  assert(PTE2PA(*pte) == pa);
  assert(*pte & PTE_R);
  assert(*pte & PTE_W);
  assert(!(*pte & PTE_X));
  printf("Pagetable test passed!\n");
}