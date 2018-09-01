/* hello - basic serial IO on the Fire3
 *
 * Grew into some exploring of system status,
 *  as well as alignment issues.
 */

void serial_init ( void );
void serial_putc ( int );

int printf(const char *format, ...);

unsigned char buf[8];

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

	asm volatile("mrs %0, sctlr_el3" : "=r" (val) : : "cc");
        return val;
}

static inline void
set_sctlr ( unsigned int val )
{
	asm volatile("msr sctlr_el3, %0" : : "r" (val) : "cc");
	asm volatile("isb");
}

#ifdef notdef
/* Pretty much bogus since this is only set during an exception */
static inline unsigned int
get_spsr(void)
{
        unsigned int val;

	asm volatile("mrs %0, spsr_el3" : "=r" (val) : : "cc");
        return val;
}
#endif

void
main ( void )
{
	unsigned int *p;
	unsigned int val;

	serial_init ();

	/*
	for ( ;; ) {
	    serial_putc ( 'x' );
	    serial_putc ( 'O' );
	}

	    val = get_spsr();
	    printf ( " SPSR = 0x%08x\n", val );
	*/

	val = get_el();
	printf ( " EL = %d\n", val );

	val = get_sctlr();
	printf ( " SCTLR = 0x%08x\n", val );


	val = 0x1234;
	printf ( " value = 0x%08x\n", val );
	printf ( "Hello world\n" );

	buf[0] = 0x11;
	buf[1] = 0x22;
	buf[2] = 0x33;
	buf[3] = 0x44;
	buf[4] = 0x55;
	buf[5] = 0x66;
	buf[6] = 0x77;
	buf[7] = 0x88;

	p = (unsigned int *) &buf[0];
	printf ( "Read from 0x%08x\n", p );
	val = *p;
	printf ( "Value = 0x%08x\n", val );
	printf ( "Done with 0x%08x\n", p );
	printf ( "\n" );

	p = (unsigned int *) &buf[2];
	printf ( "Read from 0x%08x\n", p );
	val = *p;
	printf ( "Value = 0x%08x\n", val );
	printf ( "Done with 0x%08x\n", p );
	printf ( "Finished\n" );
}

/* THE END */
