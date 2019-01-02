// System includes
#include <tice.h>

// Project includes
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "rom.h"

uint8_t RAM[0x0800];

// Create registers
uint8_t A = 0, X = 0, Y = 0, S = 0xFD;
#define STACK_ADDR 0x0100

union Flags {
	struct FlagsBool {
	// The compiler doesn't like the names without the prefixes apparently
	bool f_carry:1;
	bool f_zero:1;
	bool f_interrupt:1;
	bool f_decimal:1; // Probably can remove decimal flag
	bool f_overflow:1;
	bool f_negative:1;
	} PBool;
	uint8_t PByte;
} P;

uint16_t PC = 0xC000;

// Console status
// Probably can put into a struct and/or reduce its size
bool reset = true, nmi = false, nmi_edge_detected = false, intr = false;

// Memory Stuff
uint8_t readMem(uint16_t addr) {
    if (addr <= 0x07FF) return RAM[addr];
    else if (addr <= 0x0FFF) return RAM[addr - 0x0800];
    else if (addr <= 0x17FF) return RAM[addr - 0x1000];
    else if (addr <= 0x1FFF) return RAM[addr - 0x1800];
    else if (addr <= 0x2007) return readPPU(addr - 0x2000);
    else if (addr <= 0x3FFF) return readPPU((addr - 0x2008) % 8);
    else if (addr <= 0x4017) return readAPU(addr - 0x4000);
    else if (addr <= 0x401F) return -1; // Test mode not going to be implemented (yet)
    else return ROM[addr - 0x4020];
}

void writeMem(uint16_t addr, uint8_t byte) {
    if (addr <= 0x0800) RAM[addr] = byte;
    else if (addr <= 0x0FFF) RAM[addr - 0x0FFF] = byte;
    else if (addr <= 0x17FF) RAM[addr - 0x17FF] = byte;
    else if (addr <= 0x1FFF) RAM[addr - 0x1FFF] = byte;
    else if (addr <= 0x2007) writePPU(addr - 0x2007, byte);
    else if (addr <= 0x3FFF) writePPU(addr - 0x3FFF, byte);
    else if (addr <= 0x4017) writeAPU(addr - 0x4017, byte);
    else if (addr <= 0x401F); // Test mode not going to be implemented (yet)
    else ROM[addr - 0x4020] = byte;
}

void clearRAM(void) {
	uint16_t c;
	for (c = 0; c < 0x800; ++c) {
		RAM[c] = 0x00;
	}
}

void cpuInit(void) {
	// Set flags
	P.PBool.f_interrupt = true;

	// Clear RAM - had to be a function because reasons??? (compiler threw an error for some reason)
	clearRAM();
}

uint8_t zeropageAddr(void) {
	return readMem(PC++);
}

uint8_t zeropageXAddr(void) {
	return zeropageAddr() + X;
}

uint8_t zeropageYAddr(void) {
	return zeropageAddr() + Y;
}

uint16_t absoluteAddr(void) {
	uint8_t temp = readMem(PC++);
	return (temp | (readMem(PC++)) << 8);
}

uint16_t absoluteXAddr(void) {
	return absoluteAddr() + X;
}

uint16_t absoluteYAddr(void) {
	return absoluteAddr() + Y;
}

uint16_t indirectXAddr(void){
	uint8_t temp = readMem(PC++);
	return (temp | (readMem(PC++)) << 8) + X;
}

uint16_t indirectYAddr(void) {
	uint8_t temp = readMem(PC++);
	return (temp | (readMem(PC++)) << 8) + Y;
}

void pushByte(uint8_t byte) {
	RAM[STACK_ADDR | S--] = byte;
}

void pushWord(uint16_t word) {
	pushByte(word >> 8);
	pushByte(word & 0x00FF);
}

uint8_t popByte(void) {
	return RAM[STACK_ADDR | S++];
}

uint16_t popWord(void) {
	uint16_t temp = popByte();
	return temp + (popByte() << 8);
}

