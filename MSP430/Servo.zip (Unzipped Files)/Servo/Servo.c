//	ELEC 291 Project 2: Coin Picker Robot	
//	Akash Randhawa, Ishaan Agarwal, Janahan Dhushenthen, Masrur Mahbub, Sanid Singhal, Yekta Ataozden
//	2019/03/21
//
//	Adapted from...
//  Servo.c: Use TIMERA0 CCRO ISR to produce a servo signal at pin P1.0
//  By Jesus Calvino-Fraga (c) 2018

/* Include header files */
#include <msp430.h>
#include <stdio.h>
#include <stdlib.h>			

/* Define constants */
#define MAGNET1  BIT0	// Electro magnet at P1.0
#define PWMSPEED 2000	// Max DC motor speed: 2000
#define DCRIGHT2 BIT4	// Right wheel backwards at P2.4
#define DCRIGHT1 BIT3	// Right wheel forwards at P2.3
#define DCLEFT2 BIT5	// Left wheel backwards at P2.5
#define DCLEFT1 BIT0	// Left wheel forwards at P2.0
#define SERVO1 BIT4		// Top servo at P1.4
#define SERVO0 BIT3 	// Bottom servo at P1.3
#define RXD BIT1 		// Receive Data (RXD) at P1.1
#define TXD BIT2 		// Transmit Data (TXD) at P1.2
#define CLK 16000000L	// CLK rate
#define ISRF 100000L 	// Interrupt Service Routine Frequency for a 10 us interrupt rate
#define BAUD 115200		// Baud rate
#define CCR_1MS_RELOAD (16000000L/1000L)

/* Declare global variables */
volatile int ISR_pw1=0, ISR_pw0=0, ISR_cnt=0, ISR_frc, servoflag, DCSTATE, magnetflag; 
volatile long int count, frequency, v;
volatile float T, f;
int metaldetect=1, flag, alternate, delaya, time, ran, choice, perimeter, turntime;

/* Timer0 A0 interrupt service routine (ISR).  Only used by CC0. */ 
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer0_A0_ISR (void)
{
	TA0CCR0 += (CLK/ISRF); // Add Offset to CCR0 
   	ISR_cnt++;			   // Increment ISR count
   	
   	
   	
   	// Operate electromagnet
   	if(magnetflag == 1) {
   		P1OUT |= MAGNET1;	// Turn on magnet
   	}
   	else {
   		P1OUT &= ~MAGNET1;	// Turn off magnet
   	}v=(ADC10MEM*3290L)/1023L;
   	
	// Operate top servo
	if (servoflag == 1) {
		
		// Check if ISR count has reached pwm value
		if(ISR_cnt<ISR_pw1) {
			P1OUT |= SERVO1;	// Turn on the servo
		} 
		else {
        	P1OUT &= ~SERVO1;	// Turn off the servo 
        }
    }
        
    // Operate bottom servo	
	else {
	
		// Check if ISR count has reached pwm value
		if (ISR_cnt<ISR_pw0) {
			P1OUT |= SERVO0;	// Turn on the servo
		}
		else {
			P1OUT &= ~SERVO0;	// Turn off the servo
		}
	}
	
	// Operate DC motor
	// Check if ISR count has reached pwm value
	if (ISR_cnt < PWMSPEED) {
		
		// Move forward
		if (DCSTATE == 1){
			P2OUT &= ~DCLEFT2;
			P2OUT &= ~DCRIGHT2;
			P2OUT |= DCLEFT1;
			P2OUT |= DCRIGHT1;
		}
		
		// Move backwards
		else if (DCSTATE == 2){
			P2OUT &= ~DCLEFT1;
			P2OUT &= ~DCRIGHT1;
			P2OUT |= DCLEFT2;
			P2OUT |= DCRIGHT2;
		}
		
		// Turn right
		else if (DCSTATE == 3){
			P2OUT &= ~DCLEFT2;
			P2OUT &= ~DCRIGHT1;
			P2OUT |= DCLEFT1;
			P2OUT |= DCRIGHT2;
		}
		
		// Turn left
		else if (DCSTATE == 4) {
		    P2OUT &= ~DCLEFT1;
			P2OUT &= ~DCRIGHT2;
			P2OUT |= DCLEFT2;
			P2OUT |= DCRIGHT1;
		}
		
		// Don't move
		else {
			P2OUT &= ~DCLEFT2;
			P2OUT &= ~DCRIGHT2;
			P2OUT &= ~DCLEFT1;
			P2OUT &= ~DCRIGHT1;
		} 
	}
	
	// Don't move
	else {
		P2OUT &= ~DCLEFT2;
		P2OUT &= ~DCRIGHT2;
		P2OUT &= ~DCLEFT1;
		P2OUT &= ~DCRIGHT1;
	}
	
	// Reset ISR counter when it reaches 2000
	if(ISR_cnt>=2000) {
		ISR_cnt=0;		// 2000*10us=20ms
		ISR_frc++;
   	}
}

