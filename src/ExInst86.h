#ifndef __EXINST86_H__ 
#define __EXINST86_H__ 

//void enterFAULT(struct stMachineState *pM, uint16_t int_num, uint16_t cs, uint32_t eip, );

void enterINTwithECODE(struct stMachineState *pM, uint16_t int_num, uint16_t cs, uint32_t eip, uint32_t error_code);
void enterINT         (struct stMachineState *pM, uint16_t int_num, uint16_t cs, uint32_t eip, int isSoftInt);

int exPrefixDummy(struct stMachineState *pM, uint32_t pointer);

int exESC(struct stMachineState *pM, uint32_t pointer);
int exWAIT(struct stMachineState *pM, uint32_t pointer);
int exHLT(struct stMachineState *pM, uint32_t pointer);

int exMov(struct stMachineState *pM, uint32_t pointer);
int exPUSH(struct stMachineState *pM, uint32_t pointer);
int exPOP(struct stMachineState *pM, uint32_t pointer);
int exXCHG(struct stMachineState *pM, uint32_t pointer);
int exINOUT(struct stMachineState *pM, uint32_t pointer);
int exXLAT(struct stMachineState *pM, uint32_t pointer);
int exLEA(struct stMachineState *pM, uint32_t pointer);
int exLDSLES(struct stMachineState *pM, uint32_t pointer);
int exAHF(struct stMachineState *pM, uint32_t pointer);
int exPUSHFPOPF(struct stMachineState *pM, uint32_t pointer);

int exALU2OP(struct stMachineState *pM, uint32_t pointer);
int exINCDEC(struct stMachineState *pM, uint32_t pointer);
int exNEGNOT(struct stMachineState *pM, uint32_t pointer);
int exMUL(struct stMachineState *pM, uint32_t pointer);
int exDIV(struct stMachineState *pM, uint32_t pointer);
int exDASAAS(struct stMachineState *pM, uint32_t pointer);
int exAADAAM(struct stMachineState *pM, uint32_t pointer);
int exDAA(struct stMachineState *pM, uint32_t pointer);
int exConvSiz(struct stMachineState *pM, uint32_t pointer);

int exShift(struct stMachineState *pM, uint32_t pointer);
int exTEST(struct stMachineState *pM, uint32_t pointer);
int exStringInst(struct stMachineState *pM, uint32_t pointer);

int exCALL(struct stMachineState *pM, uint32_t pointer);
int exJMP(struct stMachineState *pM, uint32_t pointer);
int exRET(struct stMachineState *pM, uint32_t pointer);
int exCondJump(struct stMachineState *pM, uint32_t pointer);
int exLOOP(struct stMachineState *pM, uint32_t pointer);
int exJCXZ(struct stMachineState *pM, uint32_t pointer);

int exINT(struct stMachineState *pM, uint32_t pointer);
int exINTO(struct stMachineState *pM, uint32_t pointer);
int exIRET(struct stMachineState *pM, uint32_t pointer);

int exSetClearFlag(struct stMachineState *pM, uint32_t pointer);


#endif