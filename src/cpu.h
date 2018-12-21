#ifndef CPU_HEADER
#define CPU_HEADER

// System includes
#include <tice.h>

// Functions
void cpuInit(void);
void writeMem(uint16_t addr, uint8_t byte);
uint8_t readMem(uint16_t addr);
void cpuOp(void);

#endif