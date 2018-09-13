/* mini.S
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
        .word 0x00000010    // 0x044 : Load Size for this (not including header)
        .word 0x40000000    // 0x048 : Load Address for this
        .word 0x40000000    // 0x04C : Launch Address for this

        .skip 428
        .word 0x4849534E    // 0x1FC        "NSIH"

        .align 3

reset:
	ret
        .word 0x00000000
        .word 0x11223344
        .word 0xdeadbeef

/* The image size needs to be padded to a multiple of 16 to load over USB.
 * the usb_loader should do this, but I provide the my own padding just
 * to be absolutely sure
 */

// THE END
