#include <uk/asm.h>
#include <uk/arch/lcpu.h>

ENTRY(sched_yield_simulate)
/* Mask IRQ to make sure restore would not be interrupted by IRQ */
	msr	daifset, #2

	stp x19, x20, [sp, #-16]!

	/* Restore pstate and exception status register */
	ldp	x1, x2, [x0, #16 * 16]
	msr	spsr_el1, x1
	msr	esr_el1, x2

	/*
         * We want to execute uk_sched_yield at the eret to be able to schedule
         * the preload thread.
         */
        adrp x1, sched_yield
        add x1, x1, :lo12:sched_yield
	msr	elr_el1, x1

	mov x20, x30

        adrp x30, restore_point_yield
        add x30, x30, :lo12:restore_point_yield

	/* Restore stack pointer for exception from EL1 */
	ldr	x1, [x0, #__SP_OFFSET]
	mov	x19, sp
	mov	sp, x1

	eret

restore_point_yield:
// Mask IRQ again
	msr	daifset, #2
	mov x30, x20
	mov sp, x19

	ldp x19, x20, [sp], #16

	ret

ENDPROC(sched_yield_simulate)
