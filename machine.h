/***************************************************************************
*   Copyright (C) 2009 by Anders Karlsson   *
*   pugo@pugo.org   *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#ifndef MACHINE_H
#define MACHINE_H

#include <iostream>

#include "datatypes.h"
#include "mos6502.h"
#include "mos6522.h"
#include "memory.h"

class Machine
{
public:
	Machine();
	virtual ~Machine();

	Memory& getMemory() { return *memory; }
	MOS6502& getCPU() { return *cpu; }
	MOS6522& getVIA() { return *mos_6522; }

	void reset();
	void run(long steps);
	void run(word address, long steps) { cpu->setPC(address); run(steps); }
	void stop() { brk = true; }

	static inline byte read_byte(Machine &machine, word address);
	static inline byte read_byte_zp(Machine &machine, byte address);

	static inline word read_word(Machine &machine, word address);
	static inline word read_word_zp(Machine &machine, byte address);

	static inline void write_byte(Machine &machine, word address, byte val);
	static inline void write_byte_zp(Machine &machine, byte address, byte val);

	bool brk;

protected:
	MOS6502* cpu;
	MOS6522* mos_6522;
	Memory* memory;

	bool running;
};

#endif // MACHINE_H
