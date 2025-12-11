#include <iostream>
#include <fstream>
#include <vector>

#pragma once


#define flg bool
#define set true
#define reset false
#define interr bool
#define enable true
#define disable false
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
		interr IME;

	public:
		Memory_Bus();
		
		u8 Read(u16 address);
		void Write(u16 address, u8 value);
		bool LoadROM(const char* path);
		void ShowMemory(u16 start, u16 end);
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

		void JP(u16& PC, u16 address);

		void JR(u16& PC, s8 offset, bool condition);

		void XOR(u8& A, u8 val, Flags& flags);

		void DEC(u8& reg, Flags& flags);
		void DEC_Mem(Memory_Bus& bus, u16 addr, Flags& flags);

		void DI(bool& ime_flag);
		void LDI_Write(Memory_Bus& bus, u16& HL, u8 val);
		void LDI_Read(Memory_Bus& bus, u16& HL, u8& dest);
	};

	class Processor {
		private:
		bool IME;
		Register reg;
		Memory_Bus bus;
		Command com;
		u8 current_opcode;

		public:
		void Init();
		void Step();	// Fetch-Decode-Execute
		void Execute();
		u16 Fetch16();
		void SetIME(bool enabled) { IME = enable; };
	};
}
