/* Timer for the s5p6818
 *
 * Tom Trebisky  9-17-2018
 */

#include "types.h"
#include "hello.h"

#define N_TIMERS	5

struct tmr {
	vu32	count;
	vu32	cmp;
	vu32	obs;
};

struct timer {
	vu32 config0;
	vu32 config1;
	vu32 control;
	struct tmr timer[N_TIMERS];
	vu32	int_cstat;
};

#define TIMER_BASE	((struct timer *) 0xc001700)
#define PWM_BASE	((struct timer *) 0xc001800)

void
timer_init ( void )
{
	struct timer *tp = TIMER_BASE;

	printf ( "Tc0 = %08x\n", tp->config0 );
	printf ( "Tc1 = %08x\n", tp->config1 );
	printf ( "Tctl = %08x\n", tp->control );
	printf ( "T0 = %08x\n", tp->timer[0].obs );
	// printf ( "T1 = %08x\n", tp->timer[1].obs );

	tp->timer[0].count = 65536;
	tp->timer[0].cmp = 1000;
	tp->timer[0].obs = 0;

	printf ( "T0 = %08x\n", tp->timer[0].obs );
}

/* THE END */
