// 简单物理内存管理器 - 链表实现

#include "kernel.h"
#include "vm.h"
#include "memlayout.h"

// 我们使用页大小为PGSIZE的空闲页链表
struct run {
    struct run *next;
};

static struct run *free_list = 0;
static struct spinlock free_lock;

// 初始化物理内存管理器
// 参数: start - 可用物理内存的起始地址(包含)
//        end   - 可用物理内存的结束地址(不包含)
void pmm_init_range(void* start, void* end) {
    initlock(&free_lock, "pmm");

    // 对齐到页边界
    uint64 a = (uint64)PGROUNDUP((uint64)start);
    uint64 last = (uint64)PGROUNDDOWN((uint64)end);

    free_list = 0;
    for (; a + PGSIZE <= last; a += PGSIZE) {
        free_page((void*)a);
    }
}

// 使用链接器符号来避免把内核占用的页加入空闲链表
// 期望链接器脚本提供 etext 或 _ebss 符号
extern char etext[];   // 文本段结束 - 在 kernel.ld 中 PROVIDE(etext = .)
extern char _ebss[];   // BSS 段结束 - 在 kernel.ld 中 PROVIDE(_ebss = .)

void pmm_init(void) {
    initlock(&free_lock, "pmm");

    // 选择一个安全的起始地址：etext 或 _ebss 中较大者
    uint64 used_end = (uint64)etext;
    if ((uint64)_ebss > used_end) used_end = (uint64)_ebss;

    // 内核基地址到 PHYSTOP 是整个物理内存区域（在此简化假设）
    uint64 phys_start = KERNBASE;
    uint64 phys_end = PHYSTOP;

    // 我们只把 (used_end .. phys_end) 之间的页加入空闲链表
    uint64 a = PGROUNDUP(used_end);
    if (a < phys_start) a = phys_start;
    a = (a + PGSIZE - 1) & ~(PGSIZE-1);

    for (; a + PGSIZE <= phys_end; a += PGSIZE) {
        free_page((void*)a);
    }
}

// 分配单页，返回页的虚拟映射/物理地址（在我们最简实现中直接返回物理地址指针）
void* alloc_page(void) {
    struct run *r;
    acquire(&free_lock);
    r = free_list;
    if (r)
        free_list = r->next;
    release(&free_lock);

    if (r)
        return (void*)r;
    return 0;
}

// 释放单页
void free_page(void* page) {
    if (page == 0)
        return;

    // 简单检查页对齐
    if (((uint64)page & (PGSIZE-1)) != 0)
        return;

    struct run *r = (struct run*)page;
    acquire(&free_lock);
    r->next = free_list;
    free_list = r;
    release(&free_lock);
}

// 分配连续n页（简单的线性扫描；效率不高）
void* alloc_pages(int n) {
    if (n <= 0)
        return 0;

    // 简化实现：逐页调用 alloc_page，若中间失败则回滚
    void** pages = 0;
    void* first = 0;

    pages = (void**)alloc_page();
    if (!pages)
        return 0;
    first = pages;

    for (int i = 1; i < n; i++) {
        void* p = alloc_page();
        if (!p) {
            // 回滚
            for (int j = 0; j < i; j++)
                free_page((char*)first + j * PGSIZE);
            return 0;
        }
    }
    return first;
}

