#include <cstdint>
#include <random>

#define START_ADDR         0x200
#define FONTSET_SIZE       80
#define FONTSET_START_ADDR 50
#define VIDEO_WIDTH 	   64
#define VIDEO_HEIGHT       32

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
class Chip8
{
private:
	std::default_random_engine randGen;
	std::uniform_int_distribution<uint8_t> randByte;
	uint8_t get_Vx();
	uint8_t get_Vy();
	uint8_t get_byte();
	uint8_t get_nnn();

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

	typedef void (Chip8::*Chip8Func)();
	Chip8Func table[0xF + 1]{};
	Chip8Func table0[0xE + 1]{};
	Chip8Func table8[0xE + 1]{};
	Chip8Func tableE[0xE + 1]{};
	Chip8Func tableF[0x65 + 1]{};

	void Table0();
	void Table8();
	void TableE();
	void TableF();
	void OP_NULL();

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
	void OP_4xkk();
	void OP_5xy0();
	void OP_6xkk();
	void OP_7xkk();
	void OP_8xy0();
	void OP_8xy1();
	void OP_8xy2();
	void OP_8xy3();
	void OP_8xy4();
	void OP_8xy5();
	void OP_8xy6();
	void OP_8xy7();
	void OP_8xyE();
	void OP_9xy0();
	void OP_annn();
	void OP_bnnn();
	void OP_Cxkk();
	void OP_Dxyn();
	void OP_Ex9E();
	void OP_ExA1();
	void OP_Fx07();
	void OP_Fx0A();
	void OP_Fx15();
	void OP_Fx18();
	void OP_Fx1E();
	void OP_Fx29();
	void OP_Fx33();
	void OP_Fx55();
	void OP_Fx65();
	void Cycle();
};
