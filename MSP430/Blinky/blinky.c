//  blink.c: MSP430 Blink example.  Connect LED+resistor to P1.0 (pin 2 of MSP430G2553 20 DIP)

#include <msp430.h>

#define LED_BLINK_FREQ_HZ 1 //LED blinking frequency
#define LED_DELAY_CYCLES (1000000 / (2 * LED_BLINK_FREQ_HZ)) // Number of cycles to delay based on 1MHz MCLK

int main(void)
{
	WDTCTL = WDTPW + WDTHOLD; // Stop WDT
	P1DIR |= 0x01;            // Configure P1.0 as output
	while (1)
	{
	    __delay_cycles(LED_DELAY_CYCLES); // Wait for LED_DELAY_CYCLES cycles
	    P1OUT ^= 0x01; // Toggle P1.0 output
	}
}

