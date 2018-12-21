#ifndef APU_HEADER
#define APU_HEADER

// System includes
#include <tice.h>

void writeAPU(uint16_t addr, uint8_t byte);
uint8_t readAPU(uint16_t addr);

#endif