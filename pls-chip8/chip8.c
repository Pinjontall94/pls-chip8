#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

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

/* function prototypes */
boule init_chip8(struct Chip8* chip8);
boule destroy_chip8(struct Chip8* chip8);
static void assert_address_in_bounds(u16 address);
u8 peek(struct Chip8* chip8, u16 address);
boule poke(struct Chip8* chip8, u16 address, u8 value);
static void assert_stack_in_bounds(struct Chip8* chip8);
boule push(struct Chip8* chip8, u16 value);
u16 pop(struct Chip8* chip8);
static void assert_pixel_in_bounds(int x, int y);
boule get_pixel(boule** display, int x, int y);
void set_pixel(boule** display, int x, int y);

void square_oscillator(i16* buffer, int buffer_length, int long sample_rate, int pitch, float volume);

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

static char unsigned character_set[] = {
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

/* Init & Deallocate Machine Instance */
boule init_chip8(struct Chip8* chip8) {
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
static void assert_address_in_bounds(u16 address) {
	assert(address <= CHIP8_MEMORY_SIZE);
}

u8 peek(struct Chip8* chip8, u16 address) {
	assert_address_in_bounds(address);
	return chip8->memory[address];
}

boule poke(struct Chip8* chip8, u16 address, u8 value) {
	assert_address_in_bounds(address);
	chip8->memory[address] = value;
	return true;
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

static void assert_stack_in_bounds(struct Chip8* chip8) {
	/* unsigned, so no need to check for negatives */
	/* just need to make sure no greater than 0xf or 15 */
	assert(chip8->registers.SP < CHIP8_TOTAL_STACK_DEPTH);
}

boule push(struct Chip8* chip8, u16 value) {
	/* Check the stack pointer bounds before operation to
	 * see if it's 0x10 <-> 0xff (invalid). Then, push to
	 * the current SP, and increment to point to new FREE stack slot
	 */
	assert_stack_in_bounds(chip8->registers.SP);
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
	assert_stack_in_bounds(chip8->registers.SP);
	value = chip8->stack[chip8->registers.SP];
	return value;
}

/* Display */
static void assert_pixel_in_bounds(int x, int y) {
	assert(x >= 0 && x < SCREEN_WIDTH && y >= 0 && SCREEN_HEIGHT);
}

boule get_pixel(bool** screen, int x, int y) {
	assert_pixel_in_bounds(x, y);
	return screen[y][x];
}

void set_pixel(bool** screen, int x, int y) {
	assert_pixel_in_bounds(x, y);
	screen[y][x] = true;
}

void square_oscillator(i16* buffer, int buffer_length, int long sample_rate, int pitch, float volume)
{
	// Make sure freq is below nyquist and volume isn't too loud [WARNING: DO NOT USE HEADPHONES]
	int i;
	i16 MAX, value, final_value;
	float delta, phase;
	
	MAX = floor(65535.0 / 2.0);
	delta = (float)pitch / sample_rate;
	phase = 0.00;
	value = 0;
	final_value = 0;

	assert(pitch < (sample_rate / 2) && volume > 0.00 && volume < 0.1);
	for (i = 0; i < buffer_length; i++)
	{
		if (phase < 0.5) value = MAX;
		else value = -1 * MAX;

		final_value = (i16)(value * volume);
		phase += delta; // heart of the oscillator: linearly track delta as phase increases
		if (phase >= 1)
			phase -= 1;
		buffer[i] = final_value;
	}
}
