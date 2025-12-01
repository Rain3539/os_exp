// 简化的页表管理实现（Sv39风格）

#include "kernel.h"
#include "vm.h"
#include "memlayout.h"
#include "riscv.h"

// 每个页表有512个PTE
#define PTE_PER_PAGE 512

// 创建一个新的页表（分配一页并清零）
pagetable_t create_pagetable(void) {
    void* page = alloc_page();
    if (!page) return 0;
    // 将内存清零
    char *p = (char*)page;
    for (int i = 0; i < PGSIZE; i++) p[i] = 0;
    return (pagetable_t)(uint64)page;
}

// 递归释放页表：释放所有页表页（根和中间页表）
// 对每个 PTE，如果 PTE_V 且该 PTE 指向下级页表，则递归释放下级页表页，
// 最后释放本页表页本身。
static void destroy_pagetable_recursive(pte_t *pt, int level) {
    if (!pt) return;
    // 仅在非叶级遍历子表
    for (int i = 0; i < PTE_PER_PAGE; i++) {
        pte_t pte = pt[i];
        if (!(pte & PTE_V))
            continue;
        if (level > 0) {
            uint64 pa = PTE_PA(pte);
            pte_t *child = (pte_t*)pa;
            // 递归释放子表
            destroy_pagetable_recursive(child, level - 1);
            // 清除此 PTE（不是严格必要，但更干净）
            pt[i] = 0;
        }
    }
    // 释放当前页表页
    free_page((void*)pt);
}

void destroy_pagetable(pagetable_t pt) {
    if (!pt) return;
    // Sv39 三层：level 2 (root) down to 0 (leaf)
    destroy_pagetable_recursive((pte_t*)(uint64)pt, 2);
}

// 从虚拟地址提取 VPN 字段
static inline int vpn_index(uint64 va, int level) {
    // level: 2 (top) .. 0 (leaf)
    return (va >> (12 + 9*level)) & 0x1FF;
}

// 在页表中查找pte，但不创建中间页表
pte_t* walk_lookup(pagetable_t pt, uint64 va) {
    if (!pt) return 0;
    pte_t *p = (pte_t*)(uint64)pt;
    for (int level = 2; level > 0; level--) {
        pte_t pte = p[vpn_index(va, level)];
        if (!(pte & PTE_V))
            return 0;
        // 提取物理地址并转换为指针(简化假设：PA == VA)
        uint64 pa = PTE_PA(pte);
        p = (pte_t*)(uint64)pa;
    }
    return &p[vpn_index(va, 0)];
}

// 在页表中查找或创建中间页表
pte_t* walk_create(pagetable_t pt, uint64 va) {
    if (!pt) return 0;
    pte_t *p = (pte_t*)(uint64)pt;
    for (int level = 2; level > 0; level--) {
        int idx = vpn_index(va, level);
        pte_t pte = p[idx];
        if (!(pte & PTE_V)) {
            // 需要创建一个新的页表页
            void* newpage = alloc_page();
            if (!newpage) return 0;
            // 清零新页
            char *np = (char*)newpage;
            for (int i = 0; i < PGSIZE; i++) np[i] = 0;
            // 设置PTE: PA in bits [...], 低10位用于标志
            uint64 pa = (uint64)newpage;
            p[idx] = ((pa >> 12) << 10) | PTE_V;
        }
        // 取出下一层页表
        uint64 pa_next = PTE_PA(p[idx]);
        p = (pte_t*)(uint64)pa_next;
    }
    return &p[vpn_index(va, 0)];
}

// 建立单页映射
int map_page(pagetable_t pt, uint64 va, uint64 pa, int perm) {
    if (((va | pa) & (PGSIZE-1)) != 0) return -1; // 必须页对齐
    pte_t *pte = walk_create(pt, va);
    if (!pte) return -1;
    if (*pte & PTE_V) {
        // 已经存在映射
        return -1;
    }
    // 将 pa 放入 PTE 的高位。PTE格式： [ppn<<10]|flags
    uint64 ppn = (pa >> 12);
    *pte = (ppn << 10) | (perm & 0x3FF) | PTE_V;
    return 0;
}

// 打印页表（用于调试）
void dump_pagetable(pagetable_t pt, int level) {
    if (!pt) return;
    pte_t *p = (pte_t*)(uint64)pt;
    for (int i = 0; i < PTE_PER_PAGE; i++) {
        pte_t pte = p[i];
        if (pte & PTE_V) {
            uint64 pa = PTE_PA(pte);
            printf("pte[%d]=PA=0x%x flags=0x%x\n", i, (uint)pa, (uint)(pte & 0x3FF));
        }
    }
}

