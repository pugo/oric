/***************************************************************************
 *   Copyright (C) 2009 by Anders Karlsson                                 *
 *   pugo@pugo.org                                                         *
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

#include "mos6502.h"
#include "mos6502_opcodes.h"
#include "mos6502_cycles.h"

#include <iostream>
#include "stdio.h"

// Macros for addressing modes
#define READ_BYTE_IMM()     read_byte(PC++)


// Read addresses
#define READ_ADDR_ZP()      (READ_BYTE_IMM())
#define READ_ADDR_ZP_X()    ((READ_BYTE_IMM() + X) & 0xff)
#define READ_ADDR_ZP_Y()    ((READ_BYTE_IMM() + Y) & 0xff)

#define READ_ADDR_ABS()     (READ_BYTE_IMM() | READ_BYTE_IMM()<<8)
#define READ_ADDR_ABS_X()   (READ_ADDR_ABS() + X)
#define READ_ADDR_ABS_Y()   (READ_ADDR_ABS() + Y)

#define READ_ADDR_IND_X()   (read_word_zp(READ_BYTE_IMM() + X))
#define READ_ADDR_IND_Y()   (read_word_zp(READ_BYTE_IMM()) + Y)

#define READ_JUMP_ADDR()    (b1 = READ_BYTE_IMM(), b1 & 0x80 ? (PC - ((b1 ^ 0xff)+1)) : (PC + b1))


// Read data
#define READ_BYTE_ZP()      read_byte_zp(READ_ADDR_ZP())
#define READ_BYTE_ZP_X()    read_byte_zp(READ_ADDR_ZP_X())
#define READ_BYTE_ZP_Y()    read_byte_zp(READ_ADDR_ZP_Y())

#define READ_BYTE_ABS()     read_byte(READ_ADDR_ABS())
#define READ_BYTE_ABS_X()   read_byte(READ_ADDR_ABS_X())
#define READ_BYTE_ABS_Y()   read_byte(READ_ADDR_ABS_Y())

#define READ_BYTE_IND_X()   read_byte(READ_ADDR_IND_X())
#define READ_BYTE_IND_Y()   read_byte(READ_ADDR_IND_Y())

#define PUSH_BYTE_STACK(b)  (memory->mem[ (SP--) | STACK_BOTTOM ] = (b))
#define POP_BYTE_STACK      (memory->mem[ (++SP) | STACK_BOTTOM ])


// Macros for flag handling
#define SET_FLAG_NZ(B)     (N_INTERN = Z_INTERN = B)

#define Z	(!Z_INTERN)
#define N	(!!(N_INTERN & 0x80))


const char * opcodenames[256] = {
	"BRK",     "ORA_IND_X", "(none)",  "(none)", "(none)",    "ORA_ZP",    "ASL_ZP",    "(none)",  // 00
	"PHP",     "ORA_IMM",   "ASL_ACC", "(none)", "(none)",    "ORA_ABS",   "ASL_ABS",   "(none)",
	"BPL",     "ORA_IND_Y", "(none)",  "(none)", "(none)",    "ORA_ZP_X",  "ASL_ZP_X",  "(none)",  // 10
	"CLC",     "ORA_ABS_Y", "(none)",  "(none)", "(none)",    "ORA_ABS_X", "ASL_ABS_X", "(none)",
	"JSR",     "AND_IND_X", "(none)",  "(none)", "BIT_ZP",    "AND_ZP",    "ROL_ZP",    "(none)",  // 20
	"PLP",     "AND_IMM",   "ROL_ACC", "(none)", "BIT_ABS",   "AND_ABS",   "ROL_ABS",   "(none)",
	"BMI ",    "AND_IND_Y", "(none)",  "(none)", "(none)",    "AND_ZP_X",  "ROL_ZP_X",  "(none)",  // 30
	"SEC",     "AND_ABS_Y", "(none)",  "(none)", "(none)",    "AND_ABS_X", "ROL_ABS_X", "(none)",
	"RTI",     "EOR_IND_X", "(none)",  "(none)", "(none)",    "EOR_ZP",    "LSR_ZP",    "(none)",  // 40
	"PHA",     "EOR_IMM",   "LSR_ACC", "(none)", "JMP_ABS",   "EOR_ABS",   "LSR_ABS",   "(none)",
	"BVC",     "EOR_IND_Y", "(none)",  "(none)", "(none)",    "EOR_ZP_X",  "LSR_ZP_X",  "(none)",  // 50
	"CLI",     "EOR_ABS_Y", "(none)",  "(none)", "(none)",    "EOR_ABS_X", "LSR_ABS_X", "(none)",
	"RTS",     "ADC_IND_X", "(none)",  "(none)", "(none)",    "ADC_ZP",    "ROR_ZP",    "(none)",  // 60
	"PLA",     "ADC_IMM",   "ROR_ACC", "(none)", "JMP_IND",   "ADC_ABS",   "ROR_ABS",   "(none)",
	"BVS",     "ADC_IND_Y", "(none)",  "(none)", "(none)",    "ADC_ZP_X",  "ROR_ZP_X",  "(none)",  // 70
	"SEI",     "ADC_ABS_Y", "(none)",  "(none)", "(none)",    "ADC_ABS_X", "ROR_ABS_X", "(none)",
	"(none)",  "STA_IND_X", "(none)",  "(none)", "STY_ZP",    "STA_ZP",    "STX_ZP",    "(none)",  // 80
	"DEY",     "(none)",    "TXA",     "(none)", "STY_ABS",   "STA_ABS",   "STX_ABS",   "(none)",
	"BCC",     "STA_IND_Y", "(none)",  "(none)", "STY_ZP_X",  "STA_ZP_X",  "STX_ZP_Y",  "(none)",  // 90
	"TYA",     "STA_ABS_Y", "TXS",     "(none)", "(none)",    "STA_ABS_X", "(none)",    "(none)",
	"LDY_IMM", "LDA_IND_X", "LDX_IMM", "(none)", "LDY_ZP",    "LDA_ZP",    "LDX_ZP",    "(none)",  // a0
	"TAY",     "LDA_IMM",   "TAX",     "(none)", "LDY_ABS",   "LDA_ABS",   "LDX_ABS",   "(none)",
	"BCS",     "LDA_IND_Y", "(none)",  "(none)", "LDY_ZP_X",  "LDA_ZP_X",  "LDX_ZP_Y",  "(none)",  // b0
	"CLV",     "LDA_ABS_Y", "TSX",     "(none)", "LDY_ABS_X", "LDA_ABS_X", "LDX_ABS_Y", "(none)",
	"CPY_IMM", "CMP_IND_X", "(none)",  "(none)", "CPY_ZP",    "CMP_ZP",    "DEC_ZP",    "(none)",  // c0
	"INY",     "CMP_IMM",   "DEX",     "(none)", "CPY_ABS",   "CMP_ABS",   "DEC_ABS",   "(none)",
	"BNE",     "CMP_IND_Y", "(none)",  "(none)", "(none)",    "CMP_ZP_X",  "DEC_ZP_X",  "(none)",  // d0
	"CLD",     "CMP_ABS_Y", "(none)",  "(none)", "(none)",    "CMP_ABS_X", "DEC_ABS_X", "(none)",
	"CPX_IMM", "SBC_IND_X", "(none)",  "(none)", "CPX_ZP",    "SBC_ZP",    "INC_ZP",    "(none)",  // e0
	"INX",     "SBC_IMM",   "NOP",     "(none)", "CPX_ABS",   "SBC_ABS",   "INC_ABS",   "(none)",
	"BEQ",     "SBC_IND_Y", "(none)",  "(none)", "(none)",    "SBC_ZP_X",  "INC_ZP_X",  "(none)",  // f0
    "SED",     "SBC_ABS_Y", "(none)",  "(none)", "(none)",    "SBC_ABS_X", "INC_ABS_X", "(none)",
};



using namespace std;

MOS6502::MOS6502(Memory* memory) :
	A(0), X(0), Y(0), N_INTERN(0), Z_INTERN(0), V(false), B(false), D(false), I(false), C(false)
{
	this->memory = memory;
}


MOS6502::~MOS6502()
{
}


void MOS6502::reset()
{
	A = 0;
	X = 0;
	Y = 0;
	N_INTERN = 0;
	Z_INTERN = 0;
	V = false;
	B = false;
	D = false;
	I = true;	// Block interrupts after reset.
	C = false; 

	PC = read_byte(RESET_VECTOR_L) + (read_byte(RESET_VECTOR_H) << 8);
	SP = 0xff;
}


void MOS6502::printStat(word address)
{
	cout << disassemble(address) << " ";
	printf("SP: %02X  |  A: %02X, X: %02X, Y: %02X  |  N: %d, Z: %d, C: %d, V: %d --- (%02X)\n", SP, A, X, Y, N, Z, C, V, address);
}


byte inline MOS6502::read_byte(word address)
{
	if (address >= 0x300 && address < 0x400)
	{
		cout << "read: " << hex << address << endl;
		return memory_read_handler(address);
	}

	return memory->mem[address];
}

byte inline MOS6502::read_byte_zp(byte address)
{
	if (address >= 0x300 && address < 0x400)
	{
		cout << "read: " << hex << address << endl;
		return memory_read_handler(address);
	}

	return memory->mem[address];
}

word inline MOS6502::read_word(word address)
{
	if (address >= 0x300 && address < 0x400)
	{
		cout << "read: " << hex << address << endl;
		return memory_read_handler(address);
	}

	return memory->mem[address] | memory->mem[address+1]<<8;
}

word inline MOS6502::read_word_zp(byte address)
{
	if (address >= 0x300 && address < 0x400)
	{
		cout << "read: " << hex << address << endl;
		return memory_read_handler(address);
	}
	return memory->mem[address] | memory->mem[address+1 & 0xff]<<8;
}

void inline MOS6502::write_byte(word address, byte val)
{
	if (address >= 0xc000)
		return;

	if (address >= 0x300 && address < 0x400)
		memory_write_handler(address, val);
	else
		memory->mem[address] = val;
}

void inline MOS6502::write_byte_zp(byte address, byte val)
{
	if (address >= 0x00ff)
		return;

	if (address >= 0x300 && address < 0x400)
		memory_write_handler(address, val);
	else
		memory->mem[address] = val;
}




//   7                           0
// +---+---+---+---+---+---+---+---+
// | N | V |   | B | D | I | Z | C |
// +---+---+---+---+---+---+---+---+
byte MOS6502::getP()
{
	byte result = 0;
	//printf("getP: before: N=%d, V=%d, B=%d, D=%d, I=%d, Z=%d, C=%d\n", N, V, B, D, I, Z, C);
	result |= N ? FLAG_N : 0;
	result |= V ? FLAG_V : 0;
	result |= B ? FLAG_B : 0;
	result |= D ? FLAG_D : 0;
	result |= I ? FLAG_I : 0;
	result |= Z ? FLAG_Z : 0;
	result |= C ? FLAG_C : 0;
	return result;
}


void MOS6502::setP(byte new_p)
{
	//printf("setP: incoming byte: %X\n", new_p);
	N_INTERN = (new_p & FLAG_N) ? FLAG_N : 0;
	V = !! (new_p & FLAG_V);
	B = !! (new_p & FLAG_B);
	D = !! (new_p & FLAG_D);
	I = !! (new_p & FLAG_I);
	Z_INTERN = (new_p & FLAG_Z) ? 0 : 1;
	C = !! (new_p & FLAG_C);
	//printf("setP: results: N=%d, V=%d, B=%d, D=%d, I=%d, Z=%d, C=%d\n", IS_NEGATIVE, V, B, D, I, IN_ZERO, C);
}


void MOS6502::NMI()
{
	cout << "NMI interrupt" << endl;
	PUSH_BYTE_STACK(PC >> 8);
	PUSH_BYTE_STACK(PC);
	PUSH_BYTE_STACK(getP());
	PC = read_word(NMI_VECTOR_L);
	I = true;
}

void MOS6502::IRQ()
{
	if (I) // Interrupt disabled ?
		return;
	cout << "IRQ" << endl;

	PUSH_BYTE_STACK(PC >> 8);
	PUSH_BYTE_STACK(PC);
	PUSH_BYTE_STACK(getP());
	I = true;
	PC = read_word(IRQ_VECTOR_L);
}


int inline MOS6502::signedByteToInt(byte b)
{
	if (b < 0x80)
		return b;
	b ^= 0xff;
	b += 1;
	return -b;
}


void MOS6502::adc(byte val)
{
	if (D)  // Decimal mode
	{
		word low = (A & 0x0f) + (val & 0x0f) + (C ? 1 : 0);
		if (low > 9) low += 6;    // 11 + 6 = (0xb + 6) = 0x11
		word high = (A >> 4) + (val >> 4) + (low > 0x0f);	// remainder from low figure -> high


		Z_INTERN = (A + val + (C ? 1 : 0)) & 0xff;
		N_INTERN = (high & 0x08) ? FLAG_N : 0;
		V = ~(A ^ val) & (A ^ (high << 4)) & 0x80;

		if (high > 9) high += 6;  // 11 + 6 = (0xb + 6) = 0x11

		C = high > 0x0f;
		A = (high << 4) | (low & 0x0f);
	}
	else   // Normal mode
	{
		word w = A + val + (C ? 1 : 0);
		C = w > 0xff;
		V = ~(A ^ val) & (A ^ w) & 0x80;
		SET_FLAG_NZ(A = w);
	}
}


void MOS6502::sbc(byte val)
{
	if (D)
	{
		word low = (A & 0x0f) - (val & 0x0f) - (C ? 0 : 1);
		word high = (A >> 4) - (val >> 4);

		if (low & 0x10)   // Fix negative
		{
			low -= 6;
			--high;
		}

		if (high & 0x10)  // Fix negative
			high -= 6;

		word w = SET_FLAG_NZ(A - val - (C ? 0 : 1));
		C = w < 0x100;
		V = (A ^ val) & (A ^ w) & 0x80;
		A = (high << 4) | (low & 0x0f);
	}
	else
	{
		word w = A - val - (C ? 0 : 1);
		C = w < 0x100;
		V = (A ^ val) & (A ^ w) & 0x80;
		SET_FLAG_NZ(A = w);
	}

	//cout << "A now: " << hex << A << endl;
}


bool inline MOS6502::execInstructionCycles(int cycles)
{
	bool brk = false;
	while (!brk && cycles >= 0)
		cycles -= execInstruction(brk);
	return !brk;
}

short inline MOS6502::execInstruction(bool& brk)
{
	byte b1, b2;
	word addr, w;
	int i;

	word pc_initial = PC;
	byte instruction = READ_BYTE_IMM();

	switch(instruction)
	{
		case LDA_IMM:
			SET_FLAG_NZ(A = READ_BYTE_IMM());
			break;
		case LDA_ZP:
			SET_FLAG_NZ(A = READ_BYTE_ZP());
			break;
		case LDA_ZP_X:
			SET_FLAG_NZ(A = READ_BYTE_ZP_X());
			break;
		case LDA_ABS:
			SET_FLAG_NZ(A = READ_BYTE_ABS());
			break;
		case LDA_ABS_X:
			SET_FLAG_NZ(A = READ_BYTE_ABS_X());
			break;
		case LDA_ABS_Y:
			SET_FLAG_NZ(A = READ_BYTE_ABS_Y());
			break;
		case LDA_IND_X:
			SET_FLAG_NZ(A = READ_BYTE_IND_X());
			break;
		case LDA_IND_Y:
			SET_FLAG_NZ(A = READ_BYTE_IND_Y());
			break;

		case LDX_IMM:
			SET_FLAG_NZ(X = READ_BYTE_IMM());
			break;
		case LDX_ZP:
			SET_FLAG_NZ(X = READ_BYTE_ZP());
			break;
		case LDX_ZP_Y:
			SET_FLAG_NZ(X = READ_BYTE_ZP_Y());
			break;
		case LDX_ABS:
			SET_FLAG_NZ(X = READ_BYTE_ABS());
			break;
		case LDX_ABS_Y:
			SET_FLAG_NZ(X = READ_BYTE_ABS_Y());
			break;

		case LDY_IMM:
			SET_FLAG_NZ(Y = READ_BYTE_IMM());
			break;
		case LDY_ZP:
			SET_FLAG_NZ(Y = READ_BYTE_ZP());
			break;
		case LDY_ZP_X:
			SET_FLAG_NZ(Y = READ_BYTE_ZP_X());
			break;
		case LDY_ABS:
			SET_FLAG_NZ(Y = READ_BYTE_ABS());
			break;
		case LDY_ABS_X:
			SET_FLAG_NZ(Y = READ_BYTE_ABS_X());
			break;

		case STA_ZP:
			write_byte(READ_ADDR_ZP(), A);
			break;
		case STA_ZP_X:
			write_byte(READ_ADDR_ZP_X(), A);
			break;
		case STA_ABS:
			write_byte(READ_ADDR_ABS(), A);
			break;
		case STA_ABS_X:
			write_byte(READ_ADDR_ABS_X(), A);
			break;
		case STA_ABS_Y:
			write_byte(READ_ADDR_ABS_Y(), A);
			break;
		case STA_IND_X:
			write_byte(READ_ADDR_IND_X(), A);
			break;
		case STA_IND_Y:
			write_byte(READ_ADDR_IND_Y(), A);
			break;

		case STX_ZP:
			write_byte(READ_ADDR_ZP(), X);
			break;
		case STX_ZP_Y:
			write_byte(READ_ADDR_ZP_Y(), X);
			break;
		case STX_ABS:
			write_byte(READ_ADDR_ABS(), X);
			break;

		case STY_ZP:
			write_byte(READ_ADDR_ZP(), Y);
			break;
		case STY_ZP_X:
			write_byte(READ_ADDR_ZP_X(), Y);
			break;
		case STY_ABS:
			write_byte(READ_ADDR_ABS(), Y);
			break;

		// ADD to accumulator with carry
		case ADC_IMM:
			adc(READ_BYTE_IMM());
			break;
		case ADC_ZP:
			adc(READ_BYTE_ZP());
			break;
		case ADC_ZP_X:
			adc(READ_BYTE_ZP_X());
			break;
		case ADC_ABS:
			adc(READ_BYTE_ABS());
			break;
		case ADC_ABS_X:
			adc(READ_BYTE_ABS_X());
			break;
		case ADC_ABS_Y:
			adc(READ_BYTE_ABS_Y());
			break;
		case ADC_IND_X:
			adc(READ_BYTE_IND_X());
			break;
		case ADC_IND_Y:
			adc(READ_BYTE_IND_Y());
			break;

		// Subtract from accumulator with borrow
		case SBC_IMM:
			sbc(READ_BYTE_IMM());
			break;
		case SBC_ZP:
			sbc(READ_BYTE_ZP());
			break;
		case SBC_ZP_X:
			sbc(READ_BYTE_ZP_X());
			break;
		case SBC_ABS:
			sbc(READ_BYTE_ABS());
			break;
		case SBC_ABS_X:
			sbc(READ_BYTE_ABS_X());
			break;
		case SBC_ABS_Y:
			sbc(READ_BYTE_ABS_Y());
			break;
		case SBC_IND_X:
			sbc(READ_BYTE_IND_X());
			break;
		case SBC_IND_Y:
			sbc(READ_BYTE_IND_Y());
			break;

		// Increment memory by one
		case INC_ZP:
			addr = READ_ADDR_ZP();
			write_byte(addr, SET_FLAG_NZ(read_byte(addr) + 1));
			break;
		case INC_ZP_X:
			addr = READ_ADDR_ZP_X();
			write_byte(addr, SET_FLAG_NZ(read_byte(addr) + 1));
			break;
		case INC_ABS:
			addr = READ_ADDR_ABS();
			write_byte(addr, SET_FLAG_NZ(read_byte(addr) + 1));
			break;
		case INC_ABS_X:
			addr = READ_ADDR_ABS_X();
			write_byte(addr, SET_FLAG_NZ(read_byte(addr) + 1));
			break;

		// Decrease memory by one
		case DEC_ZP:
			addr = READ_ADDR_ZP();
			write_byte(addr, SET_FLAG_NZ(read_byte(addr) - 1));
			break;
		case DEC_ZP_X:
			addr = READ_ADDR_ZP_X();
			write_byte(addr, SET_FLAG_NZ(read_byte(addr) - 1));
			break;
		case DEC_ABS:
			addr = READ_ADDR_ABS();
			write_byte(addr, SET_FLAG_NZ(read_byte(addr) - 1));
			break;
		case DEC_ABS_X:
			addr = READ_ADDR_ABS_X();
			write_byte(addr, SET_FLAG_NZ(read_byte(addr) - 1));
			break;

		// Increase X by one
		case INX:
			SET_FLAG_NZ(++X);
			break;
		// Decrease X by one
		case DEX:
			SET_FLAG_NZ(--X);
			break;
		// Increase Y by one
		case INY:
			SET_FLAG_NZ(++Y);
			break;
		// Decrease Y by one
		case DEY:
			SET_FLAG_NZ(--Y);
			break;

		// And accumulator with memory
		case AND_IMM:
			SET_FLAG_NZ(A &= READ_BYTE_IMM());
			break;
		case AND_ZP:
			SET_FLAG_NZ(A &= READ_BYTE_ZP());
			break;
		case AND_ZP_X:
			SET_FLAG_NZ(A &= READ_BYTE_ZP_X());
			break;
		case AND_ABS:
			SET_FLAG_NZ(A &= READ_BYTE_ABS());
			break;
		case AND_ABS_X:
			SET_FLAG_NZ(A &= READ_BYTE_ABS_X());
			break;
		case AND_ABS_Y:
			SET_FLAG_NZ(A &= READ_BYTE_ABS_Y());
			break;
		case AND_IND_X:
			SET_FLAG_NZ(A &= READ_BYTE_IND_X());
			break;
		case AND_IND_Y :
			SET_FLAG_NZ(A &= READ_BYTE_IND_Y());
			break;

		// Or accumulator with memory
		case ORA_IMM:
			SET_FLAG_NZ(A |= READ_BYTE_IMM());
			break;
		case ORA_ZP:
			SET_FLAG_NZ(A |= READ_BYTE_ZP());
			break;
		case ORA_ZP_X:
			SET_FLAG_NZ(A |= READ_BYTE_ZP_X());
			break;
		case ORA_ABS:
			SET_FLAG_NZ(A |= READ_BYTE_ABS());
			break;
		case ORA_ABS_X:
			SET_FLAG_NZ(A |= READ_BYTE_ABS_X());
			break;
		case ORA_ABS_Y:
			SET_FLAG_NZ(A |= READ_BYTE_ABS_Y());
			break;
		case ORA_IND_X:
			SET_FLAG_NZ(A |= READ_BYTE_IND_X());
			break;
		case ORA_IND_Y:
			SET_FLAG_NZ(A |= READ_BYTE_IND_Y());
			break;

		// Exclusive or accumulator with memory
		case EOR_IMM:
			SET_FLAG_NZ(A ^= READ_BYTE_IMM());
			break;
		case EOR_ZP:
			SET_FLAG_NZ(A ^= READ_BYTE_ZP());
			break;
		case EOR_ZP_X:
			SET_FLAG_NZ(A ^= READ_BYTE_ZP_X());
			break;
		case EOR_ABS:
			SET_FLAG_NZ(A ^= READ_BYTE_ABS());
			break;
		case EOR_ABS_X:
			SET_FLAG_NZ(A ^= READ_BYTE_ABS_X());
			break;
		case EOR_ABS_Y:
			SET_FLAG_NZ(A ^= READ_BYTE_ABS_Y());
			break;
		case EOR_IND_X:
			SET_FLAG_NZ(A ^= READ_BYTE_IND_X());
			break;
		case EOR_IND_Y:
			SET_FLAG_NZ(A ^= READ_BYTE_IND_Y());
			break;

			//      +-+-+-+-+-+-+-+-+
			// C <- |7|6|5|4|3|2|1|0| <- 0              N Z C I D V
			//      +-+-+-+-+-+-+-+-+                   / / / _ _ _
		case ASL_ACC:
			C = A & 0x80;
			SET_FLAG_NZ(A <<= 1);
			break;
		case ASL_ZP:
			b1 = read_byte_zp(addr = READ_ADDR_ZP());
			C = b1 & 0x80;
			write_byte_zp(addr, SET_FLAG_NZ(b1 <<= 1));
			break;
		case ASL_ZP_X:
			b1 = read_byte_zp(addr = READ_ADDR_ZP_X());
			C = b1 & 0x80;
			write_byte_zp(addr, SET_FLAG_NZ(b1 <<= 1));
			break;
		case ASL_ABS:
			b1 = read_byte(addr = READ_ADDR_ABS());
			C = b1 & 0x80;
			write_byte(addr, SET_FLAG_NZ(b1 <<= 1));
			break;
		case ASL_ABS_X:
			b1 = read_byte(addr = READ_ADDR_ABS_X());
			C = b1 & 0x80;
			write_byte(addr, SET_FLAG_NZ(b1 <<= 1));
			break;

			//      +-+-+-+-+-+-+-+-+
			// 0 -> |7|6|5|4|3|2|1|0| -> C              N Z C I D V
			//      +-+-+-+-+-+-+-+-+                   0 / / _ _ _
		case LSR_ACC:
			C = A & 0x01;
			SET_FLAG_NZ(A >>= 1);
			break;
		case LSR_ZP:
			b1 = read_byte_zp(addr = READ_ADDR_ZP());
			C = b1 & 0x01;
			write_byte_zp(addr, SET_FLAG_NZ( b1 >>= 1));
			break;
		case LSR_ZP_X:
			b1 = read_byte_zp(addr = READ_ADDR_ZP_X());
			C = b1 & 0x01;
			write_byte_zp(addr, SET_FLAG_NZ( b1 >>= 1));
			break;
		case LSR_ABS:
			b1 = read_byte(addr = READ_ADDR_ABS());
			C = b1 & 0x01;
			write_byte(addr, SET_FLAG_NZ( b1 >>= 1));
			break;
		case LSR_ABS_X:
			b1 = read_byte(addr = READ_ADDR_ABS_X());
			C = b1 & 0x01;
			write_byte(addr, SET_FLAG_NZ( b1 >>= 1));
			break;

			// +------------------------------+
			// |         M or A               |
			// |   +-+-+-+-+-+-+-+-+    +-+   |
			// +-< |7|6|5|4|3|2|1|0| <- |C| <-+         N Z C I D V
			//     +-+-+-+-+-+-+-+-+    +-+             / / / _ _ _
		case ROL_ACC:
			b2 = A & 0x80;
			SET_FLAG_NZ(A = C ? (A<<=1) + 1 : A<<=1);
			C = b2;
			break;
		case ROL_ZP:
			b1 = read_byte_zp(addr = READ_ADDR_ZP());
			b2 = b1 & 0x80;
			write_byte_zp(addr, SET_FLAG_NZ(C ? (b1<<=1) + 1 : b1<<=1));
			C = b2;
			break;
		case ROL_ZP_X:
			b1 = read_byte_zp(addr = READ_ADDR_ZP_X());
			b2 = b1 & 0x80;
			write_byte_zp(addr, SET_FLAG_NZ(C ? (b1<<=1) + 1 : b1<<=1));
			C = b2;
			break;
		case ROL_ABS:
			b1 = read_byte(addr = READ_ADDR_ABS());
			b2 = b1 & 0x80;
			write_byte(addr, SET_FLAG_NZ(C ? (b1<<=1) + 1 : b1<<=1));
			C = b2;
			break;
		case ROL_ABS_X:
			b1 = read_byte(addr = READ_ADDR_ABS_X());
			b2 = b1 & 0x80;
			write_byte(addr, SET_FLAG_NZ(C ? (b1<<=1) + 1 : b1<<=1));
			C = b2;
			break;

			// +------------------------------+
			// |                              |
			// |   +-+    +-+-+-+-+-+-+-+-+   |
			// +-> |C| -> |7|6|5|4|3|2|1|0| >-+         N Z C I D V
			//     +-+    +-+-+-+-+-+-+-+-+             / / / _ _ _
		case ROR_ACC:
			b2 = A & 0x01;
			SET_FLAG_NZ(A = C ? (A>>=1)|0x80 : A>>=1);
			C = b2;
			break;
		case ROR_ZP:
			b1 = read_byte_zp(addr = READ_ADDR_ZP());
			b2 = b1 & 0x01;
			write_byte_zp(addr, SET_FLAG_NZ(C ? (b1>>=1)|0x80 : b1>>=1));
			C = b2;
			break;
		case ROR_ZP_X:
			b1 = read_byte_zp(addr = READ_ADDR_ZP_X());
			b2 = b1 & 0x01;
			write_byte_zp(addr, SET_FLAG_NZ(C ? (b1>>=1)|0x80 : b1>>=1));
			C = b2;
			break;
		case ROR_ABS:
			b1 = read_byte(addr = READ_ADDR_ABS());
			b2 = b1 & 0x01;
			write_byte(addr, SET_FLAG_NZ(C ? (b1>>=1)|0x80 : b1>>=1));
			C = b2;
			break;
		case ROR_ABS_X:
			b1 = read_byte(addr = READ_ADDR_ABS_X());
			b2 = b1 & 0x01;
			write_byte(addr, SET_FLAG_NZ(C ? (b1>>=1)|0x80 : b1>>=1));
			C = b2;
			break;


		// Branches
		case BCC:
			if (!C)
				PC = READ_JUMP_ADDR();
			else
				++PC;
			break;
		case BCS:
			if (C)
				PC = READ_JUMP_ADDR();
			else
				++PC;
			break;
		case BEQ:
			if (Z)
				PC = READ_JUMP_ADDR();
			else
				++PC;
			break;
		case BNE:
			if (!Z)
				PC = READ_JUMP_ADDR();
			else
				++PC;
			break;

		case BMI:
			if (N)
				PC = READ_JUMP_ADDR();
			else
				++PC;
			break;

		case BPL:
			if (!N)
				PC = READ_JUMP_ADDR();
			else
				++PC;
			break;

		case BVC:
			if (!V)
				PC = READ_JUMP_ADDR();
			else
				++PC;
			break;

		case BVS:
			if (V)
				PC = READ_JUMP_ADDR();
			else
				++PC;
			break;

		case BIT_ZP:
			b1 = READ_BYTE_ZP();
			N_INTERN = Z_INTERN = A & b1;
			V = b1 & FLAG_V;  // bit 6 -> V
			break;

		case BIT_ABS:
			b1 = READ_BYTE_ABS();
			N_INTERN = Z_INTERN = A & b1;
			V = b1 & FLAG_V;  // bit 6 -> V
			break;

		case SEC: // Set carry flag
			C = true;
			break;
		case SED: // Set decimal flag
			D = true;
			break;
		case SEI: // Set interrupt flag
			I = true;
			break;

		case CLC: // Clear carry flag
			C = false;
			break;
		case CLD: // Clear decimal flag
			D = false;
			break;
		case CLI: // Clear interrupt flag
			I = false;
			break;
		case CLV: // Clear overflow flag
			V = false;
			break;

		case CMP_IMM:
			i = A - READ_BYTE_IMM();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;
		case CMP_ZP:
			i = A - READ_BYTE_ZP();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;
		case CMP_ZP_X:
			i = A - READ_BYTE_ZP_X();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;
		case CMP_ABS:
			i = A - READ_BYTE_ABS();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;
		case CMP_ABS_X:
			i = A - READ_BYTE_ABS_X();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;
		case CMP_ABS_Y:
			i = A - READ_BYTE_ABS_Y();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;
		case CMP_IND_X:
			i = A - READ_BYTE_IND_X();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;
		case CMP_IND_Y:
			i = A - READ_BYTE_IND_Y();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;

		case CPX_IMM:
			i = X - READ_BYTE_IMM();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;
		case CPX_ZP:
			i = X - READ_BYTE_ZP();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;
		case CPX_ABS:
			i = X - READ_BYTE_ABS();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;

		case CPY_IMM:
			i = Y - READ_BYTE_IMM();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;
		case CPY_ZP:
			i = Y - READ_BYTE_ZP();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;
		case CPY_ABS:
			i = Y - READ_BYTE_ABS();
			C = i >= 0;
			SET_FLAG_NZ((byte)i);
			break;

		case JMP_ABS:
			PC = READ_ADDR_ABS();
			break;
		case JMP_IND:
			PC = read_word(READ_ADDR_ABS());
			break;

		case JSR:
			PUSH_BYTE_STACK((PC+1) >> 8); // Store 1 before next instruction
			PUSH_BYTE_STACK((PC+1) & 0xff);
			PC = READ_ADDR_ABS();
			break;

		case RTS:
			PC = (POP_BYTE_STACK | (POP_BYTE_STACK << 8)) + 1;
			break;

		case BRK:
			PUSH_BYTE_STACK((PC+1) >> 8); // Byte after BRK will not be executed on return!
			PUSH_BYTE_STACK(PC+1);
			PUSH_BYTE_STACK(getP() | FLAG_B);
			PC = read_word(IRQ_VECTOR_L);
			I = true;
			brk = true;
			break;

		case RTI:  // Return from interrupt
			setP(POP_BYTE_STACK); //  & 0xdb);
			PC = POP_BYTE_STACK | (POP_BYTE_STACK << 8);
			break;

		case NOP:
			break;

		case PHA:  // Push accumulator to stack
			PUSH_BYTE_STACK(A);
			break;
		case PLA:  // Pull accumulator from stack
			A = POP_BYTE_STACK;
			break;
		case PHP:  // Push status to stack
			PUSH_BYTE_STACK(getP());
			break;
		case PLP:  // Pull status from stack
			setP(POP_BYTE_STACK);
			break;

		case TAX:  // Transfer A to X
			SET_FLAG_NZ(X = A);
			break;
		case TXA:  // Transfer A to A
			SET_FLAG_NZ(A = X);
			break;
		case TAY:  // Transfer A to Y
			SET_FLAG_NZ(Y = A);
			break;
		case TYA:  // Transfer Y to A
			SET_FLAG_NZ(A = Y);
			break;
		case TXS:  // Transfer X to SP
			SET_FLAG_NZ(SP = X);
			break;
		case TSX:  // Transfer SP to X
			SET_FLAG_NZ(X = SP);
			break;

		default:
			cout << "ILLEGAL OPCODE!" << endl;
			break;
	};

	if (!quiet)
		printStat(pc_initial);

	return opcode_cycles[instruction].cycles;
}
