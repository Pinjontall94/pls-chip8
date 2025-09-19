CC := zig cc 
CFLAGS := -std=c89 -Wall -Werror -lm -lSDL3
.PHONY: all run clean
all: run
run: main
	./main
main: main.c

chip8.o: chip8.c

clean:
	-$(RM) main
	-$(RM) *.o
