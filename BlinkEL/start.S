/*
 *	Copyright (c) 2017 Metro94
 *
 *	File:        start.S
 *	Description: startup code for S5P6818
 *	Author:      Metro94 <flattiles@gmail.com>
 *  Date:        
 */

#define CPU_NUMBERS				8

#define INTERNAL_SRAM_BASE		0xffff0000
#define INTERNAL_SRAM_SIZE		0x00010000

#define STACK_SIZE				0x80

	.global boot_master
	.global boot_slave

	// Global constants declare
	// .global vectors

	.align 9

	// space for NSIH
	// .skip 0x800, 0x00
	// tjt - no longer need this with my loader.

	.global start
start:
	b		reset

// Returns the EL but in bits [3:2]
//	.global get_el
// get_el:
// 	mrs		x0, CurrentEL
// 	ret

	.global build_info
build_info:
	.word	0x68180001		// S5P6818, v0.1

reset:
	msr		DAIFSet, #7		// Disable IRQ & FIQ & Asynchronous Abort Exceptions
							// [2]: Asynchronous Abort
							// [1]: IRQ
							// [0]: FIQ

	bl		getCPUID
	mov		x29, x0			// Save CPUID to x29

	// Set vectors
	adr		x0, vectors
	msr		VBAR_EL3, x0

	ands		x0, x29, #3
	bne		prepare_cpu		// if (CPUID != 0 && CPUID != 4) skip clear_bss & set_cache
	cmp		x29, xzr
	bne		set_cache		// if (CPUID != 0) skip clear_bss

	// Clear BSS
	ldr		x1, =__bss_start__
	ldr		x2, =__bss_end__

clear_bss_loop:
	str		xzr, [x1], #8
	cmp		x1, x2
	bls		clear_bss_loop	// if x1 <= x2 goto clear_bss_loop

	// Set L2 cache
set_cache:
	mrs		x0, S3_1_C11_C0_2	// L2CTLR_EL1
	orr		x0, x0, #(1 << 22)		// [22] Core RAMs are implemented with ECC.
	orr		x0, x0, #(1 << 21)		// [21] L2 cache is implemented with ECC.
	orr		x0, x0, #(1 << 5)		// [5]  2-cycle input delay from L2 data RAMs.
	orr		x0, x0, #(1 << 0)		// [0]  3-cycle output delay from L2 data RAMs.
	msr		S3_1_C11_C0_2, x0

	mrs		x0, S3_1_C15_C0_0	// L2ACTLR_EL1
	bic		x0, x0, #(1 << 14)		// [14] Disables UniqueClean evictions with data. This is the reset value for ACE.
	bic		x0, x0, #(1 << 3)		// [3]  Enables clean/evict to be pushed out to external. This is the reset value for ACE.
	msr		S3_1_C15_C0_0, x0

	mrs		x0, ACTLR_EL3
	orr		x0, x0, #(1 << 6)		// [6]  L2ACTLR_EL1 is write accessible from EL2.
	orr		x0, x0, #(1 << 5)		// [5]  L2ECTLR_EL1 is write accessible from EL2.
	orr		x0, x0, #(1 << 4)		// [4]  L2CTLR_EL1 is write accessible from EL2.
	orr		x0, x0, #(1 << 1)		// [1]  CPUECTLR_EL1 is write accessible from EL2.
	orr		x0, x0, #(1 << 0)		// [0]  CPUACTLR_EL1 is write accessible from EL2.
	msr		ACTLR_EL3, x0

	// Set registers of each CPU