/* UART intialization */
void uart_init(void)
{
    P2SEL  |= (RXD | TXD); 
    P2SEL2 |= (RXD | TXD); 
	P1SEL  |= (RXD | TXD);                       
  	P1SEL2 |= (RXD | TXD);                       
  	UCA0CTL1 |= UCSSEL_2; 		// SMCLK
  	UCA0BR0 = (CLK/BAUD)%0x100;
  	UCA0BR1 = (CLK/BAUD)/0x100;
  	UCA0MCTL = UCBRS0; 			// Modulation UCBRSx = 1
  	UCA0CTL1 &= ~UCSWRST; 		// Initialize USCI state machine
}

/* UART write character */
void uart_putc (char c)
{
	if(c=='\n')
	{
		while (!(IFG2&UCA0TXIFG));	// USCI_A0 TX buffer ready?
	  	UCA0TXBUF = '\r'; 			// TX
  	}
	while (!(IFG2&UCA0TXIFG)); 		// USCI_A0 TX buffer ready?
  	UCA0TXBUF = c; 					// TX
}

/* UART read character */
unsigned char uart_getc()
{
	unsigned char c;
    while (!(IFG2&UCA0RXIFG)); // USCI_A0 RX buffer ready?
    c=UCA0RXBUF;
    uart_putc(c);
	return UCA0RXBUF;
}

/* UART write string */
void uart_puts(const char *str)
{
	while(*str) uart_putc(*str++);
}

/* UART read string */
unsigned int uart_gets(char *str, unsigned int max)
{
	char c;
	unsigned int cnt=0;
	while(1)
	{
	    c=uart_getc();
	    if( (c=='\n') || (c=='\r') )
	    {
	    	*str=0;
	    	break;
	    }
		*str=c;
		str++;
		cnt++;
		if(cnt==(max-1))
		{
		    *str=0;
	    	break;
		}
	}
	return cnt;
}

const char HexDigit[]="0123456789ABCDEF";

/* Prints number using UART */
void PrintNumber(unsigned long int val, int Base, int digits)
{ 
	int j;
	#define NBITS 32
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

/* Delay in milliseconds */
void delay_ms (int msecs)
{	
	int ticks;
	ISR_frc=0;
	ticks=msecs/20;
	while(ISR_frc<ticks);
}


// Use TA0 configured as a free running timer to delay 1 ms
void wait_1ms (void)
{
	unsigned int saved_TA0R;
	
	saved_TA0R=TA0R; // Save timer A0 free running count
	while ((TA0R-saved_TA0R)<(16000000L/1000L));
}

void waitms(int ms)
{
	while(--ms) wait_1ms();
}

#define PIN_PERIOD ( P2IN & BIT1 ) // Read period from pin P2.1

// GetPeriod() seems to work fine for frequencies between 30Hz and 300kHz.
long int GetPeriod (int n)
{
	int i, overflow;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
	overflow=0;
	TA0CTL&=0xfffe; // Clear the overflow flag
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(TA0CTL&1) { TA0CTL&=0xfffe; overflow++; if(overflow>5) return 0;}
	}
	overflow=0;
	TA0CTL&=0xfffe; // Clear the overflow flag
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(TA0CTL&1) { TA0CTL&=0xfffe; overflow++; if(overflow>5) return 0;}
	}
	
	overflow=0;
	TA0CTL&=0xfffe; // Clear the overflow flag
	saved_TCNT1a=TA0R;
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(TA0CTL&1) { TA0CTL&=0xfffe; overflow++; if(overflow>1024) return 0;}
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(TA0CTL&1) { TA0CTL&=0xfffe; overflow++; if(overflow>1024) return 0;}
		}
	}
	saved_TCNT1b=TA0R;
	if(saved_TCNT1b<saved_TCNT1a) overflow--; // Added an extra overflow.  Get rid of it.

	return overflow*0x10000L+(saved_TCNT1b-saved_TCNT1a);
}


