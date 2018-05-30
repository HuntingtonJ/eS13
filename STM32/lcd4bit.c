#include  "stm32f05xxx.h"
#include "lcd4bit.h"

#define SYS_CLK 48000000L

/*

*/

void Timer15us(unsigned char us) {
	unsigned char i;
	
	RCC_APB2ENR |= BIT16; //Enable clock on timer15
	
	TIM15_CR1 |= BIT2; //Only counter overflow generates update interrupt 
	TIM15_ARR = (SYS_CLK / TIM15_Freq); //Set for 1us
	
	TIM15_SR &= ~(BIT0); //clear update interrupt flag
	TIM15_EGR |= BIT0;  //Renitialize timer with 0 by setting UG
	ISER |= BIT20;
	TIM15_CR1 |= BIT0;
	
	//printf("0.1\r\n");
	
	for (i = 0; i < us; i++) {
		while( !(TIM15_SR & BIT0));
		TIM15_SR &= ~(BIT0); //clear update interrupt flag
	}
	
	//printf("0.2\r\n");
	
	TIM15_CR1 &= ~BIT0;
}

void waitms(unsigned int ms) {
	//printf("0\r\n");
	unsigned int i;
	for (i = 0; i < ms; i++) {
		//1ms
		Timer15us(249);
		Timer15us(249);
		Timer15us(249);
		Timer15us(249);
	}
	//printf("1\r\n");
}

void LCD_pulse() {
	GPIOB_ODR |= BIT3; //Enable E(PB3 or Pin26)
	waitms(40);
	GPIOB_ODR &= ~(BIT3);//Disable E
}

//D7: PB7 = Pin 30
//D6: PB6 = Pin 29
//D5: PB5 = Pin 28
//D4: PB4 = Pin 27

void LCD_byte(unsigned char x) {
	//Write High nibble first
	if (x & 0b10000000 > 0) {
		GPIOB_ODR |= BIT7;
	} else {
		GPIOB_ODR &= ~BIT7;
	}
	
	if (x & 0b01000000 > 0) {
		GPIOB_ODR |= BIT6;
	} else {
		GPIOB_ODR &= ~BIT6;
	}
	
	if (x & 0b00100000 > 0) {
		GPIOB_ODR |= BIT5;
	} else {
		GPIOB_ODR &= ~BIT5;
	}
	
	if (x & 0b00010000 > 0) {
		GPIOB_ODR |= BIT4;
	} else {
		GPIOB_ODR &= ~BIT4;
	}
	
	LCD_pulse();
	Timer15us(100);
	
	//Write Low nibble last
	if (x & 0b00001000 > 0) {
		GPIOB_ODR |= BIT7;
	} else {
		GPIOB_ODR &= ~BIT7;
	}
	
	if (x & 0b00000100 > 0) {
		GPIOB_ODR |= BIT6;
	} else {
		GPIOB_ODR &= ~BIT6;
	}
	
	if (x & 0b00000010 > 0) {
		GPIOB_ODR |= BIT5;
	} else {
		GPIOB_ODR &= ~BIT5;
	}
	
	if (x & 0b00000001 > 0) {
		GPIOB_ODR |= BIT4;
	} else {
		GPIOB_ODR &= ~BIT4;
	}
	LCD_pulse();
	Timer15us(40);
}

void WriteData(unsigned char x) {
	GPIOA_ODR |= BIT14; //Set RS or PA14 or Pin24
	LCD_byte(x);
	waitms(2);
}

void WriteCommand(unsigned char c) {
	GPIOA_ODR &= ~BIT14; //Clear RS
	LCD_byte(c);
	waitms(5);
}



void lcd4bit_init() {
	printf("LCD initializing\r\n");
	//Initialize pins
	
	//Give port A and B clock
	RCC_AHBENR |= ( BIT17 | BIT18 );
	
	//Enable Pins 24-30 as LCD outputs
	//PA14(RS), PA15(RW)
	GPIOA_MODER |= ( BIT28 | BIT30 ); 
	GPIOA_MODER &= ~( BIT29 | BIT31 ); //Disable SWCLK on PA14(Pin24)
	GPIOA_OTYPER &= ~(BIT14); 
	
	//PB3(E), PB4(D4), PB5(D5), PB6(D6), PB7(D7)
	GPIOB_MODER |= ( BIT3 | BIT4 | BIT5 | BIT6 | BIT7 );
	
	GPIOB_ODR &= ~( BIT3 | BIT4 | BIT5 | BIT6 | BIT7 );  //Initialize E as 0
	GPIOA_ODR &= ~( BIT14 | BIT15 ); // RW is 0 for write only
	
	waitms(20);
	
	//Set mode to 4-bit
	WriteCommand(0x33); //4bit mode, 2 lines 5x8 dots
	WriteCommand(0x32); //
	
	WriteCommand(0x28); //function set
	WriteCommand(0x08); //Display on/off control
	WriteCommand(0x01); //clear display
	
	WriteCommand(0x0c); //turn display on
	
	//GPIOA_ODR &= ~BIT14; //Clear RS
}

void LCDprint(char * string, unsigned char line, char clear) {
	int j;

	WriteCommand(line==2?0xc0:0x80);
	waitms(5);
	for(j=0; string[j]!=0; j++)	WriteData(string[j]);// Write the message
	if(clear == 1) for(; j<CHARS_PER_LINE; j++) WriteData(' '); // Clear the rest of the line
}