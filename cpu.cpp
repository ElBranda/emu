#include "cpu.h"

class CPU {
    public:
        class registers {
            public:
                u16 AF, BC, DE, HL;
                u16 SP, PC;

                void Init() {
                    this->AF = 0x0fce;
                    this->PC = 0x100;
                }
        };

        class instructions {
            public:
                void LD(u16 *n, u16 nn) {
                    *n = nn;
                }

                void JP(u16 *n, u16 nn) {
                    *n = nn;
                }
        };
        
        registers regis;
        instructions instr;

        void Start() {
            this->regis.Init();
        }
};