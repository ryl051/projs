#include <cstdint>
#include "chip8.h"
#include <fstream>
#include <random>
#include <chrono>
#include <cstring>

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

// Helpers:
uint8_t Chip8::get_Vx() {
	return this->opcode & (0x0F00) >> 8;
}

uint8_t Chip8::get_Vy() {
	return (this->opcode & 0x00F0) >> 4;
}

uint8_t Chip8::get_byte() {
	return this->opcode & 0x00FF;
}

uint8_t Chip8::get_nnn() {
	return this->opcode & 0x0FFF;
}

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
	pc = get_nnn();
}

// CALL addr: call subroutine at nnn
void Chip8::OP_2nnn() {
	stack[sp] = pc;
	sp++;
	pc = get_nnn();
}

// SE Vx, byte (kk)
void Chip8::OP_3xkk() {
	uint16_t kk = get_byte();
	uint8_t Vx = get_Vx();

	if (registers[Vx] == kk) {
		pc += 2;
	}
}

// SNE Vx, byte
void Chip8::OP_4xkk() {
	uint16_t kk = get_byte();
	uint8_t Vx = get_Vx();

	if (registers[Vx] != kk) {
		pc += 2;
	}
}

// SE Vx, Vy
void Chip8::OP_5xy0() {
	uint8_t Vx = get_Vx();
	uint8_t Vy = get_Vy();

	if (registers[Vx] == registers[Vy]) {
		pc += 2;
	}
}

// LD Vx, byte
void Chip8::OP_6xkk() {
	uint8_t Vx = get_Vx();
	uint8_t kk = get_byte();

	registers[Vx] = kk;
}

// ADD Vx, byte
void Chip8::OP_7xkk() {
	uint8_t Vx = get_Vx();
	uint8_t kk = get_byte();

	registers[Vx] += kk;
}

// LD Vx, Vy
void Chip8::OP_8xy0() {
	uint8_t Vx = get_Vx();
	uint8_t Vy = get_Vy();

	registers[Vx] = registers[Vy];
}

// OR Vx, Vy
void Chip8::OP_8xy1() {
	uint8_t Vx = get_Vx();
	uint8_t Vy = get_Vy();

	registers[Vx] |= registers[Vy];
}

// AND Vx, Vy
void Chip8::OP_8xy2() {
	uint8_t Vx = get_Vx();
	uint8_t Vy = get_Vy();

	registers[Vx] &= registers[Vy];
}

// XOR Vx, Vy
void Chip8::OP_8xy3() {
	uint8_t Vx = get_Vx();
	uint8_t Vy = get_Vy();

	registers[Vx] ^= registers[Vy];
}

// ADD Vx, Vy
void Chip8::OP_8xy4() {
	uint8_t Vx = get_Vx();
	uint8_t Vy = get_Vy();
	uint8_t VF = 0xF;

	uint16_t sum = registers[Vx] + registers[Vy];
	if (sum > 255) {
		registers[VF] = 1;
	} else {
		registers[VF] = 0;
	}

	registers[Vx] = sum & 0xFF;
}