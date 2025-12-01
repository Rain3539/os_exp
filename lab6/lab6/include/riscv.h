/**
 * RISC-V架构头文件
 * 包含RISC-V特定定义和寄存器访问宏
 */

#ifndef RISCV_H
#define RISCV_H

#include "types.h"

// RISC-V机器模式寄存器
#define MSTATUS_MIE (1L << 3)  // 机器模式中断使能
#define MIE_MTIE (1L << 7)     // 机器模式定时器中断使能
#define MIE_MEIE (1L << 11)    // 机器模式外部中断使能

// 内存映射I/O地址
#define UART0 0x10000000L      // UART0地址
#define VIRTIO0 0x10008000L    // VIRTIO设备地址
#define PLIC 0x0c000000L       // 中断控制器地址

// 寄存器访问辅助宏
#define w_mstatus(x) asm volatile("csrw mstatus, %0" : : "r" (x))  // 写mstatus寄存器
#define r_mstatus() ({ uint64 x; asm volatile("csrr %0, mstatus" : "=r" (x)); x; })  // 读mstatus寄存器

#define w_mie(x) asm volatile("csrw mie, %0" : : "r" (x))          // 写mie寄存器
#define r_mie() ({ uint64 x; asm volatile("csrr %0, mie" : "=r" (x)); x; })          // 读mie寄存器

#define w_mtvec(x) asm volatile("csrw mtvec, %0" : : "r" (x))      // 写mtvec寄存器
#define r_mtvec() ({ uint64 x; asm volatile("csrr %0, mtvec" : "=r" (x)); x; })      // 读mtvec寄存器

#define w_mepc(x) asm volatile("csrw mepc, %0" : : "r" (x))        // 写mepc寄存器
#define r_mepc() ({ uint64 x; asm volatile("csrr %0, mepc" : "=r" (x)); x; })        // 读mepc寄存器

#define w_mcause(x) asm volatile("csrw mcause, %0" : : "r" (x))    // 写mcause寄存器
#define r_mcause() ({ uint64 x; asm volatile("csrr %0, mcause" : "=r" (x)); x; })    // 读mcause寄存器

#define w_mtval(x) asm volatile("csrw mtval, %0" : : "r" (x))      // 写mtval寄存器
#define r_mtval() ({ uint64 x; asm volatile("csrr %0, mtval" : "=r" (x)); x; })      // 读mtval寄存器

#define w_mscratch(x) asm volatile("csrw mscratch, %0" : : "r" (x)) // 写mscratch寄存器
#define r_mscratch() ({ uint64 x; asm volatile("csrr %0, mscratch" : "=r" (x)); x; }) // 读mscratch寄存器

// 写 satp 寄存器 (用于激活页表)
#define w_satp(x) asm volatile("csrw satp, %0" : : "r" (x))

// 刷新整个 TLB
#define sfence_vma() asm volatile("sfence.vma zero, zero")

// 添加缺失的 Supervisor 模式 CSR 寄存器访问函数
#define w_stvec(x) asm volatile("csrw stvec, %0" : : "r" (x))      // 写stvec寄存器
#define r_stvec() ({ uint64 x; asm volatile("csrr %0, stvec" : "=r" (x)); x; })      // 读stvec寄存器

#define w_sepc(x) asm volatile("csrw sepc, %0" : : "r" (x))        // 写sepc寄存器
#define r_sepc() ({ uint64 x; asm volatile("csrr %0, sepc" : "=r" (x)); x; })        // 读sepc寄存器

#define w_scause(x) asm volatile("csrw scause, %0" : : "r" (x))    // 写scause寄存器
#define r_scause() ({ uint64 x; asm volatile("csrr %0, scause" : "=r" (x)); x; })    // 读scause寄存器

#define w_stval(x) asm volatile("csrw stval, %0" : : "r" (x))      // 写stval寄存器

#define w_sscratch(x) asm volatile("csrw sscratch, %0" : : "r" (x)) // 写sscratch寄存器
#define r_sscratch() ({ uint64 x; asm volatile("csrr %0, sscratch" : "=r" (x)); x; }) // 读sscratch寄存器

#define w_sstatus(x) asm volatile("csrw sstatus, %0" : : "r" (x))  // 写sstatus寄存器
#define r_sstatus() ({ uint64 x; asm volatile("csrr %0, sstatus" : "=r" (x)); x; })  // 读sstatus寄存器

