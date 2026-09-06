CC := gcc
CFLAGS := -g -O0 -std=c89 -Wall -lm -lSDL3
.PHONY: all run clean
all: run
run: main
	./main cartridge-slot/*.ch8
main: main.c chip8.c

chip8.o: chip8.c

clean:
	-$(RM) main
	-$(RM) *.o
