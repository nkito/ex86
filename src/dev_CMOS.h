#ifndef __DEV_CMOS_H__ 
#define __DEV_CMOS_H__ 

#define IOADDR_CMOS_BASE  0x70
#define IOADDR_CMOS_SIZE  0x02
#define IOADDR_CMOS_MASK  0x01

#define CMOS_REG_FDD      0x10
#define CMOS_REG_HDD      0x12

uint8_t readCMOSReg(struct stMachineState *pM, struct stIO_CMOS *pCMOS, uint32_t addr);
void   writeCMOSReg(struct stMachineState *pM, struct stIO_CMOS *pCMOS, uint32_t addr, uint8_t data);

#endif
