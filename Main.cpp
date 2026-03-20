#include <iostream>
#include <fstream>
#include "CPU.hpp"
#include "PPU.hpp"
#include <SDL2/SDL.h>
#include "APU.hpp"

const int SCALE = 3;
// 70224 clocks por frame / 4 = 17556 instrucciones (aprox)
const int CYCLES_PER_FRAME = 17556;

int main(int argc, char* argv[]) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		std::cout << "Error iniciando SDL: " << SDL_GetError() << std::endl;
		return -1;
	}

	SDL_Window* window = SDL_CreateWindow(
		"C++ Boy",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		160 * SCALE, 144 * SCALE,
		SDL_WINDOW_SHOWN
	);

	if (!window) {
		std::cout << "Error creando ventana: " << SDL_GetError() << std::endl;
		return -1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	if (!renderer) {
		std::cout << "Error creando renderer: " << SDL_GetError() << std::endl;
		return -1;
	}

	SDL_AudioSpec wanted_spec;
    SDL_zero(wanted_spec);
    wanted_spec.freq = 44100;         // Calidad de CD
    wanted_spec.format = AUDIO_F32SYS; // Flotantes de 32 bits (-1.0 a 1.0)
    wanted_spec.channels = 1;         // Mono
    wanted_spec.samples = 1024;       // Tamaño del buffer interno
    wanted_spec.callback = nullptr;   // Uso QueueAudio, no necesito callback

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(nullptr, 0, &wanted_spec, nullptr, 0);
    if (audio_device == 0) {
        std::cout << "Error abriendo audio: " << SDL_GetError() << std::endl;
    }
    SDL_PauseAudioDevice(audio_device, 0); // Despausar (prender) el parlante

    CPU::APU apu;

	CPU::Processor cpu;
	cpu.Init();

	CPU::PPU ppu;

	if (!cpu.LoadROM(argv[1])) {
		std::cout << "ERROR: No se pudo cargar la ROM." << std::endl;

		return -1;
	}

	std::cout << "ROM SIZE: " << cpu.GetRomSize() << std::endl;

	bool quit = false;
	bool debug_mode = false;
	bool step_requested = false;
	SDL_Event e;

	while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) quit = true;

            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                bool pressed = (e.type == SDL_KEYDOWN);
                int key = -1;

                switch (e.key.keysym.sym) {
                    case SDLK_d:      key = 0; break;
                    case SDLK_a:      key = 1; break;
                    case SDLK_w:      key = 2; break;
                    case SDLK_s:   	  key = 3; break;
                    case SDLK_l:      key = 4; break; // A = L
                    case SDLK_k:      key = 5; break; // B = K
                    case SDLK_RSHIFT:
                    case SDLK_LSHIFT: key = 6; break; // Select = Shift
                    case SDLK_RETURN: key = 7; break; // Start = Enter
                    
                    // Controles extra del emulador (solo se activan al apretar, no al mantener)
                    case SDLK_SPACE: if(pressed) step_requested = true; break;
                    case SDLK_p:     if(pressed) debug_mode = !debug_mode; break;
                }

                if (key != -1) {
                    cpu.UpdateJoypad(key, pressed);
                }
            }
		}

		if (debug_mode) {
			if (step_requested) {
				int cycles = cpu.Step();
				if (cycles > 0) {
					ppu.Tick(cycles * 4, cpu.bus);
				}
				step_requested = false;
			}
		} else {
			int cycles_this_frame = 0;

			while (cycles_this_frame < CYCLES_PER_FRAME) {
				cpu.HandleInterrupts();
				int cycles = cpu.Step();
				if (cycles == 0) { 
                    debug_mode = true; 
					std::cout << ">>> BREAKPOINT ALCANZADO: Salimos del bucle! <<<" << std::endl;
                    break; 
                }
				cycles_this_frame += cycles;

				ppu.Tick(cycles * 4, cpu.bus);
				apu.Tick(cycles * 4, cpu.bus, audio_device);
			}
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
        SDL_RenderClear(renderer);
		ppu.DrawFrame(renderer);
		SDL_RenderPresent(renderer);

		//SDL_Delay(8);
		while (SDL_GetQueuedAudioSize(audio_device) > 4096) {
			SDL_Delay(1);
		}
	}

	std::cout << ">>> Apagando consola..." << std::endl;

	cpu.SaveGame();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_CloseAudioDevice(audio_device);
	SDL_Quit();

	return 0;
}