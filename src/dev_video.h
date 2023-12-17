#ifndef __DEV_VIDEO_H__ 
#define __DEV_VIDEO_H__ 

#define VIDEO_IOADDR_BASE  0x3d0
#define VIDEO_IOADDR_SIZE  16

void    writeVideoReg(struct stMachineState *pM, uint16_t addr, uint8_t data);
uint8_t readVideoReg (struct stMachineState *pM, uint16_t addr);

#endif
