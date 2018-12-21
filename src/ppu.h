#ifndef PPU_HEADER
#define PPU_HEADER

// System includes
#include <tice.h>

void writePPU(uint16_t addr, uint8_t byte);
uint8_t readPPU(uint16_t addr);

#endif