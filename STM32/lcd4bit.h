#include <stdio.h>
#include <stdlib.h>

#define CHARS_PER_LINE 16
#define TIM15_Freq 1000000L //1us

void Timerus(unsigned char us);
void waitms(unsigned int ms);
void LCD_pulse(void);
void LCD_byte (unsigned char x);
void WriteData(unsigned char x);
void WriteCommand(unsigned char c);
void lcd4bit_init(void);
void LCDprint(char * string, unsigned char line, char clear);