#include "PPU.hpp"

using namespace CPU;

PPU::PPU() {
    mode_clock = 0;
    current_mode = OAM_SCAN;
    line_y = 0;
    std::fill(std::begin(screen_buffer), std::end(screen_buffer), 0);
}

void PPU::UpdateLY(Memory_Bus& bus, u8 value) {
    line_y = value;

    bus.UpdateLY(line_y);
}

void PPU::SetMode(PPUMode mode, Memory_Bus& bus) {
    current_mode = mode;

    u8 stat = bus.Read(0xFF41);
    stat &= 0xFC;
    stat |= (mode & 0x03);
    bus.UpdateSTAT(stat);
}

void PPU::Tick(u8 cycles, Memory_Bus& bus) {
    u8 lcdc = bus.Read(0xFF40);
    
    if (!(lcdc & 0x80)) { 
        mode_clock = 0;
        line_y = 0;
        bus.UpdateLY(0);
        current_mode = OAM_SCAN;
        return; 
    }
    mode_clock += cycles;

    switch (current_mode) {
        case OAM_SCAN:
            if (mode_clock >= 80) {
                mode_clock -= 80;
                SetMode(DRAWING, bus);
            }
            break;
        
        case DRAWING:
            if (mode_clock >= 172) {
                mode_clock -= 172;
                RenderScanline(bus); 
                SetMode(HBLANK, bus);
            }
            break;
        
        case HBLANK:
            if (mode_clock >= 204) {
                mode_clock -= 204;
                UpdateLY(bus, line_y + 1);

                if (line_y == 144) {
                    SetMode(VBLANK, bus);
                    bus.RequestInterrupt(0);
                } else {
                    SetMode(OAM_SCAN, bus);
                }
            }
            break;
        
        case VBLANK:
            if (mode_clock >= 456) {
                mode_clock -= 456;
                UpdateLY(bus, line_y + 1);

                if (line_y > 153) {
                    UpdateLY(bus, 0);
                    SetMode(OAM_SCAN, bus);
                }
            }
            break;
    }
}

void PPU::DrawPixel(SDL_Renderer* renderer, int x, int y, int color_id) {
    int r, g, b;
    const int SCALE = 3;
    switch (color_id) {
        case 0: r=255; g=255; b=255; break; // Blanco
        case 1: r=170; g=170; b=170; break; // Gris Claro
        case 2: r=85;  g=85;  b=85;  break; // Gris Oscuro
        case 3: r=0;   g=0;   b=0;   break; // Negro
    }
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_Rect rect = {x * SCALE, y * SCALE, SCALE, SCALE}; 
    SDL_RenderFillRect(renderer, &rect);
}

void PPU::DebugDrawTiles(SDL_Renderer* renderer, CPU::Memory_Bus& bus) {
    int x_draw = 0;
    int y_draw = 0;
    int tile_count = 0;

    for (int addr = 0x8000; addr < 0x9800; addr += 16) {
        
        for (int row = 0; row < 8; row++) {
            u8 byte1 = bus.Read(addr + (row * 2));
            u8 byte2 = bus.Read(addr + (row * 2) + 1);

            for (int bit = 7; bit >= 0; bit--) {
                int hi = (byte2 >> bit) & 1;
                int lo = (byte1 >> bit) & 1;
                int color = (hi << 1) | lo;

                this->DrawPixel(renderer, x_draw + (7 - bit), y_draw + row, color);
            }
        }

        x_draw += 8;
        if (x_draw >= 160) { 
            x_draw = 0;
            y_draw += 8;
        }
    }
}

void PPU::RenderScanline(Memory_Bus& bus) {
    u8 lcdc = bus.Read(0xFF40);

    if (!(lcdc & 0x01)) return; 

    u8 scy = bus.Read(0xFF42);
    u8 scx = bus.Read(0xFF43);
    u8 bgp = bus.Read(0xFF47);

    u8 y_pos = scy + line_y;

    u16 tile_row = (y_pos / 8) * 32;

    u16 map_base = (lcdc & 0x08) ? 0x9C00 : 0x9800;

    bool unsigned_tiles = (lcdc & 0x10) != 0;
    u16 tile_data_base = unsigned_tiles ? 0x8000 : 0x8800;

    for (int x = 0; x < 160; x++) {
        u8 x_pos = x + scx;

        u16 tile_col = x_pos / 8;

        u16 tile_address = map_base + tile_row + tile_col;
        u8 tile_num = bus.Read(tile_address);

        u16 tile_location = tile_data_base;
        if (unsigned_tiles) {
            tile_location += (tile_num * 16);
        } else {
            s8 signed_num = (s8)tile_num;
            tile_location = 0x9000 + (signed_num * 16);
        }

        u8 line_in_tile = y_pos % 8;
        u8 byte1 = bus.Read(tile_location + (line_in_tile * 2));
        u8 byte2 = bus.Read(tile_location + (line_in_tile * 2) + 1);

        int bit_index = 7 - (x_pos % 8);

        int lo = (byte1 >> bit_index) & 1;
        int hi = (byte2 >> bit_index) & 1;
        int color_id = (hi << 1) | lo;

        int real_color = (bgp >> (color_id * 2)) & 0x03;

        screen_buffer[line_y * 160 + x] = real_color;
    }

    if (lcdc & 0x02) {
        bool use_8x16 = (lcdc & 0x04) != 0;
        u8 sprite_height = use_8x16 ? 16 : 8;

        u8 obp0 = bus.Read(0xFF48); 
        u8 obp1 = bus.Read(0xFF49); 

        for (int sprite = 0; sprite < 40; sprite++) {
            u16 index = sprite * 4;
            u8 y_pos = bus.Read(0xFE00 + index) - 16;
            u8 x_pos = bus.Read(0xFE00 + index + 1) - 8;
            u8 tile_index = bus.Read(0xFE00 + index + 2);
            u8 attributes = bus.Read(0xFE00 + index + 3);

            if (line_y >= y_pos && line_y < (y_pos + sprite_height)) {
                
                bool y_flip = (attributes & 0x40) != 0;
                bool x_flip = (attributes & 0x20) != 0;
                u8 palette = (attributes & 0x10) ? obp1 : obp0;

                int line = line_y - y_pos;
                if (y_flip) line = sprite_height - 1 - line;

                if (use_8x16) tile_index &= 0xFE;

                u16 data_address = 0x8000 + (tile_index * 16) + (line * 2);
                u8 byte1 = bus.Read(data_address);
                u8 byte2 = bus.Read(data_address + 1);

                for (int tile_pixel = 7; tile_pixel >= 0; tile_pixel--) {
                    int color_bit = x_flip ? (7 - tile_pixel) : tile_pixel;

                    int lo = (byte1 >> color_bit) & 1;
                    int hi = (byte2 >> color_bit) & 1;
                    int color_id = (hi << 1) | lo;

                    if (color_id == 0) continue;

                    int pixel_x = x_pos + (7 - tile_pixel);
                    
                    if (pixel_x < 0 || pixel_x >= 160) continue;

                    int real_color = (palette >> (color_id * 2)) & 0x03;
                    
                    screen_buffer[line_y * 160 + pixel_x] = real_color;
                }
            }
        }
    }
}
void PPU::DrawFrame(SDL_Renderer* renderer) {
    for (int y = 0; y < 144; y++) {
        for (int x = 0; x < 160; x++) {
            int color = screen_buffer[y * 160 + x];
            DrawPixel(renderer, x, y, color);
        }
    }
}