/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_TRAP_H
#define JOS_KERN_TRAP_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/trap.h>
#include <inc/mmu.h>

/* The kernel's interrupt descriptor table */
extern struct Gatedesc idt[];
extern struct Pseudodesc idt_pd;

void trap_init(void);
void trap_init_percpu(void);
void print_regs(struct PushRegs *regs);
void print_trapframe(struct Trapframe *tf);
void page_fault_handler(struct Trapframe *);
void backtrace(struct Trapframe *);

extern char _zero_exc[];
extern char _debug_exc[];
extern char _nmi_exc[];
extern char _brkpt_exc[];
extern char _oflow_exc[];
extern char _bound_exc[];
extern char _illop_exc[];
extern char _device_exc[];

extern char _dblflt_exc[];
extern char _tss_exc[];
extern char _segnp_exc[];
extern char _stack_exc[];
extern char _gpflt_exc[];
extern char _pgflt_exc[];
extern char _fperr_exc[];
extern char _aligh_exc[];
extern char _mchk_exc[];
extern char _simderr_exc[];

extern char _syscall_exc[];
extern char _default_exc[];

extern char _irq_0[];
extern char _irq_1[];
extern char _irq_2[];
extern char _irq_3[];
extern char _irq_4[];
extern char _irq_5[];
extern char _irq_6[];
extern char _irq_7[];
extern char _irq_8[];
extern char _irq_9[];
extern char _irq_10[];
extern char _irq_11[];
extern char _irq_12[];
extern char _irq_13[];
extern char _irq_14[];
extern char _irq_15[];
#endif /* JOS_KERN_TRAP_H */
