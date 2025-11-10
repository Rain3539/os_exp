#include "def.h"
#include "type.h"
#include "utils/console.h"
#include "proc/proc.h"

/* RISC-V操作系统主函数 - 实验5: 进程管理与调度 */

// 进程测试函数声明
void test_priority_scheduling(void);

// 测试任务函数声明
void high_priority_task(void);
void medium_priority_task(void);
void low_priority_task(void);


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

  // 扩展实验 - 优先级调度测试
  
  //优先级调度测试
  test_priority_scheduling();

  
  printf("\r\n=== All Test Processes Created! ===\r\n");

  // --- 新增：在这里打印初始进程表 ---
  debug_proc_table(); 

  printf("Starting scheduler...\r\n\r\n");
  
  // 进入调度器（不返回）
  scheduler();
}

// ========== 进程测试函数实现 ==========

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


// ========== 任务函数实现 ==========

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