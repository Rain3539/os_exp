#include "def.h"
#include "type.h"
#include "utils/console.h"
#include "proc/proc.h"

/* RISC-V操作系统主函数 - 扩展实验: 优先级调度 */

// 测试函数声明
void test_priority_scheduling(void);
void test_aging_mechanism(void);
void test_same_priority(void);

// 测试任务函数声明
void high_priority_task(void);
void medium_priority_task(void);
void low_priority_task(void);
void equal_priority_task_1(void);
void equal_priority_task_2(void);
void aging_test_task_high(void);
void aging_test_task_low(void);

void main(void) {
  printf("====================================\n");
  printf("   RISC-V OS - Priority Scheduler\n");
  printf("====================================\n\n");

  // 初始化各个子系统
  printf("Initializing memory management...\n");
  kinit();
  
  printf("Initializing virtual memory...\n");
  kvminit();
  kvminithart();
  
  printf("Initializing trap handling...\n");
  trapinithart();
  
  printf("Initializing process management...\n");
  procinit();

  printf("\n=== System Initialization Complete ===\n\n");

  // 选择测试场景
  printf("=== Test Scenarios ===\n");
  printf("1. Priority Scheduling Test (Different Priorities)\n");
  printf("2. Aging Mechanism Test\n");
  printf("3. Same Priority Test (Round Robin)\n");
  printf("\n");

  // 测试1: 不同优先级测试
  // test_priority_scheduling();
  
  // 测试2: Aging机制测试（可选）
  test_aging_mechanism();
  
  // 测试3: 相同优先级测试（可选）
  // test_same_priority();

  printf("\n=== All Test Processes Created ===\n\n");

  // 打印初始进程表
  debug_proc_table();

  printf("Starting scheduler...\n\n");
  
  // 进入调度器（不返回）
  scheduler();
}

// ========== 测试函数实现 ==========

// 测试1: 不同优先级调度测试
void test_priority_scheduling(void) {
  printf("--- Test 1: Priority Scheduling (Different Priorities) ---\n");
  
  // 创建低优先级进程
  int pid_low = create_process(low_priority_task, "low_prio", 2);
  printf("Created: PID=%d, Name=low_prio, Priority=2\n", pid_low);
  
  // 创建中等优先级进程
  int pid_med = create_process(medium_priority_task, "med_prio", 5);
  printf("Created: PID=%d, Name=med_prio, Priority=5\n", pid_med);
  
  // 创建高优先级进程
  int pid_high = create_process(high_priority_task, "high_prio", 8);
  printf("Created: PID=%d, Name=high_prio, Priority=8\n", pid_high);
  
  printf("Expected: high_prio (8) should run first, then med_prio (5), then low_prio (2)\n\n");
}

// 测试2: Aging机制测试
void test_aging_mechanism(void) {
  printf("--- Test 2: Aging Mechanism Test ---\n");
  
  // 创建一个高优先级任务（占用CPU）
  int pid_high = create_process(aging_test_task_high, "cpu_hog", 9);
  printf("Created: PID=%d, Name=cpu_hog, Priority=9 (CPU intensive)\n", pid_high);
  
  // 创建一个低优先级任务（等待被aging提升）
  int pid_low = create_process(aging_test_task_low, "starving", 1);
  printf("Created: PID=%d, Name=starving, Priority=1 (should be boosted)\n", pid_low);
  
  printf("Expected: starving process should eventually get CPU time via aging\n\n");
}

// 测试3: 相同优先级测试
void test_same_priority(void) {
  printf("--- Test 3: Same Priority Test (Round Robin) ---\n");
  
  int pid1 = create_process(equal_priority_task_1, "equal_1", 5);
  printf("Created: PID=%d, Name=equal_1, Priority=5\n", pid1);
  
  int pid2 = create_process(equal_priority_task_2, "equal_2", 5);
  printf("Created: PID=%d, Name=equal_2, Priority=5\n", pid2);
  
  printf("Expected: Both processes should alternate execution\n\n");
}

// ========== 任务函数实现 ==========

