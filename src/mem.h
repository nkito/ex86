#ifndef __MEM_H__ 
#define __MEM_H__ 


#define BIOS_DATA_AREA_CURSOR_X_0    0x450
#define BIOS_DATA_AREA_CURSOR_Y_0    0x451

int checkLinearAccessible(struct stMachineState *pM, uint32_t linear);

uint8_t  fetchCodeDataByte(struct stMachineState *pM, uint32_t addr);
uint8_t  readDataMemByteAsSV(struct stMachineState *pM, uint32_t addr);
uint8_t  readDataMemByte(struct stMachineState *pM, uint32_t addr);
uint16_t readDataMemWord(struct stMachineState *pM, uint32_t addr);
uint32_t readDataMemDoubleWord(struct stMachineState *pM, uint32_t addr);

void writeDataMemByteAsSV(struct stMachineState *pM, uint32_t addr, uint8_t  data);
void writeDataMemByte(struct stMachineState *pM, uint32_t addr, uint8_t  data);
void writeDataMemWord(struct stMachineState *pM, uint32_t addr, uint16_t data);
void writeDataMemDoubleWord(struct stMachineState *pM, uint32_t addr, uint32_t data);

#endif
