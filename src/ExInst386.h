#ifndef __EXINST386_H__ 
#define __EXINST386_H__ 

int exUD(struct stMachineState *pM, uint32_t pointer);

int exShiftDouble(struct stMachineState *pM, uint32_t pointer);

int exMOVSZX(struct stMachineState *pM, uint32_t pointer);

int exCLTS(struct stMachineState *pM, uint32_t pointer);
int exLARLSL(struct stMachineState *pM, uint32_t pointer);
int exMOVCRDR(struct stMachineState *pM, uint32_t pointer);

int exLSDesc(struct stMachineState *pM, uint32_t pointer);
int exSETcc(struct stMachineState *pM, uint32_t pointer);

int exBT(struct stMachineState *pM, uint32_t pointer);

int exBitScan(struct stMachineState *pM, uint32_t pointer);

int exIMUL2Op(struct stMachineState *pM, uint32_t pointer);

#endif