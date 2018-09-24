/* serial.c
 * Simple driver for the s5p6818 console uart
 * Tom Trebisky 8-31-2018
 */

#include "hello.h"

typedef volatile unsigned int vu32;

void blink_init ( void );
void blink_run ( void );

/* The s5p6818 uart is described in chapter 25 of the user manual
 *
 * This is a start of my own driver, but it simply inherits
 *  the initialization done by bl1.
 *
 */

struct serial {
	vu32	lcon;		/* 00 */
	vu32	con;		/* 04 */
	vu32	fcon;		/* 08 */
	vu32	mcon;		/* 0c */
	vu32	status;		/* 10 */
	vu32	re_stat;	/* 14 */
	vu32	fstat;		/* 18 */
	vu32	mstat;		/* 1c */
	vu32	txh;		/* 20 */
	vu32	rxh;		/* 24 */
	vu32	brdiv;		/* 28 */
	vu32	fract;		/* 24 */

	vu32	intp;		/* 30 */
	vu32	ints;		/* 34 */
	vu32	intm;		/* 38 */
};

struct serial *uart_base[] = {
	(struct serial *) 0xC00a1000,
	(struct serial *) 0xC00a0000,
	(struct serial *) 0xC00a2000,
	(struct serial *) 0xC00a3000,
	(struct serial *) 0xC006d000,
	(struct serial *) 0xC006f000
};

#define CONSOLE_BASE	(struct serial *) 0xC00a1000;

#define	FIFO_FULL	0x01000000
#define RX_FIFO_COUNT   0x0000007f

void
serial_init ( void )
{
	// blink_init ();
	// blink_run ();
}

void
serial_putc ( int c )
{
	struct serial *up = CONSOLE_BASE;

	if ( c == '\n' )
	    serial_putc ( '\r' );

	while ( up->fstat & FIFO_FULL )
	    ;

	up->txh = c;
}

int
serial_getc ( void )
{
	struct serial *up = CONSOLE_BASE;

        while ( ! (up->fstat & RX_FIFO_COUNT) )
                ;

        return up->rxh & 0xff;
}

void
serial_test ( void )
{
	int x;

	for ( ;; ) {
	    x = serial_getc ();
	    printf ( "Serial: %02x %c\n", x, x );
	}
}

/* ----------------------------------------- */

struct gpio_regs {
	vu32 out;	/* 00 */
	vu32 oe;	/* 04 */
	vu32 dm0;	/* 08 */
	vu32 dm1;	/* 0c */
	vu32 ie;	/* 10 */
	vu32 xdet;	/* 14 */
	vu32 xpad;	/* 18 */
	int  __pad0;	/* 1c */
	vu32 alt0;	/* 20 */
	vu32 alt1;	/* 24 */
	vu32 dmx;	/* 28 */
	int  __pad1[4];
	vu32 dmen;	/* 3c */
	vu32 slew;	/* 40 */
	vu32 slewdis;	/* 44 */
	vu32 drv1;	/* 48 */
	vu32 drv1dis;	/* 4c */
	vu32 drv0;	/* 50 */
	vu32 drv0dis;	/* 54 */
	vu32 pull;	/* 58 */
	vu32 pulldis;	/* 5c */
	vu32 pullen;	/* 60 */
	vu32 pullendis;	/* 64 */
};

#define GPIOB_BASE	((struct gpio_regs *) 0xC001B000)

#define LONG  800000
#define MEDIUM  200000
#define SHORT 100000

#define LED_BIT	(1<<12)

void
delay ( int count )
{
	while ( count )
	    count--;
}

static void
pulse ( void )
{
	struct gpio_regs *gp = GPIOB_BASE;

	gp->out = LED_BIT;
	delay ( MEDIUM );
	gp->out = 0;
	delay ( SHORT );
	gp->out = LED_BIT;
}

static void
pulses ( int num )
{
	while ( num-- )
	    pulse ();
}

/* This is an experiment to find out what EL the first
 * bit of 64 bit code runs at.
 * see u-boot: arch/arm/include/asm/system.h
 */

static inline unsigned int
get_el(void)
{
        unsigned int val;

        asm volatile("mrs %0, CurrentEL" : "=r" (val) : : "cc");
        return val >> 2;
}

void
blink_init ( void )
{
	struct gpio_regs *gp = GPIOB_BASE;

	/* Configure the GPIO */
	gp->alt0 &= ~(3<<24);
	gp->alt0 |= 2<<24;
	gp->oe = LED_BIT;

	gp->out = LED_BIT;	/* off */
}

void
blink_run (void)
{
	int count;

	count = get_el();

	for ( ;; ) {
	    delay ( LONG );

	    pulses ( count );
	}
}

/* THE END */
