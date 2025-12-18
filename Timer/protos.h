/* hello.h */

int printf(const char *format, ...);

void gic_init ( void );
void gic_test ( void );

void intcon_ena ( int );
int intcon_irqwho ( void );
void intcon_irqack ( int );

void timer_init ( void );
void timer_handler ( void );

void serial_init ( void );
void serial_putc ( int );

void delay ( int );

/* THE END */
