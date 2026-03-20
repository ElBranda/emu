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
	rom.resize(0x8000, 0);
	std::fill(std::begin(vram), std::end(vram), 0);
    std::fill(std::begin(wram), std::end(wram), 0);
    std::fill(std::begin(hram), std::end(hram), 0);
    std::fill(std::begin(io), std::end(io), 0);
	std::fill(std::begin(oam), std::end(oam), 0);
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

        u8 cart_type = rom[0x0147];
        u8 ram_size_code = rom[0x0149];
        
        has_battery = (cart_type == 0x03 || cart_type == 0x06 || cart_type == 0x09 || cart_type == 0x0D || cart_type == 0x0F || cart_type == 0x10 || cart_type == 0x13 || cart_type == 0x1B || cart_type == 0x1E);

        int ram_size = 0;
        switch(ram_size_code) {
            case 0x02: ram_size = 0x2000; break;  
            case 0x03: ram_size = 0x8000; break;  
            case 0x04: ram_size = 0x20000; break; 
            case 0x05: ram_size = 0x10000; break; 
        }
        external_ram.resize(ram_size, 0);

        save_path = std::string(path);
        size_t dot_pos = save_path.find_last_of(".");
        if (dot_pos != std::string::npos) save_path = save_path.substr(0, dot_pos) + ".sav";
        else save_path += ".sav";

        if (has_battery && ram_size > 0) {
            std::ifstream sav_file(save_path, std::ios::binary);
            if (sav_file.is_open()) {
                sav_file.read((char*)external_ram.data(), external_ram.size());
                std::cout << " [EXITO] Partida cargada de: " << save_path << std::endl;
            }
        }
        return true;
	}

	std::cout << " [ERROR] Fallo al leer los datos con file.read()." << std::endl;
	return false;
}
u8 Memory_Bus::Read(u16 address) {
    if (address < 0x4000) {
        if (rom.empty()) return 0xFF;
        return rom[address];
    } else if (address >= 0x4000 && address < 0x8000) {
        if (rom.empty()) return 0xFF;
        int offset = (rom_bank * 0x4000) + (address - 0x4000);
        if (offset < rom.size()) return rom[offset];
        return 0xFF;
    } else if (address >= 0x8000 && address < 0xA000) {
        return vram[address - 0x8000];

    } else if (address >= 0xA000 && address < 0xC000) {
        if (ram_enabled && !external_ram.empty()) {
            int offset = (ram_bank * 0x2000) + (address - 0xA000);
            if (offset < external_ram.size()) return external_ram[offset];
        }
        return 0xFF; 
        
    } else if (address >= 0xC000 && address < 0xE000) {
        return wram[address - 0xC000];

    } else if (address >= 0xE000 && address < 0xFE00) {
        return wram[address - 0xE000]; 
        
    } else if (address >= 0xFE00 && address < 0xFEA0) {
        return oam[address - 0xFE00];
    } else if (address >= 0xFF00 && address < 0xFF80) {
        if (address == 0xFF00) {
            u8 selection = io[0x00];
            u8 state = 0x0F; 
            
            if (!(selection & 0x10)) state &= joypad_dir;
            if (!(selection & 0x20)) state &= joypad_action;
            
            return (selection & 0x30) | state | 0xC0; 
        }
        return io[address - 0xFF00];
    } else if (address >= 0xFF80 && address < 0xFFFF) {
        return hram[address - 0xFF80];
    } else if (address == 0xFFFF) {
        return ie_register;
    }

    return 0xFF;
}
void Memory_Bus::Write(u16 address, u8 value) {
    if (address < 0x8000) {
        if (address < 0x2000) {
            ram_enabled = ((value & 0x0F) == 0x0A);
        } else if (address >= 0x2000 && address < 0x4000) {
            rom_bank = value;
            if (rom_bank == 0) rom_bank = 1; 
        } else if (address >= 0x4000 && address < 0x6000) {
            ram_bank = value & 0x03;
        }
        return; 
        
    } else if (address >= 0x8000 && address < 0xA000) {
        vram[address - 0x8000] = value;
        
    } else if (address >= 0xA000 && address < 0xC000) {
        if (ram_enabled && !external_ram.empty()) {
            int offset = (ram_bank * 0x2000) + (address - 0xA000);
            if (offset < external_ram.size()) {
                external_ram[offset] = value;
            }
        }
        
    } else if (address >= 0xC000 && address < 0xE000) {
        wram[address - 0xC000] = value;
    } else if (address >= 0xE000 && address < 0xFE00) {
        wram[address - 0xE000] = value; 
    } else if (address >= 0xFE00 && address < 0xFEA0) {
        oam[address - 0xFE00] = value;
    } else if (address >= 0xFF00 && address < 0xFF80) {
        if (address == 0xFF44) return; 
        
        if (address == 0xFF41) { 
            u8 current_stat = io[0x41];
            io[0x41] = (value & 0xF8) | (current_stat & 0x07);
            return;
        }

        if (address == 0xFF04) {
            io[0x04] = 0;
            div_counter = 0;
            return;
        }

        if (address == 0xFF00) {
            io[0x00] = (io[0x00] & 0xCF) | (value & 0x30);
            return;
        }

        io[address - 0xFF00] = value;

        if (address == 0xFF46) {
            u16 source = value << 8;
            for (int i = 0; i < 0xA0; i++) {
                oam[i] = Read(source + i); 
            }
        }
    } else if (address >= 0xFF80 && address < 0xFFFF) {
        hram[address - 0xFF80] = value;
    } else if (address == 0xFFFF) {
        ie_register = value;
    }
}
void Memory_Bus::ShowMemory(u16 start, u16 end) {
	for (int i = start; i <= end; i++) {
		if (i % 16 == 0) std::cout << "\n" << std::hex << i << ": ";
		std::cout << std::hex << (int)Read(i) << " ";
	}
	std::cout << std::endl;
}
int Memory_Bus::GetRomSize() {
	return rom.size();
}
void Memory_Bus::UpdateJoypad(int key, bool pressed) {
    u8 bit = 1 << (key % 4); 
    bool is_dir = key < 4;   
    
    u8& target = is_dir ? joypad_dir : joypad_action;
    bool was_unpressed = (target & bit) != 0;

    if (pressed) {
        target &= ~bit; 
        
        if (was_unpressed) {
            RequestInterrupt(4);
        }
    } else {
        target |= bit; 
    }
}
void Memory_Bus::SaveGame() {
    if (has_battery && !external_ram.empty() && !save_path.empty()) {
        std::ofstream sav_file(save_path, std::ios::binary);
        if (sav_file.is_open()) {
            sav_file.write((char*)external_ram.data(), external_ram.size());
            std::cout << " [EXITO] Partida guardada en: " << save_path << std::endl;
        }
    }
}

void Command::NOP() { }

