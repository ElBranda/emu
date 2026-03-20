#include "APU.hpp"

using namespace CPU;

APU::APU() {
    audio_cycles = 0;
    phase1 = 0.0f; 
    phase2 = 0.0f; 
    phase3 = 0.0f; 
    phase4 = 0.0f;
    lfsr = 0x7FFF;
    onda_pasada_entrada = 0.0f;
    onda_pasada_salida = 0.0f;
    
    sample_buffer.reserve(2048);
}

void APU::Tick(int cycles, Memory_Bus& bus, SDL_AudioDeviceID device) {
    audio_cycles += cycles;

    while (audio_cycles >= 95) {
        audio_cycles -= 95;

        float sample1 = 0.0f, sample2 = 0.0f, sample3 = 0.0f, sample4 = 0.0f;

        // CHANNEL 1
        u8 nr11 = bus.Read(0xFF11), nr12 = bus.Read(0xFF12), nr13 = bus.Read(0xFF13), nr14 = bus.Read(0xFF14);
        int vol1 = (nr12 >> 4) & 0x0F;
        if (vol1 > 0) {
            u16 freq1 = nr13 | ((nr14 & 0x07) << 8);
            float hz1 = 131072.0f / (2048.0f - freq1);
            phase1 += hz1 / 44100.0f;
            if (phase1 >= 1.0f) phase1 -= 1.0f;
            int duty_idx = (nr11 >> 6) & 0x03;
            float duty = (duty_idx == 0) ? 0.125f : (duty_idx == 1) ? 0.25f : (duty_idx == 2) ? 0.5f : 0.75f;
            sample1 = ((phase1 < duty) ? 1.0f : -1.0f) * (vol1 / 15.0f);
        }

        // CHANNEL 2
        u8 nr21 = bus.Read(0xFF16), nr22 = bus.Read(0xFF17), nr23 = bus.Read(0xFF18), nr24 = bus.Read(0xFF19);
        int vol2 = (nr22 >> 4) & 0x0F;
        if (vol2 > 0) {
            u16 freq2 = nr23 | ((nr24 & 0x07) << 8);
            float hz2 = 131072.0f / (2048.0f - freq2);
            phase2 += hz2 / 44100.0f;
            if (phase2 >= 1.0f) phase2 -= 1.0f;
            int duty_idx = (nr21 >> 6) & 0x03;
            float duty = (duty_idx == 0) ? 0.125f : (duty_idx == 1) ? 0.25f : (duty_idx == 2) ? 0.5f : 0.75f;
            sample2 = ((phase2 < duty) ? 1.0f : -1.0f) * (vol2 / 15.0f);
        }

        // CHANNEL 3
        u8 nr30 = bus.Read(0xFF1A);
        if (nr30 & 0x80) { 
            u8 nr32 = bus.Read(0xFF1C);
            int vol_shift = (nr32 >> 5) & 0x03; // 0=Mute, 1=100%, 2=50%, 3=25%
            
            if (vol_shift > 0) {
                u16 freq3 = bus.Read(0xFF1D) | ((bus.Read(0xFF1E) & 0x07) << 8);
                float hz3 = 65536.0f / (2048.0f - freq3);
                
                phase3 += (hz3 * 32.0f) / 44100.0f; // 32 muestras por ciclo de onda
                if (phase3 >= 32.0f) phase3 -= 32.0f;

                int sample_idx = (int)phase3;
                
                u8 wave_byte = bus.Read(0xFF30 + (sample_idx / 2));
                
                u8 nibble = (sample_idx % 2 == 0) ? (wave_byte >> 4) : (wave_byte & 0x0F);
                
                int shift_amount = (vol_shift == 1) ? 0 : (vol_shift == 2) ? 1 : 2;
                float wave_sample = (float)(nibble >> shift_amount);
                
                sample3 = ((wave_sample / 7.5f) - 1.0f); // Normalizar entre -1.0 y 1.0
            }
        }

        // CHANNAL 4
        u8 nr42 = bus.Read(0xFF21);
        int vol4 = (nr42 >> 4) & 0x0F;
        if (vol4 > 0) {
            u8 nr43 = bus.Read(0xFF22);
            int shift = (nr43 >> 4) & 0x0F;
            int divisor_code = nr43 & 0x07;
            float divisor = (divisor_code == 0) ? 8.0f : (divisor_code * 16.0f);
            float freq4 = 4194304.0f / (divisor * (1 << shift));

            phase4 += freq4 / 44100.0f;
            while (phase4 >= 1.0f) {
                phase4 -= 1.0f;
                int xor_bit = (lfsr & 0x01) ^ ((lfsr >> 1) & 0x01);
                lfsr >>= 1;
                lfsr |= (xor_bit << 14);
                if (nr43 & 0x08) {
                    lfsr &= ~0x40;
                    lfsr |= (xor_bit << 6);
                }
            }
            sample4 = (((lfsr & 0x01) ? -1.0f : 1.0f) * (vol4 / 15.0f)) * 0.2f;
        }

        float entrada = (sample1 + sample2 + sample3 + sample4) * 0.05f;

        float salida = entrada - onda_pasada_entrada + 0.995f * onda_pasada_salida;

        onda_pasada_entrada = entrada;
        onda_pasada_salida = salida;

        sample_buffer.push_back(salida);

        if (sample_buffer.size() >= 1024) {
            SDL_QueueAudio(device, sample_buffer.data(), sample_buffer.size() * sizeof(float));
            sample_buffer.clear();
        }
    }
}