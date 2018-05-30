SHELL = cmd
CC = c51
OBJS = Main.obj

Main.hex: $(OBJS)
	$(CC) $(OBJS)
	@echo Complile complete

Main.obj: Main.c
	$(CC) -c Main.c

clean:
	@del &(OBJS) *.map *.hex *.map 2> nul

LoadFlash:
	EFM8_prog.exe -ft230 -r Main.hex

Dummy: Main.hex Main.map

explorer:
	cmd /c start explorer .