/* hello.h */

int printf(const char *format, ...);

void gic_init ( void );
void gic_test ( void );

int intcon_irqwho ( void );
void intcon_irqack ( int );

void timer_init ( void );

void serial_init ( void );
void serial_putc ( int );

/* THE END */