void Command::LD(u8& dest, u8 src) {
	dest = src;
}
void Command::LD(u16& dest, u16 val) {
	dest = val;
}
void Command::LD_Mem(Memory_Bus& bus, u16 addr, u8 val) {
	bus.Write(addr, val);
}
void Command::LDD_Read(Memory_Bus& bus, u16& HL, u8& A) {
	A = bus.Read(HL);

    HL--; 
}
void Command::LDD_Write(Memory_Bus& bus, u8& A, u16& HL) {
	bus.Write(HL, A);

	HL--;
}
void Command::LDHL(u16& HL, u16 SP, s8 n, Flags& flags) {
    int result = SP + n;
    
    HL = (u16)result;

    flags.SetZ(false);
    flags.SetN(false);
    
    u16 check_n = (u8)n;
    
    flags.SetH(((SP & 0x0F) + (check_n & 0x0F)) > 0x0F);
    flags.SetC(((SP & 0xFF) + (check_n & 0xFF)) > 0xFF);
}
void Command::JP(u16& PC, u16 address) {
	PC = address;
}
void Command::JR(u16& PC, s8 offset, bool condition) {
	if (condition) {
		PC += offset;
	}
}
void Command::OR(u8& A, u8 val, Flags& flags) {
	A |= val;

	flags.SetZ(A == 0);
	flags.SetN(false);
	flags.SetH(false);
	flags.SetC(false);
}
void Command::XOR(u8& A, u8 val, Flags& flags) {
	A ^= val;

	flags.SetZ(A == 0);
	flags.SetN(false);
	flags.SetH(false);
	flags.SetC(false);
}
void Command::DEC(u16& reg) {
	reg--;
}
// DEC r
void Command::DEC(u8& reg, Flags& flags) {
	bool h = (reg & 0x0F) == 0x00;
    reg--;
    flags.SetZ(reg == 0);
    flags.SetN(true);
    flags.SetH(h);
}
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
void Command::LDI_Write(Memory_Bus& bus, u16& HL, u8 val) {
	bus.Write(HL, val);
	HL++;
}
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
void Command::INC(u8& reg, Flags& flags) {
	bool h = (reg & 0x0F) == 0x0F; 
    reg++;
    flags.SetZ(reg == 0);
    flags.SetN(false);
    flags.SetH(h);
}
void Command::INC(u16& reg) {
	reg++;
}

void Command::AND(u8& A, u8 val, Flags& flags) {
	A &= val;

	flags.SetZ(A == 0);
	flags.SetN(false);
	flags.SetH(true);
	flags.SetC(false);
}

void Command::RST(Memory_Bus& bus, u16& SP, u16& PC, u16 target_addr) {
	CALL(bus, SP, PC, target_addr);
}

void Command::ADD(u8& A, u8 val, Flags& flags) {
	int res = A + val;

	flags.SetZ((res & 0xFF) == 0);
	flags.SetN(false);
	flags.SetH(((A & 0x0F) + (val & 0x0F)) > 0x0F);
	flags.SetC(res > 0xFF);

	A = (u8)res;
}

void Command::POP(Memory_Bus& bus, u16& SP, u16& dest_reg_pair) {
	u8 low = bus.Read(SP);
	//std::cout << std::hex << low << std::endl; 
	SP++;
	u8 high = bus.Read(SP);
	//std::cout << std::hex << high << std::endl; 
	SP++;
	dest_reg_pair = (high << 8) | low;
}

void Command::ADD_HL(u16& HL, u16 n, Flags& flags) {
	int res = HL + n;

	flags.SetN(false);
	flags.SetH(((HL & 0x0FFF) + (n & 0x0FFF)) > 0x0FFF);
	flags.SetC(res > 0xFFFF);

	HL = (u16)res;
}
void Command::ADC(u8& A, u8 val, Flags& flags) {
    int carry_in = flags.C() ? 1 : 0;

    int result = A + val + carry_in;

    flags.SetZ((result & 0xFF) == 0);

    flags.SetN(false);

    flags.SetH(((A & 0x0F) + (val & 0x0F) + carry_in) > 0x0F);

    flags.SetC(result > 0xFF);

    A = (u8)result;
}
void Command::SUB(u8& A, u8 val, Flags& flags) {
    int result = A - val;

    flags.SetZ((result & 0xFF) == 0);

    flags.SetN(true);

    flags.SetH((A & 0x0F) < (val & 0x0F));

    flags.SetC(A < val);

    A = (u8)result;
}
void Command::SBC(u8& A, u8 val, Flags& flags) {
    int carry_in = flags.C() ? 1 : 0;
    int result = A - val - carry_in;

    flags.SetZ((result & 0xFF) == 0);
    flags.SetN(true); 
    
    flags.SetH(((A & 0x0F) - (val & 0x0F) - carry_in) < 0);
    
    flags.SetC(result < 0);

    A = (u8)result;
}
void Command::SWAP(u8& reg, Flags& flags) {
    reg = (reg >> 4) | (reg << 4);

    flags.SetZ(reg == 0); 
    flags.SetN(false);    
    flags.SetH(false);    
    flags.SetC(false);    
}
void Command::RLC(u8& reg, Flags& flags) {
    bool bit7 = (reg >> 7) & 1;

    reg = (reg << 1) | (bit7 ? 1 : 0);

    flags.SetZ(reg == 0); 
    flags.SetN(false);
    flags.SetH(false);
    flags.SetC(bit7);   
}
void Command::RL(u8& reg, Flags& flags) {
    bool old_carry = flags.C();        
    bool new_carry = (reg >> 7) & 1;  

    reg = (reg << 1) | (old_carry ? 1 : 0);

    flags.SetZ(reg == 0);
    flags.SetN(false);
    flags.SetH(false);
    flags.SetC(new_carry);
}
void Command::RR(u8& reg, Flags& flags) {
    bool old_carry = flags.C();     
    bool new_carry = reg & 0x01;     

    reg = (reg >> 1) | (old_carry ? 0x80 : 0x00);

    flags.SetZ(reg == 0); 
    flags.SetN(false);
    flags.SetH(false);
    flags.SetC(new_carry);
}
void Command::RRC(u8& reg, Flags& flags) {
    bool bit0 = reg & 0x01;

    reg = (reg >> 1) | (bit0 ? 0x80 : 0x00);

    flags.SetZ(reg == 0);
    flags.SetN(false);
    flags.SetH(false);
    flags.SetC(bit0);     
}
void Command::SLA(u8& reg, Flags& flags) {
    bool bit7 = (reg >> 7) & 1;

    reg = reg << 1;

    flags.SetZ(reg == 0);
    flags.SetN(false);
    flags.SetH(false);
    flags.SetC(bit7);
}
void Command::SRA(u8& reg, Flags& flags) {
    bool bit0 = reg & 0x01;

    reg = (reg >> 1) | (reg & 0x80);

    flags.SetZ(reg == 0);
    flags.SetN(false);
    flags.SetH(false);
    flags.SetC(bit0);
}
void Command::SRL(u8& reg, Flags& flags) {
    bool bit0 = reg & 0x01;

    reg = reg >> 1;

    flags.SetZ(reg == 0);
    flags.SetN(false);
    flags.SetH(false);
    flags.SetC(bit0);
}
void Command::BIT(u8 val, u8 bit, Flags& flags) {
    bool is_set = (val >> bit) & 1;

    flags.SetZ(!is_set); 
    flags.SetN(false);
    flags.SetH(true);   
}



















void Processor::Init() {
	reg.Init();
	IME = false;
	bus.Write(0xFF40, 0x91); // LCDC: Prende la pantalla y el fondo
    bus.Write(0xFF41, 0x85); // STAT: Estado del LCD
    bus.Write(0xFF42, 0x00); // SCY: Scroll Y
    bus.Write(0xFF43, 0x00); // SCX: Scroll X
    bus.Write(0xFF47, 0xFC); // BGP: Paleta de colores del fondo (11 11 11 00)
	bus.Write(0xFF44, 0x90);
	halted = false;
}

