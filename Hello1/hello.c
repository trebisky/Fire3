/* hello */

void serial_init ( void );
void serial_putc ( int );

int printf(const char *format, ...);

void
main ( void )
{
	int val = 9123;
	serial_init ();

	/*
	for ( ;; ) {
	    serial_putc ( 'x' );
	    serial_putc ( 'O' );
	}
	*/
	printf ( " value = %d\n", val );
	printf ( " value = 0x%08x\n", val );
	printf ( "Hello world\n" );
}

/* THE END */
