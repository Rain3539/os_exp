// kernel/kalloc.c

// 包含 RISC-V 特定的定义，例如寄存器和特权指令。
// 在这里，它可能包含了像 uint64 这样的类型定义和硬件相关的宏。
#include <riscv.h> 
// 包含一些标准定义，例如 NULL 和 size_t。
#include <stddef.h> 
// 包含内存布局的定义，例如物理内存的起始和结束地址 (PHYSTOP)、页大小 (PGSIZE) 等。
#include <memlayout.h>

// 声明一个名为 end 的外部符号。
// 这个符号不是一个真正的变量，而是由链接器脚本(kernel.ld)在链接时定义的一个地址。
// 它标记了内核代码段、数据段和 bss 段的结束位置。
// 物理内存从 end 之后到 PHYSTOP 之前的部分，都可以被用作堆内存，由页分配器来管理。
extern char end[]; 

// 定义一个链表节点结构体，用于管理空闲的物理内存页。
// 每个空闲页的开头都会被看作是一个 `struct run`。
struct run {
    // 指向下一个空闲页的指针。
    struct run *next;
};

// 定义一个结构体来管理内核的物理内存。
struct {
    // 指向空闲物理页链表的头指针（freelist）。
    // 这个链表将所有可用的物理内存页链接在一起。
    struct run *freelist;
} kmem;

/**
 * @brief 释放一个物理内存页。
 *
 * 将给定物理地址 `pa` 所在的内存页添加到空闲链表的头部。
 * 在这个内核中，用户空间分页完全启用之前，物理地址和虚拟地址是相同的。
 *
 * @param pa 一个指向要被释放的 4096 字节内存页的指针。
 */
void kfree(void *pa) {
    struct run *r;

    // --- 基本的健全性检查 ---
    // 1. 检查地址 `pa` 是否是页对齐的（地址必须是 PGSIZE 的整数倍）。
    // 2. 检查 `pa` 是否在内核代码/数据范围之外（不能释放内核本身）。
    // 3. 检查 `pa` 是否在有效的物理内存范围内。
    if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP) {
        // 在一个真实的操作系统中，这里会调用 panic() 函数来使内核崩溃，
        // 因为这是一个严重的错误，例如内核损坏或驱动程序错误。
        // 在这个简化的版本里，我们直接返回。
        return;
    }

    // --- 将页面添加到空闲链表 ---
    // 1. 将 void* 类型的指针 `pa` 强制转换为 `struct run*` 类型。
    //    这允许我们像操作链表节点一样操作这个内存页的开头部分。
    r = (struct run*)pa;
    // 2. 将新释放的页的 `next` 指针指向当前空闲链表的头。
    r->next = kmem.freelist;
    // 3. 更新空闲链表的头指针，使其指向这个新释放的页。
    //    这样，这个页面就成了链表中的第一个空闲页。
    kmem.freelist = r;
}

/**
 * @brief 初始化物理内存分配器。
 *
 * 这个函数在操作系统启动时只被调用一次，用于建立物理页的空闲链表。
 * 它会接管从内核结束位置 (`end`) 到物理内存顶端 (`PHYSTOP`) 之间的所有内存，
 * 并将它们逐页添加到空闲链表中。
 */
void kinit() {
    // 首先，将空闲链表头指针设为 NULL，表示链表为空。
    kmem.freelist = NULL;

    // 使用 PGROUNDUP 宏计算出内核 `end` 符号之后的第一个页对齐的地址。
    // `end` 的地址不一定是页对齐的，所以我们需要找到它之后第一个可以作为独立页开始的地址。
    // `p` 是我们将要开始逐页释放的起始地址。
    char *p = (char*)PGROUNDUP((uint64)end);

    // 循环遍历从 `p` 开始到 `PHYSTOP` 之间的所有物理内存。
    // 循环条件 `p + PGSIZE <= (char*)PHYSTOP` 确保我们不会越过物理内存的边界。
    for (; p + PGSIZE <= (char*)PHYSTOP; p += PGSIZE) {
        // 在每次循环中，调用 kfree() 将当前指针 `p` 指向的内存页添加到空闲链表中。
        // kfree() 会负责将这块 4096 字节的内存转换成 `struct run` 并链接到链表头部。
        kfree(p);
    }
}

/**
 * @brief 分配一个 4096 字节的物理内存页。
 *
 * 从空闲链表中取出一个页面，并返回指向它的指针。
 * 如果空闲链表为空（即没有可用内存），则返回 NULL。
 *
 * @return 指向已分配页面的指针；如果内存耗尽，则返回 NULL。
 */
void *kalloc(void) {
    struct run *r;

    // 从全局的 kmem 结构中获取空闲链表的头指针。
    r = kmem.freelist;
    
    // 检查链表是否为空。
    if (r) {
        // 如果链表不为空（有可用页面）：
        // 将空闲链表的头指针移动到下一个节点，相当于从链表中移除了第一个节点。
        kmem.freelist = r->next;
    }

    // 返回被移除的那个节点的地址（即分配到的内存页的地址）。
    // 如果链表开始时就为空，r 为 NULL，那么这里也会返回 NULL。
    return (void*)r;
}

// === [新增] 批量分配与释放功能 ===

/**
 * @brief 批量分配 n 个 4096 字节的物理内存页。
 *
 * 从空闲链表中一次性取出 n 个页面，并将它们以一个新链表的形式返回。
 * 这种方法比循环调用 kalloc() 更高效。
 * 如果空闲页不足 n 个，则分配失败，返回 NULL。
 *
 * @param n 要分配的页面数量。
 * @return 指向已分配页面链表头部的指针；如果内存不足，则返回 NULL。
 */
void* kalloc_bulk(int n) {
    struct run *head, *tail;
    int i;

    if (n <= 0) {
        return NULL;
    }

    // 检查是否有足够的页面
    tail = kmem.freelist;
    for (i = 0; i < n; i++) {
        if (tail == NULL) {
            // 遍历未完成就遇到 NULL，说明页面不够，分配失败
            return NULL;
        }
        if (i < n - 1) {
            tail = tail->next;
        }
    }
    // 循环结束后，tail 指向第 n 个节点

    // 保存将要返回的链表头
    head = kmem.freelist;
    // 将全局空闲链表的头更新为第 n+1 个节点
    kmem.freelist = tail->next;
    // 将分配的 n 个页面的链表与主链表断开
    tail->next = NULL;

    return (void*)head;
}

/**
 * @brief 批量释放一个由页面组成的链表。
 *
 * 将一个通过 kalloc_bulk 分配的页面链表重新加到全局空闲链表的头部。
 *
 * @param pa 指向要被释放的页面链表的头指针。
 */
void kfree_bulk(void *pa) {
    struct run *r = (struct run*)pa;
    struct run *tail;

    if (r == NULL) {
        return;
    }

    // 找到要释放链表的尾部
    tail = r;
    while(tail->next) {
        // 在真实的内核中，可以在这里对链表中的每个页面进行健全性检查
        tail = tail->next;
    }
    
    // 将链表的尾部链接到当前空闲链表的头部
    tail->next = kmem.freelist;
    // 更新空闲链表的头，使其指向被释放链表的头
    kmem.freelist = r;
}