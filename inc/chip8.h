#include <cstdint>
#include <random>

class Chip8
{
private:
	std::default_random_engine randGen;
	std::uniform_int_distribution<uint8_t> randByte;

public: 
	uint8_t  registers[16]{};
	uint8_t  memory[4096]{};
	uint16_t index{};          // special reg for memory addr for use in ins
	uint16_t pc{};
	uint16_t stack[16]{};      // stores pc's
	uint8_t  sp{};
	uint8_t  delayTimer{};
	uint8_t  soundTimer{};
	uint8_t  keypad[16]{};      // mapping described below class
	uint32_t video[64 * 32]{}; // storing graphics !! 
	uint16_t opcode;

	Chip8();
	void LoadROM(const char* filename);
	void OP_00E0();
	void OP_00EE();
	void OP_1nnn();
	void OP_2nnn();
	void OP_3xkk();

};
