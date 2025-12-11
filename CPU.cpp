#include <fstream>
#include <iomanip>
#include "CPU.hpp"

using namespace CPU;

void Register::Init() {
	val.AF = 0x01B0;
	val.BC = 0x0013;
	val.DE = 0x00D8;
	val.HL = 0x014D;
	val.SP = 0xFFFE;
	val.PC = 0x0100;
}

Memory_Bus::Memory_Bus() {
	// ROM vaciada por seguridad
	rom.resize(0x8000, 0);
}
bool Memory_Bus::LoadROM(const char* path) {
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) return false;

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	rom.resize(size);
	if (file.read((char*)rom.data(), size)) return true;
	return false;
}
u8 Memory_Bus::Read(u16 address) {
	if (address < 0x8000) {
		// Cartucho (ROM)
		// Si la ROM es chica, evita el crash
		if (address < rom.size()) return rom[address];
		return 0xFF;
	} else if (address >= 0x8000 && address < 0xA000) {
		return vram[address - 0x8000];
	} else if (address >= 0xC000 && address < 0xE000) {
		return wram[address - 0xC000];
	} else if (address >= 0xFF00 && address < 0xFF80) {
		return io[address - 0xFF00];
	} else if (address >= 0xFF80 && address < 0xFFFF) {
		return hram[address - 0xFF80];
	}

	// Default Return
	return 0xFF;
}
void Memory_Bus::Write(u16 address, u8 value) {
	if (address < 0x8000) {
		// ROM es solo lectura
		return;
	} else if (address >= 0x8000 && address < 0xA000) {
		vram[address - 0x8000] = value;
	} else if (address >= 0xC000 && address < 0xE000) {
		wram[address - 0xC000] = value;
	} else if (address >= 0xFF00 && address < 0xFF80) {
		io[address - 0xFF00] = value;
	} else if (address >= 0xFF80 && address < 0xFFFF) {
		hram[address - 0xFF80] = value;
	}
}
void Memory_Bus::ShowMemory(u16 start, u16 end) {
	for (int i = start; i <= end; i++) {
		if (i % 16 == 0) std::cout << "\n" << std::hex << i << ": ";
		// Se usa Read() para que busque en el array correcto (ROM, VRAM, WRAM)
		std::cout << std::hex << (int)Read(i) << " ";
	}
	std::cout << std::endl;
}

void Command::NOP() { return; }
// LD r, r'
void Command::LD(u8& dest, u8 src) {
	dest = src;
}
// LD rr, nn
void Command::LD(u16& dest, u16 val) {
	dest = val;
}
// LD (HL), A
void Command::LD_Mem(Memory_Bus& bus, u16 addr, u8 val) {
	bus.Write(addr, val);
}
void Command::JP(u16& PC, u16 address) {
	PC = address;
}
void Command::JR(u16& PC, s8 offset, bool condition) {
	if (condition) {
		PC += offset;
	}
}
// XOR n
void Command::XOR(u8& A, u8 val, Flags& flags) {
	A ^= val;

	flags.SetZ(A == 0);
	flags.SetN(false);
	flags.SetH(false);
	flags.SetC(false);
}
// DEC r
void Command::DEC(u8& reg, Flags& flags) {
	// Se calcula el Half Carry ANTES de restar
	// (reg & 0x0F) == 0 significa que va a haber borrow del bit 4
	bool half_carry = (reg & 0x0F) == 0x00;

	reg--;

	flags.SetZ(reg == 0);
	flags.SetN(true);
	flags.SetH(half_carry);
}
// DEC (HL)
void Command::DEC_Mem(Memory_Bus& bus, u16 addr, Flags& flags) {
	u8 val = bus.Read(addr);

	bool h = (val & 0x0F) == 0;
	val--;

	bus.Write(addr, val);

	flags.SetZ(val == 0);
	flags.SetN(true);
	flags.SetH(h);
}
void Command::DI(bool& ime_flag) {
	ime_flag = false;
}
// LDI (HL), A
void Command::LDI_Write(Memory_Bus& bus, u16& HL, u8 val) {
	bus.Write(HL, val);
	HL++;
}
// A, LDI (HL)
void Command::LDI_Read(Memory_Bus& bus, u16& HL, u8& dest) {
	dest = bus.Read(HL);
	HL++;
}



void Processor::Init() {
	reg.Init();
}

void Processor::Step() {
	// FETCH
	current_opcode = bus.Read(reg.val.PC);
	reg.val.PC++;

	Execute();
}

u16 Processor::Fetch16() {
	u8 low = bus.Read(reg.val.PC);
	reg.val.PC++;

	u8 high = bus.Read(reg.val.PC);
	reg.val.PC++;

	return (high << 8) | low;
}

void Processor::Execute() {
	switch (current_opcode) {
		case 0x00: com.NOP(); break;
		case 0x01: com.LD(reg.val.BC, Fetch16()); break;

		// case 0x01: com.LD16(reg.BC, Memory_Bus::GetLSBF(bus)); break;
		// case 0x05: com.DEC8(reg_select::B, reg.BC, bus); break;
		// case 0x06: com.LD8(reg_select::B, reg.BC, bus.GetMemoryAt(reg.PC.Get())); break;
		// case 0x0a: com.LD8(reg_select::A, reg.AF, bus.GetMemoryAt(reg.BC.Get())); break;
		// case 0x0d: com.DEC8(reg_select::C, reg.BC, bus); break;
		// case 0x0e: com.LD8(reg_select::C, reg.BC, bus.GetMemoryAt(reg.PC.Get())); break;
		// case 0x11: com.LD16(reg.DE, Memory_Bus::GetLSBF(bus)); break;
		// case 0x16: com.LD8(reg_select::D, reg.DE, bus.GetMemoryAt(reg.PC.Get())); break;
		// case 0x1e: com.LD8(reg_select::E, reg.DE, bus.GetMemoryAt(reg.PC.Get())); break;
		// case 0x20: com.JR(bus, reg.flag.Z.Get() == reset); break;
		// case 0x21: com.LD16(reg.HL, Memory_Bus::GetLSBF(bus)); break;
		// case 0x22: com.LDI(reg_select::HL, &bus); break;
		// case 0x26: com.LD8(reg_select::H, reg.HL, bus.GetMemoryAt(reg.PC.Get())); break;
		// case 0x2e: com.LD8(reg_select::L, reg.HL, bus.GetMemoryAt(reg.PC.Get())); break;
		// case 0x31: com.LD16(reg.SP, Memory_Bus::GetLSBF(bus)); break;
		// case 0x3e: com.LD8(reg_select::A, reg.AF, opcode); break;
		// case 0xaf: com.XOR(reg_select::A, reg.AF); break;
		// case 0xc3: com.JP(bus); break;
		// case 0xea: com.LD8(reg_select::NN, reg.NN, reg.AF.GetFirst(), &bus); break;
		// case 0xf3: com.DI(bus); break;
	}
}