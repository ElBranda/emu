#pragma once
#include "CPU.hpp"
#include <SDL2/SDL.h>

namespace CPU {
    using u32 = uint32_t; 

    enum PPUMode {
        OAM_SCAN    = 2,
        DRAWING     = 3,
        HBLANK      = 0,
        VBLANK      = 1
    };

    class PPU {
        private:
        int mode_clock;
        PPUMode current_mode;
        u8 line_y;

        u32 screen_buffer[160 * 144];

        public:
        PPU();

        void Tick(u8 cycles, Memory_Bus& bus);
        void DebugDrawTiles(SDL_Renderer* renderer, Memory_Bus& bus);
        void DrawFrame(SDL_Renderer* renderer);
        void RenderScanline(Memory_Bus& bus);

        private:
        void SetMode(PPUMode mode, Memory_Bus& bus);
        void UpdateLY(Memory_Bus& bus, u8 value);
        void DrawPixel(SDL_Renderer* renderer, int x, int y, int color_id);
    };
}