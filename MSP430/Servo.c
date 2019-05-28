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
#define MAGNET1  BIT6	// Electro magnet at P1.6
#define PWMSPEED 1500	// Max DC motor speed: 2000
#define DCRIGHT2 BIT4	// Right wheel backwards at P2.4
#define DCRIGHT1 BIT3	// Right wheel forwards at P2.3
#define DCLEFT2 BIT5	// Left wheel backwards at P2.5
#define DCLEFT1 BIT0	// Left wheel forwards at P2.0
#define SERVO1 BIT4		// Top servo at P1.4
#define SERVO0 BIT0 	// Bottom servo at P1.0
#define RXD BIT1 		// Receive Data (RXD) at P1.1
#define TXD BIT2 		// Transmit Data (TXD) at P1.2
#define CLK 16000000L	// CLK rate
#define ISRF 100000L 	// Interrupt Service Routine Frequency for a 10 us interrupt rate
#define BAUD 115200		// Baud rate
#define CCR_1MS_RELOAD (16000000L/1000L)

/* Declare global variables */
volatile int ISR_pw1=0, ISR_pw0=0, ISR_cnt=0, ISR_frc, servoflag, DCSTATE, magnetflag = 0; 
volatile long int count, frequency, v;
volatile float T, f, lastf;
volatile int metaldetect=0, flag, alternate, delaya, time, ran, choice, perimeter, turntime, counter, rpiflag;

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
   	}
   	
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


//Frequency for Raspberry Pi

#define PIN_PERIODb (P2IN & BIT2)

// GetPeriod() seems to work fine for frequencies between 30Hz and 300kHz.
long int GetPeriodb (int n)
{
	int i, overflow;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
	overflow=0;
	TA0CTL&=0xfffe; // Clear the overflow flag
	while (PIN_PERIODb!=0) // Wait for square wave to be 0
	{
		if(TA0CTL&1) { TA0CTL&=0xfffe; overflow++; if(overflow>5) return 0;}
	}
	overflow=0;
	TA0CTL&=0xfffe; // Clear the overflow flag
	while (PIN_PERIODb==0) // Wait for square wave to be 1
	{
		if(TA0CTL&1) { TA0CTL&=0xfffe; overflow++; if(overflow>5) return 0;}
	}
	
	overflow=0;
	TA0CTL&=0xfffe; // Clear the overflow flag
	saved_TCNT1a=TA0R;
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIODb!=0) // Wait for square wave to be 0
		{
			if(TA0CTL&1) { TA0CTL&=0xfffe; overflow++; if(overflow>1024) return 0;}
		}
		while (PIN_PERIODb==0) // Wait for square wave to be 1
		{
			if(TA0CTL&1) { TA0CTL&=0xfffe; overflow++; if(overflow>1024) return 0;}
		}
	}
	saved_TCNT1b=TA0R;
	if(saved_TCNT1b<saved_TCNT1a) overflow--; // Added an extra overflow.  Get rid of it.

	return overflow*0x10000L+(saved_TCNT1b-saved_TCNT1a);
}


void detect(){
	// Move backward for 1 second
			DCSTATE = 2;
			delay_ms(260);
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
			ISR_pw0 = 40+60;
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
			metaldetect = 0;     //Turn off the flag once the sequence is done so it doesn't become an infinite loop
			count=GetPeriod(100);     
	        T=count/(CLK*100.0);
	        f=1/T;
	        delay_ms(200);
	        
}

void per(){
//Sequence if Perimeter is detected
	        uart_puts("Executing");
	        DCSTATE = 2;
	        delay_ms(750);
		    DCSTATE = 3;
		     //Make the robot turn 
		     //This controls for how long the robot will turn if a perimenter is sensed between 0 and 1 second
		    turntime = rand()%500 + 600;
		    delay_ms(turntime); //Time to turn will be random so the angle at which it turns is random
		    
		    DCSTATE = 0;        //Stay in a standstill to ensure it will not cross perimeter 
		    delay_ms(300); 
		    ADC10CTL0 |= ENC + ADC10SC; 
		    v=(ADC10MEM*3290L)/1023L;           
		    //Turn off flag once done the sequence so it does not become an infinite loop 
		    }
		    
		    
void read() {
count=GetPeriod(1000);     
	   T=count/(CLK*100.0);
	   f=1/T;  
	   uart_puts("f=");
		 PrintNumber((f), 10, 6);
			uart_puts("Hz, count=");
			PrintNumber(count, 10, 6);
			uart_puts("\r"); 
         if ( f  > 5875 && f < 10000) {
		    //uart_puts("metal detected"); 
		    metaldetect = 1; }
if (metaldetect == 1){
detect(); 
counter++;
}
		                                
}		   



void backout(){
ADC10CTL0 |= ENC + ADC10SC; 
	    v=(ADC10MEM*3290L)/1023L; 
	    delay_ms(100);
	    if (v > 1000) {
	       per();
	    } 
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
	
	uart_puts("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.
    uart_puts("Servo signal generator for the MSP430.\r\n");
    uart_puts("By Jesus Calvino-Fraga (c) 2018.\r\n");
    uart_puts("Pulse width between 60 (for 0.6ms) and 240 (for 2.4ms)\r\n");


    
    // Reset servos and turn off DC motors
        f = 0;
        metaldetect = 0;
		servoflag = 0;
	    ISR_pw0 = 60;
	    delay_ms(1000);
	    servoflag = 1;
	    ISR_pw1 = 60;
	    delay_ms(1000);
	    DCSTATE = 2;
	    magnetflag=0;
	    count = 0;
	    rpiflag = 0;
	// Forever loop/ Sequence of robot
	
	if (rpiflag != 1){
	
	while(1)
	{   
	
	if (counter < 20){
	   f = 0;
       DCSTATE = 1;
	   read(); 
          
	    
	   backout(); }
	   
	  else{
	  DCSTATE = 0; 
	  }
	
	} }
	
	else { //code for raspberry pi
	while(1) {
	count=GetPeriodb(20);     
	T=count/(CLK*100.0);
	f=1/T;
	uart_puts("f=");
    PrintNumber((f), 10, 6);
	uart_puts("Hz, count=");
	PrintNumber(count, 10, 6);
	uart_puts("\r");   
	if (f >  650 && f < 750) {
	DCSTATE = 0; }  
	
	if (f > 270 && f < 330) {
	DCSTATE = 1; } 
	
	if (f > 370 && f < 430) {
	DCSTATE = 2; } 
	
	if (f > 570 && f < 630) {
	DCSTATE = 3; } 
	
	if (f > 470 && f < 530) {
	DCSTATE = 4; }
	
	/*if (f > 90 && f < xxx) {
	detect(); } */
	   
	
	
	}
	}
	
}
