#include "stm32f05xxx.h"
#include <stdio.h>
#include <stdlib.h>
#include "serial.h"
#include "lcd4bit.h"

#define SYS_CLK 48000000L
#define TIM2_Freq 1000L  //1ms
#define TIM3_Freq 60L //0.1ms

volatile unsigned char mode = 0;
unsigned char error_flag = 0;
unsigned char error_bus = 0;

volatile unsigned char seconds_flag = 0;

volatile unsigned int ms_count = 0;
volatile unsigned char s_count = 0;
volatile unsigned char m_count = 30;
volatile unsigned char h_count = 2;
volatile unsigned char state = 2;

volatile unsigned char tim3_count = 0;

unsigned char output_state0 = 0;  //Initialize all off
unsigned char output_state1 = 0;  //Initialize all off

void init_timer2(void);
//void isr_timer2(void); //declared in startup.c
void init_timer3();
//void isr_timer3(void); //declared in startup.c
void init_IO(void);
void updateMode(void);
void update_IO(void);

void _update_seconds(void);
void _update_minutes(void);
void _update_hours(void);

void LED_state_update(void);

void clearbuffer(char* buffer);

void command(char* buff);
void _print_time(void);
void _set_time(void);

void init_timer2() {
	printf("Init timer2\r\n");
	disable_interrupts();
	
	//Init Pin13(PA7)
	GPIOA_MODER |= BIT14; //Enable output from PA7 Pin13
	GPIOA_OSPEEDR |= (BIT14 | BIT15); //High Speed
	
	RCC_AHBENR |= 0x00020000; //peripheral clock enable for port A
	RCC_APB1ENR |= BIT0; //Enable clock on timer2
	
	ISER |= BIT15;  //Enable TIM2 interrupt
	
	//TIM2 Registers
	TIM2_CR1 |= ( BIT0 | BIT4 ); //Down count and TIM2_ARR is buffered
	TIM2_ARR = (SYS_CLK / TIM2_Freq);
	TIM2_DIER |= BIT0; //Enable Update interrupt
	
	enable_interrupts();
	printf("timer2 done.\r\n");
}

void isr_timer2() {
	TIM2_SR &= ~(BIT0); //Clear Update Interrupt flag 
	
	ms_count++;
	if (ms_count == 1000) {
		ms_count = 0;
		//seconds_flag = 1;
		LED_state_update();
		_update_seconds();
	}	
}

void init_timer3() {
	printf("Init timer3\r\n");
	disable_interrupts();
	
	RCC_AHBENR |= 0x00020000; //peripheral clock enable for port A
	RCC_APB1ENR |= BIT1; //Enable clock on timer3
	
	ISER |= BIT16;
	
	TIM3_CR1 |= ( BIT0 | BIT4); //Down count and TIM3_ARR is buffered
	TIM3_ARR = (SYS_CLK / (TIM3_Freq*100));
	//TIM3_DIER |= BIT0; //Enable Update interrupt
	
	enable_interrupts();
	printf("timer2 done.\r\n");
}

void isr_timer3() {
	TIM3_SR &= ~(BIT0);
	
	if (tim3_count == 50) {
		GPIOA_ODR &= ~BIT6;
	} else if (tim3_count == 100) {
		GPIOA_ODR |= BIT6;
		tim3_count = 0;
	} 
	tim3_count++;
	
}

void init_IO() {
	//Enable Pin6(PA0), Pin7(PA1), Pin8(PA2), Pin9(PA3) as outputs;
	GPIOA_MODER |= (BIT0 | BIT2 | BIT4 | BIT6 | BIT12);
	GPIOA_OTYPER |= (BIT12 | BIT13);
	GPIOA_ODR &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT6); //Set outputs to 0
	
	//Enable Pin21(PA11), Pin22(PA12), Pin23(PA13) as pull-up(active low) inputs;
	GPIOA_MODER &= ~(BIT22 | BIT23 | BIT24 | BIT25| BIT26 | BIT27 );
	GPIOA_PUPDR &= ~(BIT23 | BIT25 | BIT27 );
	GPIOA_PUPDR |= (BIT22 | BIT24 | BIT26 );
}

void updateMode() {
	if (!(GPIOA_IDR & BIT11)) {
		printf("0\r\n");
		while(!(GPIOA_IDR & BIT11));
		mode = 1;
		printf("External Communication Mode\r\n");
	} else {
		mode = 0;
	}
}

