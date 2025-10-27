#include "def.h"
#include "type.h"
#include "utils/console.h"
#include "proc/proc.h"

/* RISC-V操作系统主函数 - 实验5: 进程管理与调度 */

// 进程测试函数声明
void test_process_creation(void);
void test_scheduler(void);
void test_priority_scheduling(void);
void test_synchronization(void);

// 测试任务函数声明
void simple_task(void);
void cpu_intensive_task(void);
void high_priority_task(void);
void medium_priority_task(void);
void low_priority_task(void);
void producer_task(void);
void consumer_task(void);

// 生产者-消费者共享变量
static volatile int buffer = 0;
static volatile int buffer_full = 0;
static void *buffer_chan = (void*)&buffer;

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

  // 实验5测试 - 创建所有测试进程
  
  //1.进程创建测试
  test_process_creation();
  
  //2.调度器测试
  test_scheduler();
  
  //3.优先级调度测试
  test_priority_scheduling();
  
  //4.同步机制测试
  test_synchronization();
  
  printf("\r\n=== All Test Processes Created! ===\r\n");

  // --- 新增：在这里打印初始进程表 ---
  debug_proc_table(); 

  printf("Starting scheduler...\r\n\r\n");
  
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
  
  printf("Process creation test setup completed!\r\n\r\n");
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
    printf("Created CPU-intensive process: PID=%d, Name=%s\r\n", pid, name);
  }
  
  printf("Scheduler test setup completed!\r\n\r\n");
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
  
  printf("Priority scheduling test setup completed!\r\n");
  printf("Expected: high_prio should run first!\r\n\r\n");
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
  
  printf("Synchronization test setup completed!\r\n\r\n");
}


// 简单任务
void simple_task(void) {
  struct proc *p = myproc();
  printf("[%s] Simple task started (PID=%d)\r\n", p->name, p->pid);
  
  for(int i = 0; i < 3; i++) {
    printf("[%s] Working... iteration %d\r\n", p->name, i+1);
    
    // 模拟工作负载
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
  for(int i = 0; i < 5; i++) {
    // 执行大量计算
    for(volatile uint64 j = 0; j < 5000000; j++) {
      sum += j;
    }
    printf("[%s] Computed iteration %d\r\n", p->name, i+1);
    yield();  // 让出CPU
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
    
    // 模拟工作
    for(volatile int j = 0; j < 10000000; j++);
    
    yield();
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
    
    // 模拟工作
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
    
    // 模拟工作
    for(volatile int j = 0; j < 10000000; j++);
    
    yield();
  }
  
  printf("[LOW PRIORITY %s] Completed\r\n", p->name);
  exit(0);
}

// 生产者任务
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
    
    // 模拟生产时间
    for(volatile int j = 0; j < 10000000; j++);
    
    yield();
  }
  
  printf("[PRODUCD] Completed\r\n"); // (修正：应该是 PRODUCER)
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
    
    // 模拟消费时间
    for(volatile int j = 0; j < 10000000; j++);
    
    yield();
  }
  
  printf("[CONSUMER] Completed\r\n");
  exit(0);
}