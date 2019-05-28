#include <msp430.h>
#include <stdio.h>

#define RXD BIT1 // Receive Data (RXD) at P1.1
#define TXD BIT2 // Transmit Data (TXD) at P1.2
#define CLK 16000000L
#define BAUD 115200

typedef void (*pfn_outputchar)(char c);
void set_stdout_to(pfn_outputchar p);
int printf2 (const char *format, ...);

void uart_init(void)
{
	P1SEL  |= (RXD | TXD);                       
  	P1SEL2 |= (RXD | TXD);                       
  	UCA0CTL1 |= UCSSEL_2; // SMCLK
  	UCA0BR0 = (CLK/BAUD)%0x100;
  	UCA0BR1 = (CLK/BAUD)/0x100;
  	UCA0MCTL = UCBRS0; // Modulation UCBRSx = 1
  	UCA0CTL1 &= ~UCSWRST; // Initialize USCI state machine
}

unsigned char uart_getc()
{
    while (!(IFG2&UCA0RXIFG)); // USCI_A0 RX buffer ready?
	return UCA0RXBUF;
}

void uart_putc (char c)
{
	if(c=='\n')
	{
		while (!(IFG2&UCA0TXIFG)); // USCI_A0 TX buffer ready?
	  	UCA0TXBUF = '\r'; // TX
  	}
	while (!(IFG2&UCA0TXIFG)); // USCI_A0 TX buffer ready?
  	UCA0TXBUF = c; // TX
}

void uart_puts(const char *str)
{
     while(*str) uart_putc(*str++);
}

int main(void)
{
    volatile unsigned int i;
    char buff[20];
    double x=1.23456789012345;
    
    WDTCTL  = WDTPW | WDTHOLD; 	// Stop WDT
    
    if (CALBC1_16MHZ != 0xFF) 
    {
		BCSCTL1 = CALBC1_16MHZ; // Set DCO
	  	DCOCTL  = CALDCO_16MHZ;
	}
 
	P1DIR |= (BIT0 | BIT6); 		// P1.0 and P1.6 are the red+green LEDs	
	P1OUT |= (BIT0 | BIT6); 		// All LEDs off

    uart_init();
	set_stdout_to(uart_putc);

	printf("printf() tests.\n");

	printf("float: %f\n", (float)(355.0/113.0));
	printf("float: %g\n", (float)(355.0/113.0));
	printf("float: %e\n", (float)(355.0/113.0));
	printf("float: %e\n", 0.1);
	printf("float: %f\n", 0.1);
	printf("float: %g\n", 0.1);
	printf("string: %s\n", "my string");
	printf("int: %d\n", 42);
	printf("int: %08x\n", 0xABCD);
	printf("char: %c\n", 'a');
	printf("pointer: %p\n", "some string");
	
	printf(" %08.3f, %8.3f, %20.6e, %15.8E\n", 355.0/113.0, 355.0/113.0, -355.0/113.0, (355.0/113.0)*1.0e22);
	sprintf(buff, "%+10.2f", 355.0/113.0);
	printf("Created string:`%s`\n", buff);
	printf("double: %20.16lf\n", x);
	
    while(1)
    {
		P1OUT ^= BIT0;
		for(i=0; i<50000; i++);
    } 
}

