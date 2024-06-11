#include <fstream>
#include <iomanip>
#include "CPU.hpp"

using namespace CPU;

u8 CPU::opcode;
enum CPU::reg_select;
Register CPU::reg;

u16 Register16::Get() const { return ((first_val << 8) + last_val); }
u16 Register16::GetFirst() const { return first_val; }
u16 Register16::GetLast() const { return last_val; }
void Register16::Set(u16 val) { first_val = (val >> 8); last_val = (val & 0xff); }
void Register16::SetFirst(u8 val) { first_val = val; };
void Register16::SetLast(u8 val) { last_val = val; };
void Register16::SetVirtual(u8 f_val, u8 l_val) { first_val = f_val; last_val = l_val; }
void Register16::Increment(u8 val) {
	u16 reg = ((first_val << 8) + last_val + val);
	first_val = ((reg & 0xff00) >> 8);
	last_val = (reg & 0xff);
}
void Register16::Decrement(u8 val) {
	u16 reg = ((first_val << 8) + last_val - val);
	first_val = ((reg & 0xff00) >> 8);
	last_val = (reg & 0xff);
}

flg RegisterFlag::Get() const { return value; }
void RegisterFlag::Set(flg val) { value = val; }

void Register::Init() {
	AF.Set(0x01b0);
	BC.Set(0x0013);
	DE.Set(0x00d8);
	HL.Set(0x014d);
	SP.Set(0xfffe);
	PC.Set(0x0100);
	flag.Z.Set(set);
	flag.N.Set(reset);
	flag.H.Set(set);
	flag.C.Set(set);
}

