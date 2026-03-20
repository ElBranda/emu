#pragma once
#include "CPU.hpp"
#include <SDL2/SDL.h>
#include <vector>

namespace CPU {
    class APU {
    private:
        int audio_cycles;
        float phase1;
        float phase2;
        float phase3;
        float phase4;
        uint16_t lfsr;
        std::vector<float> sample_buffer;
        float onda_pasada_entrada;
        float onda_pasada_salida;

    public:
        APU();
        void Tick(int cycles, Memory_Bus& bus, SDL_AudioDeviceID device);
    };
}