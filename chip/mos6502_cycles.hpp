// =========================================================================
//   Copyright (C) 2009-2023 by Anders Piniesjö <pugo@pugo.org>
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

#ifndef OPCODE_CYCLES_H
#define OPCODE_CYCLES_H

typedef struct cycle_info {
	uint8_t cycles;
	uint8_t cross_page;
} cycle_info;


cycle_info opcode_cycles[] = {
	{7, 0}, {6, 0}, {0, 0}, {8, 0}, {0, 0}, {3, 0}, {5, 0}, {0, 0},  // 00
	{3, 0}, {2, 0}, {2, 0}, {0, 0}, {0, 0}, {4, 0}, {6, 0}, {0, 0},
	{2, 1}, {5, 1}, {0, 0}, {0, 0}, {0, 0}, {4, 0}, {6, 0}, {0, 0},  // 10
	{2, 0}, {4, 1}, {0, 0}, {0, 0}, {0, 0}, {4, 1}, {7, 0}, {0, 0},
	{6, 0}, {6, 0}, {0, 0}, {0, 0}, {3, 0}, {3, 0}, {5, 0}, {0, 0},  // 20
	{4, 0}, {2, 0}, {2, 0}, {0, 0}, {4, 0}, {4, 0}, {6, 0}, {0, 0},
	{2, 1}, {5, 1}, {0, 0}, {8, 0}, {0, 0}, {4, 0}, {6, 0}, {0, 0},  // 30
	{2, 0}, {4, 1}, {0, 0}, {0, 0}, {0, 0}, {4, 1}, {7, 0}, {0, 0},
	{6, 0}, {6, 0}, {0, 0}, {0, 0}, {0, 0}, {3, 0}, {5, 0}, {0, 0},  // 40
	{3, 0}, {2, 0}, {2, 0}, {0, 0}, {3, 0}, {4, 0}, {6, 0}, {0, 0},
	{2, 1}, {5, 1}, {0, 0}, {0, 0}, {0, 0}, {4, 0}, {6, 0}, {0, 0},  // 50
	{2, 0}, {4, 1}, {0, 0}, {0, 0}, {0, 0}, {4, 1}, {7, 0}, {0, 0},
	{6, 0}, {6, 0}, {0, 0}, {0, 0}, {0, 0}, {3, 0}, {5, 0}, {0, 0},  // 60
	{4, 0}, {2, 0}, {2, 0}, {0, 0}, {5, 0}, {4, 0}, {6, 0}, {0, 0},
	{2, 1}, {5, 1}, {0, 0}, {0, 0}, {0, 0}, {4, 0}, {6, 0}, {0, 0},  // 70
	{2, 0}, {4, 1}, {0, 0}, {0, 0}, {0, 0}, {4, 1}, {7, 0}, {0, 0},
	{0, 0}, {6, 0}, {0, 0}, {0, 0}, {3, 0}, {3, 0}, {3, 0}, {0, 0},  // 80
	{2, 0}, {0, 0}, {2, 0}, {0, 0}, {4, 0}, {4, 0}, {4, 0}, {0, 0},
	{2, 1}, {6, 0}, {0, 0}, {0, 0}, {4, 0}, {4, 0}, {4, 0}, {0, 0},  // 90
	{2, 0}, {5, 0}, {2, 0}, {0, 0}, {0, 0}, {5, 0}, {0, 0}, {0, 0},
	{2, 0}, {6, 0}, {2, 0}, {0, 0}, {3, 0}, {3, 0}, {3, 0}, {0, 0},  // a0
	{2, 0}, {2, 0}, {2, 0}, {0, 0}, {4, 0}, {4, 0}, {4, 0}, {0, 0},
	{2, 1}, {5, 1}, {0, 0}, {0, 0}, {4, 0}, {4, 0}, {4, 0}, {0, 0},  // b0
	{2, 0}, {4, 1}, {2, 0}, {0, 0}, {4, 1}, {4, 1}, {4, 1}, {0, 0},
	{2, 0}, {6, 0}, {0, 0}, {0, 0}, {3, 0}, {3, 0}, {5, 0}, {0, 0},  // c0
	{2, 0}, {2, 0}, {2, 0}, {0, 0}, {4, 0}, {4, 0}, {6, 0}, {0, 0},
	{2, 1}, {5, 1}, {0, 0}, {0, 0}, {0, 0}, {4, 0}, {6, 0}, {0, 0},  // d0
	{2, 0}, {4, 1}, {0, 0}, {0, 0}, {0, 0}, {4, 1}, {7, 0}, {0, 0},
	{2, 0}, {6, 0}, {0, 0}, {0, 0}, {3, 0}, {3, 0}, {5, 0}, {0, 0},  // e0
	{2, 0}, {2, 0}, {2, 0}, {0, 0}, {4, 0}, {4, 0}, {6, 0}, {0, 0},
	{2, 1}, {5, 1}, {0, 0}, {0, 0}, {0, 0}, {4, 0}, {6, 0}, {0, 0},  // f0
	{2, 0}, {4, 1}, {0, 0}, {0, 0}, {0, 0}, {4, 1}, {7, 0}, {0, 0}
};

#endif // OPCODE_CYCLES_H
