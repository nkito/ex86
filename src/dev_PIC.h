#ifndef __DEV_PIC_H__ 
#define __DEV_PIC_H__ 

void    writePICReg(struct stMachineState *pM, uint8_t addr, uint8_t data);
uint8_t readPICReg(struct stMachineState *pM, uint8_t addr);

#endif
