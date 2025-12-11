#include <iostream>
#include <fstream>
#include "CPU.hpp"
#include "Helper.hpp"
#include <SDL2/SDL.h>

const int SCALE = 3;
// 70224 clocks por frame / 4 = 17556 instrucciones (aprox)
const int CYCLES_PER_FRAME = 17556;

int main(int argc, char* argv[]) {
	// Inicializar SDL (Video)
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "Error iniciando SDL: " << SDL_GetError() << std::endl;
		return -1;
	}

	// Crear ventana
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

	// Renderer
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	if (!renderer) {
		std::cout << "Error creando renderer: " << SDL_GetError() << std::endl;
		return -1;
	}

	CPU::Processor cpu;
	cpu.Init();

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

			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_SPACE) {
					step_requested = true;
				}
				if (e.key.keysym.sym == SDLK_p) {
					debug_mode = !debug_mode;
				}
			}
		}

		if (debug_mode) {
			if (step_requested) {
				cpu.Step();
				step_requested = false;
			}
		} else {
			int cycles_this_frame = 0;

			while (cycles_this_frame < CYCLES_PER_FRAME) {
				if (cpu.GetHL() == 0xE000) { 
                    debug_mode = true; // Activamos la pausa manual
                    std::cout << ">>> BREAKPOINT ALCANZADO: Salimos del bucle! <<<" << std::endl;
                    break; // Cortamos el frame actual para que atiendas
                }
				int cycles = cpu.Step();
				cycles_this_frame += cycles;
			}
		}

		SDL_RenderPresent(renderer);

		SDL_Delay(1);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}