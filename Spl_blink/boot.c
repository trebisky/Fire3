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

#define DELAY_COUNT 400000

#define LED_BIT	(1<<12)

void
boot_master (void)
{
	struct gpio_regs *gp = GPIOB_BASE;
	int count;
	int led;

	gp->alt0 &= ~(3<<24);
	gp->alt0 |= 2<<24;
	gp->oe = LED_BIT;

	led = 0;
	for ( ;; ) {
	    for ( count=DELAY_COUNT; count; count-- )
		;
	    gp->out = led;
	    led ^= LED_BIT;
	}
}

void boot_slave(void) { }

/* THE END */
