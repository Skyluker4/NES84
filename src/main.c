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

void main(void) {
	// Set up CPU
	cpuInit();

	// Main loop
	while(true) {
		kb_Scan();
		if(kb_Data[6] & kb_Clear)
			return;
		cpuOp();
	}
}