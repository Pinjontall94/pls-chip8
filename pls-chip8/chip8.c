#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

/* CONSTANTS */
#define WINDOW_MULTIPLIER 10
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define CHIP8_MEMORY_SIZE 0x1000
#define CHIP8_WINDOW_MULTIPLIER 10
#define CHIP8_NUM_DATA_REGISTERS 0xf
#define CHIP8_TOTAL_STACK_DEPTH 0xf
#define CHIP8_TOTAL_KEYS 0xf
#define CHIP8_CHARACTER_SET_LOAD_ADDRESS 0x00

/* My husband is a baker */
#define boule bool

boule init_chip8(struct Chip8* chip8);
boule destroy_chip8(struct Chip8* chip8);
boule address_in_bounds(short unsigned address);
char unsigned peek(struct Chip8* chip8, short unsigned address);
boule poke(struct Chip8* chip8, short unsigned address, char unsigned value);
boule stack_out_of_bounds(struct Chip8* chip8);
boule push(struct Chip8* chip8, short unsigned value);
short unsigned pop(struct Chip8* chip8);

/* Structs */
struct Registers {
	char unsigned V[CHIP8_NUM_DATA_REGISTERS];
	short unsigned I;
	char unsigned DL, DS;
	char unsigned SP;
	short unsigned PC;
};

struct Chip8 {
	bool screen[SCREEN_WIDTH][SCREEN_HEIGHT];
	char unsigned memory[0x1000];
	char unsigned keyboard[0xf];
	struct Registers registers;
	short unsigned stack[CHIP8_TOTAL_STACK_DEPTH];
};

/* Init & Deallocate Machine Instance */
boule init_chip8(struct Chip8* chip8) {
	chip8 = malloc(sizeof(struct Chip8));
	return chip8 == NULL ? false : true;
}

boule destroy_chip8(struct Chip8* chip8) {
	free(chip8);
}

/* MEMORY */
boule address_in_bounds(short unsigned address) {
	return 0 <= address <= CHIP8_MEMORY_SIZE ? true : false;
}

char unsigned peek(struct Chip8* chip8, short unsigned address) {
	if (!address_in_bounds(address)) return NULL;
	return chip8->memory[address];
}

boule poke(struct Chip8* chip8, short unsigned address, char unsigned value) {
	if (!address_in_bounds(address)) {
		return false;
	}
	else {
		chip8->memory[address] = value;
		return true;
	}
}

/* STACK
* Fundamentally, the confusing thing about these stack operations w/r/t bounds-
* checking is that the SP doesn't point to capacity, nor the last pushed value,
* but rather it points to the NEXT FREE INDEX in the stack. ergo:
*
*   SP == 0:
*     push OKAY (push then SP++)
*     pop  NO
*
*   SP == 0x0f:
*     push NO
*     pop  OKAY (SP-- then pop)
*/

boule stack_out_of_bounds(struct Chip8* chip8) {
	/* unsigned, so no need to check for negatives */
	/* just need to make sure no greater than 0xf or 15 */
	return chip8->registers.SP >= CHIP8_TOTAL_STACK_DEPTH ? true : false;
}

boule push(struct Chip8* chip8, short unsigned value) {
	/* Check the stack pointer bounds before operation to
	 * see if it's 0x10 <-> 0xff (invalid). Then, push to
	 * the current SP, and increment to point to new FREE stack slot
	 */
	if (stack_out_of_bounds(chip8->registers.SP)) {
		fprintf(stderr, "stack overflow!");
		return false;
	}
	chip8->stack[chip8->registers.SP] = value;
	chip8->registers.SP++;
	return true;
}

short unsigned pop(struct Chip8* chip8) {
	char unsigned value = NULL;
	/* Decrement SP before pulling value,
	 * bc it's the index of the next FREE stack slot, NOT the amount stored
	 *
	 *     i.e. SP-- on SP == 1 is fine
	 *          SP-- on SP == 0 is NOT
	 */
	chip8->registers.SP--;
	if (stack_out_of_bounds(chip8)) {
		fprintf(stderr, "stack underflow!");
		return NULL;
	}
	value = chip8->stack[chip8->registers.SP];
	return value;
}