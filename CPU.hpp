#include <iostream>
#include <fstream>

#pragma once

#define u8 uint8_t
#define s8 char
#define u16 uint16_t
#define flg bool
#define set true
#define reset false
#define interr bool
#define enable true
#define disable false
#define RUN_ROM true
#define ROM_LIMIT 0x7fff

namespace CPU {
	extern u8 opcode;
	enum reg_select { A = 0, B = 0, D = 0, H = 0, F = 1, C = 1, E = 1, L = 1, NN = 2, HL = 3};

	class Register16 {
	private:
		u8 first_val, last_val;
	public:
		u16 Get() const;
		u16 GetFirst() const;
		u16 GetLast() const;
		void Set(u16 val);
		void SetFirst(u8 val);
		void SetLast(u8 val);
		void SetVirtual(u8 f_val, u8 l_val);
		void Increment(u8 val);
		void Decrement(u8 val);
	};

	class RegisterFlag {
	private:
		flg value;
	public:
		flg Get() const;
		void Set(flg val);
	};

	struct Register {
		Register16 AF, BC, DE, HL, SP, PC, NN;

		struct Flag_Register {
			RegisterFlag Z, H, N, C;
		};

		Flag_Register flag;

		void Init();
	};

	extern Register reg;

	class Memory_Bus {
	private:
		u8* memory = new u8[0x10000];
		interr IME;

	public:
		void Fetch();
		void LoadGame(std::basic_ifstream<u8>& gb_rom);
		void ShowMemory();
		u8 GetMemoryAt(u16 PC);
		void SetMemory(u16 memory_pos, u8 data);
		void Execute(Memory_Bus& bus);

		static u16 GetLSBF(Memory_Bus& bus);

		void SetIMEDisable();
	};

	class Command {
	public:
		void NOP();
		void LD8(char reg_name, Register16& reg, u8 val, Memory_Bus* bus = nullptr);
		void LD16(Register16& reg, u16 val, Memory_Bus* bus = nullptr);
		void JP(Memory_Bus& bus);
		void DI(Memory_Bus& bus);
		void XOR(char reg_name, Register16 regis);
		void LDI(char reg_name, Memory_Bus* bus);
		void DEC8(char reg_name, Register16& regis, Memory_Bus& bus);
		void JR(Memory_Bus& bus, flg condition);
	};
}
