/*
 * Blink the on-board LED on a NanoPi Fire3
 *   Tom Trebisky  8-25-2018
 *
 * This blinks the on board LED on GPIO B12
 *  It works for the NanoPi M3 and Fire3 boards
 */

typedef volatile unsigned int vu32;

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

static void
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

/* This business is an experiment to find out if other cores
 * are starting (they are not, it turns out).
 * There is code in start.S (from Metro94) that looks like
 * it will handle other cores, and will call "boot_slave"
 * for any cores that start.  I made a simple change to start.S
 * to pass the cpu_id number (1-7) to boot_slave.
 * cpu_id == 0 gets routed to boot_master.
 * I use an array to avoid race conditions that might
 * arise if several cores were starting at once.
 * The result will be that this code will blink a number
 * of times that indicates the number of cores that start.
 * It blinks only once.
 */

#define NUM_CPU	8
int cpu_active[NUM_CPU] = { 0, 0, 0, 0, 0, 0, 0, 0 };

void
boot_master (void)
{
	struct gpio_regs *gp = GPIOB_BASE;
	int i;
	int count;

	gp->alt0 &= ~(3<<24);
	gp->alt0 |= 2<<24;
	gp->oe = LED_BIT;

	gp->out = LED_BIT;	/* off */

	for ( ;; ) {
	    delay ( LONG );
	    count = 1;
	    for ( i=0; i<NUM_CPU; i++ )
		if ( cpu_active[i] )
		    count++;

	    pulses ( count );
	}
}

void
boot_slave ( int who )
{
	cpu_active[who]++;
}

/* THE END */
