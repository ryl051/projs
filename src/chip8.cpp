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

	// set up function pointer table
	table[0x0] = &Chip8::Table0;
	table[0x1] = &Chip8::OP_1nnn;
	table[0x2] = &Chip8::OP_2nnn;
	table[0x3] = &Chip8::OP_3xkk;
	table[0x4] = &Chip8::OP_4xkk;
	table[0x5] = &Chip8::OP_5xy0;
	table[0x6] = &Chip8::OP_6xkk;
	table[0x7] = &Chip8::OP_7xkk;
	table[0x8] = &Chip8::Table8;
	table[0x9] = &Chip8::OP_9xy0;
	table[0xA] = &Chip8::OP_annn;
	table[0xB] = &Chip8::OP_bnnn;
	table[0xC] = &Chip8::OP_Cxkk;
	table[0xD] = &Chip8::OP_Dxyn;
	table[0xE] = &Chip8::TableE;
	table[0xF] = &Chip8::TableF;

	for (uint8_t i = 0; i <= 0xE; i++) {
		table0[i] = &Chip8::OP_NULL;
		table8[i] = &Chip8::OP_NULL;
		tableE[i] = &Chip8::OP_NULL;
	}

	table0[0x0] = &Chip8::OP_00E0;
	table0[0xE] = &Chip8::OP_00EE;

	table8[0x0] = &Chip8::OP_8xy0;
	table8[0x1] = &Chip8::OP_8xy1;
	table8[0x2] = &Chip8::OP_8xy2;
	table8[0x3] = &Chip8::OP_8xy3;
	table8[0x4] = &Chip8::OP_8xy4;
	table8[0x5] = &Chip8::OP_8xy5;
	table8[0x6] = &Chip8::OP_8xy6;
	table8[0x7] = &Chip8::OP_8xy7;
	table8[0xE] = &Chip8::OP_8xyE;

	tableE[0x1] = &Chip8::OP_ExA1;
	tableE[0xE] = &Chip8::OP_Ex9E;

	for (uint8_t i = 0; i <= 0x65; i++) {
		tableF[i] = &Chip8::OP_NULL;
	}

	tableF[0x07] = &Chip8::OP_Fx07;
	tableF[0x0A] = &Chip8::OP_Fx0A;
	tableF[0x15] = &Chip8::OP_Fx15;
	tableF[0x18] = &Chip8::OP_Fx18;
	tableF[0x1E] = &Chip8::OP_Fx1E;
	tableF[0x29] = &Chip8::OP_Fx29;
	tableF[0x33] = &Chip8::OP_Fx33;
	tableF[0x55] = &Chip8::OP_Fx55;
	tableF[0x65] = &Chip8::OP_Fx65;
}

void Chip8::Table0() {
	((*this).*(table0[opcode & 0x000Fu]))();
}

void Chip8::Table8() {
	((*this).*(table8[opcode & 0x000Fu]))();
}

void Chip8::TableE() {
	((*this).*(tableE[opcode & 0x000Fu]))();
}

void Chip8::TableF() {
	((*this).*(tableF[opcode & 0x00FFu]))();
}

void Chip8::OP_NULL() {}

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

// SUB Vx, Vy
void Chip8::OP_8xy5() {
	uint8_t Vx = get_Vx();
	uint8_t Vy = get_Vy();
	uint8_t VF = 0xF;

	if (registers[Vx] > registers[Vy]) {
		registers[VF] = 1;
	} else {
		registers[VF] = 0;
	}

	registers[Vx] -= registers[Vy];
}

// SHR Vx
void Chip8::OP_8xy6() {
	uint8_t Vx = get_Vx();
	uint8_t VF = 0xF;

	if (Vx & 1) {
		registers[VF] = 1;
	} else {
		registers[VF] = 0;
	}

	registers[Vx] >>= 1;
}

// SUBN Vx, Vy
void Chip8::OP_8xy7() {
	uint8_t Vx = get_Vx();
	uint8_t Vy = get_Vy();

	if (registers[Vy] > registers[Vx]) {
		registers[0xF] = 1;
	} else {
		registers[0xF] = 0;
	}

	registers[Vx] = registers[Vy] - registers[Vx];
}

// SHL Vx {, Vy}
void Chip8::OP_8xyE() {
	uint8_t Vx = get_Vx();

	registers[0xF] = (registers[Vx] & 0x80) >> 7;
	registers[Vx] <<= 1;
}

// SNE Vx, Vy
void Chip8::OP_9xy0() {
	uint8_t Vx = get_Vx();
	uint8_t Vy = get_Vy();

	if (registers[Vx] != registers[Vy]) {
		pc += 2;
	}
}

// LD I, addr
void Chip8::OP_annn() {
	index = get_nnn();
}

