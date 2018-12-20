// System includes
#include <tice.h>
#include <keypadc.h>

// Standard includes
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Project includes
#include "cpu.h"

// Create RAM
uint8_t RAM[0x800];

// Create registers
uint8_t A = 0, X = 0, Y = 0, S = 0xFD;
#define STACK_ADDR 0x0100

struct Flags {
	// The compiler doesn't like the names without the prefixes apparently
	bool f_carry:1;
	bool f_zero:1;
	bool f_interrupt:1;
	bool f_decimal:1; // Probably can remove decimal flag
	bool f_overflow:1;
	bool f_negative:1;
} P;

uint16_t PC = 0xC000;

// Console status
// Probably can put into a struct and/or reduce its size
bool reset = true, nmi = false, nmi_edge_detected = false, intr = false;

void clearRAM(void) {
	uint16_t c;
	for (c = 0; c < 0x800; ++c) {
		RAM[c] = 0x00;
	}
}

void cpuInit(void) {
	// Set flags
	P.f_interrupt = true;

	// Clear RAM - had to be a function because reasons??? (compiler threw an error for some reason)
	clearRAM();
}

uint8_t zeropageXAddr(void) {
	return PC++ + X;
}

uint16_t absoluteAddr(void) {
	return RAM[PC++] + (RAM[PC++] << 8);
}

uint16_t absoluteXAddr(void) {
	return absoluteAddr() + X;
}

uint16_t absoluteYAddr(void) {
	return absoluteAddr() + Y;
}

uint16_t indirectXAddr(void){
	return RAM[PC++ + X] + (RAM[PC++ + X] << 8);
}

uint16_t indirectYAddr(void) {
	return RAM[PC++] + (RAM[PC++] << 8) + Y;
}

void pushWord(uint16_t word){
	RAM[STACK_ADDR | S--] = word >> 8;
	RAM[STACK_ADDR | S--] = word & 0x00FF;
}

uint16_t popWord(void){
	return RAM[STACK_ADDR | S++] + (RAM[STACK_ADDR | S++] << 8);
}

