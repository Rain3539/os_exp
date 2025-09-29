// kernel/riscv.h

#ifndef __RISCV_H__
#define __RISCV_H__

#include <stddef.h>

// Define standard integer types
typedef unsigned long uint64;
typedef unsigned long* pagetable_t;
typedef uint64 pte_t;

// --- SATP Register Manipulation ---
// The SATP register holds the physical page number (PPN) of the root page table.
// For Sv39, it also specifies the paging mode.
#define SATP_SV39 (8L << 60)
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))

// Write to satp register
static inline void w_satp(uint64 x) {
    asm volatile("csrw satp, %0" : : "r" (x));
}

// Flush the TLB (Translation Lookaside Buffer) for all virtual addresses
static inline void sfence_vma() {
    // This ensures that all previous memory accesses are completed before
    // the new address translations are used.
    asm volatile("sfence.vma zero, zero");
}

// --- Page Table Constants and Macros ---
#define PGSIZE 4096 // Page size in bytes
#define PGSHIFT 12  // Bits of offset within a page

// Page Rounding
#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

// Page Table Entry (PTE) flags
#define PTE_V (1L << 0) // Valid
#define PTE_R (1L << 1) // Read
#define PTE_W (1L << 2) // Write
#define PTE_X (1L << 3) // Execute
#define PTE_U (1L << 4) // User

// Extract physical address from a PTE
#define PTE2PA(pte) (((pte) >> 10) << 12)
// Construct a PTE from a physical address
#define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)

// Extract the Page Index (VPN) for a given level from a virtual address
#define PXSHIFT(level) (PGSHIFT + (9 * (level)))
#define PX(level, va) ((((uint64) (va)) >> PXSHIFT(level)) & 0x1FF)

// Maximum virtual address for Sv39 (bits 38-63 must be the same as bit 38)
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))

#endif // __RISCV_H__