prepare_cpu:
	mrs		x0, SCR_EL3
	orr		x0, x0, #(1 << 10)		// [10] The next lower level is AArch64.
	bic		x0, x0, #(1 << 8)		// [8]  The HVC instruction is UNDEFINED at all exception levels.
	bic		x0, x0, #(1 << 7)		// [7]  The SMC instruction is enabled at EL1, EL2, and EL3. This is the reset value.
	orr		x0, x0, #(1 << 5)		// [5]  RES1
	orr		x0, x0, #(1 << 4)		// [4]	RES1
	orr		x0, x0, #(1 << 2)		// [2]  Physical FIQ while executing at all exception levels are taken in EL3.
	bic		x0, x0, #(1 << 0)		// [0]	EL0 and EL1 are in Secure state, memory accesses from those exception levels can access Secure memory.
	msr		SCR_EL3, x0

	mrs		x0, CPTR_EL3
	bic		x0, x0, #(1 << 31)		// [31] Does not cause access to the CPACR_EL1 or CPTR_EL2 to be trapped.
	bic		x0, x0, #(1 << 10)		// [10] Does not cause any instruction to be trapped. This is the reset value if the Advanced SIMD and Floating-point Extension is implemented.
	msr		CPTR_EL3, x0

	mrs		x0, S3_1_C15_C2_1	// CPUECTLR_EL1
	orr		x0, x0, #(1 << 6)		// [6]  Enables data coherency with other cores in the cluster.
	msr		S3_1_C15_C2_1, x0
	isb

	mrs		x0, CPACR_EL1
	orr		x0, x0, #(3 << 20)		// [21:20] No instructions in EL0 or EL1 that uses registers associated with Advanced SIMD and Floating-point execution are trapped.
	msr		CPACR_EL1, x0

	mrs		x0, HCR_EL2
	orr		x0, x0, #(1 << 31)		// [31] EL1 is AArch64. EL0 is determined by the register width described in the current processing state when executing at EL0.
	bic		x0, x0, #(1 << 27)		// [27] Does not traps general exceptions.
	orr		x0, x0, #(1 << 4)		// [4]  Physical IRQ while executing at EL2 or lower are taken in EL2 unless routed by SCTLR_EL3.IRQ bit to EL3. Virtual IRQ interrupt is enabled.
	msr		HCR_EL2, x0

	mrs		x0, CPTR_EL2
	bic		x0, x0, #(1 << 31)		// [31] Access to CPACR is not trapped.
	bic		x0, x0, #(1 << 10)		// [10] Instructions are not trapped. This is the reset value if Advanced SIMD and Floating-point are implemented.
	msr		CPTR_EL2, x0

	// TODO: make out if not skip codes below

	mrs		x0, SCTLR_EL3
	bic		x0, x0, #(1 << 12)		// [12] Instruction caches disabled.
	msr		SCTLR_EL3, x0
	ic		ialluis				// Invalidate all instruction caches in Inner Shareable domain to Point of Unification.
	isb		sy

	mrs		x0, SCTLR_EL3
	orr		x0, x0, #(1 << 12)		// [12] Instruction caches enabled.
	msr		SCTLR_EL3, x0

	mrs		x0, SCTLR_EL2
	bic		x0, x0, #(1 << 25)		// [25] Little endian.
	bic		x0, x0, #(1 << 19)		// [19] Regions with write permissions are not forced XN.
	bic		x0, x0, #(1 << 12)		// [12] Instruction caches disabled.
	bic 		x0, x0, #(1 << 3)		// [3]  Disables stack alignment check.
	bic		x0, x0, #(1 << 2)		// [2]  Disables data and unified caches.
	bic		x0, x0, #(1 << 1)		// [1]  Disables alignment fault checking.
	bic		x0, x0, #(1 << 0)		// [0]  Disables EL2 MMU.
	msr		SCTLR_EL2, x0

	cmp		x29, xzr
	bne		0f

	// Make stack for each CPU
	// CPU0: -0x1c0
	// CPU1: -0x180
	// CPU2: -0x140
	// CPU3: -0x100
	// CPU4: -0x0c0
	// CPU5: -0x080
	// CPU6: -0x040
	// CPU7: -0x000
make_stack:
	ldr		x0, =INTERNAL_SRAM_BASE
	add		x0, x0, #INTERNAL_SRAM_SIZE
	ldr		x1, =CPU_NUMBERS
	sub		x1, x1, #1
	sub		x1, x1, x29
	ldr		x2, =STACK_SIZE
	mul		x1, x1, x2
	sub		x0, x0, x1

	mov		sp, x0
	msr		SP_EL2, x0

0:
	cmp		x29, xzr
	bne		1f

	b		boot_master
	b		.

1:
	ands		x0, x29, #3		// tjt -- call with cpu number
	b		boot_slave
	b		.

// *****
// uint64_t getCPUID(void);

getCPUID:
	mrs		x1, MPIDR_EL1
	and		x0, x1, #0x3		// Get Affinity Level 0
	lsr		x1, x1, #8
	and		x1, x1, #0xF		// Get Affinity Level 1
	orr		x0, x0, x1
	ret

// append exceptions.S

	.align 11
//	.global vectors
vectors:

	.align 7
sync_el0:
	b		.

	.align 7
irq_el0:
	b		.

	.align 7
fiq_el0:
	b		.

	.align 7
serror_el0:
	b		.

	.align 7
sync_elx:
	b		.

	.align 7
irq_elx:
	b		.

	.align 7
fiq_elx:
	b		.

	.align 7
serror_elx:
	b		.

// THE END
