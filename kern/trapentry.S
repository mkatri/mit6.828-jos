/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

TRAPHANDLER_NOEC(_zero_exc, T_DIVIDE)
TRAPHANDLER_NOEC(_debug_exc, T_DEBUG)
TRAPHANDLER_NOEC(_nmi_exc, T_NMI)
TRAPHANDLER_NOEC(_brkpt_exc, T_BRKPT)
TRAPHANDLER_NOEC(_oflow_exc, T_OFLOW)
TRAPHANDLER_NOEC(_bound_exc, T_BOUND)
TRAPHANDLER_NOEC(_illop_exc, T_ILLOP)
TRAPHANDLER_NOEC(_device_exc, T_DEVICE)

TRAPHANDLER(_dblflt_exc, T_DBLFLT)
TRAPHANDLER(_tss_exc, T_TSS)
TRAPHANDLER(_segnp_exc, T_SEGNP)
TRAPHANDLER(_stack_exc, T_STACK)
TRAPHANDLER(_gpflt_exc, T_GPFLT)
TRAPHANDLER(_pgflt_exc, T_PGFLT)
TRAPHANDLER(_fperr_exc, T_FPERR)
TRAPHANDLER(_aligh_exc, T_ALIGN)
TRAPHANDLER(_mchk_exc, T_MCHK)
TRAPHANDLER(_simderr_exc, T_SIMDERR)

TRAPHANDLER_NOEC(_syscall_exc, T_SYSCALL)
TRAPHANDLER_NOEC(_default_exc, T_DEFAULT)

/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
	pushl	%ds
	pushl	%es
	pushal
	mov	$GD_KD, %al
	mov	%eax, %es
	mov	%eax, %ds
	pushl	%esp
	call	trap
	popl	%esp
	popal
	popl	%es
	popl	%ds
	addl	$0x8, %esp
	iret
