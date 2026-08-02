#ifndef CHIP8_H

#include <stdint.h>

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
#define CHIP8_FRAMES_PER_SECOND 60
#define CHIP8_INSTRUCTIONS_PER_FRAME 11
#define CHIP8_MAX_ROM_SIZE 3584

/* My husband is a baker */
/* #define boule bool */

/* aliases */
#define u8 uint8_t
#define i8 int8_t
#define u16 uint16_t
#define i16 int16_t

/* Structs, Enums */
struct Registers {
	u8 V[CHIP8_NUM_DATA_REGISTERS];
	u16 I;
	u8 DL, DS;
	u8 SP;
	u16 PC;
};

typedef struct Chip8 {
	bool screen[SCREEN_HEIGHT][SCREEN_WIDTH];
	u8 memory[CHIP8_MEMORY_SIZE];
	bool keyboard[CHIP8_TOTAL_KEYS];
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
bool init_chip8(Chip8* chip8);
void destroy_chip8(Chip8* chip8);
static void assert_address_in_bounds(u16 address);
u8 peek(Chip8* chip8, u16 address);
bool poke(Chip8* chip8, u16 address, u8 value);
static void assert_stack_in_bounds(u16 SP);
bool push(Chip8* chip8, u16 value);
u16 pop(Chip8* chip8);
static void assert_pixel_in_bounds(int x, int y);
bool get_pixel(bool** display, int x, int y);
void set_pixel(bool** display, int x, int y);
static void assert_key_in_bounds(u8 key);
void key_up(bool* keyboard, u8 key);
void key_down(bool* keyboard, u8 key);

/* Emulation cycle */
void fetch(Chip8* chip8, union Instruction instruction);
void decode_and_execute(Chip8* chip8, union Instruction instruction);
static u8 get_nybble(union Instruction instruction, int position);
static u16 get_address(union Instruction instruction);


/* external hardware prototypes */
i8 keyboard_code_to_chip8(enum ScanCode kbd_code);
void square_oscillator(float* buffer, int buffer_length, int long sample_rate, int pitch, float volume);

#endif /* CHIP8_H */
