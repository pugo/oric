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

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <boost/assign.hpp>

#include "machine.hpp"
#include "oric.hpp"
#include "frontend.hpp"

// VIA Lines        Oric usage
// ----------       ---------------------------------
// PA0..PA7         PSG data bus, printer data lines
// CA1              printer acknowledge line
// CA2              PSG BC1 line
// PB0..PB2         keyboard lines-demultiplexer
// PB3              keyboard sense line
// PB4              printer strobe line
// PB5              (not connected)
// PB6              tape connector motor control
// PB7              tape connector output
// CB1              tape connector input
// CB2              PSG BDIR line


static int32_t keytab[] = {
    '7'        , 'n'        , '5'        , 'v'        , 0 ,          '1'        , 'x'        , '3'        ,     // 7
    'j'        , 't'        , 'r'        , 'f'        , 0          , SDLK_ESCAPE, 'q'        , 'd'        ,     // 15
    'm'        , '6'        , 'b'        , '4'        , SDLK_LCTRL , 'z'        , '2'        , 'c'        ,     // 23
    'k'        , '9'        , ';'        , '-'        , 0          , 0          , '\\'       , '\''       ,     // 31
    SDLK_SPACE , ','        , '.'        , SDLK_UP    , SDLK_LSHIFT, SDLK_LEFT  , SDLK_DOWN  , SDLK_RIGHT ,     //
    'u'        , 'i'        , 'o'        , 'p'        , SDLK_LALT  , SDLK_BACKSPACE, ']'     , '['        ,
    'y'        , 'h'        , 'g'        , 'e'        , 0          , 'a'        , 's'        , 'w'        ,
    '8'        , 'l'        , '0'        , '/'        , SDLK_RSHIFT, SDLK_RETURN, 0          , SDLK_EQUALS };


constexpr uint8_t cycles_per_raster = 64;
constexpr uint16_t raster_max = 312;
constexpr uint16_t raster_visible_lines = 224;
//constexpr uint16_t raster_visible_first = 65;
constexpr uint16_t raster_visible_first = 44;
constexpr uint16_t raster_visible_last = raster_visible_first + raster_visible_lines;

constexpr uint32_t sound_pause_target = 1000;


Machine::Machine(Oric* oric) :
    oric(oric),
    memory(65535),
    tape(nullptr),
    cycle_count(0),
    is_running(false),
    warpmode_on(false),
    break_exec(false),
    raster_current(0),
    sound_paused(true),
    sound_pause_counter(0),
    current_key_row(0)
{
    for (uint8_t i=0; i < 8; i++) {
        key_rows[i] = 0;
    }

    boost::assign::insert(key_translations)
        (std::make_pair(0xe4, false), std::make_pair('/', false))
        (std::make_pair(0xe4, true), std::make_pair('/', false))
        (std::make_pair(0xf6, false), std::make_pair(';', false))
        (std::make_pair(0xf6, true), std::make_pair(';', false))
        (std::make_pair(0x2b, false), std::make_pair('=', false))
        (std::make_pair(0x2b, true), std::make_pair('=', false));
}

Machine::~Machine()
{}

void Machine::init(Frontend* frontend)
{
    this->frontend = frontend;
    init_cpu();
    init_mos6522();
    init_ay3();
    init_tape();

    for (uint8_t i=0; i < 64; ++i) {
        if (keytab[i] != 0) {
            key_map[keytab[i]] = i;
        }
    }
}

void Machine::init_cpu()
{
    cpu = new MOS6502(*this);
    cpu->memory_read_byte_handler = read_byte;
    cpu->memory_read_byte_zp_handler = read_byte_zp;
    cpu->memory_read_word_handler = read_word;
    cpu->memory_read_word_zp_handler = read_word_zp;
    cpu->memory_write_byte_handler = write_byte;
    cpu->memory_write_byte_zp_handler = write_byte_zp;
}

void Machine::init_mos6522()
{
    mos_6522 = new MOS6522(*this);

    // CA1 is connected to printer ACK line.
    // -- printer not supported.

    // CA2 is connected to AY BC1 line.
    mos_6522->ca2_changed_handler = AY3_8912::set_bc1;

    // CB1 is connected to tape connector input, tape_tap.cpp writes directly to CB1.

    // CB2 is connected to AY BDIR line.
    mos_6522->cb2_changed_handler = AY3_8912::set_bdir;
}

void Machine::init_ay3()
{
    ay3 = new AY3_8912(*this);

    // AY data bus reads from VIA ORA (Output Register A).
    ay3->m_read_data_handler = read_via_ora;
    //	ay3->m_write_data_handler = write_vi
}

void Machine::init_tape()
{
    if (! oric->get_config().tape_path().empty()) {
        tape = new TapeTap(*mos_6522, oric->get_config().tape_path());
        if (!tape->init()) {
            exit(1);
        }
    }
    else {
        std::cout << "No tape specified." << std::endl;
        tape = new TapeBlank();
    }
}

void Machine::reset()
{
    cpu->Reset();
}

/**
 * run machine.
 * \param steps number of instructions to run. If 0: run infinite (or to BRK).
 */
