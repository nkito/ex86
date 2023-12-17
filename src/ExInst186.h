#ifndef __EXINST186_H__ 
#define __EXINST186_H__ 

int exPUSHimm(struct stMachineState *pM, uint32_t pointer);
int exPUSHA(struct stMachineState *pM, uint32_t pointer);
int exPOPA(struct stMachineState *pM, uint32_t pointer);
int exINSOUTS(struct stMachineState *pM, uint32_t pointer);

int exBOUND(struct stMachineState *pM, uint32_t pointer);

int exIMULimm(struct stMachineState *pM, uint32_t pointer);
int exENTER(struct stMachineState *pM, uint32_t pointer);
int exLEAVE(struct stMachineState *pM, uint32_t pointer);

#endif