uint16_t readWord(uint16_t addr) {
	return readMem(addr) + (readMem(addr + 0x0001) << 8);
}

// Operations
void ORA(uint16_t addr) {
	A = A | readMem(addr);
	P.PBool.f_zero = !A;
	P.PBool.f_negative = A >> 7;
}

void LDA(uint16_t addr) {
	A = readMem(addr);
	P.PBool.f_zero = !A;
	P.PBool.f_negative = A >> 7;
}

void CMP(uint16_t addr) {
	uint8_t temp = A - readMem(addr);
	P.PBool.f_carry = A >= temp;
	P.PBool.f_zero = !temp;
	P.PBool.f_negative = temp >> 7;
}

void BIT(uint16_t addr) {
	uint8_t temp = readMem(addr);
	P.PBool.f_negative = temp >> 7;
	P.PBool.f_overflow = (temp >> 6) & 1;
	P.PBool.f_zero = A & temp ? false : true;
}

void INC(uint16_t addr) {
	uint8_t temp = readMem(addr);
	temp++;
	P.PBool.f_zero = !temp;
	P.PBool.f_negative = temp >> 7;
	writeMem(addr, temp);
}

void LSR(uint16_t addr) {
	uint8_t temp = readMem(addr);
	P.PBool.f_carry = temp & 1;
	temp = temp >> 1;
	P.PBool.f_zero = !temp;
	P.PBool.f_negative = false;
	writeMem(addr, temp);
}

void AND(uint16_t addr) {
	A = A & readMem(addr);
	P.PBool.f_zero = !A;
	P.PBool.f_negative = A >> 7;
}

void ASL(uint16_t addr) {
	uint8_t temp = readMem(addr);
	P.PBool.f_carry = temp >> 7;
	temp = temp << 1;
	P.PBool.f_zero = !temp;
	P.PBool.f_negative = temp >> 7;
	writeMem(addr, temp);
}

void ROL(uint16_t addr) {
	uint8_t temp = readMem(addr);
	bool carryTemp = temp >> 7;
	temp = (temp << 1) | P.PBool.f_carry;
	P.PBool.f_carry = carryTemp;
	P.PBool.f_zero = !temp;
	P.PBool.f_negative = temp >> 7;
	writeMem(addr, temp);
}

void EOR(uint16_t addr) {
	A = A ^ readMem(addr);
	P.PBool.f_zero = !A;
	P.PBool.f_negative = A >> 7;
}

void ADC(uint16_t addr) {
	uint16_t temp = A + readMem(addr) + P.PBool.f_carry;
	P.PBool.f_overflow = temp >> 8;
	A = temp;
	P.PBool.f_zero = !A;
	P.PBool.f_negative = A >> 7;
}

void ROR(uint16_t addr) {
	uint8_t temp = readMem(addr);
	bool carryTemp = P.PBool.f_carry;
	P.PBool.f_carry = temp & 1;
	temp = (temp >> 1) | (carryTemp << 7);
	P.PBool.f_zero = !temp;
	P.PBool.f_negative = temp >> 7;
	writeMem(addr, temp);
}

void LDY(uint16_t addr) {
	Y = readMem(addr);
	P.PBool.f_zero = !Y;
	P.PBool.f_negative = Y >> 7;
}

void LDX(uint16_t addr) {
	X = readMem(addr);
	P.PBool.f_zero = !X;
	P.PBool.f_negative = Y >> X;
}

void CPY(uint16_t addr) {
	uint8_t temp = Y - readMem(addr);
	P.PBool.f_carry = Y >= temp;
	P.PBool.f_zero = !temp;
	P.PBool.f_negative = temp >> 7;
}

void DEC(uint16_t addr) {
	uint8_t temp = readMem(addr) - 1;
	P.PBool.f_zero = !temp;
	P.PBool.f_negative = temp >> 7;
	writeMem(addr, temp);
}