void cpuOp(void) {
    uint8_t op = RAM[PC++];
	uint8_t temp;

	switch(op){
		// 27 most frequently used opcodes at top
		case 0xA5: // LDA zero-page
			A = RAM[PC++];
			P.f_zero = !A;
			P.f_negative = A >> 7;
			break;
		case 0xD0: // BNE relative
			if (!P.f_zero) PC += (int8_t)RAM[PC++];
			break;
		case 0x4C: // JMP absolute
			PC = RAM[absoluteAddr()];
			break;
		case 0xE8: // INX
			X++;
			P.f_zero = !X;
			P.f_negative = X >> 7;
			break;
		case 0x10: // BPL relative
			if(!P.f_negative) PC += (int8_t)RAM[PC++];
			break;
		case 0xC9: // CMP immediate
			PC = RAM[PC++];
			break;
		case 0x30: // BMI relative
			if(P.f_negative) PC += (int8_t)RAM[PC++];
			break;
		case 0xF0: // BEQ relative
			if(P.f_zero) PC += (int8_t)RAM[PC++];
			break;
		case 0x24: // BIT zero-page
			temp = RAM[PC++];
			P.f_negative = temp >> 7;
			P.f_overflow = temp >> 6;
			P.f_zero = A & temp ? false : true;
			break;
		case 0x85: // STA zero-page
			RAM[PC++] = A;
			break;
		case 0x88: // DEY
			Y--;
			P.f_zero = !Y;
			P.f_negative = Y >> 7;
			break;
		case 0xC8: // INY
			Y++;
			P.f_zero = !Y;
			P.f_negative = Y >> 7;
			break;
		case 0xA8: // TAY
			Y = A;
			P.f_zero = !Y;
			P.f_negative = Y >> 7;
			break;
		case 0xE6: // INC zero-page
			temp = RAM[RAM[PC]];
			temp++;
			P.f_zero = !temp;
			P.f_negative = temp >> 7;
			RAM[RAM[PC++]] = temp;
			break;
		case 0xB0: // BCS relative
			if(P.f_carry) PC += RAM[PC++];
			break;
		case 0xBD: // LDA absolute,X
			A = RAM[absoluteXAddr()];
			P.f_zero = !A;
			P.f_negative = A >> 7;
			break;
		case 0xB5: // LDA zero-page,X
			A = RAM[zeropageXAddr()];
			P.f_zero = !A;
			P.f_negative = A >> 7;
			break;
		case 0xAD: // LDA absolute
			A = RAM[absoluteAddr()];
			P.f_zero = !A;
			P.f_negative = A >> 7;
			break;
		case 0x20: // JSR absolute
			pushWord(PC);
			PC = RAM[absoluteAddr()];
			break;
		case 0x4A: // LSR accumulator
			P.f_carry = 0x01 & A;
			A = A >> 1;
			P.f_zero = A;
			P.f_negative = false;
			break;
		case 0x60: // RTS
			PC = popWord();
			break;
		case 0xB1: // LDA indirect,Y
			A = RAM[indirectYAddr()];
			P.f_zero = !A;
			P.f_negative = A >> 7;
			break;
		case 0x29: // AND immediate
			A = A & RAM[PC++];
			P.f_zero = !A;
			P.f_negative = A >> 7;
			break;
		case 0x9D: // STA absolute,X
			RAM[absoluteXAddr()] = A;
			break;
		case 0x8D: // STA absolute
			RAM[absoluteAddr()] = A;
			break;
		case 0x18: // CLC
			P.f_carry = false;
			break;
		case 0xA9: // LDA immediate
			A = RAM[PC++];
			P.f_zero = !A;
			P.f_negative = A >> 7;
			break;

		// Remaining opcodes sorted from lowest to highest temp
		case 0x00: // BRK
			break;
		case 0x01: // ORA indirect,X
			break;
		case 0x05: // ORA zero-page
			break;
		case 0x06: // ASL zero-page
			break;
		case 0x08: // PHP
			break;
		case 0x09: // ORA immediate
			break;
		case 0x0A: // ASL accumulator
			break;
		case 0x0D: // ORA absolute
			break;
		case 0x0E: // ASL absolute
			break;

		case 0x11: // ORA indirect,Y
			break;
		case 0x15: // ORA zero-page,X
			break;
		case 0x16: // ASL zero-page,X
			break;
		case 0x19: // ORA absolute,Y
			break;
		case 0x1D: // ORA absolute,X
			break;
		case 0x1E: // ASL absolute,X
			break;

		case 0x21: // AND indirect,X
			break;
		case 0x25: // AND zero-page
			break;
		case 0x26: // ROL zero-page
			break;
		case 0x28: // PLP
			break;
		case 0x2A: // ROL accumulator
			break;
		case 0x2C: // BIT absolute
			break;
		case 0x2D: // AND absolute
			break;
		case 0x2E: // ROL absolute
			break;

		case 0x31: // AND indirect,Y
			break;
		case 0x35: // AND zero-page,X
			break;
		case 0x36: // ROL zero-page,X
			break;
		case 0x38: // SEC
			break;
		case 0x39: // AND absolute,Y
			break;
		case 0x3D: // AND absolute,X
			break;
		case 0x3E: // ROL absolute,X
			break;

		case 0x40: // RTI
			break;
		case 0x41: // EOR indirect,X
			break;
		case 0x45: // EOR zero-page
			break;
		case 0x46: // LSR zero-page
			break;
		case 0x48: // PHA
			break;
		case 0x49: // EOR immediate
			break;
		case 0x4D: // EOR absolute
			break;
		case 0x4E: // LSR absolute
			break;

		case 0x50: // BVC relative
			break;
		case 0x51: // EOR indirect,Y
			break;
		case 0x55: // EOR zero-page,X
			break;
		case 0x56: // LSR zero-page,X
			break;
		case 0x58: // CLI
			break;
		case 0x59: // EOR absolute,Y
			break;
		case 0x5D: // EOR absolute,X
			break;
		case 0x5E: // LSR absolute,X
			break;

		case 0x61: // ADC indirect,X
			break;
		case 0x65: // ADC zero-page
			break;
		case 0x66: // ROR zero-page
			break;
		case 0x68: // PLA
			break;
		case 0x69: // ADC immediate
			break;
		case 0x6A: // ROR accumulator
			break;
		case 0x6C: // JMP indirect
			break;
		case 0x6D: // ADC absolute
			break;
		case 0x6E: // ROR absolute
			break;

		case 0x70: // BVS relative
			break;
		case 0x71: // ADC indirect,Y
			break;
		case 0x75: // ADC zero-page,X
			break;
		case 0x76: // ROR zero-page,X
			break;
		case 0x78: // SEI
			break;
		case 0x79: // ADC absolute,Y
			break;
		case 0x7D: // ADC absolute,X
			break;
		case 0x7E: // ROR absolute,X
			break;

		case 0x81: // STA indirect,X
			break;
		case 0x84: // STY zero-page
			break;
		case 0x86: // STX zero-page
			break;
		case 0x8A: // TXA
			break;
		case 0x8C: // STY absolute
			break;
		case 0x8E: // STX absolute
			break;

		case 0x90: // BCC relative
			break;
		case 0x91: // STA indirect,Y
			break;
		case 0x94: // STY zero-page,X
			break;
		case 0x95: // STA zero-page,X
			break;
		case 0x96: // STX zero-page,Y
			break;
		case 0x98: // TYA
			break;
		case 0x99: // STA absolute,Y
			break;
		case 0x9A: // TXS
			break;

		case 0xA0: // LDY immediate
			break;
		case 0xA1: // LDA indirect,X
			break;
		case 0xA2: // LDX immediate
			break;
		case 0xA4: // LDY zero-page
			break;
		case 0xA6: // LDX zero-page
			break;
		case 0xAA: // TAX
			break;
		case 0xAC: // LDY absolute
			break;
		case 0xAE: // LDX absolute
			break;

		case 0xB4: // LDY zero-page,X
			break;
		case 0xB6: // LDX zero-page,Y
			break;
		case 0xB8: // CLV
			break;
		case 0xB9: // LDA absolute,Y
			break;
		case 0xBA: // TSX
			break;
		case 0xBC: // LDY absolute,X
			break;
		case 0xBE: // LDX absolute,Y
			break;

		case 0xC0: // CPY immediate
			break;
		case 0xC1: // CMP indirect,X
			break;
		case 0xC4: // CPY zero-page
			break;
		case 0xC5: // CMP zero-page
			break;
		case 0xC6: // DEC zero-page
			break;
		case 0xCA: // DEX
			break;
		case 0xCC: // CPY absolute
			break;
		case 0xCD: // CMP absolute
			break;
		case 0xCE: // DEC absolute
			break;

		case 0xD1: // CMP indirect,Y
			break;
		case 0xD5: // zero-page,X
			break;
		case 0xD6: // DEC zero-page,X
			break;
		case 0xD8: // CLD
			break;
		case 0xD9: // CMP absolute,Y
			break;
		case 0xDD: // CMP absolute,X
			break;
		case 0xDE: // DEC absolute,X
			break;

		case 0xE0: // CPX immediate
			break;
		case 0xE1: // SBC indirect,X
			break;
		case 0xE4: // CPX zero-page
			break;
		case 0xE5: // SBC zero-page
			break;
		case 0xE9: // SBC immediate
			break;
		case 0xEA: // NOP
			break;
		case 0xEC: // CPX absolute
			break;
		case 0xED: // SBC absolute
			break;
		case 0xEE: // INC absolute
			break;

		case 0xF1: // SBC indirect,Y
			break;
		case 0xF5: // SBC zero-page,X
			break;
		case 0xF6: // INC zero-page,X
			break;
		case 0xF8: // SED
			break;
		case 0xF9: // SBC absolute,Y
			break;
		case 0xFD: // SBC absolute,X
			break;
		case 0xFE: // INC absolute,X
			break;

        default: // Error in memory/ROM
			// Output some kind of error here and end execution
            return;
    }
}