#define w_sie(x) asm volatile("csrw sie, %0" : : "r" (x))          // 写sie寄存器
#define r_sie() ({ uint64 x; asm volatile("csrr %0, sie" : "=r" (x)); x; })          // 读sie寄存器

#define w_sip(x) asm volatile("csrw sip, %0" : : "r" (x))          // 写sip寄存器
#define r_sip() ({ uint64 x; asm volatile("csrr %0, sip" : "=r" (x)); x; })          // 读sip寄存器

// 时间相关寄存器
#define w_stimecmp(x) asm volatile("csrw stimecmp, %0" : : "r" (x)) // 写stimecmp寄存器
#define r_time() ({ uint64 x; asm volatile("csrr %0, time" : "=r" (x)); x; })        // 读time寄存器

// SSTATUS 标志位
#define SSTATUS_SPP (1L << 8)   // 之前的权限模式：1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5)  // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4)  // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)   // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)   // User Interrupt Enable

// SIE 标志位
#define SIE_SEIE (1L << 9)      // Supervisor External Interrupt Enable
#define SIE_STIE (1L << 5)      // Supervisor Timer Interrupt Enable
#define SIE_SSIE (1L << 1)      // Supervisor Software Interrupt Enable

// 中断委托寄存器
#define w_medeleg(x) asm volatile("csrw medeleg, %0" : : "r" (x))  // 写medeleg寄存器
#define r_medeleg() ({ uint64 x; asm volatile("csrr %0, medeleg" : "=r" (x)); x; })  // 读medeleg寄存器

#define w_mideleg(x) asm volatile("csrw mideleg, %0" : : "r" (x))  // 写mideleg寄存器
#define r_mideleg() ({ uint64 x; asm volatile("csrr %0, mideleg" : "=r" (x)); x; })  // 读mideleg寄存器

// 计数器使能寄存器
#define w_mcounteren(x) asm volatile("csrw mcounteren, %0" : : "r" (x))  // 写mcounteren寄存器
#define r_mcounteren() ({ uint64 x; asm volatile("csrr %0, mcounteren" : "=r" (x)); x; })  // 读mcounteren寄存器

// 环境配置寄存器
#define w_menvcfg(x) asm volatile("csrw menvcfg, %0" : : "r" (x))  // 写menvcfg寄存器
#define r_menvcfg() ({ uint64 x; asm volatile("csrr %0, menvcfg" : "=r" (x)); x; })  // 读menvcfg寄存器

// MSTATUS 标志位
#define MSTATUS_MPP_MASK (3L << 11) // 之前的机器权限模式掩码
#define MSTATUS_MPP_S (1L << 11)    // Supervisor模式

// MIE 标志位
#define MIE_MEIE (1L << 11)     // 机器外部中断使能
#define MIE_MTIE (1L << 7)      // 机器定时器中断使能
#define MIE_MSIE (1L << 3)      // 机器软件中断使能

// 添加缺失的寄存器读取函数
static inline uint64 r_satp(void) {
    uint64 x;
    asm volatile("csrr %0, satp" : "=r"(x));
    return x;
}

static inline uint64 r_tp(void) {
    uint64 x;
    // tp 是通用寄存器，不是 CSR，使用不同的方式读取
    asm volatile("mv %0, tp" : "=r"(x));
    return x;
}

// 添加缺失的中断委托寄存器访问宏
#define w_mideleg(x) asm volatile("csrw mideleg, %0" : : "r" (x))  // 写mideleg寄存器
#define r_mideleg() ({ uint64 x; asm volatile("csrr %0, mideleg" : "=r" (x)); x; })  // 读mideleg寄存器

// CLINT 寄存器地址
#define CLINT_BASE      0x2000000L
#define CLINT_MTIME     (CLINT_BASE + 0xBFF8)
#define CLINT_MTIMECMP(hartid) (CLINT_BASE + 0x4000 + 8 * (hartid))

// 读取 mhartid 寄存器
static inline uint64
r_mhartid()
{
    uint64 x;
    asm volatile("csrr %0, mhartid" : "=r" (x));
    return x;
}

// 读取 stval 寄存器
static inline uint64
r_stval()
{
    uint64 x;
    asm volatile("csrr %0, stval" : "=r" (x));
    return x;
}

// 声明 kernelvec 汇编函数
void kernelvec(void);

#endif /* RISCV_H */
