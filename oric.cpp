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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <signal.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <stdarg.h>


#include <iostream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>

#include "oric.h"

using namespace std;


Oric::Oric() : 
	running(false),
	brk(false),
	last_command("")
{
	memory = new Memory(65536);
	cpu = new MOS6502(this, memory);
	mos_6522 = new MOS6522(this, memory);
	
	cpu->memory_read_byte_handler = read_byte;
	cpu->memory_read_byte_zp_handler = read_byte_zp;
	cpu->memory_read_word_handler = read_word;
	cpu->memory_read_word_zp_handler = read_word_zp;
	cpu->memory_write_byte_handler = write_byte;
	cpu->memory_write_byte_zp_handler = write_byte_zp;
}


Oric::~Oric()
{
	delete cpu;
	delete memory;
}


void Oric::reset()
{
	cpu->reset();
}




void Oric::run(long steps)
{
	long cycles = 0;
	long steps_count = 0;
	brk = false;

	while(!brk)
	{
		cycles += cpu->execInstruction(brk);
		mos_6522->exec();

		++steps_count;
		if (steps > 0 && steps_count == steps)
			return;
	}
}


void Oric::monitor()
{
	string cmd;

	while (true)
	{
		cout << ">> " << flush;
		cin.clear();
		getline(cin, cmd);

		if (! handleCommand(cmd))
			break;
	}
}


word Oric::stringToWord(string addr)
{
	word x;
	stringstream ss;
	ss << std::hex << addr;
	ss >> x;
	return x;
}



bool Oric::handleCommand(string line)
{
	if (line.length() == 0)
	{
		if (last_command.length() == 0)
			return true;

		line = last_command;
	}
	else
		last_command = line;

	std::vector<std::string> parts;
	boost::split(parts, line, boost::is_any_of("\t "));
	string cmd = parts[0];

	if (cmd == "h")
	{
		cout << "Available monitor commands:" << endl << endl;
		cout << "h              : help (showing this text)" << endl;
		cout << "g <address>    : go to address and run" << endl;
		cout << "pc <address>   : set program counter to address" << endl;
		cout << "s [n]          : step one or possible n steps" << endl;
		cout << "i              : print machine info" << endl;
		cout << "m <address> <n>: dump memory from address and n bytes ahead" << endl;
		cout << "" << endl;
		return true;
	}

	if (cmd == "g")			// go <address>>> >> 
	{
		long steps = 0;
		if (parts.size() == 2)
			steps = std::stol(parts[1]);

		run(steps);
	}

	else if (cmd == "pc")	// set pc
	{
		if (parts.size() < 2)
		{
			cout << "Error: missing address" << endl;
			return true;
		}
		word addr = stringToWord(parts[1]);
		cpu->setPC(addr);
		cpu->printStat(cpu->getPC());
	}

	else if (cmd == "s")	// step
	{
		if (parts.size() == 2)
			run(std::stol(parts[1]));
		else
		{
			bool brk = false;
			cpu->execInstruction(brk);
			if (brk) cout << "Instruction BRK executed." << endl;
		}
	}

	else if (cmd == "i")	// info
	{
		cout << "PC: " << cpu->getPC() << endl;
		cpu->printStat(cpu->getPC());
	}

	else if (cmd == "m")	// info
	{
		if (parts.size() < 3)
		{
			cout << "Use: m <start address> <length>" << endl;
			return true;
		}
		memory->show(stringToWord(parts[1]), stringToWord(parts[2]));
	}

	else if (cmd == "quiet")
	{
		cpu->setQuiet(true);
	}

	else if (cmd == "q")	// quit
	{
		cout << "quit" << endl;
		return false;
	}

	return true;
}


// --- Memory functions

byte inline Oric::read_byte(Oric &oric, uint16_t address)
{
	if (address >= 0x300 && address < 0x400)
	{
		cout << "read: " << hex << address << endl;
		return oric.getVIA().readByte(address);
	}

	return oric.getMemory().mem[address];
}

byte inline Oric::read_byte_zp(Oric &oric, byte address)
{
	return oric.getMemory().mem[address];
}

uint16_t inline Oric::read_word(Oric &oric, uint16_t address)
{
	if (address >= 0x300 && address < 0x400)
	{
		cout << "read word: " << hex << address << endl;
	}

	return oric.getMemory().mem[address] | oric.getMemory().mem[address+1]<<8;
}

uint16_t inline Oric::read_word_zp(Oric &oric, byte address)
{
	return oric.getMemory().mem[address] | oric.getMemory().mem[address+1 & 0xff]<<8;
}

void inline Oric::write_byte(Oric &oric, uint16_t address, byte val)
{
	if (address >= 0xc000)
		return;
	if (address >= 0x300 && address < 0x400)
	{
		oric.getVIA().writeByte(address, val);
	}

	oric.getMemory().mem[address] = val;
}

void inline Oric::write_byte_zp(Oric &oric, byte address, byte val)
{
	if (address >= 0x00ff)
		return;

	oric.getMemory().mem[address] = val;
}




// -------------------------------------------------------

static void signal_handler(int);
void init_signals(void);

struct sigaction sigact;

Oric oric;


static void signal_handler(int sig)
{
	cout << "Signal: " << sig << endl;
	if (sig == SIGINT)
		oric.stop();
}


void init_signals(void)
{
	sigact.sa_handler = signal_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, (struct sigaction *)NULL);
}



int main(int argc, char *argv[])
{
	char pwd[1024];
	getcwd(pwd, 1024);
	cout << "pwd: " << pwd << endl;

	init_signals();
	
	//oric.getMemory()->load("/home/pugo/projekt/oric/ROMS/basic10.rom", 0xc000);
	// 	oric.getMemory()->load("/home/pugo/projekt/oric/ROMS/dayofweek.rom", 0xc000);
	//oric.getMemory()->load("/home/pugo/projekt/oric/ROMS/AllSuiteA.rom", 0x4000);

	oric.getMemory().load("ROMS/basic11b.rom", 0xc000);
	oric.getMemory().load("ROMS/font.rom", 0xb400);

	oric.reset();
	cout << endl;
	oric.monitor();

	return 0;
}