#include <msp430.h>
#include <stdio.h>

#define RXD BIT1 // Receive Data (RXD) at P1.1
#define TXD BIT2 // Transmit Data (TXD) at P1.2
#define CLK 16000000L
#define BAUD 115200

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

char HexDigit[]="0123456789ABCDEF";

void PrintNumber(int val, int Base, int digits)
{ 
	int j;
	#define NBITS 16
	char buff[NBITS+1];
	buff[NBITS]=0;

	j=NBITS-1;
	while ( (val>0) | (digits>0) )
	{
		buff[j--]=HexDigit[val%Base];
		val/=Base;
		if(digits!=0) digits--;
	}
	uart_puts(&buff[j+1]);
}

int main(void)
{
	volatile unsigned long int i; // volatile to prevent optimization
	char buff[16];
	unsigned long int v;
	
	WDTCTL = WDTPW + WDTHOLD; // Stop WDT
    
    if (CALBC1_16MHZ != 0xFF) 
    {
		BCSCTL1 = CALBC1_16MHZ; // Set DCO
	  	DCOCTL  = CALDCO_16MHZ;
	}
    uart_init();
	
	uart_puts("\nADC test program.  Measuring the voltage at input A3 (pin 5 in DIP 20 package)\n");
	
	ADC10CTL1 = INCH_3; // input A3
    ADC10AE0 |= 0x08;   // PA.3 ADC option select
	ADC10CTL0 = SREF_0 + ADC10SHT_3 + REFON + ADC10ON; // Use Vcc (around 3.3V) as reference

	while (1)
	{
		ADC10CTL0 |= ENC + ADC10SC;    // Sampling and conversion start
		while (ADC10CTL1 & ADC10BUSY); // ADC10BUSY?
		
		uart_puts("ADC[A3]=0x");
		PrintNumber(ADC10MEM, 16, 3);
		uart_puts(", ");
		v=(ADC10MEM*3290L)/1023L; // 3.290 is VDD
		PrintNumber(v/1000, 10, 1);
		uart_puts(".");
		PrintNumber(v%1000, 10, 3);
		uart_puts("V\r");

		for(i=0; i<200000; i++);
	}
}