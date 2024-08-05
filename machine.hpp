// =========================================================================
//   Copyright (C) 2009-2024 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>
// =========================================================================

#ifndef MACHINE_H
#define MACHINE_H

#include <iostream>
#include <memory>
#include <map>

#include "chip/mos6502.hpp"
#include "chip/mos6522.hpp"
#include "chip/ay3_8912.hpp"
#include "memory.hpp"

#include "tape/tape_tap.hpp"
#include "tape/tape_blank.hpp"

class Oric;
class Frontend;
class AY3_8912;

typedef std::map<int32_t, uint8_t> KeyMap_t;

typedef std::pair<int32_t, bool> KeyPress_t;
typedef std::map<KeyPress_t, KeyPress_t> KeyTranslation_t;

class Machine
{
public:
    Machine(Oric* oric);
    virtual ~Machine();

    void init(Frontend* frontend);

    void init_cpu();
    void init_mos6522();
    void init_ay3();
    void init_tape();

    void reset();
    void run(uint32_t steps, Oric* oric);
    void run(uint16_t address, long steps, Oric* oric) { cpu->set_pc(address); run(steps, oric); }
    void stop() { break_exec = true; }

    void irq() { cpu->irq(); }
    void irq_clear() { cpu->irq_clear(); }

    void key_press(uint8_t a_KeyBits, bool a_Down);
    void update_key_output();
    void via_orb_changed(uint8_t a_Orb);

//    static uint8_t read_byte(Machine& a_machine, uint16_t a_Address);

    static uint8_t inline read_byte(Machine& a_Machine, uint16_t a_Address)
    {
        if (a_Address >= 0x300 && a_Address < 0x400) {
            return a_Machine.mos_6522->read_byte(a_Address);
        }
        return a_Machine.memory.mem[a_Address];
    }



    static uint8_t read_byte_zp(Machine& a_Machine, uint8_t a_Address);

//    static uint16_t read_word(Machine& a_Machine, uint16_t a_Address);
    static uint16_t inline read_word(Machine &a_Machine, uint16_t a_Address)
    {
        return a_Machine.memory.mem[a_Address] | a_Machine.memory.mem[a_Address + 1] << 8;
    }

//    static uint16_t read_word_zp(Machine& a_Machine, uint8_t a_Address);
    static uint16_t inline read_word_zp(Machine &a_Machine, uint8_t a_Address)
    {
        return a_Machine.memory.mem[a_Address] | a_Machine.memory.mem[a_Address + 1 & 0xff] << 8;
    }

    static void write_byte(Machine& a_Machine, uint16_t a_Address, uint8_t a_Val);
//    static void write_byte_zp(Machine& a_Machine, uint8_t a_Address, uint8_t a_Val);

    static void inline write_byte_zp(Machine &a_Machine, uint8_t a_Address, uint8_t a_Val)
    {
        if (a_Address > 0x00ff) {
            return;
        }
        a_Machine.memory.mem[a_Address] = a_Val;
    }

//    static uint8_t read_via_ora(Machine& a_Machine);

    static uint8_t inline read_via_ora(Machine& a_Machine)
    {
        return a_Machine.mos_6522->read_ora();
    }

    static uint8_t read_via_orb(Machine& a_Machine);

    MOS6502* cpu;
    MOS6522* mos_6522;
    AY3_8912* ay3;
    bool break_exec;
    Memory memory;
    Frontend* frontend;

protected:
    inline bool paint_raster(Oric* oric);

    Oric* oric;
    Tape* tape;

    int32_t cycle_count;
    bool is_running;
    bool warpmode_on;

    uint16_t raster_current;

    bool sound_paused;
    uint32_t sound_pause_counter;

    KeyMap_t key_map;
    KeyTranslation_t key_translations;

    uint8_t current_key_row;
    uint8_t key_rows[8];
};

#endif // MACHINE_H
