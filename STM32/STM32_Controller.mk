SHELL=cmd
CC=arm-none-eabi-gcc
AS=arm-none-eabi-as
LD=arm-none-eabi-ld

CCFLAGS=-mcpu=cortex-m0 -mthumb -g 

# Search for the path of the right libraries.  Works only on Windows.
GCCPATH=$(subst \bin\arm-none-eabi-gcc.exe,\,$(shell where $(CC)))
LIBPATH1=$(subst \libgcc.a,,$(shell dir /s /b "$(GCCPATH)*libgcc.a" | find "v6-m"))
LIBPATH2=$(subst \libc_nano.a,,$(shell dir /s /b "$(GCCPATH)*libc_nano.a" | find "v6-m"))
LIBSPEC=-L"$(LIBPATH1)" -L"$(LIBPATH2)"

OBJS=main.o serial.o lcd4bit.o startup.o newlib_stubs.o

PORTN=$(shell type COMPORT.inc)

main.elf : $(OBJS)
	$(LD) $(OBJS) $(LIBSPEC) -T stm32f05xxx.ld --cref -Map main.map -nostartfiles -o main.elf
	arm-none-eabi-objcopy -O ihex main.elf main.hex
	@echo Success!
	
main.o: main.c 
	$(CC) -c $(CCFLAGS) main.c -o main.o
	
serial.o: serial.c
	$(CC) -c $(CCFLAGS) serial.c -o serial.o
	
lcd4bit.o: lcd4bit.c
	$(CC) -c $(CCFLAGS) lcd4bit.c -o lcd4bit.o

startup.o: startup.c stm32f05xxx.h serial.h lcd4bit.h
	$(CC) -c $(CCFLAGS) startup.c -o startup.o

newlib_stubs.o: newlib_stubs.c
	$(CC) -c $(CCFLAGS) newlib_stubs.c -o newlib_stubs.o

clean: 
	@del $(OBJS) 2>NUL
	@del main.elf main.hex main.map 2>NUL
	@del *.lst 2>NUL
	
Flash_Load:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	STMFlashLoader -c -i STM32F0_5x_3x_64K -e --all -d --fn main.hex --v

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

dummy: stm32f05xxx.ld main.map
	@echo :-)
	
explorer:
	@explorer .