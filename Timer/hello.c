/* hello - basic serial IO on the Fire3
 *
 * Grew into some exploring of system status,
 *  as well as alignment issues.
 */

#include <types.h>
#include <hello.h>

#ifdef notdef
/*
 * see u-boot: arch/arm/include/asm/system.h
 */

static inline unsigned int
get_el(void)
{
        unsigned int val;

        asm volatile("mrs %0, CurrentEL" : "=r" (val) : : "cc");
        return val >> 2;
}

static inline unsigned int
get_sctlr(void)
{
        unsigned int val;

	asm volatile("mrs %0, sctlr_el2" : "=r" (val) : : "cc");
        return val;
}

static inline void
set_sctlr ( unsigned int val )
{
	asm volatile("msr sctlr_el2, %0" : : "r" (val) : "cc");
	asm volatile("isb");
}

/* Pretty much bogus since this is only set during an exception */
static inline unsigned int
get_spsr(void)
{
        unsigned int val;

	asm volatile("mrs %0, spsr_el2" : "=r" (val) : : "cc");
        return val;
}
#endif

/* These only go inline with gcc -O of some kind */
static inline void
INT_unlock ( void )
{
	asm volatile("msr DAIFClr, #3" : : : "cc");
}

static inline void
INT_lock ( void )
{
	asm volatile("msr DAIFSet, #3" : : : "cc");
}

/* ARM v8 Performance monitor testing */

void
pm_test ( void )
{
	unsigned long ccval;
	unsigned long val;
	int i;

	asm volatile("mrs %0, pmcntenset_el0" : "=r" (val) : : "cc");
	printf ( "PMCNTENSET = %x\n", val );
	asm volatile("mrs %0, pmcr_el0" : "=r" (val) : : "cc");
	printf ( "PMCR = %x\n", val );

	val = 0x80000000;
	asm volatile("msr pmcntenset_el0, %0" : : "r" (val) : "cc");
	val = 1;
	asm volatile("msr pmcr_el0, %0" : : "r" (val) : "cc");

	asm volatile("mrs %0, pmcntenset_el0" : "=r" (val) : : "cc");
	printf ( "PMCNTENSET = %x\n", val );
	asm volatile("mrs %0, pmcr_el0" : "=r" (val) : : "cc");
	printf ( "PMCR = %x\n", val );

	for ( i=0; i<5; i++ ) {
	    asm volatile("mrs %0, pmccntr_el0" : "=r" (ccval) : : "cc");
	    printf ( "CCNT = %d\n", ccval );
	}
}


void
main ( void )
{
	serial_init ();
	gic_init ();
	timer_init ();

	printf ( "\n" );
	printf ( "Hello world\n" );

	// INT_unlock ();

	// gic_test ();

	printf ( "int is: %d bytes\n", sizeof(int) );
	printf ( "long is: %d bytes\n", sizeof(long) );
	printf ( "ll is: %d bytes\n", sizeof(long long) );

	printf ( "Waiting ...\n" );

	// serial_test ();

	pm_test ();

	for ( ;; )
	    ;

	/* return to the boot loader */
}

/* THE END */
