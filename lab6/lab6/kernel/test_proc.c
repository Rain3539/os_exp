/**
 * 进程管理与调度测试
 * 包含：进程创建、调度与同步的简化测试用例
 */

#include "kernel.h"
#include "printf.h"
extern volatile int need_resched; // 来自 sched.c

static void simple_task(void) {
    printf("[simple_task] started\n");
    for (int i = 0; i < 3; i++) {
        printf("[simple_task] tick %d\n", i);
    }
    printf("[simple_task] exiting\n");
}

static void cpu_intensive_task(void) {
    volatile int x = 0;
    for (int i = 0; i < 100000; i++) x += i;
    printf("[cpu_task] done work, sum=%d\n", x);
}

// 生产者/消费者（使用 buf_lock 的正确实现）
static int buffer = 0;
static int have_item = 0;
static struct spinlock buf_lock;

static void shared_buffer_init(void) {
    initlock(&buf_lock, "buf_lock");
    buffer = 0;
    have_item = 0;
}

static void producer_task(void) {
    for (int i = 1; i <= 3; i++) {
        acquire(&buf_lock);
        while (have_item) {
            sleep(&have_item, &buf_lock); // releases buf_lock while sleeping
        }
        buffer = i;
        have_item = 1;
        printf("[producer] produced %d\n", i);
        wakeup(&have_item);
        release(&buf_lock);
    }
    printf("[producer] finished\n");
}

static void consumer_task(void) {
    for (int i = 1; i <= 3; i++) {
        acquire(&buf_lock);
        while (!have_item) {
            sleep(&have_item, &buf_lock);
        }
        int v = buffer;
        have_item = 0;
        printf("[consumer] consumed %d\n", v);
        wakeup(&have_item);
        release(&buf_lock);
    }
    printf("[consumer] finished\n");
}

static void test_process_creation(void);
static void test_scheduler(void);
static void test_synchronization(void);

void run_proc_tests(void) {
    printf("=== 开始进程管理测试 ===\n\n");
    test_process_creation();
    test_scheduler();
    test_synchronization();
    printf("=== 进程管理测试结束 ===\n\n");
}

// ----------------- 正式测试实现 -----------------
// 简单断言宏
#define ASSERT(x) do { if (!(x)) { printf("ASSERT FAILED: %s\n", #x); panic("assert"); } } while(0)

static void wait_all_children(int total) {
    for (int i = 0; i < total; i++) {
        wait_process(0);
    }
}

static void drive_scheduler_for_cycles(uint64 cycles) {
    uint64 stop = get_time() + cycles;
    while (get_time() < stop) {
        need_resched = 1;
        scheduler_step();
    }
}

void test_process_creation(void) {
    printf("Testing process creation...\n");

    // 测试基本的进程创建
    int first_pid = create_process(simple_task);
    ASSERT(first_pid > 0);

    // 测试进程表限制
    int count = 0;
    for (int i = 0; i < NPROC + 5; i++) {
        int pid = create_process(simple_task);
        if (pid > 0) {
            count++;
        } else {
            break;
        }
    }
    printf("Created %d processes\n", count);

    wait_all_children(count);
    wait_process(0); // 等待 first_pid

    printf("Process creation test completed\n\n");
}

void test_scheduler(void) {
    printf("Testing scheduler...\n");

    int created = 0;
    for (int i = 0; i < 3; i++) {
        if (create_process(cpu_intensive_task) > 0)
            created++;
    }

    uint64 start_time = get_time();
    drive_scheduler_for_cycles(1000000); // 约 1ms
    uint64 end_time = get_time();

    printf("Scheduler test completed in %lu cycles\n\n", end_time - start_time);

    wait_all_children(created);
}

void test_synchronization(void) {
    printf("Testing synchronization (producer-consumer)...\n");
    shared_buffer_init();

    int producers = create_process(producer_task) > 0 ? 1 : 0;
    int consumers = create_process(consumer_task) > 0 ? 1 : 0;

    wait_all_children(producers + consumers);

    printf("Synchronization test completed\n\n");
}

void debug_proc_table(void) {
    printf("=== Process Table ===\n");
    for (int i = 0; i < NPROC; i++) {
        struct proc *p = &proc[i];
        if (p->state != UNUSED) {
            printf("PID:%d State:%d Name:%s\n", p->pid, p->state, p->name);
        }
    }
}
