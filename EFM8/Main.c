#include <EFM8LB1.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef SYSCLK
#define SYSCLK 72000000L
#endif

#ifndef BAUDRATE
#define BAUDRATE 115200L
#endif

#define SWITCH_1_1     P1_0
#define SWITCH_1_2     P1_1

#define GAUGE_OUTPUT_1 P2_0
#define GAUGE_OUTPUT_2 P2_1

#define BLUE_LED       P2_2

volatile unsigned int milliseconds = 0;
volatile unsigned char seconds = 0;
volatile unsigned char minutes = 0;
volatile unsigned char hours = 0;

unsigned char gauge_state = 0;

void Timer1_init(void);
void Timer2_init(void);
void UART0_init(void);
void Pin_init(void);
void getIO(void);
void gauge_update(void);

char _c51_external_startup (void)
{
	// Disable Watchdog with key sequence
	SFRPAGE = 0x00;
	WDTCN = 0xDE; //First key
	WDTCN = 0xAD; //Second key

	SFRPAGE = 0x10;
	PFE0CN  = 0x20; // SYSCLK < 75 MHz.
		
	// Transition to 24.5 MHz first
	SFRPAGE = 0x00;
	CLKSEL  = 0x00;
	CLKSEL  = 0x00;
	while ((CLKSEL & 0x80) == 0);
	
	// Now switch to 72 MHz
	CLKSEL = 0x03;
	CLKSEL = 0x03;
	while ((CLKSEL & 0x80) == 0);

	XBR0 = 0X00;
	XBR1 = 0X00;
	XBR2 = 0x40; // Enable crossbar and weak pull-ups
	
	return 0;
}

void UART0_init(void) {
    SCON0 = 0x10;
    
	Timer1_init();

    P0MDOUT |= 0x10; // Enable UART0 TX as push-pull output and UART1 Tx (pin 0.2)
    XBR0    |= 0b_0000_0001; // Enable UART0 on P0.4(TX) and P0.5(RX) and SMB0 I/O on (0.0 SDA) and (0.1 SCL)
    XBR1     = 0x00;
    XBR2     = 0x40; // Enable crossbar and weak pull-ups

}

void Timer1_init(void) {
	CKCON0 |= 0b_0000_0000; // Timer 1 uses the system clock divided by 12.
	TH1 = 0x100-((SYSCLK/BAUDRATE)/(2L*12L));
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit auto-reload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready
}

void Timer2_init(void) {
    CKCON0 &= 0xF7; //Timer2 set to use T2XCLK

    TMR2CN0 = 0b_0000_0000; // T2XCLK = (SYSCLK/12)
    TMR2RL = 65536 - (SYSCLK/12)/1000; // F = 1000hz, T = 1ms
    TMR2 = 0xffff;
    ET2 = 1; //Enable Timer2 interrupts
    TR2 = 1; //Start Timer2
}

void Timer2_ISR(void) interrupt 5 {
    TF2H = 0;

    milliseconds++;
    
    if (milliseconds == 1000) {
        milliseconds = 0;
        seconds++;
        BLUE_LED = !BLUE_LED; //Changes Pin2.2 every 1 second
        if (seconds == 60) {
            seconds = 0;
            minutes++;
            if (minutes == 60) {
                minutes = 0;
                hours++;
                if (hours == 24) {
                    hours = 0;
                } 
            }
        }
    }
}

void Pin_init(void) {
    PRTDRV = 0x04; //Port 2 High Drive Strength
    P1MDOUT = 0b_0000_0011; //P1.0, P1.1 set out push_pull
    P2MDOUT = 0b_0000_0111; //P2.0, P2.1, P2.2 set as push_pull output

    GAUGE_OUTPUT_1 = 0;
    GAUGE_OUTPUT_2 = 0;
    BLUE_LED = 0;
}
 
void getIO(void) {
    if (SWITCH_1_1 == 0) {
        gauge_state = 0;
    } else if (SWITCH_1_2 == 0) {
        gauge_state = 2;
    } else {
        gauge_state = 1;
    }
}

void gauge_update(void) {
    switch(gauge_state) {
        case 0:
            GAUGE_OUTPUT_1 = 0; // Both outputs off to turn the gauges off.
            GAUGE_OUTPUT_2 = 0; // Switch Up
            break;
        case 1:
        	GAUGE_OUTPUT_1 = 1; // Turn on base color output (white)
            GAUGE_OUTPUT_2 = 0; // Switch Middle
            break;
        case 2:
            GAUGE_OUTPUT_1 = 1; // Turn on auxillary color output (blue)
            GAUGE_OUTPUT_2 = 1; // Switch Bottom
            break;
        default:
            gauge_state = 0;
    }
}

void main (void) {
    Pin_init();     //Initialize pins for input and output
    Timer2_init();  //Initialize Timer2 for keeping time at a resolution of 1ms
    UART0_init();   //Initialize UART0 for serial communication

    EA = 1; //Enable interrupts

    printf("Device Done Initializing\r\n");
    
    while(1) {
        getIO();
        gauge_update();
    }
}