void Machine::run(uint32_t steps, Oric* oric)
{
    uint32_t instructions = 0;
    uint64_t next_frame = SDL_GetTicks64();
    SDL_Event event;

    break_exec = false;
    uint8_t ran = 0;

    cycle_count += cycles_per_raster;

    while (! break_exec) {
        if (sound_paused) {
            sound_pause_counter += 1;

            if (sound_pause_counter > sound_pause_target) {
                sound_paused = false;
                frontend->pause_sound(false);
            }
        }

        while (cycle_count > 0) {
            tape->exec();
            mos_6522->exec();
            ay3->exec(1);

            if (cpu->exec_instruction(break_exec)) {
                update_key_output();
            }

            cycle_count -= 1;
        }

//        if (cycle_count <= 0) {
            if (paint_raster(oric)) {
                next_frame += 20;

                while (SDL_PollEvent(&event)) {
                    switch (event.type)
                    {
                        case SDL_KEYDOWN:
                        case SDL_KEYUP:
                        {
                            auto sym = event.key.keysym.sym;

                            if (event.key.keysym.sym == SDLK_F12 && event.type == SDL_KEYDOWN) {
                                warpmode_on = !warpmode_on;
                                std::cout << "Warp mode: " << (warpmode_on ? "on" : "off") << std::endl;
                                if (! warpmode_on) {
                                    next_frame = SDL_GetTicks64();
                                }
                            }

                            if (event.key.keysym.sym == SDLK_F10 && event.type == SDL_KEYDOWN) {
                                cpu->NMI();
                            }

                            auto trans = key_translations.find(std::make_pair(sym, event.key.keysym.mod));
                            if (trans != key_translations.end()) {
                                sym = trans->second.first;
                            }

                            auto key = key_map.find(sym);
                            if (key != key_map.end()) {
                                key_press(key->second, event.type == SDL_KEYDOWN);
                            }
                            break;
                        }

                        case SDL_WINDOWEVENT:
                            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                                oric->do_quit();
                                break_exec = true;
                            }
                    }
                }

                uint64_t now = SDL_GetTicks64();

                if (now > next_frame) {
                    next_frame = now;
                }
                else {
                    if (! warpmode_on) {
                        SDL_Delay(next_frame - now);
                    }
                }
            }
        cycle_count += cycles_per_raster;
    }
}

inline bool Machine::paint_raster(Oric* oric)
{
    bool render_screen = false;

    if (++raster_current == raster_max) {
        raster_current = 0;
        render_screen = true;
        frontend->render_graphics();
    }

    if ((raster_current < raster_visible_first) || (raster_current >= raster_visible_last)) {
        return render_screen;
    }

    frontend->update_graphics(raster_current - raster_visible_first, memory.mem);
    return render_screen;
}

void Machine::key_press(uint8_t a_KeyBits, bool a_Down)
{
//	std::cout << "key: " << (int)a_KeyBits << ", " << (a_Down ? "down" : "up") << std::endl;
    if (a_Down) {
        key_rows[a_KeyBits >> 3] |= (1 << (a_KeyBits & 0x07));
    }
    else {
        key_rows[a_KeyBits >> 3] &= ~(1 << (a_KeyBits & 0x07));
    }

    // ???
//	if (current_key_row == (a_KeyBits >> 3)) {
//		update_key_output();
//	}
}

void Machine::update_key_output()
{
    current_key_row = mos_6522->read_orb() & 0x07;

    if (! (ay3->get_register(AY3_8912::ENABLE) & 0x40))
    {
        mos_6522->set_irb_bit(3, false);
        return;
    }

    if (key_rows[current_key_row] & (ay3->get_register(AY3_8912::IO_PORT_A) ^ 0xff)) {
        mos_6522->set_irb_bit(3, true);
    }
    else {
        mos_6522->set_irb_bit(3, false);
    }
}


void Machine::via_orb_changed(uint8_t a_Orb)
{
    bool motor_on = a_Orb & 0x40;
    if (motor_on != tape->do_run_motor) {
        tape->set_motor(motor_on);
    }
}


// --- Memory functions -------------------

//uint8_t inline Machine::read_byte(Machine& a_Machine, uint16_t a_Address)
//{
//    if (a_Address >= 0x300 && a_Address < 0x400) {
//        return a_Machine.mos_6522->read_byte(a_Address);
//    }
//    return a_Machine.memory.mem[a_Address];
//}

uint8_t inline Machine::read_byte_zp(Machine &a_Machine, uint8_t a_Address)
{
    return a_Machine.memory.mem[a_Address];
}

//uint16_t inline Machine::read_word(Machine &a_Machine, uint16_t a_Address)
//{
//    return a_Machine.memory.mem[a_Address] | a_Machine.memory.mem[a_Address + 1] << 8;
//}

//uint16_t inline Machine::read_word_zp(Machine &a_Machine, uint8_t a_Address)
//{
//    return a_Machine.memory.mem[a_Address] | a_Machine.memory.mem[a_Address + 1 & 0xff] << 8;
//}

void inline Machine::write_byte(Machine &a_Machine, uint16_t a_Address, uint8_t a_Val)
{
    if (a_Address >= 0xc000) {
        return;
    }

    if (a_Address >= 0x300 && a_Address < 0x400) {
        a_Machine.mos_6522->write_byte(a_Address, a_Val);
    }

    a_Machine.memory.mem[a_Address] = a_Val;
}

//void inline Machine::write_byte_zp(Machine &a_Machine, uint8_t a_Address, uint8_t a_Val)
//{
//    if (a_Address > 0x00ff) {
//        return;
//    }
//    a_Machine.memory.mem[a_Address] = a_Val;
//}


//uint8_t inline Machine::read_via_ora(Machine& a_Machine)
//{
//    return a_Machine.mos_6522->read_ora();
//}

uint8_t inline Machine::read_via_orb(Machine& a_Machine)
{
    return a_Machine.mos_6522->read_orb();
}
