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
	unsigned long int i;
    WDTCTL  = WDTPW | WDTHOLD; 	// Stop WDT
    
    if (CALBC1_16MHZ != 0xFF) 
    {
		BCSCTL1 = CALBC1_16MHZ; // Set DCO
	  	DCOCTL  = CALDCO_16MHZ;
	}
 
 	// Configure P2.3.  Check section 8.2 of the reference manual (slau144j.pdf)
	P2DIR &= ~(BIT3); // P2.3 is an input	
	P2OUT |= BIT3;    // Select pull-up for P2.3
	P2REN |= BIT3;    // Enable pull-up for P2.3

    uart_init();
	set_stdout_to(uart_putc);

	printf("Push button test for the MSP430G2553.  Connect push button between P2.3 and gnd.\n");
	
    while(1)
    {
		printf("P2.3 (pin 11)=%c\r", (P2IN&BIT3)?'1':'0');
		for(i=0; i<200000; i++); // Don't overload PuTTy!  Wait a bit before sending again...
    } 
}

