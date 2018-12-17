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

struct Flags {
	// The compiler doesn't like the names without the prefixes apparently
	bool f_carry:1;
	bool f_zero:1;
	bool f_interrupt:1;
	bool f_decimal:1; // Probably can remove decimal flag
	bool f_overflow:1;
	bool f_negative:1;
} P;

uint16_t PC = 0;

// Console status
// Probably can put into a struct and/or reduce its size
bool reset = true, nmi = false, nmi_edge_detected = false, intr = false;

void clearRAM(){
	uint16_t c;
	for (c = 0; c < 0x800; ++c) {
		RAM[c] = (c & 4) ? 0xFF : 0x00;
	}
}

void cpuInit() {
	// Set flags
	P.f_interrupt = true;

	// Clear RAM - had to be a function becuase reasons??? (compiler threw an error for some reason)
	clearRAM();
}

void cpuOp() {
    uint8_t op = RAM[PC++];

    switch(op){
		// 27 most frequently used opcodes at top
		case 0xA5: // LDA zero-page
			break;
		case 0xD0: // BNE
			break;
		case 0x4C: // JMP absolute
			break;
		case 0xE8: // INX
			break;
		case 0x10: // BPL
			break;
		case 0xC9: // CMP immediate
			break;
		case 0x30: // BMI
			break;
		case 0xF0: // BEQ
			break;
		case 0x24: // BIT zero-page
			break;
		case 0x85: // STA zero-page
			break;
		case 0x88: // DEX
			break;
		case 0xC8: // INY
			break;
		case 0xA8: // TAY
			break;
		case 0xE6: // INC zero-page
			break;
		case 0xB0: // BCS
			break;
		case 0xBD: // LDA absolute,X
			break;
		case 0xB5: // LDA zero-page,X
			break;
		case 0xAD: // LDA absolute
			break;
		case 0x20: // JSR absolute
			break;
		case 0x4A: // LSR A
			break;
		case 0x60: // RTS
			break;
		case 0xB1: // LDA zero-page,Y
			break;
		case 0x29: // AND immediate
			break;
		case 0x9D: // STA absolute,X
			break;
		case 0x8D: // STA absolute
			break;
		case 0x18: // CLC
			break;
		case 0xA9: // LDA immediate
			break;
		// Remaining opcodes sorted from lowest to highest value
        default: // Error in memory/ROM
			//throwError("Memory or ROM corrupted!");
            return;
    }
}