#ifndef __IO_H__ 
#define __IO_H__ 

#define IOADDR_PCCOM2_BASE   0x2F8
#define IOADDR_TCU_BASE      0x040

#define IOADDR_SYSCTRL_A 0x92
#define IOADDR_SYSCTRL_B 0x61

#define IOADDR_PIC_MAIN_BASE  0x020
#define IOADDR_PIC_SUB_BASE   0x0A0

uint8_t  readIOByte (struct stMachineState *pM, uint32_t addr);
uint16_t readIOWord (struct stMachineState *pM, uint32_t addr);
uint32_t readIODoubleWord(struct stMachineState *pM, uint32_t addr);

void writeIOByte (struct stMachineState *pM, uint32_t addr, uint8_t  data);
void writeIOWord (struct stMachineState *pM, uint32_t addr, uint16_t data);
void writeIODoubleWord (struct stMachineState *pM, uint32_t addr, uint32_t data);

#endif