void update_IO() {
	unsigned char flag0 = 1;
	unsigned char flag1 = 1;
	
	//Check input pins for port0
	if (1) {
		
	} else {
		if (output_state0 == 1) {
			flag0 = 0;
		}
		output_state0 = 1;
	}
	
	//Check input pins for port1
	if (!(GPIOA_IDR & BIT13)) {
		if (output_state1 == 0) {
			flag1 = 0;
		}
		output_state1 = 0;
	} else if (!(GPIOA_IDR & BIT12)) {
		if (output_state1 == 2) {
			flag1 = 0;
		}
		output_state1 = 2;
	} else {
		if (output_state1 == 1) {
			flag1 = 0;
		}
		output_state1 = 1;
	}
	
	//Set output pins for port0
	if (flag0 == 1) {
		switch(output_state0) {
			case 0: GPIOA_ODR &= ~(BIT0 | BIT1); //Both outputs off
					break;
			case 1: GPIOA_ODR &= ~(BIT0 | BIT1); //PA0 on
					GPIOA_ODR |= (BIT0);
					break;
			case 2: GPIOA_ODR &= ~(BIT0 | BIT1); //PA0 and PA1 on
					GPIOA_ODR |= (BIT0 | BIT1);
					break;
			default: output_state0 = 0;
					error_flag = 1;
					error_bus |= 1<<1;
					
		}
	}
	
	//Set output pins for port0
	if (flag1 == 1) {
		switch(output_state1) {
			case 0: GPIOA_ODR &= ~(BIT2 | BIT3); //Both outputs off
					break;
			case 1: GPIOA_ODR &= ~(BIT2 | BIT3); //PA2 on
					GPIOA_ODR |= (BIT2);
					break;
			case 2: GPIOA_ODR &= ~(BIT2 | BIT3); //PA2 and PA3 on
					GPIOA_ODR |= (BIT2 | BIT3);
					break;
			default: output_state1 = 0;
					error_flag = 1;
					error_bus |= 1<<2;
					
		}
	}
}

void _update_seconds() {
	s_count++;
	if (s_count == 60) {
		s_count = 0;
		_update_minutes();
	}
}

void _update_minutes() {
	m_count++;
	if (m_count == 60) {
		m_count = 0;
		_update_hours();
	}
}

void _update_hours() {
	h_count++;
	if (h_count == 24) {
		h_count = 0;
	}
}

void LED_state_update() {
	if (state == 0) {
		GPIOA_ODR |= BIT7;
		state = 1;
	} else {
		GPIOA_ODR &= ~BIT7;
		state = 0;
	}
}

int main(void) {
	char buffer[16];

    printf("INITIALIZING\r\n");
    init_IO();
    init_timer2();
    init_timer3();

	TIM2_CNT = 0;
	TIM3_CNT = 0;
	
	lcd4bit_init();
	LCDprint("ready",1,0);
	
    while(1) {
    	updateMode();
    	
    	//Check for errors
    	if (error_flag == 1) {
    		mode = 2;
    		TIM3_DIER &= ~BIT0; //Enable Update interrupt
    	}
    	
    	//Enter mode state
    	if (mode == 0) { //Regular IO mode
			update_IO();
		} else if (mode == 1) { //Command interface mode
			printf("Enter text: \r\n");
			
			egets(buffer, 16);
			printf("\x1b[0K");
			printf(">%s\r\n", buffer);
			command(buffer);
			clearbuffer(buffer);
		} else if (mode == 2) { //Error state
			printf("error\r\n");
		}
		
    }
}

void clearbuffer(char* buffer) {
	signed char i = 0;
	for (i = 0; i < 16; i++) {
		buffer[i] = '\0';
	}
}

void command(char* buff) {
	if ( (buff[0] == 't') && (buff[1] == 'i') && (buff[2] == 'm') && (buff[3] == 'e') ) {
		_print_time();
	} else if ( (buff[0] == 's') && (buff[1] == 't') ) {
		_set_time();
	} else {
		printf("Invalid Command\r\n");
	}
}

void _print_time(void) {
	printf("[hh:mm:ss:ms] = [%u:%u:%u:%d]\r\n", h_count, m_count, s_count, ms_count);
}

void _set_time(void) {
	char time_buf[5];
	unsigned int var;
	printf("Set hours: \r\n");
	egets(time_buf, 5);
	printf("\x1b[0K");
	printf(">%s\r\n", time_buf);
	sscanf(time_buf, "%d", &var);
	h_count = (char)var;
	if (h_count >= 24) h_count = 0;
	
	printf("\x1b[0K");
	printf("Set minutes: \r\n");
	egets(time_buf, 5);
	printf("\x1b[0K");
	printf(">%s\r\n", time_buf);
	sscanf(time_buf, "%d", &var);
	m_count = (char)var;
	if (m_count >= 60) m_count = 0;
	
	printf("\x1b[0K");
	printf("\r\nSet seconds: \r\n");
	egets(time_buf, 5);
	printf("\x1b[0K");
	printf(">%s\r\n", time_buf);
	sscanf(time_buf, "%d", &var);
	s_count = (char)var;
	if (s_count >= 60) s_count = 0;
}