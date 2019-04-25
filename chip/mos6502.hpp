// =========================================================================
//   Copyright (C) 2009-2018 by Anders Piniesjö <pugo@pugo.org>
// =========================================================================

#ifndef MOS6502_H
#define MOS6502_H

#include "mos6502_opcodes.hpp"

#include <memory>


#define STACK_BOTTOM 0x0100

#define NMI_VECTOR_L 0xFFFA
#define NMI_VECTOR_H 0xFFFB

#define RESET_VECTOR_L 0xFFFC
#define RESET_VECTOR_H 0xFFFD

#define IRQ_VECTOR_L 0xFFFE
#define IRQ_VECTOR_H 0xFFFF

class Machine;
class MOS6502;
class Memory;

typedef uint8_t (*f_memory_read_byte_handler)(Machine &oric, uint16_t address);
typedef uint8_t (*f_memory_read_byte_zp_handler)(Machine &oric, uint8_t address);

typedef uint16_t (*f_memory_read_word_handler)(Machine &oric, uint16_t address);
typedef uint16_t (*f_memory_read_word_zp_handler)(Machine &oric, uint8_t address);

typedef void (*f_memory_write_byte_handler)(Machine &oric, uint16_t address, uint8_t val);
typedef void (*f_memory_write_byte_zp_handler)(Machine &oric, uint8_t address, uint8_t val);


class MOS6502
{
public:
	MOS6502(Machine& a_Machine);
	~MOS6502();

	void SetPC(uint16_t a_PC) { PC = a_PC; }
	uint16_t GetPC() { return PC; }
	uint8_t GetSP() { return SP; }
	uint8_t GetP();
	void SetP(uint8_t a_P);

	void SetQuiet(bool a_Val) { m_Quiet = a_Val; }

	void Reset();
	void PrintStat();
	void PrintStat(uint16_t a_Address);

	bool ExecInstructionCycles(int16_t a_Cycles);
	short ExecInstruction(bool& a_Brk);

	// Registers
	uint8_t A;
	uint8_t X;
	uint8_t Y;

	// Flags
	//   7                           0
	// +---+---+---+---+---+---+---+---+
	// | N | V |   | B | D | I | Z | C |  <-- flag, 0/1 = reset/set
	// +---+---+---+---+---+---+---+---+
	uint8_t N_INTERN; // negative
	uint8_t Z_INTERN; // zero
	bool V; // overflow
	bool B; // break
	bool D; // decimal
	bool I; // interrupt
	bool C; // carry

	void NMI();
	void IRQ();

	int inline SignedByteToInt(uint8_t a_B);

	// Add and sub are complex
	void ADC(uint8_t a_Val);
	void SBC(uint8_t a_Val);

	f_memory_read_byte_handler memory_read_byte_handler;
	f_memory_read_byte_zp_handler memory_read_byte_zp_handler;

	f_memory_read_word_handler memory_read_word_handler;
	f_memory_read_word_zp_handler memory_read_word_zp_handler;

	f_memory_write_byte_handler memory_write_byte_handler;
	f_memory_write_byte_zp_handler memory_write_byte_zp_handler;

	std::string Disassemble(uint16_t a_Address);

protected:
	void Handle_IRQ();
	
	Machine& m_Machine;
	Memory& m_Memory;

	uint16_t PC;
	uint8_t SP;
	bool m_Quiet;
	bool m_IRQFlag;
};

#endif // MOS6502_H
