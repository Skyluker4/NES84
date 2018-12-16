// System includes
#include <tice.h>
#include <keypadc.h>

// Project includes
#include "cpu.h"

// Create RAM
uint8_t RAM[0x800];

void clearRAM(){
	int c;
	for (c = 0; c < 0x800; ++c) {
		RAM[c] = (c & 4) ? 0xFF : 0x00;
	}
	return 0;
}

// Create registers
uint8_t A = 0, X = 0, Y = 0, S = 0;
uint16_t PC = 0;
struct flags {
	uint8_t carry:1;
	uint8_t zero:1;
	uint8_t interrupt:1;
	uint8_t decimal:1; // Probably can remove decimal flag
	uint8_t overflow:1;
	uint8_t negative:1;
};

// Console status
bool reset = true, nmi = false, nmi_edge_detected = false, intr = false;

void cpuOp() {
	struct flags P;
    uint8_t op = RAM[PC++];

    switch(op){
        case 0x69:
            A = A + (bool)P.carry;
            if(A % 2 == 1) P.carry = true;
            break;
        case 0x65:
            break;
        default:
            // Error in memory/ROM
            return;
    }
	return;
}