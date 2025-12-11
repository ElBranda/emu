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
	std::cout << " [DEBUG] Intentando abrir ROM en ruta: " << path << std::endl;
	
	std::ifstream file(path, std::ios::binary | std::ios::ate);

	if (!file.is_open()) {
		std::cout << " [ERROR] file.is_open() dio FALSE. El archivo no existe o esta bloqueado." << std::endl;
		return false;
	}

	std::streamsize size = file.tellg();
	std::cout << " [DEBUG] file.tellg() devolvio: " << size << std::endl;

	if (size <= 0) {
        std::cout << " [ERROR] El archivo tiene tamanio 0 o es invalido." << std::endl;
        return false;
    }

	file.seekg(0, std::ios::beg);

	rom.resize(size);

	std::cout << " [DEBUG] Vector redimensionado a: " << rom.size() << " bytes." << std::endl;

	if (file.read((char*)rom.data(), size)) {
		std::cout << " [EXITO] ROM cargada en memoria correctamente." << std::endl;

		return true;
	}

	std::cout << " [ERROR] Fallo al leer los datos con file.read()." << std::endl;
	return false;
}
u8 Memory_Bus::Read(u16 address) {
	if (address == 0xFF44) {
        return 0x94; 
    }
	if (address < 0x8000) {
		// Cartucho (ROM)
		// Si la ROM es chica, evita el crash
		if (rom.empty() || address >= rom.size()) return 0xFF;
		return rom[address];
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
int Memory_Bus::GetRomSize() {
	return rom.size();
}

void Command::NOP() { }
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

void Command::CP(u8 A, u8 val, Flags& flags) {
	flags.SetZ(A == val);

	flags.SetN(true);

	flags.SetH((A & 0x0F) < (val & 0x0F));

	flags.SetC(A < val);
}

void Command::PUSH(Memory_Bus& bus, u16& SP, u16 val) {
	SP--;
	bus.Write(SP, (val >> 8) & 0xFF);
	SP--;
	bus.Write(SP, val & 0xFF);
}

void Command::CALL(Memory_Bus& bus, u16& SP, u16& PC, u16 target_addr) {
	PUSH(bus, SP, PC);

	PC = target_addr;
}




void Processor::Init() {
	reg.Init();
}

u8 Processor::Step() {
	u16 curr_pc = reg.val.PC;

	// FETCH
	u8 opcode = bus.Read(reg.val.PC++);

	// LOG
	std::cout << std::hex << std::uppercase << std::setfill('0')
			  << "PC:0x" << std::setw(4) << curr_pc
			  << " | OP:0x" << std::setw(2) << (int)opcode
			  << " | AF:0x" << std::setw(4) << reg.val.AF
              << " | BC:0x" << std::setw(4) << reg.val.BC
              << " | HL:0x" << std::setw(4) << reg.val.HL
              << " | SP:0x" << std::setw(4) << reg.val.SP
			  << " | Z:" << reg.flag.Z()
			  << " | H:" << reg.flag.H()
			  << " | N:" << reg.flag.N()
			  << " | C:" << reg.flag.C()
			  << " | IME:" << IME
			  << std::endl;

	u8 cycles = Execute(opcode);

	return cycles;
}

u16 Processor::Fetch16() {
	u8 low = bus.Read(reg.val.PC);
	reg.val.PC++;

	u8 high = bus.Read(reg.val.PC);
	reg.val.PC++;

	return (high << 8) | low;
}

u8 Processor::Execute(u8 opcode) {
	switch (opcode) {
		case 0x00: com.NOP(); return 1;
		case 0x01: com.LD(reg.val.BC, Fetch16()); return 3;
		case 0x05: com.DEC(reg.val.B, reg.flag); return 1;
		case 0x0d: com.DEC(reg.val.C, reg.flag); return 1;
		case 0x11: com.LD(reg.val.DE, Fetch16()); return 3;
		case 0x20: {
			s8 offset = (s8)bus.Read(reg.val.PC++);
			if (!reg.flag.Z()) {
				com.JR(reg.val.PC, offset, true);
				return 3;
			}
			return 2;
		}
		case 0x21: com.LD(reg.val.HL, Fetch16()); return 3;
		case 0x22: com.LDI_Write(bus, reg.val.HL, reg.val.A); return 2;
		case 0x31: com.LD(reg.val.SP, Fetch16()); return 3;
		case 0x3e: com.LD(reg.val.A, bus.Read(reg.val.PC++)); return 2;
		case 0x40: com.LD(reg.val.B, reg.val.B); return 1;
		case 0xaf: com.XOR(reg.val.A, reg.val.A, reg.flag); return 1;
		case 0xc3: com.JP(reg.val.PC, Fetch16()); return 3;
		case 0xc5: com.PUSH(bus, reg.val.SP, reg.val.BC); return 4;
		case 0xcd: com.CALL(bus, reg.val.SP, reg.val.PC, Fetch16()); return 6;
		case 0xd5: com.PUSH(bus, reg.val.SP, reg.val.DE); return 4;
		case 0xe0: {
			u8 offset = bus.Read(reg.val.PC++);
			u16 address = 0xFF00 + offset;
			com.LD_Mem(bus, address, reg.val.A);
			return 3;
		}
		case 0xe5: com.PUSH(bus, reg.val.SP, reg.val.HL); return 4;
		case 0xea: com.LD_Mem(bus, Fetch16(), reg.val.A); return 4;
		case 0xf0: {
			u8 offset = bus.Read(reg.val.PC++);
			u16 address = 0xFF00 + offset;
			com.LD(reg.val.A, bus.Read(address));
			return 3;
		}
		case 0xf3: com.DI(IME); return 1;
		case 0xf5: com.PUSH(bus, reg.val.SP, reg.val.AF); return 4;
		case 0xfa: {
			u16 address = Fetch16();
			u8 val = bus.Read(address);
			com.LD(reg.val.A, val);
			return 4;
		}
		case 0xfe: {
			u8 n = bus.Read(reg.val.PC++);
			com.CP(reg.val.A, n, reg.flag);
			return 2;
		}

		// case 0x01: com.LD16(reg.BC, Memory_Bus::GetLSBF(bus)); break;
		// case 0x06: com.LD8(reg_select::B, reg.BC, bus.GetMemoryAt(reg.PC.Get())); break;
		// case 0x0a: com.LD8(reg_select::A, reg.AF, bus.GetMemoryAt(reg.BC.Get())); break;
		// case 0x0e: com.LD8(reg_select::C, reg.BC, bus.GetMemoryAt(reg.PC.Get())); break;
		// case 0x11: com.LD16(reg.DE, Memory_Bus::GetLSBF(bus)); break;
		// case 0x16: com.LD8(reg_select::D, reg.DE, bus.GetMemoryAt(reg.PC.Get())); break;
		// case 0x1e: com.LD8(reg_select::E, reg.DE, bus.GetMemoryAt(reg.PC.Get())); break;
		// case 0x26: com.LD8(reg_select::H, reg.HL, bus.GetMemoryAt(reg.PC.Get())); break;
		// case 0x2e: com.LD8(reg_select::L, reg.HL, bus.GetMemoryAt(reg.PC.Get())); break;
	}

	return 0;
}
bool Processor::LoadROM(const char* path) {
	return bus.LoadROM(path);
}
int Processor::GetRomSize() {
	return bus.GetRomSize();
}