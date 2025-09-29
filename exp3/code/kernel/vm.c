// kernel/vm.c

// 包含 RISC-V 特定的定义，例如特权指令 `w_satp` 和 `sfence_vma`。
#include <riscv.h>
// 包含一些标准定义，例如 NULL。
#include <stddef.h>
// 包含内存布局的定义，例如物理内存的起始和结束地址、页大小 (PGSIZE) 等。
#include <memlayout.h>

// 声明物理内存分配器函数 kalloc。
// `void* kalloc(void);` 用于从物理内存中分配一个 4096 字节的页面。
void* kalloc(void);

// 全局的内核页表。
// 这是一个指向一级页表物理地址的指针。
// 内核在启动时会创建这个页表，并且所有 CPU 核心都会共享它。
pagetable_t kernel_pagetable;

// 声明一个由链接器脚本(kernel.ld)定义的符号 `etext`。
// `etext` 标记了内核代码段 (.text) 的结束位置。
// 这个地址用于区分代码和数据，以便设置不同的内存权限。
extern char etext[]; 

/**
 * @brief 遍历页表以查找给定虚拟地址对应的页表项(PTE)。
 *
 * 这是虚拟内存管理的核心函数。它会遍历 RISC-V Sv39 标准下的三级页表结构。
 * 
 * 如果 `alloc` 参数被设置为 1，函数会在必要时（即下一级页表不存在时）分配新的页表页。
 *
 * @param pagetable 要遍历的根页表的物理地址。
 * @param va 需要查找的虚拟地址。
 * @param alloc 如果为 1，则在页表项不存在时分配新的页表；如果为 0，则不分配。
 * @return 指向最底层（L0）页表项（PTE）的指针。如果找不到且 alloc 为 0，则返回 NULL。
 */
pte_t* walk(pagetable_t pagetable, uint64 va, int alloc) {
    // 检查虚拟地址是否超过了 Sv39 模式下支持的最大虚拟地址 (MAXVA)。
    // 如果超过了，这是一个无效地址，直接返回 NULL。
    if (va >= MAXVA) return NULL; 

    // RISC-V Sv39 使用三级页表。我们从最高级（L2）开始向下遍历到 L1。
    // L0 是最后一级，它直接指向物理页面，所以循环不包含 level 0。
    for (int level = 2; level > 0; level--) {
        // 使用 PX 宏从虚拟地址 `va` 中提取当前级别的页表索引。
        pte_t *pte = &pagetable[PX(level, va)];
        
        // 检查当前页表项(PTE)的有效位 (PTE_V)。
        if (*pte & PTE_V) {
            // 如果有效位为 1，说明该 PTE 指向一个有效的下一级页表。
            // 我们使用 PTE2PA 宏从 PTE 中提取下一级页表的物理地址，
            // 并将其作为下一次循环的 `pagetable`。
            pagetable = (pagetable_t)PTE2PA(*pte);
        } else {
            // 如果有效位为 0，说明下一级页表不存在。
            // 检查 `alloc` 参数，看是否允许我们创建新的页表。
            // 如果 `!alloc` (不允许分配) 或者 `kalloc()` 失败 (内存耗尽)，则返回 NULL。
            if (!alloc || (pagetable = (pagetable_t)kalloc()) == NULL)
                return NULL;
            
            // 如果成功分配了一个新的页表页，需要将其内容清零。
            // 因为所有新的 PTE 的有效位都应该是 0。
            for(int i=0; i<512; i++) pagetable[i] = 0;

            // 将当前级别的 PTE 指向新创建的下一级页表。
            // PA2PTE 将页表的物理地址转换为 PTE 格式，并设置有效位 PTE_V。
            *pte = PA2PTE(pagetable) | PTE_V;
        }
    }
    // 循环结束后，`pagetable` 指向最底层(L0)的页表。
    // 我们返回该表中对应 `va` 的 PTE 的地址。
    return &pagetable[PX(0, va)];
}


