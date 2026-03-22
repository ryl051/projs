#include <cstdint>
#include "chip8.h"
#include <fstream>
#include <random>
#include <chrono>
#include <cstring>

// keypad mapping
/*
Keypad       Keyboard
+-+-+-+-+    +-+-+-+-+
|1|2|3|C|    |1|2|3|4|
+-+-+-+-+    +-+-+-+-+
|4|5|6|D|    |Q|W|E|R|
+-+-+-+-+ => +-+-+-+-+
|7|8|9|E|    |A|S|D|F|
+-+-+-+-+    +-+-+-+-+
|A|0|B|F|    |Z|X|C|V|
+-+-+-+-+    +-+-+-+-+
 */

const uint32_t START_ADDR = 0x200;
const uint32_t FONTSET_SIZE = 80;
const uint32_t FONTSET_START_ADDR = 0x50;

// loading the fonts, where do we get the bytes from?
// i.e. for the letter 'F'
// 11110000
// 10000000
// 11110000
// 10000000
// 10000000
//
// notice how the 1's create the letter 'F'?

uint8_t fontset[FONTSET_SIZE] =
{
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

Chip8::Chip8 () 
	: randGen(std::chrono::system_clock::now().time_since_epoch().count()),
	  pc(START_ADDR),
	  randByte(0, 255U) {

	// load fonts into memory
	for (uint32_t i = 0; i < FONTSET_SIZE; i++) {
		memory[FONTSET_START_ADDR + i] = fontset[i];
	}
}

// don't understand how this function works, and i dont bother to
void Chip8::LoadROM(char const* filename) {
	// open file as stream of binary, move filer pointer to end ??
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	if (file.is_open()) {
		// get size of file, alloc buffer to hold contents
		std::streampos size = file.tellg();
		char* buffer = new char[size];

		// now we go back to the beginning of file and fill buffer
		file.seekg(0, std::ios::beg);
		file.read(buffer, size);
		file.close();

		// load the ROM contents into the Chip8's memory, starting at START_ADDR
		for (uint32_t i = 0; i < size; i++) {
			memory[START_ADDR + i] = buffer[i];
		}

		delete[] buffer;
	}
}

/*
--------------INSTRUCTIONS--------------
*/

// CLS
void Chip8::OP_00E0() {
	memset(video, 0, sizeof(video)); // note: "video" gives pointer to first element in array.
}

// RET
void Chip8::OP_00EE() {
	// pc = stack[sp];
	// sp--;
	sp--;
	pc = stack[sp];
}

// Jp addr: jump to loc nnn
void Chip8::OP_1nnn() {
	uint16_t nnn = opcode & 0x0FFF;

	pc = nnn;
}

// CALL addr: call subroutine at nnn
void Chip8::OP_2nnn() {
	uint16_t nnn = opcode & 0x0FFF;

	stack[sp] = pc;
	sp++;
	pc = nnn;
}

// SE Vx, byte (kk)
void Chip8::OP_3xkk() {
	uint16_t kk = opcode & 0x00FF;
	uint8_t Vx = (opcode & 0x0F00) >> 8;

	if (registers[Vx] == kk) {
		pc += 2;
	}
}

// SNE Vx, byte
void Chip8::OP_4xkk() {
	uint16_t kk = opcode & 0x00FF;
	uint8_t Vx = (opcode & 0x0F00) >> 8;

	if (registers[Vx] != kk) {
		pc += 2;
	}
}

// SE Vx, Vy
void Chip8::OP_5xy0() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;

	if (registers[Vx] == registers[Vy]) {
		pc += 2;
	}
}

// LD Vx, byte
void Chip8::OP_6xkk() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t kk = opcode & 0x00FF;

	registers[Vx] == kk;
}

// ADD Vx, byte
void Chip8::OP_7xkk() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t kk = opcde & 0x00FF;

	registers[Vx] += kk;
}

// LD Vx, Vy
void Chip8::OP_8xy0() {
	uint8_t Vx = (opcode & 0x0F00) >> 8;
	uint8_t Vy = (opcode & 0x00F0) >> 4;

	registers[Vx] = registers[Vy];
}