void Memory_Bus::Fetch() {
	opcode = GetMemoryAt(reg.PC.Get());

	//std::cout << std::endl << std::hex << "IN FETCH: " << static_cast<int>(reg.PC.Get()) << std::endl;

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
void Memory_Bus::SetMemory(u16 memory_pos, u8 data) { memory[memory_pos] = data; }
void Memory_Bus::Execute(Memory_Bus& bus) {
	Command com;

	switch (opcode) {
	case 0x00: com.NOP(); break;
	case 0x01: com.LD16(reg.BC, Memory_Bus::GetLSBF(bus)); break;
	case 0x05: com.DEC8(reg_select::B, reg.BC, bus); break;
	case 0x06: com.LD8(reg_select::B, reg.BC, bus.GetMemoryAt(reg.PC.Get())); break;
	case 0x0a: com.LD8(reg_select::A, reg.AF, bus.GetMemoryAt(reg.BC.Get())); break;
	case 0x0d: com.DEC8(reg_select::C, reg.BC, bus); break;
	case 0x0e: com.LD8(reg_select::C, reg.BC, bus.GetMemoryAt(reg.PC.Get())); break;
	case 0x11: com.LD16(reg.DE, Memory_Bus::GetLSBF(bus)); break;
	case 0x16: com.LD8(reg_select::D, reg.DE, bus.GetMemoryAt(reg.PC.Get())); break;
	case 0x1e: com.LD8(reg_select::E, reg.DE, bus.GetMemoryAt(reg.PC.Get())); break;
	case 0x20: com.JR(bus, reg.flag.Z.Get() == reset); break;
	case 0x21: com.LD16(reg.HL, Memory_Bus::GetLSBF(bus)); break;
	case 0x22: com.LDI(reg_select::HL, &bus); break;
	case 0x26: com.LD8(reg_select::H, reg.HL, bus.GetMemoryAt(reg.PC.Get())); break;
	case 0x2e: com.LD8(reg_select::L, reg.HL, bus.GetMemoryAt(reg.PC.Get())); break;
	case 0x31: com.LD16(reg.SP, Memory_Bus::GetLSBF(bus)); break;
	case 0x3e: com.LD8(reg_select::A, reg.AF, opcode); break;
	case 0xaf: com.XOR(reg_select::A, reg.AF); break;
	case 0xc3: com.JP(bus); break;
	case 0xea: com.LD8(reg_select::NN, reg.NN, reg.AF.GetFirst(), &bus); break;
	case 0xf3: com.DI(bus); break;
	}
}
u16 Memory_Bus::GetLSBF(Memory_Bus& bus) {
	u8 last = bus.GetMemoryAt(reg.PC.Get());

	bus.Fetch();

	u8 first = bus.GetMemoryAt(reg.PC.Get());

	bus.Fetch();

	return ((first << 8) + last);
}
void Memory_Bus::SetIMEDisable() { IME = disable; }

void Command::NOP() { return; }
void Command::LD8(char reg_name, Register16& reg, u8 val, Memory_Bus* bus) {
	switch (reg_name) {
	case 0: reg.SetFirst(val); break;
	case 1: reg.SetLast(val); break;
	case 2: {
		u16 mem_pos = Memory_Bus::GetLSBF(*bus);

		if (mem_pos > ROM_LIMIT) bus->SetMemory(mem_pos, val);

		break;
	}
	case 3: {
		if (reg.Get() > ROM_LIMIT) {
			bus->SetMemory(reg.Get(), val);
		}

		break;
	}
	}
}
void Command::LD16(Register16& reg, u16 val, Memory_Bus* bus) {
	reg.Set(val);
}
void Command::JP(Memory_Bus& bus) {
	u8 last = bus.GetMemoryAt(reg.PC.Get());
	u8 first = bus.GetMemoryAt(reg.PC.Get()+1);

	//std::cout << std::endl << std::hex << "IN JP: " << std::endl << "first: " << static_cast<int>(first) << "\tlast: " << static_cast<int>(last) << std::endl;

	reg.PC.SetVirtual(first, last);
}
void Command::DI(Memory_Bus& bus) { bus.SetIMEDisable(); }
void Command::XOR(char reg_name, Register16 regis) {
	switch (reg_name) {
	case 0: reg.AF.SetFirst(reg.AF.GetFirst() ^ regis.GetFirst()); break;
	case 1: reg.AF.SetFirst(reg.AF.GetFirst() ^ regis.GetLast()); break;
	}

	if (reg.AF.GetFirst() == 0x00) reg.flag.Z.Set(set);
	reg.flag.N.Set(reset);
	reg.flag.H.Set(reset);
	reg.flag.C.Set(reset);
}
void Command::LDI(char reg_name, Memory_Bus* bus) {
	//std::cout << std::endl << std::hex << "IN MEM: " << static_cast<int>(bus->GetMemoryAt(reg.HL.Get())) << "A: " << static_cast<int>(reg.AF.GetFirst()) << std::endl;

	switch (reg_name) {
	case 0: LD8(reg_name, reg.AF, bus->GetMemoryAt(reg.HL.Get())); break;
	case 3: LD8(reg_name, reg.HL, reg.AF.GetFirst(), bus); break;
	}
	
	//std::cout << std::endl << std::hex << "IN MEM: " << static_cast<int>(bus->GetMemoryAt(reg.HL.Get())) << "A: " << static_cast<int>(reg.AF.GetFirst()) << std::endl;

	reg.HL.Increment(1);
}
void Command::DEC8(char reg_name, Register16& regis, Memory_Bus& bus) {
	switch (reg_name) {
		
	case 0: {
		regis.SetFirst(regis.GetFirst() - 1);
		if (regis.GetFirst() == 0x00) reg.flag.Z.Set(set);
		else reg.flag.Z.Set(reset);
		if ((regis.GetLast() & 0x0f) == 0x0f) reg.flag.H.Set(set);

		

		
		break;
	}
	case 1: {
		regis.SetLast(regis.GetLast() - 1);
		if (regis.GetLast() == 0x00) reg.flag.Z.Set(set);
		else reg.flag.Z.Set(reset);
		if ((regis.GetLast() & 0x0f) == 0x0f) reg.flag.H.Set(set);

		

		
		break;
	}
	case 3: {
		bus.SetMemory(regis.Get(), bus.GetMemoryAt(regis.Get()) - 1);
		if (bus.GetMemoryAt(regis.Get()) == 0x00) reg.flag.Z.Set(set);
		else reg.flag.Z.Set(reset);
		if ((bus.GetMemoryAt(regis.Get()) & 0x0f) == 0x0f) reg.flag.H.Set(set);

		


		break;
	}
	}

	
	reg.flag.N.Set(set);
}
void Command::JR(Memory_Bus& bus, flg condition) {
	s8 pos = bus.GetMemoryAt(reg.PC.Get());

	//std::cout << std::endl << std::hex << "pos: " << static_cast<int>(pos + 1 + reg.PC.Get()) << std::endl;

	if (condition) reg.PC.Set(pos+1+reg.PC.Get());

	//std::cout << std::endl << std::hex << "IN JR: " << static_cast<int>(reg.PC.Get()) << std::endl;
}