/* start.S
 * This is a minimal startup file for a program to do
 * simple serial IO on the Fire3
 *
 * Tom Trebisky
 *
 * It is intended to be loaded by the bl1 loader,
 *  so it will need a NSIH header that indicates
 *  the load address and such.
 */

.globl  start
start:
        b       reset

        .skip   60
        .word 0x00000000    // 0x040 : Device Read Address for next boot
        .word 0x00010000    // 0x044 : Load Size for this
        .word 0x40000000    // 0x048 : Load Address for this
        .word 0x40000000    // 0x04C : Launch Address for this

        .skip 428
        .word 0x4849534E    // 0x1FC        "NSIH"

        .align 3

reset:
	bl	main
	b	.

// THE END