// 内核页表初始化：创建并做简单映射（恒等映射内核区域和设备）
pagetable_t kernel_pagetable = 0;
extern char etext; // 链接器提供内核文本结束符

void kvminit(void) {
    kernel_pagetable = create_pagetable();
    if (!kernel_pagetable) return;
    // 映射 kernel text (RX)
    uint64 text_sz = ((uint64)&etext > KERNBASE) ? ((uint64)&etext - KERNBASE) : PGSIZE;
    for (uint64 a = KERNBASE; a < KERNBASE + text_sz; a += PGSIZE)
        map_page(kernel_pagetable, a, a, PTE_R | PTE_X);

    // 映射内核数据 (RW)
    for (uint64 a = (uint64)&etext; a < PHYSTOP; a += PGSIZE)
        map_page(kernel_pagetable, a, a, PTE_R | PTE_W);

    // 映射 UART 设备
    map_page(kernel_pagetable, UART0, UART0, PTE_R | PTE_W);
}

// 激活当前hart的页表（写satp和刷新TLB）
void kvminithart(void) {
    if (!kernel_pagetable) return;
    // MAKE_SATP: mode=8(Sv39), asid=0, ppn=ppn_of(kernel_pagetable)
    uint64 ppn = ((uint64)kernel_pagetable) >> 12;
    uint64 satp = (8UL << 60) | ppn;
    // 写satp寄存器并刷新TLB
    w_satp(satp);
    sfence_vma();
}


// 简单断言宏（失败时打印并 panic）
#define KASSERT(cond, msg) do { \
    if (!(cond)) { \
      printf("ASSERT FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
      panic(msg); \
    } \
  } while (0)

// 物理内存分配器测试（严格按步骤）
void test_physical_memory(void) {
    void *page1 = alloc_page();
    void *page2 = alloc_page();
    KASSERT(page1 != 0 && page2 != 0, "alloc returned null");
    KASSERT(page1 != page2, "alloc returned same page twice");
    KASSERT((((uint64)page1) & 0xFFF) == 0, "page1 not page-aligned");

    // 测试写入与读取
    *(int*)page1 = 0x12345678;
    KASSERT(*(int*)page1 == 0x12345678, "write/read mismatch on page1");

    // 释放与重新分配
    free_page(page1);
    void *page3 = alloc_page();
    KASSERT(page3 != 0, "realloc failed");

    free_page(page2);
    free_page(page3);

    printf("test_physical_memory passed\n");
}

// 页表功能测试（严格）
void test_pagetable(void) {
    pagetable_t pt = create_pagetable();
    KASSERT(pt != 0, "create_pagetable failed");

    void* p = alloc_page();
    KASSERT(p != 0, "alloc_page failed");

    uint64 va = 0x1000000;
    KASSERT(map_page(pt, va, (uint64)p, PTE_R | PTE_W) == 0, "map_page failed");

    pte_t *pte = walk_lookup(pt, va);
    KASSERT(pte != 0 && (*pte & PTE_V), "walk_lookup failed or pte not valid");
    KASSERT(PTE_PA(*pte) == (uint64)p, "PTE PA mismatch");
    KASSERT((*pte & PTE_R), "PTE missing R");
    KASSERT((*pte & PTE_W), "PTE missing W");
    KASSERT(!(*pte & PTE_X), "PTE has unexpected X");

    destroy_pagetable(pt);
    printf("test_pagetable passed\n");
}

// 全局变量用于虚拟内存启用后验证内核数据可访问性
static volatile int vm_test_var = 0x5555;

// 虚拟内存激活测试（严格）
void test_virtual_memory(void) {
    printf("Before enabling paging...\n");

    // 启用页表
    kvminit();
    kvminithart();

    printf("After enabling paging...\n");

    // 测试内核代码仍然可执行（通过打印）
    printf("kernel printf after paging\n");

    // 测试内核数据仍然可访问：读写全局变量
    int before = vm_test_var;
    vm_test_var = 0x789A;
    KASSERT(vm_test_var == 0x789A, "vm_test_var write/read failed after paging");
    vm_test_var = before; // 恢复

    // 测试设备访问（写 UART）
    uart_putc('V');
    uart_putc('\n');

    printf("test_virtual_memory passed\n");
}