// 高优先级任务
void high_priority_task(void) {
  struct proc *p = myproc();
  printf("\n[HIGH PRIORITY] Process %d (%s) started\n", p->pid, p->name);
  printf("[HIGH] Priority=%d\n\n", p->priority);
  
  for(int i = 0; i < 5; i++) {
    printf("[HIGH %s] Iteration %d/5\n", p->name, i+1);
    
    // 模拟工作
    for(volatile int j = 0; j < 5000000; j++);
    
    yield();
  }
  
  printf("\n[HIGH PRIORITY] Process %d (%s) completed\n\n", p->pid, p->name);
  exit(0);
}

// 中等优先级任务
void medium_priority_task(void) {
  struct proc *p = myproc();
  printf("\n[MEDIUM PRIORITY] Process %d (%s) started\n", p->pid, p->name);
  printf("[MEDIUM] Priority=%d\n\n", p->priority);
  
  for(int i = 0; i < 5; i++) {
    printf("[MEDIUM %s] Iteration %d/5\n", p->name, i+1);
    
    // 模拟工作
    for(volatile int j = 0; j < 5000000; j++);
    
    yield();
  }
  
  printf("\n[MEDIUM PRIORITY] Process %d (%s) completed\n\n", p->pid, p->name);
  exit(0);
}

// 低优先级任务
void low_priority_task(void) {
  struct proc *p = myproc();
  printf("\n[LOW PRIORITY] Process %d (%s) started\n", p->pid, p->name);
  printf("[LOW] Priority=%d\n\n", p->priority);
  
  for(int i = 0; i < 5; i++) {
    printf("[LOW %s] Iteration %d/5\n", p->name, i+1);
    
    // 模拟工作
    for(volatile int j = 0; j < 5000000; j++);
    
    yield();
  }
  
  printf("\n[LOW PRIORITY] Process %d (%s) completed\n\n", p->pid, p->name);
  exit(0);
}

// 相同优先级任务1
void equal_priority_task_1(void) {
  struct proc *p = myproc();
  printf("[EQUAL_1] Process %d started (Priority=%d)\n", p->pid, p->priority);
  
  for(int i = 0; i < 3; i++) {
    printf("[EQUAL_1] Working %d/3\n", i+1);
    for(volatile int j = 0; j < 5000000; j++);
    yield();
  }
  
  printf("[EQUAL_1] Process %d completed\n", p->pid);
  exit(0);
}

// 相同优先级任务2
void equal_priority_task_2(void) {
  struct proc *p = myproc();
  printf("[EQUAL_2] Process %d started (Priority=%d)\n", p->pid, p->priority);
  
  for(int i = 0; i < 3; i++) {
    printf("[EQUAL_2] Working %d/3\n", i+1);
    for(volatile int j = 0; j < 5000000; j++);
    yield();
  }
  
  printf("[EQUAL_2] Process %d completed\n", p->pid);
  exit(0);
}

// Aging测试 - CPU密集型任务
void aging_test_task_high(void) {
  struct proc *p = myproc();
  printf("[CPU_HOG] Process %d started (Priority=%d)\n", p->pid, p->priority);
  
  for(int i = 0; i < 10; i++) {
    printf("[CPU_HOG] Running %d/10\n", i+1);
    

    for(volatile int j = 0; j < 10000000; j++);  
    yield();
  }
  
  printf("[CPU_HOG] Process %d completed\n", p->pid);
  exit(0);
}

// Aging测试 - 低优先级任务（等待被提升）
void aging_test_task_low(void) {
  struct proc *p = myproc();
  printf("[STARVING] Process %d started (Priority=%d)\n", p->pid, p->priority);
  
  for(int i = 0; i < 5; i++) {
    printf("[STARVING] Finally running! Iteration %d/5 (Current Priority=%d)\n", 
           i+1, p->priority);
    for(volatile int j = 0; j < 3000000; j++);
    yield();
  }
  
  printf("[STARVING] Process %d completed (Final Priority=%d)\n", 
         p->pid, p->priority);
  exit(0);
}