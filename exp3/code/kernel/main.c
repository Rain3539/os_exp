#include <stdio.h>
#include <console.h>
#include <stddef.h>
#include <riscv.h>

// --- 函数前向声明 ---
// 这些函数定义在其他文件中（kalloc.c, vm.c），但我们需要在这里调用它们。
// 声明它们可以告诉编译器这些函数的存在及其签名，避免编译错误。

// 物理内存管理函数 (kalloc.c)
void kinit();      // 初始化物理内存分配器
void* kalloc(void); // 分配一个物理页
void kfree(void*); // 释放一个物理页

// 虚拟内存管理函数 (vm.c)
void kvminit();      // 创建并初始化内核页表
void kvminithart();  // 在当前 CPU 核心上激活页表

// --- 测试所需的虚拟内存函数前向声明 ---
// (这些函数在 vm.c 中，但我们需要在这里调用它们来进行测试)
pte_t* walk(pagetable_t, uint64, int);
int mappages(pagetable_t, uint64, uint64, uint64, int);

// --- 用于测试的断言宏 ---
// assert(x) 宏用于在测试中验证某个条件 (x) 是否为真。
// 如果条件 (x) 为假，它会：
// 1. 打印一条包含文件名、行号和失败条件的错误信息。
// 2. 进入一个死循环 (while(1))，使系统停机，以便开发者可以观察到错误。
#define assert(x) do { \
    if (!(x)) { \
        printf("Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #x); \
        while(1); \
    } \
} while(0)

/**
 * @brief 测试物理内存分配器 (kalloc, kfree)
 */
void test_physical_memory(void) {
    printf("--- Running Test: Physical Memory Allocator ---\n");
    
    // 1. 测试基本分配
    // 尝试分配两个物理页
    void *page1 = kalloc();
    printf("Allocated page 1 at: 0x%x\n", page1);
    assert(page1 != NULL); // 断言 page1 不是 NULL，表示分配成功

    void *page2 = kalloc();
    printf("Allocated page 2 at: 0x%x\n", page2);
    assert(page2 != NULL); // 断言 page2 也分配成功

    // 验证分配的两个页地址不同，并且是页对齐的
    assert(page1 != page2);
    assert(((uint64)page1 & 0xFFF) == 0); // 页对齐检查：地址的低12位应为0

    // 2. 测试对分配的内存进行读写
    *(int*)page1 = 0x12345678; // 向 page1 写入一个整数
    assert(*(int*)page1 == 0x12345678); // 读出并验证数据是否正确
    printf("Data write/read to page 1 OK.\n");

    // 3. 测试释放和重新分配
    // kfree 将页面放回空闲链表的头部，所以下一次 kalloc 应该会重新分配这个页面。
    kfree(page1);
    printf("Freed page 1.\n");
    void *page3 = kalloc();
    printf("Allocated page 3 at: 0x%x\n", page3);
    // 断言新分配的 page3 就是刚刚被释放的 page1
    assert(page3 == page1); 
    printf("Re-allocation test OK (page3 == page1).\n");

    // 清理：释放所有已分配的页面
    kfree(page2);
    kfree(page3);
    printf("Freed pages 2 and 3.\n");
    printf("--- Test Passed: Physical Memory Allocator ---\n\n");
}

/**
 * @brief 测试页表功能 (walk, mappages)
 */
void test_pagetable(void) {
    printf("--- Running Test: Page Table Management ---\n");
    // 首先，为测试创建一个临时的根页表
    pagetable_t pt = (pagetable_t)kalloc();
    for(int i=0; i<512; i++) pt[i] = 0; // 清零页表，确保所有PTE都无效

    // 1. 测试基本映射功能
    uint64 va = 0x1000; // 选择一个虚拟地址
    uint64 pa = (uint64)kalloc(); // 分配一个物理页作为映射目标
    printf("Mapping VA 0x%x to PA 0x%x\n", va, pa);
    // 调用 mappages 将 va 映射到 pa，权限为可读可写
    int result = mappages(pt, va, pa, PGSIZE, PTE_R | PTE_W);
    assert(result == 0); // 断言映射成功

    // 2. 测试地址翻译功能 (walk)
    // 使用 walk 函数查找 va 对应的页表项(PTE)
    pte_t *pte = walk(pt, va, 0); // 第三个参数为0，表示如果PTE不存在，不要创建
    assert(pte != NULL); // 断言找到了 PTE
    assert(*pte & PTE_V); // 断言该 PTE 是有效的
    assert(PTE2PA(*pte) == pa); // 断言 PTE 指向的物理地址正是我们之前映射的 pa
    printf("Address translation lookup OK.\n");

    // 3. 测试权限位
    // 检查 PTE 中的权限位是否与 mappages 中设置的一致
    assert(*pte & PTE_R); // 应该是可读的
    assert(*pte & PTE_W); // 应该是可写的
    assert(!(*pte & PTE_X)); // 应该不是可执行的
    printf("Permission bits check OK.\n");
    
    // 清理：释放测试中分配的所有内存
    kfree((void*)pa); // 释放物理页
    kfree(pt);        // 释放临时页表
    printf("--- Test Passed: Page Table Management ---\n\n");
}

/**
 * @brief 内核主函数，现在作为测试入口
 */
void kmain(void) {
    // 1. 初始化物理页分配器
    // 这是所有内存操作的基础，必须首先完成。
    kinit();
    
    // --- 运行单元测试 ---
    // 在启用虚拟内存之前，对核心内存管理模块进行测试，确保它们工作正常。
    test_physical_memory();
    test_pagetable();
    
    // --- 激活虚拟内存 ---
    printf("--- Activating Virtual Memory ---\n");
    kvminit();     // 创建内核页表，建立内核代码、数据和设备的映射关系
    kvminithart(); // 将页表地址加载到 satp 寄存器，并刷新 TLB，正式开启分页模式
    printf("Virtual Memory Activated. Now running with paging.\n");
    
    // 此时，CPU 已经处于分页模式下。
    // 之后的所有指令获取和数据访问都将通过 MMU 进行地址转换。
    // 如果能成功打印下面的信息，说明内核页表的映射是正确的，
    // CPU 能够正确地翻译 printf 函数及相关数据的虚拟地址。
    printf("Kernel still running after enabling paging. Success!\n");

    printf("\n======== All Tests Completed! ========\n");
    console_flush(); // 确保所有输出都显示在控制台上
    while (1) {} // 测试完成，系统停机
}