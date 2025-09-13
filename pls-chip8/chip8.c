#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

/* CONSTANTS */
#define WINDOW_MULTIPLIER 10
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define CHIP8_MEMORY_SIZE 0x1000
#define CHIP8_WINDOW_MULTIPLIER 10
#define CHIP8_NUM_DATA_REGISTERS 16
#define CHIP8_TOTAL_STACK_DEPTH 16
#define CHIP8_TOTAL_KEYS 16
#define CHIP8_CHARACTER_SET_LOAD_ADDRESS 0x00

/* My husband is a baker */
#define boule bool

/* aliases */
#define u8 char unsigned
#define i8 char signed
#define u16 short unsigned
#define i16 short signed

boule init_chip8(struct Chip8* chip8);
boule destroy_chip8(struct Chip8* chip8);
boule address_in_bounds(u16 address);
u8 peek(struct Chip8* chip8, u16 address);
boule poke(struct Chip8* chip8, u16 address, u8 value);
boule stack_out_of_bounds(struct Chip8* chip8);
boule push(struct Chip8* chip8, u16 value);
u16 pop(struct Chip8* chip8);

/* Structs */
struct Registers {
	u8 V[CHIP8_NUM_DATA_REGISTERS];
	u16 I;
	u8 DL, DS;
	u8 SP;
	u16 PC;
};

struct Chip8 {
	boule screen[SCREEN_HEIGHT][SCREEN_WIDTH];
	u8 memory[0x1000];
	u8 keyboard[0xf];
	struct Registers registers;
	u16 stack[CHIP8_TOTAL_STACK_DEPTH];
};

/* Init & Deallocate Machine Instance */
boule init_chip8(struct Chip8* chip8) {
	char unsigned character_set[] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

	chip8 = malloc(sizeof(struct Chip8));
	if (chip8) {
		memcpy(chip8->memory, character_set, sizeof(character_set));
		return true;
	}
	else {
		return false;
	}
}


boule destroy_chip8(struct Chip8* chip8) {
	free(chip8);
}

/* MEMORY */
boule address_in_bounds(u16 address) {
	return 0 <= address <= CHIP8_MEMORY_SIZE ? true : false;
}

u8 peek(struct Chip8* chip8, u16 address) {
	if (!address_in_bounds(address)) return NULL;
	return chip8->memory[address];
}

boule poke(struct Chip8* chip8, u16 address, u8 value) {
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

boule push(struct Chip8* chip8, u16 value) {
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

u16 pop(struct Chip8* chip8) {
	u8 value = NULL;
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