/**
 * @brief 将一段虚拟地址映射到一段物理地址。
 *
 * 为指定的地址范围创建页表项（PTEs），并赋予它们指定的权限。
 *
 * @param pagetable 根页表的物理地址。
 * @param va 起始虚拟地址。
 * @param pa 起始物理地址。
 * @param size 要映射的内存区域大小（字节）。
 * @param perm 映射的权限 (例如 PTE_R, PTE_W, PTE_X)。
 * @return 成功返回 0，失败（例如 kalloc 失败）返回 -1。
 */
int mappages(pagetable_t pagetable, uint64 va, uint64 pa, uint64 size, int perm) {
    uint64 a, last;
    pte_t *pte;

    // 使用 PGROUNDDOWN 将起始和结束虚拟地址向下对齐到页边界。
    // 虚拟内存是以页为单位进行管理的。
    a = PGROUNDDOWN(va);
    last = PGROUNDDOWN(va + size - 1);

    for (;;) {
        // 为当前的虚拟地址 `a` 查找或创建（因为第三个参数是 1）对应的 L0 PTE。
        if ((pte = walk(pagetable, a, 1)) == NULL)
            return -1; // walk 失败，很可能是 kalloc 内存耗尽。

        // 检查该 PTE 是否已经有效。如果有效，说明这块虚拟地址已经被映射过了。
        // 在这个内核中，我们不允许重新映射，所以这是一个错误。
        if (*pte & PTE_V)
            return -2; // 在 xv6 中，panic("remap")

        // 设置 PTE：将物理地址 `pa` 转换为 PTE 格式，并或上权限位和有效位。
        *pte = PA2PTE(pa) | perm | PTE_V;
        
        // 如果已经处理完最后一个页面，则跳出循环。
        if (a == last)
            break;
        
        // 移动到下一个虚拟地址页和物理地址页。
        a += PGSIZE;
        pa += PGSIZE;
    }
    return 0;
}

/**
 * @brief 创建初始的内核页表。
 *
 * 这个函数设置内核运行所必需的内存映射，以便启用分页机制。
 * 它主要映射了内核的代码段、数据段以及 UART 设备。
 */
void kvminit(void) {
    // 1. 为内核根页表分配一个物理页。
    kernel_pagetable = (pagetable_t)kalloc();
    // 将新分配的页表清零。
    for(int i=0; i<512; i++) kernel_pagetable[i] = 0;

    // 2. 映射 UART 设备 (内存映射 I/O)。
    // 这是一个恒等映射（虚拟地址 == 物理地址）。
    // 权限为可读可写 (PTE_R | PTE_W)。
    mappages(kernel_pagetable, UART0, UART0, PGSIZE, PTE_R | PTE_W);

    // 3. 映射内核的代码段 (.text) 为可读-可执行 (Read-Execute)。
    // 从 KERNBASE 开始，到 etext 结束。
    mappages(kernel_pagetable, KERNBASE, KERNBASE, (uint64)etext - KERNBASE, PTE_R | PTE_X);
    
    // 4. 映射内核的数据段以及其后所有的物理内存为可读-可写 (Read-Write)。
    // 从 etext 开始，一直到物理内存的末尾 (PHYSTOP)。
    mappages(kernel_pagetable, (uint64)etext, (uint64)etext, PHYSTOP - (uint64)etext, PTE_R | PTE_W);
}

/**
 * @brief 为当前的 HART (CPU 核心) 激活内核页表。
 *
 * 这个函数会将内核页表的物理地址写入 `satp` 寄存器，
 * 然后刷新 TLB (快表)，以确保新的地址映射立即生效。
 */
void kvminithart(void) {
    // 将内核页表的物理地址写入 `satp` (Supervisor Address Translation and Protection) 寄存器。
    // MAKE_SATP 宏会设置好页表模式 (Sv39) 和页表的物理页号 (PPN)。
    w_satp(MAKE_SATP(kernel_pagetable));
    
    // 刷新 TLB (Translation Lookaside Buffer)。
    // `satp` 寄存器更新后，旧的地址翻译缓存可能仍然存在于 TLB 中。
    // `sfence_vma` 指令会使这些缓存失效，强制 CPU 使用新的页表进行地址翻译。
    sfence_vma();
}