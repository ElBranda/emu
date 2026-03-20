#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>

#pragma once

#define RUN_ROM true
#define ROM_LIMIT 0x7fff

namespace CPU {
	using u8 = uint8_t;
	using u16 = uint16_t;
	using s8 = int8_t;
	
	struct RegisterPair {
		union {
			struct {
				u8 F;
				u8 A;
			};
			u16 AF;
		};

		union {
			struct {
				u8 C;
				u8 B;
			};
			u16 BC;
		};

		union {
			struct {
				u8 E;
				u8 D;
			};
			u16 DE;
		};

		union {
			struct {
				u8 L;
				u8 H;
			};
			u16 HL;
		};

		u16 SP;
		u16 PC;
	};

	class Flags {
		private:
		u8& f_reg;

		public:
		Flags(u8& f) : f_reg(f) {}

		bool Z() const { return (f_reg >> 7) & 1; }
		bool N() const { return (f_reg >> 6) & 1; }
		bool H() const { return (f_reg >> 5) & 1; }
		bool C() const { return (f_reg >> 4) & 1; }

		void SetZ(bool v) { if (v) f_reg |= (1 << 7); else f_reg &= ~(1 << 7); }
		void SetN(bool v) { if (v) f_reg |= (1 << 6); else f_reg &= ~(1 << 6); }
		void SetH(bool v) { if (v) f_reg |= (1 << 5); else f_reg &= ~(1 << 5); }
		void SetC(bool v) { if (v) f_reg |= (1 << 4); else f_reg &= ~(1 << 4); }
	};

	struct Register {
		RegisterPair val;
		Flags flag;

		Register() : flag(val.F) {}

		void Init();
	};

	class Memory_Bus {
	private:
		std::vector<u8> rom;	// Cartucho
		u8 vram[0x2000];		// Video RAM (8KB)
		u8 wram[0x2000];		// Work RAM (8KB)
		u8 hram[0x80];			// High RAM
		u8 io[0x80];			// IO Registers
		u8 oam[0xA0];
		u8 ie_register;
		int div_counter = 0;
		int tima_counter = 0;
		u8 joypad_dir = 0x0F;
		u8 joypad_action = 0x0F;
		std::vector<u8> external_ram; 
        bool ram_enabled = false;
        u8 rom_bank = 1;
        u8 ram_bank = 0;
        bool has_battery = false;
        std::string save_path = "";

	public:
		Memory_Bus();
		
		u8 Read(u16 address);
		void Write(u16 address, u8 value);
		bool LoadROM(const char* path);
		void RequestInterrupt(u8 bit) {
			u8 current_if = Read(0xFF0F);
			Write(0xFF0F, current_if | (1 << bit));
		}
		void SetIE(u8 val) { ie_register = val; }
		void TickTimer(int cycles) {
            div_counter += cycles;
            if (div_counter >= 64) { 
                div_counter -= 64;
                io[0x04]++; 
            }

            u8 tac = io[0x07]; 
            
            if (tac & 0x04) { 
                tima_counter += cycles;
                
                int freq_cycles = 0;
                switch (tac & 0x03) {
                    case 0: freq_cycles = 256; break; 
                    case 1: freq_cycles = 4; break;   
                    case 2: freq_cycles = 16; break;  
                    case 3: freq_cycles = 64; break;  
                }
                
                while (tima_counter >= freq_cycles) {
                    tima_counter -= freq_cycles;
                    
                    if (io[0x05] == 0xFF) { 
                        io[0x05] = io[0x06]; 
                        RequestInterrupt(2); 
                    } else {
                        io[0x05]++;
                    }
                }
            }
        }
		void UpdateLY(u8 value) { io[0x44] = value; }
        void UpdateSTAT(u8 value) { io[0x41] = value; }
		void UpdateJoypad(int key, bool pressed);
		void SaveGame();

		// DEBUG
		void ShowMemory(u16 start, u16 end);
		int GetRomSize();
	};

	class Command {
	public:
		void NOP();

		// Carga generica de 8 bits: LD r1, r2
		void LD(u8& dest, u8 src);
		// Carga de 16 bits: LD rr, nn
		void LD(u16& dest, u16 val);
		// Carga en memoria: LD (HL), r
		void LD_Mem(Memory_Bus& bus, u16 addr, u8 val);
		void LDD_Read(Memory_Bus& bus, u16& HL, u8& A);
		void LDD_Write(Memory_Bus& bus, u8& A, u16& HL);
		void LDHL(u16& HL, u16 SP, s8 n, Flags& flags);

		void JP(u16& PC, u16 address);

		void JR(u16& PC, s8 offset, bool condition);

		void OR(u8& A, u8 val, Flags& flags);
		void XOR(u8& A, u8 val, Flags& flags);
		
		void DEC(u16& reg);
		void DEC(u8& reg, Flags& flags);
		void DEC_Mem(Memory_Bus& bus, u16 addr, Flags& flags);

		void DI(bool& ime_flag);
		void LDI_Write(Memory_Bus& bus, u16& HL, u8 val);
		void LDI_Read(Memory_Bus& bus, u16& HL, u8& dest);

		void CP(u8 A, u8 val, Flags& flags);

		void PUSH(Memory_Bus& bus, u16& SP, u16 val);

		void CALL(Memory_Bus& bus, u16& SP, u16& PC, u16 target_addr);

		void INC(u8& reg, Flags& flags);
		void INC(u16& reg);

		void AND(u8& A, u8 val, Flags& flags);

		void RST(Memory_Bus& bus, u16& SP, u16& PC, u16 target_addr);

		void ADD(u8& A, u8 val, Flags& flags);
		void ADD_HL(u16& HL, u16 n, Flags& flags);
		void ADC(u8& A, u8 val, Flags& flags);

		void SUB(u8& A, u8 val, Flags& flags);
		void SBC(u8& A, u8 val, Flags& flags);

		void POP(Memory_Bus& bus, u16& SP, u16& dest_reg_pair);

		void SWAP(u8& reg, Flags& flags);
		void RL(u8& reg, Flags& flags);
		void RR(u8& reg, Flags& flags);
		void RLC(u8& reg, Flags& flags);
		void RRC(u8& reg, Flags& flags);

		void SLA(u8& reg, Flags& flags);
		void SRA(u8& reg, Flags& flags);
		void SRL(u8& reg, Flags& flags);

		void BIT(u8 val, u8 bit, Flags& flags);
	};

	class Processor {
		private:
		bool IME;
		bool halted;
		Register reg;
		Command com;
		u8 Execute(u8 opcode);
		
		public:
		Memory_Bus bus;
		void Init();
		u8 Step();	// Fetch-Decode-Execute
		u16 Fetch16();
		void SetIME(bool enabled) { IME = enabled; };
		bool LoadROM(const char* path);
		int GetRomSize();
		u16 GetPC() const { return reg.val.PC; }
		u16 GetHL() const { return reg.val.HL; }
		void HandleInterrupts();
		void UpdateJoypad(int key, bool pressed) { bus.UpdateJoypad(key, pressed); }
		void SaveGame() { bus.SaveGame(); }
	};
}