// JP V0, addr
void Chip8::OP_bnnn() {
	pc = registers[0] + get_nnn();
}

// RND Vx, byte
void Chip8::OP_Cxkk() {
	registers[get_Vx()] = randByte(randGen) & get_byte(); 
}

// DRW Vx, Vy, nibble
void Chip8::OP_Dxyn() {
	uint8_t Vx = get_Vx();
	uint8_t Vy = get_Vy();
	uint8_t h = opcode & 0x000F;

	// wrap
	uint8_t x = registers[Vx] & VIDEO_WIDTH;
	uint8_t y = registers[Vy] & VIDEO_HEIGHT;

	registers[0xF] = 0;

	for (uint32_t row = 0; row < h; row++) {
		uint8_t spriteByte = memory[index + row];

		for (uint32_t col = 0; col < 8; col++) {
			uint8_t spritePixel = spriteByte & (0x80 >> col);
			uint32_t* screenPixel = &video[(y + row) * VIDEO_WIDTH + (x + col)];
			
			// sprite pixel is on
			if (spritePixel) {
				// screen pixel is also on; it is a collision!
				if (*screenPixel == 0xFFFFFFFF) {
					registers[0xF] = 1;
				}

				*screenPixel ^= 0xFFFFFFFF;
			}
		}
	}
}

// SKP Vx
void Chip8::OP_Ex9E() {
	uint8_t Vx = get_Vx();

	if (keypad[registers[Vx]]) {
		pc += 2;
	}
}

// SKNP Vx
void Chip8::OP_ExA1() {
	uint8_t Vx = get_Vx();

	if (!keypad[registers[Vx]]) {
		pc += 2;
	}
}

// LD Vx, DT
void Chip8::OP_Fx07() {
	registers[get_Vx()] = delayTimer;
}

// LD Vx, K
void Chip8::OP_Fx0A() {
	uint8_t Vx = get_Vx();

	if (keypad[0])
	{
		registers[Vx] = 0;
	}
	else if (keypad[1])
	{
		registers[Vx] = 1;
	}
	else if (keypad[2])
	{
		registers[Vx] = 2;
	}
	else if (keypad[3])
	{
		registers[Vx] = 3;
	}
	else if (keypad[4])
	{
		registers[Vx] = 4;
	}
	else if (keypad[5])
	{
		registers[Vx] = 5;
	}
	else if (keypad[6])
	{
		registers[Vx] = 6;
	}
	else if (keypad[7])
	{
		registers[Vx] = 7;
	}
	else if (keypad[8])
	{
		registers[Vx] = 8;
	}
	else if (keypad[9])
	{
		registers[Vx] = 9;
	}
	else if (keypad[10])
	{
		registers[Vx] = 10;
	}
	else if (keypad[11])
	{
		registers[Vx] = 11;
	}
	else if (keypad[12])
	{
		registers[Vx] = 12;
	}
	else if (keypad[13])
	{
		registers[Vx] = 13;
	}
	else if (keypad[14])
	{
		registers[Vx] = 14;
	}
	else if (keypad[15])
	{
		registers[Vx] = 15;
	}
	else
	{
		pc -= 2;
	}
}

// LD DT, Vx
void Chip8::OP_Fx15() {
	delayTimer = registers[get_Vx()];
}

// LD ST, Vx
void Chip8::OP_Fx18() {
	soundTimer = registers[get_Vx()];
}

// ADD I, Vx
void Chip8::OP_Fx1E() {
	index += registers[get_Vx()];
}

// LD F, Vx
void Chip8::OP_Fx29() {
	index = FONTSET_START_ADDR + (5 * registers[get_Vx()]);
}

// LD B, Vx
void Chip8::OP_Fx33() {
	uint8_t temp = registers[get_Vx()];

	// ones digit
	memory[index + 2] = temp % 10;
	temp /= 10;

	// tens digit
	memory[index + 1] = temp % 10;
	temp /= 10;

	// hundreds digit
	memory[index] = temp % 10;
}

// LD [I], Vx
void Chip8::OP_Fx55() {
	uint8_t Vx = get_Vx();
	for (uint32_t i = 0; i <= Vx; i++) {
		memory[index + i] = registers[i];
	}
}

// LD Vx, [I]
void Chip8::OP_Fx65() {
	uint8_t Vx = get_Vx();
	for(uint32_t i = 0; i <= Vx; i++) {
		registers[i] = memory[index + i];
	}
}

// Cycle
void Chip8::Cycle() {
	// if
	opcode = (memory[pc] << 8) | memory[pc + 1];
	pc += 2;

	// id and ex
	((*this).*(table[(opcode & 0xF000) >> 12]))();

	if (delayTimer > 0) {
		delayTimer--;
	}

	if (soundTimer > 0) {
		soundTimer--;
	}
}