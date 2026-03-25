#include "chip8.h"
#include "platform.h"
#include <iostream>
#include <chrono>

int main(int argc, char** argv) {
	if (argc != 4) {
		std::cerr << "Usage: " << argv[0] << "<Scale> <Delay> <ROM>\n";
		std::exit(EXIT_FAILURE);
	}

	int videoScale = std::stoi(argv[1]);
	int cycleDelay = std::stoi(argv[2]);
	char const* romFileName = argv[3];

	Platform platform("CHIP-8 Emulator", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);

	Chip8 chip8;
	chip8.LoadROM(romFileName);

	int videoPitch = sizeof(chip8.video[0]) * VIDEO_WIDTH;
	auto lastCycleTime = std::chrono::high_resolution_clock::now();
	bool quit = false;
	bool paused = false;

	while (!quit) {
		quit = platform.ProcessInput(chip8.keypad, &paused);
		
		// skip cycle if user hit the pause key
		if (paused) {
			continue;
		}
		
		// skip cycle if we are in debug mode
		if (chip8.stepping && !chip8.stepReady) {
			continue;
		}

		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();
		if (dt > cycleDelay) {
			lastCycleTime = currentTime;
			chip8.Cycle();
			platform.Update(chip8.video, videoPitch);
		}
	}

	return 0;
}
