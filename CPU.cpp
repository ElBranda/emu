#include <fstream>
#include <iomanip>
#include "CPU.hpp"

using namespace CPU;

u8 CPU::opcode;

u8 Register8::Get() const { return value; }
void Register8::Set(u8 val) { value = val; }
void Register8::Increment(u8 val) { value+=val; }

u16 Register16::Get() const { return ((first_val << 8) + second_val); }
void Register16::Set(u16 val) { first_val = (val >> 8); second_val = (val & 0xff); }
void Register16::SetVirtual(u8 f_val, u8 s_val) { first_val = f_val; second_val = s_val; }
void Register16::Increment(u8 val) {
	u16 reg = ((first_val << 8) + second_val + val);
	first_val = ((reg & 0xff00) >> 8);
	second_val = (reg & 0xff);
}

flg RegisterFlag::Get() const { return value; }
void RegisterFlag::Set(flg val) { value = val; }

void Register::Init() {
	PC.Set(0x100);
}

void Memory_Bus::Fetch(Register& reg) {
	opcode = GetMemoryAt(reg.PC.Get());

	reg.PC.Increment(1);
}
void Memory_Bus::LoadGame(std::basic_ifstream<u8>& gb_rom) { for (int i = 0; i < 0x10000; i++) gb_rom.get(memory[i]); }
void Memory_Bus::ShowMemory() {
	for (int i = 0; i < 0x10000; i++) {
		if (i % 16 == 0) std::cout << std::endl << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(i) << ": ";
		
		std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(memory[i]) << " ";
	}
}
u8 Memory_Bus::GetMemoryAt(u16 PC) { return memory[PC]; }
void Memory_Bus::Execute(Memory_Bus& bus, Register& reg) {
	Command com;

	switch (opcode) {
		case 0x00: com.NOP(reg); break;
		case 0x06: com.LD8(nullptr, 'B', reg, opcode); break;
		case 0x0e: com.LD8(nullptr, 'C', reg, opcode); break;
		case 0x16: com.LD8(nullptr, 'D', reg, opcode); break;
		case 0x1e: com.LD8(nullptr, 'E', reg, opcode); break;
		case 0x26: com.LD8(nullptr, 'H', reg, opcode); break;
		case 0x2e: com.LD8(nullptr, 'L', reg, opcode); break;
	}
}

void Command::NOP(Register& reg) { reg.PC.Increment(1); }
void Command::LD8(Memory_Bus* bus, char reg_name, Register& reg, u8 val) {
	if (bus == nullptr) {
		switch (reg_name) {
			case 'B': reg.B.Set(val);
			case 'C': reg.C.Set(val);
			case 'D': reg.D.Set(val);
			case 'E': reg.E.Set(val);
			case 'H': reg.H.Set(val);
			case 'L': reg.L.Set(val);
		}
	} else {

	}
}