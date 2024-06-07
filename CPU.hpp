#include <iostream>
#include <fstream>

#ifndef CPU_H
#define CPU_H

#define u8 uint8_t
#define u16 uint16_t
#define flg bool
#define set true
#define reset false
#define RUN_ROM true

namespace CPU {
	extern u8 opcode;

	class Register8 {
	private:
		u8 value;
	public:
		u8 Get() const;
		void Set(u8 val);
		void Increment(u8 val);
	};

	class Register16 {
	private:
		u8 first_val, second_val;
	public:
		u16 Get() const;
		void Set(u16 val);
		void SetVirtual(u8 f_val, u8 s_val);
		void Increment(u8 val);
	};

	class RegisterFlag {
	private:
		flg value;
	public:
		flg Get() const;
		void Set(flg val);
	};

	struct Register {
		Register8 A, B, C, D, E, F, H, L;
		Register16 AF, DB, DE, HL, SP, PC;

		struct Flag_Register {
			RegisterFlag Z, H, N, C;
		};

		Flag_Register flag;

		void Init();
	};

	class Memory_Bus {
	private:
		u8* memory = new u8[0x10000];

	public:
		void LoadGame(std::basic_ifstream<u8>& gb_rom);
		void ShowMemory();
		u8 GetMemoryAt(u16 PC);
		void Execute(Memory_Bus bus, Register reg);
	};

	class Command {
	public:
		void NOP(Register& reg);
		void LD(Memory_Bus& bus, Register& reg);
	};
}



#endif // !CPU_H