/* Main function */
int main(void)
{	
	// Declare variables
	char buf[32];
	int pw;
	
	WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
	
	// Initialize pins
	P1DIR |= (SERVO1|SERVO0|MAGNET1); 				// Set pins 1.x 
    P2DIR |= (DCRIGHT1|DCRIGHT2|DCLEFT1|DCLEFT2);	// Set pins 2.x
    // Pin 2.1 is what will be used for reading frequency for metal detection
    P2DIR &= ~(BIT1); // P2.1 is an input	
	P2OUT |= BIT1;    // Select pull-up for P2.1
	P2REN |= BIT1;    // Enable pull-up for P2.1
	
	//Initializing ADC Pin
	
	ADC10CTL1 = INCH_7; // input A7 (Pin 1.7)
    ADC10AE0 |= 0x08;   // PA.7 ADC option select
	ADC10CTL0 = SREF_0 + ADC10SHT_3 + REFON + ADC10ON; // Use Vcc (around 3.3V) as reference

    
	
    if (CALBC1_16MHZ != 0xFF)	// Warning: calibration lost after mass erase
    {
		BCSCTL1 = CALBC1_16MHZ; // Set DCO
	  	DCOCTL  = CALDCO_16MHZ;
	}
	
    uart_init();
	// Handle interrupts
    TA0CCTL0 = CCIE; 			// CCR0 interrupt enabled
    TA0CCR0 = (CLK/ISRF);
    TA0CTL = TASSEL_2 + MC_2;  	// SMCLK, contmode
    _BIS_SR(GIE); 				// Enable interrupt

	// Give putty a chance to start
	delay_ms(500); // wait 500 ms
	
	// Print message using UART
	uart_puts("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.
    uart_puts("Servo signal generator for the MSP430.\r\n");
    uart_puts("By Jesus Calvino-Fraga (c) 2018.\r\n");
    uart_puts("Pulse width between 0 (for 0.6ms) and 180 degrees (for 2.4ms)\r\n");
    
    
    // Reset servos and turn off DC motors
		servoflag = 0;
	    ISR_pw0 = 60;
	    delay_ms(1000);
	    servoflag = 1;
	    ISR_pw1 = 60;
	    delay_ms(1000);
	    DCSTATE = 0;
	    magnetflag=0;
	
	// Forever loop/ Sequence of robot
	while(1)
	{   
		
	    
	    DCSTATE = 1;   //Default sequence robot will just go forward in direction placed
	    
	    ADC10CTL0 |= ENC + ADC10SC; 
	    v=(ADC10MEM*3290L)/1023L;
	    
	    magnetflag = 1;
	    delay_ms(2000);
	    magnetflag = 0;
	    delay_ms(2000);
	    if (v >= 3290) {
	     perimeter = 1;  //flag perimeter
	    }
	    
	   count=GetPeriod(100);
		if(count>0)
		{
			T=count/(CLK*100.0);
			f=1/T;
			// if f > certain number, flag the metal detector
		
		    //printf("metal detected!");
			uart_puts("f=");
			PrintNumber(f, 10, 6);
			uart_puts("Hz, count=");
			PrintNumber(count, 10, 6);
			uart_puts("\r");
			
		}
		else
		{
			uart_puts("NO SIGNAL                     \r");
		}
		waitms(200); 
	    
	    if (perimeter == 1){  //Sequence if Perimeter is detected
		    DCSTATE = 3;          //Make the robot turn 
		    turntime = rand()%1000; //This controls for how long the robot will turn if a perimenter is sensed between 0 and 1 second
		    delay_ms(turntime); //Time to turn will be random so the angle at which it turns is random
		    DCSTATE = 0;        //Stay in a standstill to ensure it will not cross perimeter
		    perimeter = 0;      //Turn off flag once done the sequence so it does not become an infinite loop
	    } 
	    
		if ( metaldetect == 1) {
			
			// Move backward for 1 second
			DCSTATE = 2;
			delay_ms(1000);
			
			// Rotate arm into ready position
			DCSTATE = 0; 
			servoflag = 0;
			ISR_pw0 = 180+60;
			delay_ms(1000);
			servoflag = 1;
			ISR_pw1 = 180+60;
			magnetflag = 1;			//////////////
			delay_ms(1000);
			
			// Sweep floor for coin
			servoflag = 0;
			ISR_pw0 = 35+60;
			delay_ms(1000);
			ISR_pw0 = 170+60;
			delay_ms(1000);
			
			// Drop coin into box
			servoflag = 1;
			ISR_pw1 = 0+60;
			delay_ms(1000);
			servoflag = 0;
			ISR_pw0 = 50+60;
			delay_ms(1000);
			servoflag = 1;
			ISR_pw1 = 60+60;
			delay_ms(1000);
			magnetflag = 0;			////////////
			delay_ms(1000);
			
			// Reset servos
			servoflag = 1;
			ISR_pw1 = 0+60;
			delay_ms(1000);
			ISR_pw0 = 0+60;
			delay_ms(1000);
			servoflag = 0;
			metaldetect = 0;      //Turn off the flag once the sequence is done so it doesn't become an infinite loop
		} 
	
		
	   uart_puts("Which servo: ");
    	uart_gets(buf, sizeof(buf)-1); // wait here until data is received
  
    	uart_puts("\n");
	    servoflag =atoi(buf);
	
	
    	uart_puts("New pulse width: ");
    	uart_gets(buf, sizeof(buf)-1); // wait here until data is received
  
    	uart_puts("\n");
	    pw=atoi(buf)+60;
	    if( (pw>=60) && (pw<=240) )
	    {
	        if (servoflag == 1) {
	    	ISR_pw1=pw; }
	    	else {
	    	ISR_pw0=pw; }
        }
        else
        {
        	PrintNumber(pw-60, 10, 1);
        	uart_puts(" is out of the valid range\r\n");
        } 
	}
}