void CPX(uint16_t addr) {
	uint8_t temp = X - readMem(addr);
	P.PBool.f_carry = X >= temp;
	P.PBool.f_zero = !temp;
	P.PBool.f_negative = temp >> 7;
}

void SBC(uint16_t addr) {
	uint16_t temp = A - readMem(addr) - !P.PBool.f_carry;
	P.PBool.f_carry = temp >> 8;
	A = temp;
	P.PBool.f_zero = !A;
	P.PBool.f_negative = A >> 7;
}

// Execute operations
void cpuOp(void) {
    uint8_t op = readMem(PC++);

	switch(op) {
		// 27 most frequently used opcodes at top
		case 0xA5: // LDA zero-page
			LDA(zeropageAddr());
			break;
		case 0xD0: // BNE relative
			if (!P.PBool.f_zero) PC += (int8_t)readMem(PC++);
			break;
		case 0x4C: // JMP absolute
			PC = readMem(absoluteAddr());
			break;
		case 0xE8: // INX
			X++;
			P.PBool.f_zero = !X;
			P.PBool.f_negative = X >> 7;
			break;
		case 0x10: // BPL relative
			if(!P.PBool.f_negative) PC += (int8_t)readMem(PC++);
			break;
		case 0xC9: // CMP immediate
			CMP(PC++);
			break;
		case 0x30: // BMI relative
			if(P.PBool.f_negative) PC += (int8_t)readMem(PC++);
			break;
		case 0xF0: // BEQ relative
			if(P.PBool.f_zero) PC += (int8_t)readMem(PC++);
			break;
		case 0x24: // BIT zero-page
			BIT(zeropageAddr());
			break;
		case 0x85: // STA zero-page
			writeMem(PC++, A);
			break;
		case 0x88: // DEY
			Y--;
			P.PBool.f_zero = !Y;
			P.PBool.f_negative = Y >> 7;
			break;
		case 0xC8: // INY
			Y++;
			P.PBool.f_zero = !Y;
			P.PBool.f_negative = Y >> 7;
			break;
		case 0xA8: // TAY
			Y = A;
			P.PBool.f_zero = !Y;
			P.PBool.f_negative = Y >> 7;
			break;
		case 0xE6: // INC zero-page
			INC(zeropageAddr());
			break;
		case 0xB0: // BCS relative
			if(P.PBool.f_carry) PC += readMem(PC++);
			break;
		case 0xBD: // LDA absolute,X
			LDA(absoluteXAddr());
			break;
		case 0xB5: // LDA zero-page,X
			LDA(zeropageXAddr());
			break;
		case 0xAD: // LDA absolute
			LDA(absoluteAddr());
			break;
		case 0x20: // JSR absolute
			pushWord(PC);
			PC = readMem(absoluteAddr());
			break;
		case 0x4A: // LSR accumulator
			P.PBool.f_carry = 0x01 & A;
			A = A >> 1;
			P.PBool.f_zero = A;
			P.PBool.f_negative = false;
			break;
		case 0x60: // RTS
			PC = popWord();
			break;
		case 0xB1: // LDA indirect,Y
			LDA(indirectYAddr());
			break;
		case 0x29: // AND immediate
			AND(PC++);
			break;
		case 0x9D: // STA absolute,X
			writeMem(absoluteXAddr(), A);
			break;
		case 0x8D: // STA absolute
			writeMem(absoluteAddr(), A);
			break;
		case 0x18: // CLC
			P.PBool.f_carry = false;
			break;
		case 0xA9: // LDA immediate
			LDA(PC++);
			break;

		// Remaining opcodes sorted from lowest to highest temp
		case 0x00: // BRK
			pushWord(PC);
			pushByte(P.PByte);
			PC = readWord(0xFFFE);
			P.PBool.f_interrupt = true;
			break;
		case 0x01: // ORA indirect,X
			ORA(indirectXAddr());
			break;
		case 0x05: // ORA zero-page
			ORA(zeropageAddr());
			break;
		case 0x06: // ASL zero-page
			ASL(zeropageAddr());
			break;
		case 0x08: // PHP
			pushByte(P.PByte);
			break;
		case 0x09: // ORA immediate
			ORA(PC++);
			break;
		case 0x0A: // ASL accumulator
			P.PBool.f_carry = A >> 7;
			A = A << 1;
			P.PBool.f_zero = !A;
			P.PBool.f_negative = A >> 7;
			break;
		case 0x0D: // ORA absolute
			ORA(absoluteAddr());
			break;
		case 0x0E: // ASL absolute
			ASL(absoluteAddr());
			break;

		case 0x11: // ORA indirect,Y
			ORA(indirectYAddr());
			break;
		case 0x15: // ORA zero-page,X
			ORA(zeropageXAddr());
			break;
		case 0x16: // ASL zero-page,X
			ASL(zeropageXAddr());
			break;
		case 0x19: // ORA absolute,Y
			ORA(absoluteYAddr());
			break;
		case 0x1D: // ORA absolute,X
			ORA(absoluteXAddr());
			break;
		case 0x1E: // ASL absolute,X
			ASL(absoluteXAddr());
			break;

		case 0x21: // AND indirect,X
			AND(indirectXAddr());
			break;
		case 0x25: // AND zero-page
			AND(zeropageAddr());
			break;
		case 0x26: // ROL zero-page
			ROL(zeropageAddr());
			break;
		case 0x28: // PLP
			P.PByte = popByte();
			break;
		case 0x2A: // ROL accumulator
		{
			bool carryTemp = A >> 7;
			A = A << 1;
			A += P.PBool.f_carry;
			P.PBool.f_carry = carryTemp;
			P.PBool.f_zero = !A;
			P.PBool.f_negative = A >> 7;
			break;
		}
		case 0x2C: // BIT absolute
			BIT(absoluteAddr());
			break;
		case 0x2D: // AND absolute
			AND(absoluteAddr());
			break;
		case 0x2E: // ROL absolute
			ROL(absoluteAddr());
			break;

		case 0x31: // AND indirect,Y
			AND(indirectYAddr());
			break;
		case 0x35: // AND zero-page,X
			AND(zeropageXAddr());
			break;
		case 0x36: // ROL zero-page,X
			ROL(zeropageXAddr());
			break;
		case 0x38: // SEC
			P.PBool.f_carry = true;
			break;
		case 0x39: // AND absolute,Y
			AND(absoluteYAddr());
			break;
		case 0x3D: // AND absolute,X
			AND(absoluteXAddr());
			break;
		case 0x3E: // ROL absolute,X
			ROL(absoluteXAddr());
			break;

		case 0x40: // RTI
			P.PByte = popByte();
			PC = popWord();
			break;
		case 0x41: // EOR indirect,X
			EOR(indirectXAddr());
			break;
		case 0x45: // EOR zero-page
			EOR(zeropageAddr());
			break;
		case 0x46: // LSR zero-page
			LSR(zeropageAddr());
			break;
		case 0x48: // PHA
			pushByte(A);
			break;
		case 0x49: // EOR immediate
			EOR(PC++);
			break;
		case 0x4D: // EOR absolute
			EOR(absoluteAddr());
			break;
		case 0x4E: // LSR absolute
			LSR(absoluteAddr());
			break;

		case 0x50: // BVC relative
			if (!P.PBool.f_overflow) PC += (int8_t)readMem(PC++);
			break;
		case 0x51: // EOR indirect,Y
			EOR(indirectYAddr());
			break;
		case 0x55: // EOR zero-page,X
			EOR(zeropageXAddr());
			break;
		case 0x56: // LSR zero-page,X
			LSR(zeropageXAddr());
			break;
		case 0x58: // CLI
			P.PBool.f_interrupt = false;
			break;
		case 0x59: // EOR absolute,Y
			EOR(absoluteYAddr());
			break;
		case 0x5D: // EOR absolute,X
			EOR(absoluteXAddr());
			break;
		case 0x5E: // LSR absolute,X
			LSR(absoluteXAddr());
			break;

		case 0x61: // ADC indirect,X
			ADC(indirectXAddr());
			break;
		case 0x65: // ADC zero-page
			ADC(zeropageAddr());
			break;
		case 0x66: // ROR zero-page
			ROR(zeropageAddr());
			break;
		case 0x68: // PLA
			A = popByte();
			P.PBool.f_zero = !A;
			P.PBool.f_negative = A >> 7;
			break;
		case 0x69: // ADC immediate
			ADC(PC++);
			break;
		case 0x6A: // ROR accumulator
		{
			bool carryTemp = P.PBool.f_carry;
			P.PBool.f_carry = A & 1;
			A = (A >> 1) | (carryTemp << 7);
			P.PBool.f_zero = !A;
			P.PBool.f_negative = A >> 7;
			break;
		}
		case 0x6C: // JMP indirect
			PC = readWord(absoluteAddr());
			break;
		case 0x6D: // ADC absolute
			ADC(absoluteAddr());
			break;
		case 0x6E: // ROR absolute
			ROR(absoluteAddr());
			break;

		case 0x70: // BVS relative
			if (P.PBool.f_overflow) PC += (int8_t)readMem(PC++);
			break;
		case 0x71: // ADC indirect,Y
			ADC(indirectYAddr());
			break;
		case 0x75: // ADC zero-page,X
			ADC(zeropageXAddr());
			break;
		case 0x76: // ROR zero-page,X
			ROR(zeropageXAddr());
			break;
		case 0x78: // SEI
			P.PBool.f_interrupt = true;
			break;
		case 0x79: // ADC absolute,Y
			ADC(absoluteYAddr());
			break;
		case 0x7D: // ADC absolute,X
			ADC(absoluteXAddr());
			break;
		case 0x7E: // ROR absolute,X
			ROR(absoluteXAddr());
			break;

		case 0x81: // STA indirect,X
			writeMem(indirectXAddr(), A);
			break;
		case 0x84: // STY zero-page
			writeMem(zeropageAddr(), Y);
			break;
		case 0x86: // STX zero-page
			writeMem(zeropageAddr(), X);
			break;
		case 0x8A: // TXA
			A = X;
			P.PBool.f_zero = !A;
			P.PBool.f_negative = A >> 7;
			break;
		case 0x8C: // STY absolute
			writeMem(absoluteAddr(), Y);
			break;
		case 0x8E: // STX absolute
			writeMem(absoluteAddr(), X);
			break;

		case 0x90: // BCC relative
			if (!P.PBool.f_carry) PC += (int8_t)readMem(PC++);
			break;
		case 0x91: // STA indirect,Y
			writeMem(indirectYAddr(), A);
			break;
		case 0x94: // STY zero-page,X
			writeMem(zeropageXAddr(), Y);
			break;
		case 0x95: // STA zero-page,X
			writeMem(zeropageXAddr(), A);
			break;
		case 0x96: // STX zero-page,Y
			writeMem(zeropageYAddr(), X);
			break;
		case 0x98: // TYA
			A = Y;
			P.PBool.f_zero = !A;
			P.PBool.f_negative = A >> 7;
			break;
		case 0x99: // STA absolute,Y
			writeMem(absoluteYAddr(), A);
			break;
		case 0x9A: // TXS
			S = X;
			break;

		case 0xA0: // LDY immediate
			LDY(PC++);
			break;
		case 0xA1: // LDA indirect,X
			LDA(indirectXAddr());
			break;
		case 0xA2: // LDX immediate
			LDX(PC++);
			break;
		case 0xA4: // LDY zero-page
			LDY(zeropageAddr());
			break;
		case 0xA6: // LDX zero-page
			LDX(zeropageAddr());
			break;
		case 0xAA: // TAX
			X = A;
			P.PBool.f_zero = !X;
			P.PBool.f_negative = X >> 7;
			break;
		case 0xAC: // LDY absolute
			LDY(absoluteAddr());
			break;
		case 0xAE: // LDX absolute
			LDX(absoluteAddr());
			break;

		case 0xB4: // LDY zero-page,X
			LDY(zeropageXAddr());
			break;
		case 0xB6: // LDX zero-page,Y
			LDX(zeropageYAddr());
			break;
		case 0xB8: // CLV
			P.PBool.f_overflow = false;
			break;
		case 0xB9: // LDA absolute,Y
			LDA(absoluteYAddr());
			break;
		case 0xBA: // TSX
			X = S;
			P.PBool.f_zero = !X;
			P.PBool.f_negative = X >> 7;
			break;
		case 0xBC: // LDY absolute,X
			LDY(absoluteXAddr());
			break;
		case 0xBE: // LDX absolute,Y
			LDX(absoluteYAddr());
			break;

		case 0xC0: // CPY immediate
			CPY(PC++);
			break;
		case 0xC1: // CMP indirect,X
			CMP(indirectXAddr());
			break;
		case 0xC4: // CPY zero-page
			CPY(zeropageAddr());
			break;
		case 0xC5: // CMP zero-page
			CMP(zeropageAddr());
			break;
		case 0xC6: // DEC zero-page
			DEC(zeropageAddr());
			break;
		case 0xCA: // DEX
			X--;
			P.PBool.f_zero = !X;
			P.PBool.f_negative = X >> 7;
			break;
		case 0xCC: // CPY absolute
			CPY(absoluteAddr());
			break;
		case 0xCD: // CMP absolute
			CMP(absoluteAddr());
			break;
		case 0xCE: // DEC absolute
			DEC(absoluteAddr());
			break;

		case 0xD1: // CMP indirect,Y
			CMP(indirectYAddr());
			break;
		case 0xD5: // CMP zero-page,X
			CMP(zeropageXAddr());
			break;
		case 0xD6: // DEC zero-page,X
			DEC(zeropageXAddr());
			break;
		case 0xD8: // CLD
			P.PBool.f_decimal = false;
			break;
		case 0xD9: // CMP absolute,Y
			CMP(absoluteYAddr());
			break;
		case 0xDD: // CMP absolute,X
			CMP(absoluteXAddr());
			break;
		case 0xDE: // DEC absolute,X
			DEC(absoluteXAddr());
			break;

		case 0xE0: // CPX immediate
			CPX(PC++);
			break;
		case 0xE1: // SBC indirect,X
			SBC(indirectXAddr());
			break;
		case 0xE4: // CPX zero-page
			CPX(zeropageAddr());
			break;
		case 0xE5: // SBC zero-page
			SBC(zeropageAddr());
			break;
		case 0xE9: // SBC immediate
			SBC(PC++);
			break;
		case 0xEC: // CPX absolute
			CPX(absoluteAddr());
			break;
		case 0xED: // SBC absolute
			SBC(absoluteAddr());
			break;
		case 0xEE: // INC absolute
			INC(absoluteAddr());
			break;

		case 0xF1: // SBC indirect,Y
			SBC(indirectYAddr());
			break;
		case 0xF5: // SBC zero-page,X
			SBC(zeropageXAddr());
			break;
		case 0xF6: // INC zero-page,X
			INC(zeropageXAddr());
			break;
		case 0xF8: // SED
			P.PBool.f_decimal = true;
			break;
		case 0xF9: // SBC absolute,Y
			SBC(absoluteYAddr());
			break;
		case 0xFD: // SBC absolute,X
			SBC(absoluteXAddr());
			break;
		case 0xFE: // INC absolute,X
			INC(absoluteXAddr());
			break;
		// NOP at the bottom since it doesn't do anything
		case 0xEA: // NOP
			break;
		default: // Error in memory/ROM
			// TODO: Output an error here and end execution
            return;
    }
}