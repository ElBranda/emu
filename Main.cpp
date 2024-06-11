#include <iostream>
#include <fstream>
#include "CPU.hpp"
#include "Helper.hpp"

using namespace CPU;
using namespace std;

int main(int argc, char* argv[]) {
	basic_ifstream<u8> gb_rom;
	Memory_Bus bus;

	if (!isAttachedFile(argc)) gb_rom.open("example.gb", ios::binary);
	else gb_rom.open(argv[1], ios::binary);

	if (!gb_rom.is_open()) { cout << "No hay ningún archivo..." << endl; return 0xffB; }

	reg.Init();

	bus.LoadGame(gb_rom);
	bus.ShowMemory();

	while (RUN_ROM) {
		bus.Fetch();

		cout << endl << hex << static_cast<int>(reg.PC.Get()) << " " << static_cast<int>(opcode) << endl;

		bus.Execute(bus);

		cin.get();
	}

	return 0;
}