/* Timer for the s5p6818
 *
 * Tom Trebisky  9-17-2018
 */

#include "types.h"
#include "hello.h"
#include "fire3_ints.h"

#define N_TIMERS	5

struct tmr {
	vu32	count;
	vu32	cmp;
	vu32	obs;
};

struct timer {
	vu32 config0;			/* 00 */
	vu32 config1;			/* 04 */
	vu32 control;			/* 08 */
	struct tmr timer[N_TIMERS-1];	/* 0C */
	vu32 t4_count;			/* 08 */
	vu32 t4_obs;			/* 08 */
	vu32	int_cstat;		/* 44 */
};

#define TIMER_BASE	((struct timer *) 0xc0017000)
#define PWM_BASE	((struct timer *) 0xc0018000)

/* Timer is a down counter */

#define T0_AUTO		0x8
#define T0_INVERT	0x4
#define T0_LOAD		0x2
#define T0_RUN		0x1

#define T0_IENABLE		0x1
#define T1_IENABLE		0x2
#define T2_IENABLE		0x4
#define T3_IENABLE		0x8
#define T4_IENABLE		0x10

#define T0_ISTAT		0x20
#define T1_ISTAT		0x40
#define T2_ISTAT		0x80
#define T3_ISTAT		0x100
#define T4_ISTAT		0x200

void
timer_init ( void )
{
	struct timer *tp = TIMER_BASE;

	printf ( "Timer cstat = %08x\n", &tp->int_cstat );
	printf ( "Tc0 = %08x\n", tp->config0 );
	printf ( "Tc1 = %08x\n", tp->config1 );
	printf ( "Tctl = %08x\n", tp->control );
	printf ( "Cstat = %08x\n", tp->int_cstat );
	tp->int_cstat = T0_ISTAT;
	printf ( "Cstat = %08x\n", tp->int_cstat );
	printf ( "T0 = %08x\n", tp->timer[0].obs );
	// printf ( "T1 = %08x\n", tp->timer[1].obs );

	tp->control = 0;
	tp->timer[0].count = 0x40000;
	tp->timer[0].cmp = 1;
	tp->control = T0_AUTO | T0_LOAD;
	tp->control = T0_AUTO;

	printf ( "T0 = %08x\n", tp->timer[0].obs );
	printf ( "T0 = %08x\n", tp->timer[0].obs );
	printf ( "--\n" );

	tp->int_cstat |= T0_IENABLE;

	intcon_ena ( IRQ_TIMER0 );
	tp->control |= T0_RUN;

	/*
	for ( ;; ) {
	    printf ( "T0 = %08x\n", tp->timer[0].obs );
	    delay ( 100000 );
	}
	*/

	/*
	for ( ;; ) {
	    if ( tp->int_cstat & T0_ISTAT ) {
		serial_putc ( '.' );
		tp->int_cstat = T0_ISTAT;
	    }
	}
	*/
}

void
timer_handler ( void )
{
	struct timer *tp = TIMER_BASE;

	tp->int_cstat |= T0_ISTAT;
	serial_putc ( '.' );
}

/* THE END */
