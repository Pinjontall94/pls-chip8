#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <math.h>

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

/* Structs, Enums */
struct Registers {
	u8 V[CHIP8_NUM_DATA_REGISTERS];
	u16 I;
	u8 DL, DS;
	u8 SP;
	u16 PC;
};

typedef struct Chip8 {
	boule screen[SCREEN_HEIGHT][SCREEN_WIDTH];
	u8 memory[CHIP8_MEMORY_SIZE];
	boule keyboard[CHIP8_TOTAL_KEYS];
	struct Registers registers;
	u16 stack[CHIP8_TOTAL_STACK_DEPTH];
} Chip8;

union Instruction {
	struct {
		u8 hi_byte;
		u8 lo_byte;
	} bytes;
	u16 word;
};

enum ScanCode {
	KEY_1 = 2, KEY_2 = 3, KEY_3 = 4, KEY_4 = 5,
	KEY_q = 16, KEY_w = 17, KEY_e = 18, KEY_r = 19,
	KEY_a = 30, KEY_s = 31, KEY_d = 32, KEY_f = 33,
	KEY_z = 44, KEY_x = 45, KEY_c = 46, KEY_v = 47
};

static u8 character_set[] = {
0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
};

/****************************************************************************** 
 * Function Prototypes 
 *****************************************************************************/
boule init_chip8(Chip8* chip8);
void destroy_chip8(Chip8* chip8);
static void assert_address_in_bounds(u16 address);
u8 peek(Chip8* chip8, u16 address);
boule poke(Chip8* chip8, u16 address, u8 value);
static void assert_stack_in_bounds(u16 SP);
boule push(Chip8* chip8, u16 value);
u16 pop(Chip8* chip8);
static void assert_pixel_in_bounds(int x, int y);
boule get_pixel(boule** display, int x, int y);
void set_pixel(boule** display, int x, int y);
static void assert_key_in_bounds(u8 key);
void key_up(boule* keyboard, u8 key);
void key_down(boule* keyboard, u8 key);

/* Emulation cycle */
void fetch(Chip8* chip8, union Instruction* instruction);
void decode_and_execute(Chip8* chip8, union Instruction* instruction);

/* external hardware prototypes */
i8 keyboard_code_to_chip8(enum ScanCode kbd_code);
void square_oscillator(i16* buffer, int buffer_length, int long sample_rate, int pitch, float volume);


/****************************************************************************** 
* IMPLEMENTATION
******************************************************************************/

/* Init & Deallocate Machine Instance */
boule init_chip8(Chip8* chip8) {
	chip8 = malloc(sizeof(Chip8));
	if (chip8) {
		memcpy(chip8->memory, character_set, sizeof(character_set));
		return true;
	}
	else {
		return false;
	}
}


void destroy_chip8(Chip8* chip8) {
	free(chip8);
}

/* MEMORY */
static void assert_address_in_bounds(u16 address) {
	assert(address <= CHIP8_MEMORY_SIZE);
}

u8 peek(Chip8* chip8, u16 address) {
	assert_address_in_bounds(address);
	return chip8->memory[address];
}

boule poke(Chip8* chip8, u16 address, u8 value) {
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

static void assert_stack_in_bounds(u16 SP) {
	/* unsigned, so no need to check for negatives */
	/* just need to make sure no greater than 0xf or 15 */
	assert(SP < CHIP8_TOTAL_STACK_DEPTH);
}

boule push(Chip8* chip8, u16 value) {
	/* Check the stack pointer bounds before operation to
	 * see if it's 0x10 <-> 0xff (invalid). Then, push to
	 * the current SP, and increment to point to new FREE stack slot
	 */
	assert_stack_in_bounds(chip8->registers.SP);
	chip8->stack[chip8->registers.SP] = value;
	chip8->registers.SP++;
	return true;
}

u16 pop(Chip8* chip8) {
	u8 value = 0;
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

/* DISPLAY */
static void assert_pixel_in_bounds(int x, int y) {
	assert(x >= 0 && x < SCREEN_WIDTH && y >= 0 && SCREEN_HEIGHT);
}

boule get_pixel(boule** screen, int x, int y) {
	assert_pixel_in_bounds(x, y);
	return screen[y][x];
}

void set_pixel(boule** screen, int x, int y) {
	assert_pixel_in_bounds(x, y);
	screen[y][x] = true;
}

/* KEYBOARD */
static void assert_key_in_bounds(u8 key) {
	assert(key >= 0 && key < CHIP8_TOTAL_KEYS);
}

void key_up(boule* keyboard, u8 key) {
	assert_key_in_bounds(key);
	keyboard[key] = false;
}

void key_down(boule* keyboard, u8 key) {
	assert_key_in_bounds(key);
	keyboard[key] = true;
}

/******************************************************************************
* FETCH/DECODE/EXECUTE LOOP 
******************************************************************************/

void fetch(Chip8* chip8, union Instruction* instruction) {
	u8* ram;
	u16* PC;
	
	ram = &chip8->memory;
	PC = &chip8->registers.PC;

	instruction->bytes.hi_byte = peek(chip8, (*PC) + 0);
	instruction->bytes.lo_byte = peek(chip8, (*PC) + 1);
	*PC = *PC + 2;
}
void decode_and_execute(Chip8* chip8, union Instruction* instruction) {
	int bitmask;
	u8 opcode;
	bitmask = 0xF000; /* 1111 0000 0000 0000 */
	opcode = (instruction->word & bitmask) >> 3; /* Pull the first nybble off; ---- ---- ---> 1111 */
	switch (opcode) {
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:
		break;
	case 7000:
		break;
	case 8:
		break;
	case 9:
		break;
	case 0xa:
		break;
	case 0xb:
		break;
	case 0xc:
		break;
	case 0xd:
		break;
	case 0xe:
		break;
	case 0xf:
		break;
		default:
			fprintf(stderr, "invalid opcode! %i\n", opcode);
			exit(EXIT_FAILURE);
			break;
	}
}

/******************************************************************************
* External Hardware (i.e. not in the chip8 spec)
* ****************************************************************************/
/* KEYBOARD MAPPING */
i8 keyboard_code_to_chip8(enum ScanCode kbd_code) {
	int i;
	u8 keymap[CHIP8_TOTAL_KEYS] = {
		/*
		* 1 2 3 c       1 2 3 4
		* 4 5 6 d  <=>  q w e r
		* 7 8 9 e       a s d f
		* a 0 b f       z x c v
		*/
		KEY_x, KEY_1, KEY_2, KEY_3,
		KEY_q, KEY_w, KEY_e, KEY_a,
		KEY_s, KEY_d, KEY_z, KEY_c,
		KEY_4, KEY_r, KEY_f, KEY_v
	};

	for (i = 0; i < CHIP8_TOTAL_KEYS; i++) {
		if (keymap[i] == kbd_code) return i;
	}
	/* failure case */
	return -1;
}

/* BEEPER */
void square_oscillator(
	i16* buffer,
	int buffer_length,
	int long sample_rate,
	int pitch,
	float volume
)
{
	/* Make sure freq is below nyquist and volume isn't too loud 
	 * [WARNING: DO NOT USE HEADPHONES] */
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
		phase += delta; /* heart of the oscillator: inc phase by [delta] amount */
		if (phase >= 1)
			phase -= 1;
		buffer[i] = final_value;
	}
}