u8 Processor::Step() {
	if (halted) {
        u8 IF = bus.Read(0xFF0F);
        u8 IE = bus.Read(0xFFFF);
        if ((IF & IE) > 0) {
            halted = false;
        } else {
			bus.TickTimer(1);
            return 1;
        }
    }

	u16 curr_pc = reg.val.PC;

	if (reg.val.PC >= 0xFF00 && reg.val.PC < 0xFF80) {
        std::cout << ">>> CRASH DETECTADO: PC entró en IO (0x" << std::hex << reg.val.PC << ") <<<" << std::endl;
        return 0;
    }

	// if (reg.val.SP >= 0xA000 && reg.val.SP < 0xC000) {
    //     // std::cout << "DEBUG: SP corregido de " << reg.val.SP << " a 0xDFFF" << std::endl;
    //     reg.val.SP = 0xDFFF; 
    // }

	if (reg.val.PC >= 0x8000 && reg.val.PC < 0x9FFF) {
        std::cout << ">>> CRASH: EL CPU SALTÓ A VRAM! <<<" << std::endl;
        return 0; 
    }

	// FETCH
	u8 opcode = bus.Read(reg.val.PC++);

	if (opcode == 0xC3 || opcode == 0xCD) { 
		u8 low = bus.Read(reg.val.PC);
		u8 high = bus.Read(reg.val.PC + 1);
		u16 target = (high << 8) | low;
		
		if (target == 0xFF00) {
			std::cout << ">>> CRASH INMINENTE: Opcode " << std::hex << (int)opcode 
					<< " en PC: " << reg.val.PC - 1 
					<< " intenta saltar a 0xFF00! <<<" << std::endl;
			return 0;
		}
	}

	// std::cout << "PC:" << std::hex << (reg.val.PC - 1) 
    //           << " | OP:" << (int)opcode << std::endl;

	// LOG
	// std::cout << std::hex << std::uppercase << std::setfill('0')
	// 		  << "PC:0x" << std::setw(4) << curr_pc
	// 		  << " | OP:0x" << std::setw(2) << (int)opcode
	// 		  << " | AF:0x" << std::setw(4) << reg.val.AF
    //           << " | BC:0x" << std::setw(4) << reg.val.BC
    //           << " | HL:0x" << std::setw(4) << reg.val.HL
    //           << " | SP:0x" << std::setw(4) << reg.val.SP
	// 		  << " | Z:" << reg.flag.Z()
	// 		  << " | H:" << reg.flag.H()
	// 		  << " | N:" << reg.flag.N()
	// 		  << " | C:" << reg.flag.C()
	// 		  << " | IME:" << IME
	// 		  << std::endl;

	u8 cycles = Execute(opcode);

	bus.TickTimer(cycles);

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
		case 0x02: com.LD_Mem(bus, reg.val.BC, reg.val.A); return 2;
		case 0x03: com.INC(reg.val.BC); return 2;
		case 0x04: com.INC(reg.val.B, reg.flag); return 1;
		case 0x05: com.DEC(reg.val.B, reg.flag); return 1;
		case 0x06: com.LD(reg.val.B, bus.Read(reg.val.PC++)); return 2;
		case 0x07: {
			u8 a = reg.val.A;
			bool bit7 = (a >> 7) & 1;
			reg.val.A = (a << 1) | bit7;
			reg.flag.SetZ(false); 
			reg.flag.SetN(false);
			reg.flag.SetH(false);
			reg.flag.SetC(bit7);
			return 1;
		}
		case 0x08: {
			u16 address = Fetch16();
			u8 low = reg.val.SP & 0xFF;
			bus.Write(address, low);
			u8 high = (reg.val.SP >> 8) & 0xFF;
			bus.Write(address + 1, high);
			return 5;
		}
		case 0x09: com.ADD_HL(reg.val.HL, reg.val.BC, reg.flag); return 2;
		case 0x0a: com.LD(reg.val.A, bus.Read(reg.val.BC)); return 2;
		case 0x0b: com.DEC(reg.val.BC); return 2;
		case 0x0c: com.INC(reg.val.C, reg.flag); return 1;
		case 0x0d: com.DEC(reg.val.C, reg.flag); return 1;
		case 0x0e: com.LD(reg.val.C, bus.Read(reg.val.PC++)); return 2;
		case 0x0F: {
			u8 a = reg.val.A;
			bool bit0 = a & 0x01;

			reg.val.A = (a >> 1) | (bit0 ? 0x80 : 0x00);

			reg.flag.SetZ(false); 
			reg.flag.SetN(false);
			reg.flag.SetH(false);
			reg.flag.SetC(bit0);  

			return 1; 
		}
		case 0x10: {
			reg.val.PC++; 

			return 1; 
		}
		case 0x11: com.LD(reg.val.DE, Fetch16()); return 3;
		case 0x12: com.LD_Mem(bus, reg.val.DE, reg.val.A); return 2;
		case 0x13: com.INC(reg.val.DE); return 2;
		case 0x14: com.INC(reg.val.D, reg.flag); return 1;
		case 0x15: com.DEC(reg.val.D, reg.flag); return 1;
		case 0x16: com.LD(reg.val.D, bus.Read(reg.val.PC++)); return 2;
		case 0x17: {
			bool old_carry = reg.flag.C();
			bool new_carry = (reg.val.A >> 7) & 1;
			reg.val.A = (reg.val.A << 1) | old_carry;
			reg.flag.SetZ(false);
			reg.flag.SetN(false);
			reg.flag.SetH(false);
			reg.flag.SetC(new_carry);
			return 1;
		}
		case 0x18: {
			s8 offset = (s8)bus.Read(reg.val.PC++);
			com.JR(reg.val.PC, offset, true);
			return 3;
		}
		case 0x19: com.ADD_HL(reg.val.HL, reg.val.DE, reg.flag); return 2;
		case 0x1a: com.LD(reg.val.A, bus.Read(reg.val.DE)); return 2;
		case 0x1b: com.DEC(reg.val.DE); return 2;
		case 0x1c: com.INC(reg.val.E, reg.flag); return 1;
		case 0x1d: com.DEC(reg.val.E, reg.flag); return 1;
		case 0x1e: com.LD(reg.val.E, bus.Read(reg.val.PC++)); return 2;
		case 0x1F: {
			u8 a = reg.val.A;
			bool old_carry = reg.flag.C();
			
			bool new_carry = a & 0x01;

			reg.val.A = (a >> 1) | (old_carry ? 0x80 : 0x00);

			reg.flag.SetZ(false); 
			reg.flag.SetN(false);
			reg.flag.SetH(false);
			reg.flag.SetC(new_carry);

			return 1; 
		}
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
		case 0x23: com.INC(reg.val.HL); return 2;
		case 0x24: com.INC(reg.val.H, reg.flag); return 1;
		case 0x25: com.DEC(reg.val.H, reg.flag); return 1;
		case 0x26: com.LD(reg.val.H, bus.Read(reg.val.PC++)); return 2;
		case 0x27: {
			u8 a = reg.val.A;
			int adjust = 0;
			if (reg.flag.H() || (!reg.flag.N() && (a & 0x0F) > 9)) {
				adjust |= 0x06;
			}
			if (reg.flag.C() || (!reg.flag.N() && a > 0x99)) {
				adjust |= 0x60;
				reg.flag.SetC(true);
			}
			if (reg.flag.N()) {
				a -= adjust;
			} else {
				a += adjust;
			}
			reg.flag.SetH(false);
			reg.flag.SetZ(a == 0);
			reg.val.A = a;
			return 1;
		}
		case 0x28: {
            s8 offset = (s8)bus.Read(reg.val.PC++);
            if (reg.flag.Z()) {
                com.JR(reg.val.PC, offset, true);
                return 3;
            }
            return 2;
        }
		case 0x29: com.ADD_HL(reg.val.HL, reg.val.HL, reg.flag); return 2;
		case 0x2a: com.LDI_Read(bus, reg.val.HL, reg.val.A); return 2;
		case 0x2b: com.DEC(reg.val.HL); return 2;
		case 0x2c: com.INC(reg.val.L, reg.flag); return 1;
		case 0x2d: com.DEC(reg.val.L, reg.flag); return 1;
		case 0x2e: com.LD(reg.val.L, bus.Read(reg.val.PC++)); return 2;
		case 0x2f: {
			reg.val.A = ~reg.val.A; 
			reg.flag.SetN(true);    
			reg.flag.SetH(true);   
			return 1;
		}
		case 0x30: { 
			s8 offset = (s8)bus.Read(reg.val.PC++);
			if (!reg.flag.C()) {
				com.JR(reg.val.PC, offset, true);
				return 3;
			}
			return 2;
		}
		case 0x31: com.LD(reg.val.SP, Fetch16()); return 3;
		case 0x32: com.LDD_Write(bus, reg.val.A, reg.val.HL); return 2;
		case 0x33: com.INC(reg.val.SP); return 2;
		case 0x34: {
			u8 val = bus.Read(reg.val.HL);
			com.INC(val, reg.flag);
			bus.Write(reg.val.HL, val);
			return 3;
		}
		case 0x35: {
			u8 val = bus.Read(reg.val.HL); 
			com.DEC(val, reg.flag);        
			bus.Write(reg.val.HL, val);    
			return 3;
		}
		case 0x36: com.LD_Mem(bus, reg.val.HL, bus.Read(reg.val.PC++)); return 3;
		case 0x37: {
			reg.flag.SetN(false);
			reg.flag.SetH(false);
			reg.flag.SetC(true);
			return 1;
		}
		case 0x38: { 
			s8 offset = (s8)bus.Read(reg.val.PC++);
			if (reg.flag.C()) {
				com.JR(reg.val.PC, offset, true);
				return 3;
			}
			return 2;
		}
		case 0x39: com.ADD_HL(reg.val.HL, reg.val.SP, reg.flag); return 2;
		case 0x3a: com.LDD_Read(bus, reg.val.HL, reg.val.A); return 2;
		case 0x3b: com.DEC(reg.val.SP); return 2;
		case 0x3c: com.INC(reg.val.A, reg.flag); return 1;
		case 0x3d: com.DEC(reg.val.A, reg.flag); return 1;
		case 0x3e: com.LD(reg.val.A, bus.Read(reg.val.PC++)); return 2;
		case 0x3F: {
			reg.flag.SetN(false);
			reg.flag.SetH(false);
			reg.flag.SetC(!reg.flag.C()); 
			return 1;
		}
		case 0x40: com.LD(reg.val.B, reg.val.B); return 1;
		case 0x41: com.LD(reg.val.B, reg.val.C); return 1;
		case 0x42: com.LD(reg.val.B, reg.val.D); return 1;
		case 0x43: com.LD(reg.val.B, reg.val.E); return 1;
		case 0x44: com.LD(reg.val.B, reg.val.H); return 1;
		case 0x45: com.LD(reg.val.B, reg.val.L); return 1;
		case 0x46: com.LD(reg.val.B, bus.Read(reg.val.HL)); return 2;
		case 0x47: com.LD(reg.val.B, reg.val.A); return 1;
		case 0x48: com.LD(reg.val.C, reg.val.B); return 1;
		case 0x49: com.LD(reg.val.C, reg.val.C); return 1;
		case 0x4a: com.LD(reg.val.C, reg.val.D); return 1;
		case 0x4b: com.LD(reg.val.C, reg.val.E); return 1;
		case 0x4c: com.LD(reg.val.C, reg.val.H); return 1;
		case 0x4d: com.LD(reg.val.C, reg.val.L); return 1;
		case 0x4e: com.LD(reg.val.C, bus.Read(reg.val.HL)); return 2;
		case 0x4f: com.LD(reg.val.C, reg.val.A); return 1;
		case 0x50: com.LD(reg.val.D, reg.val.B); return 1;
		case 0x51: com.LD(reg.val.D, reg.val.C); return 1;
		case 0x52: com.LD(reg.val.D, reg.val.D); return 1;
		case 0x53: com.LD(reg.val.D, reg.val.E); return 1;
		case 0x54: com.LD(reg.val.D, reg.val.H); return 1;
		case 0x55: com.LD(reg.val.D, reg.val.L); return 1;
		case 0x56: com.LD(reg.val.D, bus.Read(reg.val.HL)); return 2;
		case 0x57: com.LD(reg.val.D, reg.val.A); return 1;
		case 0x58: com.LD(reg.val.E, reg.val.B); return 1;
		case 0x59: com.LD(reg.val.E, reg.val.C); return 1;
		case 0x5a: com.LD(reg.val.E, reg.val.D); return 1;
		case 0x5b: com.LD(reg.val.E, reg.val.E); return 1;
		case 0x5c: com.LD(reg.val.E, reg.val.H); return 1;
		case 0x5d: com.LD(reg.val.E, reg.val.L); return 1;
		case 0x5e: com.LD(reg.val.E, bus.Read(reg.val.HL)); return 2;
		case 0x5f: com.LD(reg.val.E, reg.val.A); return 1;
		case 0x60: com.LD(reg.val.H, reg.val.B); return 1;
		case 0x61: com.LD(reg.val.H, reg.val.C); return 1;
		case 0x62: com.LD(reg.val.H, reg.val.D); return 1;
		case 0x63: com.LD(reg.val.H, reg.val.E); return 1;
		case 0x64: com.LD(reg.val.H, reg.val.H); return 1;
		case 0x65: com.LD(reg.val.H, reg.val.L); return 1;
		case 0x66: com.LD(reg.val.H, bus.Read(reg.val.HL)); return 2;
		case 0x67: com.LD(reg.val.H, reg.val.A); return 1;
		case 0x68: com.LD(reg.val.L, reg.val.B); return 1;
		case 0x69: com.LD(reg.val.L, reg.val.C); return 1;
		case 0x6a: com.LD(reg.val.L, reg.val.D); return 1;
		case 0x6b: com.LD(reg.val.L, reg.val.E); return 1;
		case 0x6c: com.LD(reg.val.L, reg.val.H); return 1;
		case 0x6d: com.LD(reg.val.L, reg.val.L); return 1;
		case 0x6e: com.LD(reg.val.L, bus.Read(reg.val.HL)); return 2;
		case 0x6f: com.LD(reg.val.L, reg.val.A); return 1;
		case 0x70: com.LD_Mem(bus, reg.val.HL, reg.val.B); return 2;
		case 0x71: com.LD_Mem(bus, reg.val.HL, reg.val.C); return 2;
		case 0x72: com.LD_Mem(bus, reg.val.HL, reg.val.D); return 2;
		case 0x73: com.LD_Mem(bus, reg.val.HL, reg.val.E); return 2;
		case 0x74: com.LD_Mem(bus, reg.val.HL, reg.val.H); return 2;
		case 0x75: com.LD_Mem(bus, reg.val.HL, reg.val.L); return 2;
		case 0x76: {
			halted = true; 
			return 1;
		}
		case 0x77: com.LD_Mem(bus, reg.val.HL, reg.val.A); return 2;
		case 0x78: com.LD(reg.val.A, reg.val.B); return 1;
		case 0x79: com.LD(reg.val.A, reg.val.C); return 1;
		case 0x7a: com.LD(reg.val.A, reg.val.D); return 1;
		case 0x7b: com.LD(reg.val.A, reg.val.E); return 1;
		case 0x7c: com.LD(reg.val.A, reg.val.H); return 1;
		case 0x7d: com.LD(reg.val.A, reg.val.L); return 1;
		case 0x7e: com.LD(reg.val.A, bus.Read(reg.val.HL)); return 2;
		case 0x7f: com.LD(reg.val.A, reg.val.A); return 1;
		case 0x80: com.ADD(reg.val.A, reg.val.B, reg.flag); return 1;
		case 0x81: com.ADD(reg.val.A, reg.val.C, reg.flag); return 1;
		case 0x82: com.ADD(reg.val.A, reg.val.D, reg.flag); return 1;
		case 0x83: com.ADD(reg.val.A, reg.val.E, reg.flag); return 1;
		case 0x84: com.ADD(reg.val.A, reg.val.H, reg.flag); return 1;
		case 0x85: com.ADD(reg.val.A, reg.val.L, reg.flag); return 1;
		case 0x86: com.ADD(reg.val.A, bus.Read(reg.val.HL), reg.flag); return 2;
		case 0x87: com.ADD(reg.val.A, reg.val.A, reg.flag); return 1;
		case 0x88: com.ADC(reg.val.A, reg.val.B, reg.flag); return 1; 
		case 0x89: com.ADC(reg.val.A, reg.val.C, reg.flag); return 1; 
		case 0x8a: com.ADC(reg.val.A, reg.val.D, reg.flag); return 1; 
		case 0x8b: com.ADC(reg.val.A, reg.val.E, reg.flag); return 1; 
		case 0x8c: com.ADC(reg.val.A, reg.val.H, reg.flag); return 1; 
		case 0x8d: com.ADC(reg.val.A, reg.val.L, reg.flag); return 1;
		case 0x8e: com.ADC(reg.val.A, bus.Read(reg.val.HL), reg.flag); return 2;
		case 0x8f: com.ADC(reg.val.A, reg.val.A, reg.flag); return 1;
		case 0x90: com.SUB(reg.val.A, reg.val.B, reg.flag); return 1; 
		case 0x91: com.SUB(reg.val.A, reg.val.C, reg.flag); return 1; 
		case 0x92: com.SUB(reg.val.A, reg.val.D, reg.flag); return 1; 
		case 0x93: com.SUB(reg.val.A, reg.val.E, reg.flag); return 1; 
		case 0x94: com.SUB(reg.val.A, reg.val.H, reg.flag); return 1; 
		case 0x95: com.SUB(reg.val.A, reg.val.L, reg.flag); return 1;
		case 0x96: com.SUB(reg.val.A, bus.Read(reg.val.HL), reg.flag); return 2;
		case 0x97: com.SUB(reg.val.A, reg.val.A, reg.flag); return 1;
		case 0x98: com.SBC(reg.val.A, reg.val.B, reg.flag); return 1;
		case 0x99: com.SBC(reg.val.A, reg.val.C, reg.flag); return 1;
		case 0x9A: com.SBC(reg.val.A, reg.val.D, reg.flag); return 1;
		case 0x9B: com.SBC(reg.val.A, reg.val.E, reg.flag); return 1;
		case 0x9C: com.SBC(reg.val.A, reg.val.H, reg.flag); return 1;
		case 0x9D: com.SBC(reg.val.A, reg.val.L, reg.flag); return 1;
		case 0x9E: com.SBC(reg.val.A, bus.Read(reg.val.HL), reg.flag); return 2;
		case 0x9F: com.SBC(reg.val.A, reg.val.A, reg.flag); return 1;
		case 0xA0: com.AND(reg.val.A, reg.val.B, reg.flag); return 1;
		case 0xA1: com.AND(reg.val.A, reg.val.C, reg.flag); return 1;
		case 0xA2: com.AND(reg.val.A, reg.val.D, reg.flag); return 1;
		case 0xA3: com.AND(reg.val.A, reg.val.E, reg.flag); return 1;
		case 0xA4: com.AND(reg.val.A, reg.val.H, reg.flag); return 1;
		case 0xA5: com.AND(reg.val.A, reg.val.L, reg.flag); return 1;
		case 0xA6: com.AND(reg.val.A, bus.Read(reg.val.HL), reg.flag); return 2;
		case 0xa7: com.AND(reg.val.A, reg.val.A, reg.flag); return 1;
		case 0xA8: com.XOR(reg.val.A, reg.val.B, reg.flag); return 1;
		case 0xA9: com.XOR(reg.val.A, reg.val.C, reg.flag); return 1;
		case 0xaa: com.XOR(reg.val.A, reg.val.D, reg.flag); return 1;
		case 0xAB: com.XOR(reg.val.A, reg.val.E, reg.flag); return 1;
		case 0xAC: com.XOR(reg.val.A, reg.val.H, reg.flag); return 1;
		case 0xAD: com.XOR(reg.val.A, reg.val.L, reg.flag); return 1;
		case 0xae: com.XOR(reg.val.A, bus.Read(reg.val.HL), reg.flag); return 2;
		case 0xaf: com.XOR(reg.val.A, reg.val.A, reg.flag); return 1;
		case 0xB0: com.OR(reg.val.A, reg.val.B, reg.flag); return 1;
		case 0xB1: com.OR(reg.val.A, reg.val.C, reg.flag); return 1;
		case 0xB2: com.OR(reg.val.A, reg.val.D, reg.flag); return 1;
		case 0xB3: com.OR(reg.val.A, reg.val.E, reg.flag); return 1;
		case 0xB4: com.OR(reg.val.A, reg.val.H, reg.flag); return 1;
		case 0xB5: com.OR(reg.val.A, reg.val.L, reg.flag); return 1;
		case 0xB6: com.OR(reg.val.A, bus.Read(reg.val.HL), reg.flag); return 2;
		case 0xB7: com.OR(reg.val.A, reg.val.A, reg.flag); return 1;
		case 0xB8: com.CP(reg.val.A, reg.val.B, reg.flag); return 1;
		case 0xB9: com.CP(reg.val.A, reg.val.C, reg.flag); return 1;
		case 0xBA: com.CP(reg.val.A, reg.val.D, reg.flag); return 1;
		case 0xBB: com.CP(reg.val.A, reg.val.E, reg.flag); return 1;
		case 0xBC: com.CP(reg.val.A, reg.val.H, reg.flag); return 1;
		case 0xBD: com.CP(reg.val.A, reg.val.L, reg.flag); return 1;
		case 0xBE: com.CP(reg.val.A, bus.Read(reg.val.HL), reg.flag); return 2;
		case 0xBF: com.CP(reg.val.A, reg.val.A, reg.flag); return 1;
		case 0xC0: {
			if (!reg.flag.Z()) { 
				u16 ret_addr;
				com.POP(bus, reg.val.SP, ret_addr);
				reg.val.PC = ret_addr;
				return 5;
			}
			return 2; 
		}
		case 0xc1: com.POP(bus, reg.val.SP, reg.val.BC); return 3;
		case 0xC2: { 
			u16 target = Fetch16();
			if (!reg.flag.Z()) com.JP(reg.val.PC, target);
			return 3; 
		}
		case 0xc3: com.JP(reg.val.PC, Fetch16()); return 3;
		case 0xC4: {
			u16 target = Fetch16();      
			if (!reg.flag.Z()) {
				com.CALL(bus, reg.val.SP, reg.val.PC, target); 
				return 6;
			}
			return 3;
		}
		case 0xc5: com.PUSH(bus, reg.val.SP, reg.val.BC); return 4;
		case 0xc6: com.ADD(reg.val.A, bus.Read(reg.val.PC++), reg.flag); return 2;
		case 0xc7: com.RST(bus, reg.val.SP, reg.val.PC, 0x0000); return 4;
		case 0xC8: {
			if (reg.flag.Z()) { 
				u16 ret_addr;
				com.POP(bus, reg.val.SP, ret_addr);
				reg.val.PC = ret_addr;
				return 5; 
			}
			return 2; 
		}
		case 0xc9: {
			u16 return_addr;
			com.POP(bus, reg.val.SP, return_addr);
			reg.val.PC = return_addr;
			return 4;
		}
		case 0xCA: { 
			u16 target = Fetch16();
			if (reg.flag.Z()) com.JP(reg.val.PC, target);
			return 3;
		}
		case 0xCB: {
			u8 cb_op = bus.Read(reg.val.PC++);

			if (cb_op >= 0x80) {
					u8 bit = (cb_op >> 3) & 0x07; 
					u8 reg_code = cb_op & 0x07;   
					bool is_set = (cb_op >= 0xC0); 

					if (reg_code == 6) {
						u8 val = bus.Read(reg.val.HL);
						if (is_set) val |= (1 << bit);
						else val &= ~(1 << bit);
						bus.Write(reg.val.HL, val);
						return 4; 
					}

					u8* target_reg = nullptr;
					switch(reg_code) {
						case 0: target_reg = &reg.val.B; break;
						case 1: target_reg = &reg.val.C; break;
						case 2: target_reg = &reg.val.D; break;
						case 3: target_reg = &reg.val.E; break;
						case 4: target_reg = &reg.val.H; break;
						case 5: target_reg = &reg.val.L; break;
						case 7: target_reg = &reg.val.A; break;
					}

					if (is_set) *target_reg |= (1 << bit);
					else *target_reg &= ~(1 << bit);

					return 2; 
				}
			
			switch (cb_op) {
				case 0x00: com.RLC(reg.val.B, reg.flag); return 2;
				case 0x01: com.RLC(reg.val.C, reg.flag); return 2; 
				case 0x02: com.RLC(reg.val.D, reg.flag); return 2; 
				case 0x03: com.RLC(reg.val.E, reg.flag); return 2; 
				case 0x04: com.RLC(reg.val.H, reg.flag); return 2; 
				case 0x05: com.RLC(reg.val.L, reg.flag); return 2; 

				case 0x06: {
					u8 val = bus.Read(reg.val.HL);
					com.RLC(val, reg.flag);
					bus.Write(reg.val.HL, val);
					return 4; 
				}

				case 0x07: com.RLC(reg.val.A, reg.flag); return 2; 

				case 0x08: com.RRC(reg.val.B, reg.flag); return 2; 
				case 0x09: com.RRC(reg.val.C, reg.flag); return 2; 
				case 0x0A: com.RRC(reg.val.D, reg.flag); return 2; 
				case 0x0B: com.RRC(reg.val.E, reg.flag); return 2; 
				case 0x0C: com.RRC(reg.val.H, reg.flag); return 2; 
				case 0x0D: com.RRC(reg.val.L, reg.flag); return 2; 

				case 0x0E: {
					u8 val = bus.Read(reg.val.HL);
					com.RRC(val, reg.flag);
					bus.Write(reg.val.HL, val);
					return 4;
				}

				case 0x0F: com.RRC(reg.val.A, reg.flag); return 2; 

				case 0x10: com.RL(reg.val.B, reg.flag); return 2;
				case 0x11: com.RL(reg.val.C, reg.flag); return 2; 
				case 0x12: com.RL(reg.val.D, reg.flag); return 2; 
				case 0x13: com.RL(reg.val.E, reg.flag); return 2; 
				case 0x14: com.RL(reg.val.H, reg.flag); return 2; 
				case 0x15: com.RL(reg.val.L, reg.flag); return 2; 

				case 0x16: {
					u8 val = bus.Read(reg.val.HL);
					com.RL(val, reg.flag);
					bus.Write(reg.val.HL, val);
					return 4;
				}

				case 0x17: com.RL(reg.val.A, reg.flag); return 2; 

				case 0x18: com.RR(reg.val.B, reg.flag); return 2; 
				case 0x19: com.RR(reg.val.C, reg.flag); return 2; 
				case 0x1A: com.RR(reg.val.D, reg.flag); return 2; 
				case 0x1B: com.RR(reg.val.E, reg.flag); return 2; 
				case 0x1C: com.RR(reg.val.H, reg.flag); return 2; 
				case 0x1D: com.RR(reg.val.L, reg.flag); return 2; 

				case 0x1E: {
					u8 val = bus.Read(reg.val.HL);
					com.RR(val, reg.flag);
					bus.Write(reg.val.HL, val);
					return 4;
				}

				case 0x1F: com.RR(reg.val.A, reg.flag); return 2; 

				case 0x20: com.SLA(reg.val.B, reg.flag); return 2; 
				case 0x21: com.SLA(reg.val.C, reg.flag); return 2; 
				case 0x22: com.SLA(reg.val.D, reg.flag); return 2;
				case 0x23: com.SLA(reg.val.E, reg.flag); return 2; 
				case 0x24: com.SLA(reg.val.H, reg.flag); return 2; 
				case 0x25: com.SLA(reg.val.L, reg.flag); return 2; 

				case 0x26: {
					u8 val = bus.Read(reg.val.HL);
					com.SLA(val, reg.flag);
					bus.Write(reg.val.HL, val);
					return 4;
				}

				case 0x27: com.SLA(reg.val.A, reg.flag); return 2; 

				case 0x28: com.SRA(reg.val.B, reg.flag); return 2; 
				case 0x29: com.SRA(reg.val.C, reg.flag); return 2; 
				case 0x2A: com.SRA(reg.val.D, reg.flag); return 2; 
				case 0x2B: com.SRA(reg.val.E, reg.flag); return 2; 
				case 0x2C: com.SRA(reg.val.H, reg.flag); return 2; 
				case 0x2D: com.SRA(reg.val.L, reg.flag); return 2; 

				case 0x2E: {
					u8 val = bus.Read(reg.val.HL);
					com.SRA(val, reg.flag);
					bus.Write(reg.val.HL, val);
					return 4;
				}

				case 0x2F: com.SRA(reg.val.A, reg.flag); return 2; 

				case 0x30: com.SWAP(reg.val.B, reg.flag); return 2; 
				case 0x31: com.SWAP(reg.val.C, reg.flag); return 2; 
				case 0x32: com.SWAP(reg.val.D, reg.flag); return 2; 
				case 0x33: com.SWAP(reg.val.E, reg.flag); return 2; 
				case 0x34: com.SWAP(reg.val.H, reg.flag); return 2; 
				case 0x35: com.SWAP(reg.val.L, reg.flag); return 2; 
				
				case 0x36: {
					u8 val = bus.Read(reg.val.HL);
					com.SWAP(val, reg.flag);
					bus.Write(reg.val.HL, val);
					return 4; 
				}
				
				case 0x37: com.SWAP(reg.val.A, reg.flag); return 2; 

				case 0x38: com.SRL(reg.val.B, reg.flag); return 2; 
				case 0x39: com.SRL(reg.val.C, reg.flag); return 2; 
				case 0x3A: com.SRL(reg.val.D, reg.flag); return 2; 
				case 0x3B: com.SRL(reg.val.E, reg.flag); return 2; 
				case 0x3C: com.SRL(reg.val.H, reg.flag); return 2; 
				case 0x3D: com.SRL(reg.val.L, reg.flag); return 2; 

				case 0x3E: {
					u8 val = bus.Read(reg.val.HL);
					com.SRL(val, reg.flag);
					bus.Write(reg.val.HL, val);
					return 4;
				}

				case 0x3F: com.SRL(reg.val.A, reg.flag); return 2;

				case 0x40: com.BIT(reg.val.B, 0, reg.flag); return 2;
				case 0x41: com.BIT(reg.val.C, 0, reg.flag); return 2;
				case 0x42: com.BIT(reg.val.D, 0, reg.flag); return 2;
				case 0x43: com.BIT(reg.val.E, 0, reg.flag); return 2;
				case 0x44: com.BIT(reg.val.H, 0, reg.flag); return 2;
				case 0x45: com.BIT(reg.val.L, 0, reg.flag); return 2;
				case 0x46: com.BIT(bus.Read(reg.val.HL), 0, reg.flag); return 3; 
				case 0x47: com.BIT(reg.val.A, 0, reg.flag); return 2;

				case 0x48: com.BIT(reg.val.B, 1, reg.flag); return 2;
				case 0x49: com.BIT(reg.val.C, 1, reg.flag); return 2;
				case 0x4A: com.BIT(reg.val.D, 1, reg.flag); return 2;
				case 0x4B: com.BIT(reg.val.E, 1, reg.flag); return 2;
				case 0x4C: com.BIT(reg.val.H, 1, reg.flag); return 2;
				case 0x4D: com.BIT(reg.val.L, 1, reg.flag); return 2;
				case 0x4E: com.BIT(bus.Read(reg.val.HL), 1, reg.flag); return 3;
				case 0x4F: com.BIT(reg.val.A, 1, reg.flag); return 2;

				case 0x50: com.BIT(reg.val.B, 2, reg.flag); return 2;
				case 0x51: com.BIT(reg.val.C, 2, reg.flag); return 2;
				case 0x52: com.BIT(reg.val.D, 2, reg.flag); return 2;
				case 0x53: com.BIT(reg.val.E, 2, reg.flag); return 2;
				case 0x54: com.BIT(reg.val.H, 2, reg.flag); return 2;
				case 0x55: com.BIT(reg.val.L, 2, reg.flag); return 2;
				case 0x56: com.BIT(bus.Read(reg.val.HL), 2, reg.flag); return 3;
				case 0x57: com.BIT(reg.val.A, 2, reg.flag); return 2;

				case 0x58: com.BIT(reg.val.B, 3, reg.flag); return 2;
				case 0x59: com.BIT(reg.val.C, 3, reg.flag); return 2;
				case 0x5A: com.BIT(reg.val.D, 3, reg.flag); return 2;
				case 0x5B: com.BIT(reg.val.E, 3, reg.flag); return 2;
				case 0x5C: com.BIT(reg.val.H, 3, reg.flag); return 2;
				case 0x5D: com.BIT(reg.val.L, 3, reg.flag); return 2;
				case 0x5E: com.BIT(bus.Read(reg.val.HL), 3, reg.flag); return 3;
				case 0x5F: com.BIT(reg.val.A, 3, reg.flag); return 2;

				case 0x60: com.BIT(reg.val.B, 4, reg.flag); return 2;
				case 0x61: com.BIT(reg.val.C, 4, reg.flag); return 2;
				case 0x62: com.BIT(reg.val.D, 4, reg.flag); return 2;
				case 0x63: com.BIT(reg.val.E, 4, reg.flag); return 2;
				case 0x64: com.BIT(reg.val.H, 4, reg.flag); return 2;
				case 0x65: com.BIT(reg.val.L, 4, reg.flag); return 2;
				case 0x66: com.BIT(bus.Read(reg.val.HL), 4, reg.flag); return 3;
				case 0x67: com.BIT(reg.val.A, 4, reg.flag); return 2;

				case 0x68: com.BIT(reg.val.B, 5, reg.flag); return 2;
				case 0x69: com.BIT(reg.val.C, 5, reg.flag); return 2;
				case 0x6A: com.BIT(reg.val.D, 5, reg.flag); return 2;
				case 0x6B: com.BIT(reg.val.E, 5, reg.flag); return 2;
				case 0x6C: com.BIT(reg.val.H, 5, reg.flag); return 2;
				case 0x6D: com.BIT(reg.val.L, 5, reg.flag); return 2;
				case 0x6E: com.BIT(bus.Read(reg.val.HL), 5, reg.flag); return 3;
				case 0x6F: com.BIT(reg.val.A, 5, reg.flag); return 2;

				case 0x70: com.BIT(reg.val.B, 6, reg.flag); return 2;
				case 0x71: com.BIT(reg.val.C, 6, reg.flag); return 2;
				case 0x72: com.BIT(reg.val.D, 6, reg.flag); return 2;
				case 0x73: com.BIT(reg.val.E, 6, reg.flag); return 2;
				case 0x74: com.BIT(reg.val.H, 6, reg.flag); return 2;
				case 0x75: com.BIT(reg.val.L, 6, reg.flag); return 2;
				case 0x76: com.BIT(bus.Read(reg.val.HL), 6, reg.flag); return 3;
				case 0x77: com.BIT(reg.val.A, 6, reg.flag); return 2;

				case 0x78: com.BIT(reg.val.B, 7, reg.flag); return 2;
				case 0x79: com.BIT(reg.val.C, 7, reg.flag); return 2;
				case 0x7A: com.BIT(reg.val.D, 7, reg.flag); return 2;
				case 0x7B: com.BIT(reg.val.E, 7, reg.flag); return 2;
				case 0x7C: com.BIT(reg.val.H, 7, reg.flag); return 2;
				case 0x7D: com.BIT(reg.val.L, 7, reg.flag); return 2;
				case 0x7E: com.BIT(bus.Read(reg.val.HL), 7, reg.flag); return 3;
				case 0x7F: com.BIT(reg.val.A, 7, reg.flag); return 2;

				

				default:
					// std::cout << "[WARN] Unimplemented CB opcode: " << std::hex << (int)cb_op << std::endl;
					return 2;
			}
		}
		case 0xCC: {
			u16 target = Fetch16();
			if (reg.flag.Z()) {
				com.CALL(bus, reg.val.SP, reg.val.PC, target);
				return 6;
			}
			return 3;
		}
		case 0xcd: {
			u16 target = Fetch16(); 
			com.CALL(bus, reg.val.SP, reg.val.PC, target); 
			return 6;
		}
		case 0xce: com.ADC(reg.val.A, bus.Read(reg.val.PC++), reg.flag); return 2;
		case 0xcf: com.RST(bus, reg.val.SP, reg.val.PC, 0x0008); return 4;
		case 0xD0: {
			if (!reg.flag.C()) { 
				u16 ret_addr;
				com.POP(bus, reg.val.SP, ret_addr);
				reg.val.PC = ret_addr;
				return 5; 
			}
			return 2; 
		}
		case 0xd1: com.POP(bus, reg.val.SP, reg.val.DE); return 3;
		case 0xD2: { 
			u16 target = Fetch16();
			if (!reg.flag.C()) com.JP(reg.val.PC, target);
			return 3;
		}
		case 0xd4: {
			u16 target = Fetch16();
			if (!reg.flag.C()) {
				com.CALL(bus, reg.val.SP, reg.val.PC, target);
				return 6;
			}
			return 3;
		}
		case 0xd5: com.PUSH(bus, reg.val.SP, reg.val.DE); return 4;
		case 0xd6: com.SUB(reg.val.A, bus.Read(reg.val.PC++), reg.flag); return 2;
		case 0xd7: com.RST(bus, reg.val.SP, reg.val.PC, 0x0010); return 4;
		case 0xd8: {
			if (reg.flag.C()) { 
				u16 ret_addr;
				com.POP(bus, reg.val.SP, ret_addr);
				reg.val.PC = ret_addr;
				return 5; 
			}
			return 2; 
		}
		case 0xd9: {
			u16 return_addr;
			//std::cout << "[DEBUG RETI] Popping from SP: 0x" << std::hex << reg.val.SP;
			com.POP(bus, reg.val.SP, return_addr);
			//std::cout << " -> Recovered PC: 0x" << return_addr << std::endl;
			reg.val.PC = return_addr;
			IME = true;
			return 4;
		}
		case 0xDA: { 
			u16 target = Fetch16();
			if (reg.flag.C()) com.JP(reg.val.PC, target);
			return 3;
		}
		case 0xdc: {
			u16 target = Fetch16();
			if (reg.flag.C()) {
				com.CALL(bus, reg.val.SP, reg.val.PC, target);
				return 6;
			}
			return 3;
		}
		case 0xDE: com.SBC(reg.val.A, bus.Read(reg.val.PC++), reg.flag); return 2;
		case 0xdf: com.RST(bus, reg.val.SP, reg.val.PC, 0x0018); return 4;
		case 0xe0: {
			u8 offset = bus.Read(reg.val.PC++);
			u16 address = 0xFF00 + offset;
			com.LD_Mem(bus, address, reg.val.A);
			return 3;
		}
		case 0xe1: com.POP(bus, reg.val.SP, reg.val.HL); return 3;
		case 0xe2: {
			u16 address = 0xFF00 + reg.val.C;
			com.LD_Mem(bus, address, reg.val.A);
			return 2;
		}
		case 0xe5: com.PUSH(bus, reg.val.SP, reg.val.HL); return 4;
		case 0xe6: com.AND(reg.val.A, bus.Read(reg.val.PC++), reg.flag); return 2;
		case 0xe7: com.RST(bus, reg.val.SP, reg.val.PC, 0x0020); return 4;
		case 0xE8: {
			s8 n = (s8)bus.Read(reg.val.PC++);
			
			u16 check_sp = reg.val.SP;
			u16 check_n = (u8)n;

			reg.flag.SetZ(false); 
			reg.flag.SetN(false); 
			reg.flag.SetH(((check_sp & 0x0F) + (check_n & 0x0F)) > 0x0F);
			
			reg.flag.SetC(((check_sp & 0xFF) + (check_n & 0xFF)) > 0xFF);

			reg.val.SP = reg.val.SP + n;

			return 4;
		}
		case 0xe9: com.JP(reg.val.PC, reg.val.HL); return 1;
		case 0xea: com.LD_Mem(bus, Fetch16(), reg.val.A); return 4;
		case 0xee: {
			u8 n = bus.Read(reg.val.PC++);   
			com.XOR(reg.val.A, n, reg.flag); 
			return 2; 
		}
		case 0xef: com.RST(bus, reg.val.SP, reg.val.PC, 0x0028); return 4;
		case 0xf0: {
			u8 offset = bus.Read(reg.val.PC++);
			u16 address = 0xFF00 + offset;
			com.LD(reg.val.A, bus.Read(address));
			return 3;
		}
		case 0xf1: {
			com.POP(bus, reg.val.SP, reg.val.AF); 
			reg.val.F &= 0xF0;
			return 3;
		}
		case 0xf2: {
			u16 address = 0xFF00 + reg.val.C;
			com.LD(reg.val.A, bus.Read(address));
			return 2;
		}
		case 0xf3: com.DI(IME); return 1;
		case 0xf5: com.PUSH(bus, reg.val.SP, reg.val.AF); return 4;
		case 0xF6: {
			u8 n = bus.Read(reg.val.PC++); 
			com.OR(reg.val.A, n, reg.flag); 
			return 2; 
		}
		case 0xf7: com.RST(bus, reg.val.SP, reg.val.PC, 0x0030); return 4;
		case 0xf8: {
			s8 n = (s8)bus.Read(reg.val.PC++);
			com.LDHL(reg.val.HL, reg.val.SP, n, reg.flag);
			return 3;
		}
		case 0xf9: com.LD(reg.val.SP, reg.val.HL); return 2;
		case 0xfa: {
			u16 address = Fetch16();
			u8 val = bus.Read(address);
			com.LD(reg.val.A, val);
			return 4;
		}
		case 0xfb: IME = true; return 1;
		case 0xfe: {
			u8 n = bus.Read(reg.val.PC++);
			com.CP(reg.val.A, n, reg.flag);
			return 2;
		}
		case 0xff: com.RST(bus, reg.val.SP, reg.val.PC, 0x0038); return 4;
	}

	return 0;
}
bool Processor::LoadROM(const char* path) {
	return bus.LoadROM(path);
}
int Processor::GetRomSize() {
	return bus.GetRomSize();
}
void Processor::HandleInterrupts() {
    u8 IF = bus.Read(0xFF0F);
    u8 IE = bus.Read(0xFFFF);
    if (!IME) return;

    u8 fired = IF & IE;
    if (fired > 0) {
        for (int i = 0; i < 5; i++) {
            if (fired & (1 << i)) {
                IME = false; 
				halted = false;
                bus.Write(0xFF0F, IF & ~(1 << i));
                com.PUSH(bus, reg.val.SP, reg.val.PC); 

                switch (i) {
                    case 0: reg.val.PC = 0x0040; break; 
                    case 1: reg.val.PC = 0x0048; break; 
                    case 2: reg.val.PC = 0x0050; break; 
                    case 3: reg.val.PC = 0x0058; break; 
                    case 4: reg.val.PC = 0x0060; break; 
                }
                return; 
            }
        }
    }
}