#pragma once

#include <SDL2/SDL.h>
#include <cstdint>
#include "chip8.h"

class Platform
{

private:
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    SDL_Texture* texture{};
    
    public:
    Platform(char const* title, int windowWidth, int windowHeight, int textureWidth, int textureHeight);
    ~Platform();
    void Update(void const* buffer, int pitch);
    bool ProcessInput(Chip8 chip8, uint8_t* keys, bool* paused);
};
