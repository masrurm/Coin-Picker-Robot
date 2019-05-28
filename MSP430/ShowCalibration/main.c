#include <msp430.h>
#include <stdio.h>
#include "uart.h"

#define RXD BIT1 // Receive Data (RXD) at P1.1
#define TXD BIT2 // Transmit Data (TXD) at P1.2
#define CLK 16000000L
#define BAUD 115200

void uart_puts(const char *str);
char pfbuf[128]; // used by sprintf()
#define printf(...) sprintf(pfbuf, __VA_ARGS__); uart_puts(pfbuf)


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

void uart_putc(unsigned char c)
{
	while (!(IFG2&UCA0TXIFG)); // USCI_A0 TX buffer ready?
  	UCA0TXBUF = c; // TX
}

void uart_puts(const char *str)
{
     while(*str) uart_putc(*str++);
}

char ASCII[]="0123456789ABCDEF";

void OutByte(unsigned char c)
{
	uart_putc(ASCII[c/0x10]);
	uart_putc(ASCII[c%0x10]);
}

void Out_hex (unsigned int Flash_Start, unsigned int Flash_Len)
{
	unsigned int k, chksum;
	unsigned int long j;
	unsigned char * p;
	
	p=(unsigned char *)Flash_Start;
	
	for(j=0; j<Flash_Len; j+=16)
	{
		uart_putc(':');
		OutByte(0x10);
		k=Flash_Start+j;
		OutByte((k/0x100));				
		OutByte((k%0x100));				
		OutByte(0x00);				
		chksum=0x10+(k/0x100)+(k%0x100);
		for(k=0; k<16; k++)
		{
			OutByte(p[j+k]);
			chksum+=p[j+k];
		}
		OutByte(-chksum&0xff);
		uart_putc('\n');
		uart_putc('\r');
	}
	uart_puts(":00000001FF\n\r"); // End record
}

int main(void)
{
    volatile unsigned int i;
    
    WDTCTL  = WDTPW | WDTHOLD; 	// Stop WDT
    
    if (CALBC1_16MHZ != 0xFF) 
    {
		BCSCTL1 = CALBC1_16MHZ; // Set DCO
	  	DCOCTL  = CALDCO_16MHZ;
	}
 
	P1DIR |= (BIT0 | BIT6); 		// P1.0 and P1.6 are the red+green LEDs	
	P1OUT |= (BIT0 | BIT6); 		// All LEDs off

    uart_init();

    uart_puts((char *)"Info memory (using Intel's HEX format):\n\r");
	Out_hex(0x1000, 0x100);

    uart_puts("Jump table and interrupt vector table:\n\r");
	Out_hex((unsigned int)0xff80, 0x80);
	
    while(1)
    {
		P1OUT ^= BIT0;
		for(i=0; i<50000; i++);
    } 
}

