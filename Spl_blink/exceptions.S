/*
 *	Copyright (c) 2017 Metro94
 *
 *	File:        exceptions.S
 *	Description: exception code for ARM-v8 AArch64 state
 *	Author:      Metro94 <flattiles@gmail.com>
 *  Date:        
 */

 	.global handle_exception

	.align 11
	.global vectors
vectors:

	.align 7
sync_el0:
	b		.
// 	bl		enter_exception
// 	adr		x1, sync_el0_desc
// 	bl		handle_exception
// 	b		exit_exception
// sync_el0_desc:
// 	.asciiz "Synchronous Exception, using SP_EL0"

	.align 7
irq_el0:
	b		.
// 	bl		enter_exception
// 	adr		x1, irq_el0_desc
// 	bl		handle_exception
// 	b		exit_exception
// irq_el0_desc:
// 	.asciiz "IRQ/vIRQ Exception, using SP_EL0"

	.align 7
fiq_el0:
	b		.
// 	bl		enter_exception
// 	adr		x1, fiq_el0_desc
// 	bl		handle_exception
// 	b		exit_exception
// fiq_el0_desc:
// 	.asciiz "FIQ/vFIQ Exception, using SP_EL0"

	.align 7
serror_el0:
	b		.
// 	bl		enter_exception
// 	adr		x1, serror_el0_desc
// 	bl		handle_exception
// 	b		exit_exception
// serror_el0_desc:
// 	.asciiz "SError/vSError Exception, using SP_EL0"

	.align 7
sync_elx:
	b		.
// 	bl		enter_exception
// 	adr		x1, sync_elx_desc
// 	bl		handle_exception
// 	b		exit_exception
// sync_elx_desc:
// 	.asciiz "Synchronous Exception, using SP_ELx"

	.align 7
irq_elx:
	b		.
// 	bl		enter_exception
// 	adr		x1, irq_elx_desc
// 	bl		handle_exception
// 	b		exit_exception
// irq_elx_desc:
// 	.asciiz "IRQ/vIRQ Exception, using SP_ELx"

	.align 7
fiq_elx:
	b		.
// 	bl		enter_exception
// 	adr		x1, fiq_elx_desc
// 	bl		handle_exception
// 	b		exit_exception
// fiq_elx_desc:
// 	.asciiz "FIQ/vFIQ Exception, using SP_ELx"

	.align 7
serror_elx:
	b		.
// 	bl		enter_exception
// 	adr		x1, serror_elx_desc
// 	bl		handle_exception
// 	b		exit_exception
// serror_elx_desc:
// 	.asciiz "SError/vSError Exception, using SP_ELx"

// 	.align 7
// enter_exception:# 
// 	stp		x29, x30, [sp, #-16]!
// 	stp		x27, x28, [sp, #-16]!
// 	stp		x25, x26, [sp, #-16]!
// 	stp		x23, x24, [sp, #-16]!
// 	stp		x21, x22, [sp, #-16]!
// 	stp		x19, x20, [sp, #-16]!
// 	stp		x17, x18, [sp, #-16]!
// 	stp		x15, x16, [sp, #-16]!
// 	stp		x13, x14, [sp, #-16]!
// 	stp		x11, x12, [sp, #-16]!
// 	stp		x9, x10, [sp, #-16]!
// 	stp		x7, x8, [sp, #-16]!
// 	stp		x5, x6, [sp, #-16]!
// 	stp		x3, x4, [sp, #-16]!
// 	stp		x1, x2, [sp, #-16]!
// 
// 	mrs		x3, CurrentEL
// 	cmp		x3, 0xc
// 	beq		3f
// 	cmp		x3, 0x8
// 	beq		2f
// 	cmp		x3, 0x4
// 	beq		1f
// 3:
// 	mrs		x1, ESR_EL3
// 	mrs		x2, ELR_EL3
// 	b		0f
// 2:
// 	mrs		x1, ESR_EL2
// 	mrs		x2, ELR_EL2
// 	b		0f
// 1:
// 	mrs		x1, ESR_EL1
// 	mrs		x2, ELR_EL1
// 0:
// 	stp		x2, x0, [sp, #-16]!
// 	mov		x0, sp
// 
// 	ret
// 
// exit_exception:
// 	ldp		x2, x0, [sp], #16
// 
// 	mrs		x3, CurrentEL
// 	cmp		x3, 0xc
// 	beq		3f
// 	cmp		x3, 0x8
// 	beq		2f
// 	cmp		x3, 0x4
// 	beq		1f
// 3:
// 	msr		ELR_EL3, x2
// 	b		0f
// 2:
// 	msr		ELR_EL2, x2
// 	b		0f
// 1:
// 	msr		ELR_EL1, x2
// 0:
// 	
// 	ldp		x1, x2, [sp], #16
// 	ldp		x3, x4, [sp], #16
// 	ldp		x5, x6, [sp], #16
// 	ldp		x7, x8, [sp], #16
// 	ldp		x9, x10, [sp], #16
// 	ldp		x11, x12, [sp], #16
// 	ldp		x13, x14, [sp], #16
// 	ldp		x15, x16, [sp], #16
// 	ldp		x17, x18, [sp], #16
// 	ldp		x19, x20, [sp], #16
// 	ldp		x21, x22, [sp], #16
// 	ldp		x23, x24, [sp], #16
// 	ldp		x25, x26, [sp], #16
// 	ldp		x27, x28, [sp], #16
// 	ldp		x29, x30, [sp], #16
// 
// 	eret
// // 