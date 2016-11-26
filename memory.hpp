// Copyright (C) 2009-2016 by Anders Piniesjö <pugo@pugo.org>

#ifndef MEMORY_H
#define MEMORY_H

#include <iostream>
#include <ostream>

class Memory
{
public:
	Memory(uint32_t a_Size);
	~Memory();

	void Load(const std::string& a_Path, uint32_t a_Address);
	uint32_t GetSize() { return m_Size; }
	
	void SetMemPos(uint16_t a_Address) { m_Mempos = a_Address; }

	friend Memory& operator<<(Memory& a_Os, unsigned int a_In) {
		a_Os.m_Mem[a_Os.m_Mempos++] = static_cast<uint8_t>(a_In & 0xff);
		return a_Os;
	}

	void Show(uint32_t a_Pos, uint32_t a_Length);

	uint8_t* m_Mem;

protected:
	uint32_t m_Size;
	uint32_t m_Mempos;
};



#endif // MEMORY_H