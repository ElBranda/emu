# Game Boy Emulator (emu)

A fully functional Game Boy emulator developed in C++ using the SDL2 library. This project focuses on low-level systems architecture, hardware emulation, and precise timing.

## Features
### CPU (Sharp LR35902)
* Core implementation of the instruction set, supporting all essential opcodes for functional gameplay.
* Optimized 8/16-bit register architecture using C++ unions for efficient memory access.
* Integrated interrupt handling (VBlank, LCD Stat, Timer, Serial, and Joypad).

### PPU (Pixel Processing Unit)
* State-machine based timing covering OAM_SCAN, DRAWING, HBLANK, and VBLANK modes.
* Scanline-accurate rendering for both background and window layers.
* Full sprite support, including 8x8 and 8x16 modes, with X/Y flipping and palette mapping.

### APU (Audio Processing Unit)
* 4-channel sound implementation: two Pulse channels, one custom Wave channel, and one Noise channel.
* High-pass filtering applied to the final audio output to ensure clean sound quality.
* Real-time audio streaming using SDL_QueueAudio.

### Memory & Persistence
* Comprehensive Memory Bus handling ROM, VRAM, WRAM, OAM, and HRAM.
* Support for cartridge battery-backed saves, automatically generating and loading .sav files.
* Joypad state management with interrupt requests on key presses.

## Controls
The emulator uses the following SDL2 key mappings:
| Game Boy Key | Keyboard Key |
| :-- | --: |
| D-Pad | W/A/S/D |
| A | L |
| B | K |
| Select | Left/Right Shift |
| Start | Enter |
* P: Toggle Debug Mode.
* Space: Step instruction (when in Debug Mode).

## Technical Stack
* Language: C++.
* Libraries: SDL2 (Video, Audio, and Input).
* Architecture: Modular design with separated Bus, CPU, PPU, and APU components.

## How to Run
1. Ensure you have SDL2 installed on your system.
2. Compile the project using your preferred C++ compiler (e.g., G++ or MSVC).
3. Run the executable passing the ROM path as an argument:
```Bash
./emulator path/to/your/rom.gb
```

## Project Status
The emulator is currently in a functional and playable state. It successfully runs popular titles (such as Tetris or Wario Land) while the instruction set and memory banking support continue to be expanded.