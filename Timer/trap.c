/* trap.c */

#include "protos.h"
#include "types.h"
#include "fire3_ints.h"

unsigned int GetCurrentSMode(void);
unsigned int GetCPUID(void);

// void psciHandler(unsigned long*);
// void boardReset(void);

#ifdef notdef
--- #define GIC_BASE	0xC0009000
--- 
--- struct gic64 {
--- 	char	_pad0[3840];
--- 	vu32	sgir;		/* 0x9f00 */
--- 	int	_pad1[3];
--- 	vu32	sgi_cpend[4];	/* 0x9f10 */
--- };
--- 
--- union {
--- 	u64	big;
--- 	u32	val[2];
--- } conv;
--- 
--- void
--- gic_init ( void )
--- {
--- 	struct gic64 *gp = (struct gic64 *) GIC_BASE;
--- 	u32 cur_el;
--- 	u32 addr;
--- 
--- 	/*
--- 	addr = 0x1234;
--- 	printf ( "gic at %x\n", addr );
--- 	addr = 0xfedc1234;
--- 	printf ( "gic at %x\n", addr );
--- 	*/
--- 
--- 	/*
--- 	conv.big = (u64) &gp->sgi_cpend[0];
--- 	addr = conv.val[0];
--- 	printf ( "gic at %x\n", addr );
--- 	addr = conv.val[1];
--- 	printf ( "gic at %x\n", addr );
--- 	*/
--- 
--- 	addr = (u32) &gp->sgi_cpend[0];
--- 	printf ( "gic at %08x\n", addr );
--- 
--- 	/*
--- 	gp = (struct gic64 *) 0;
--- 	addr = (u32) &gp->sgi_cpend[0];
--- 	printf ( "gic at %08x\n", addr );
--- 	*/
--- 
--- 	asm volatile("mrs %0, CurrentEL" : "=r" (cur_el) : : "cc");
--- 	cur_el >>= 2;
--- 	printf ( "Current EL = %d\n", cur_el );
--- }
--- 
--- void
--- gic_clear ( void )
--- {
--- 	struct gic64 *gp = (struct gic64 *) GIC_BASE;
--- 	// unsigned int *lp;
--- 	
---         // WriteIO32(0xc0009f10, -1);  // clear GIC interrupt
--- 	// lp = (unsigned int *) 0xC0009f10;
--- 	// *lp = 0xffffffff;
--- 	gp->sgi_cpend[0] = 0xffffffff;
--- }
#endif

struct b_reset {
	int	_pad;
	volatile unsigned int areg;
	volatile unsigned int breg;
};

void
boardReset(void)
{
	struct b_reset *rp = (struct b_reset *) 0xC0010220;

	printf("\nBoard reset.\n");

	rp->areg = 0x8;
	rp->breg = 0x1000;

	// WriteIO32(0xc0010224, 0x8);
	// WriteIO32(0xc0010228, 0x1000);

	for ( ;; )
	    ;
}

static void
handle_irq ( void )
{
	int n;

	// printf ( "trap.c -- IRQ!!\n" );
	n = intcon_irqwho();
	if ( n == IRQ_TIMER0 )
	    timer_handler ();
	else
	    printf ( "IRQ: %d\n", n );
	intcon_irqack ( n );
}

/* Lazy way to avoid compiler warning about
 * the regs argument not being used.
 */
volatile int shutup;

/* exc is offset of the exception
 * esr is the esr (exception syndrome register)
 * regs is the regs
 */
void
sync_handler ( unsigned exc, unsigned esr, unsigned long *regs )
{
	unsigned int class = esr >> 26;		/* 31:26 */
	unsigned int syndrome = esr & 0x1ffffff;	/* 24:0 */

	shutup = regs[0];

	switch( exc ) {
	    case 0x280:     // IRQ
		// gic_clear ();
		handle_irq ();
		return;
	    case 0x400:
		/* Synch from lower level */
		if( class == 0x17 ) {   // smc
		    if( syndrome == 0 ) {
			// psciHandler(regs);
			return;
		    }
		}
	}

	printf("\n--- PANIC! exception 0x%x on CPU%d EL %d, class=0x%x syndrome=0x%x\n",
            exc, GetCPUID(), GetCurrentSMode(), class, syndrome );

    boardReset();
}

/* THE END */
