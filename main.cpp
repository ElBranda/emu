#include <iostream>
#include "cpu.cpp"

using namespace std;

int main() {
    CPU cpu;

    cpu.Start();

    cout << hex << cpu.regis.AF << endl;

    cpu.instr.LD(&cpu.regis.AF, 0xce);
    cpu.instr.JP(&cpu.regis.PC, 0xff);

    return 0;
}