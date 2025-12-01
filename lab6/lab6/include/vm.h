#ifndef VM_H
#define VM_H

#include "types.h"
#include "memlayout.h"

// 页表项类型
typedef uint64 pte_t;

// PTE 权限位定义（与 xv6 风格兼容）
#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)

// 常用宏
#define PGROUNDUP(sz) ((((sz)+PGSIZE-1)) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) ((a) & ~(PGSIZE-1))
#define PTE_PA(pte) ((((pte) >> 10)) << 12)

// 页表接口
pagetable_t create_pagetable(void);
int map_page(pagetable_t pt, uint64 va, uint64 pa, int perm);
void destroy_pagetable(pagetable_t pt);

// 辅助查询
pte_t* walk_lookup(pagetable_t pt, uint64 va);
pte_t* walk_create(pagetable_t pt, uint64 va);

// Kernel helpers
void kvminit(void);
void kvminithart(void);

// Tests
void test_physical_memory(void);
void test_pagetable(void);
void test_virtual_memory(void);

// Physical memory allocator APIs (provided by kernel/pmm.c)
void pmm_init(void);
void pmm_init_range(void* start, void* end);
void* alloc_page(void);
void free_page(void* page);
void* alloc_pages(int n);

#endif /* VM_H */
