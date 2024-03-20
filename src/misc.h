#ifndef __MISC_H__ 
#define __MISC_H__ 

void log_printOpl(int loglevel, struct stMachineState *pM, struct stOpl *pOp);

void log_printReg16(int loglevel, struct stMachineState *pM);
void log_printReg32(int loglevel, struct stMachineState *pM);

uint32_t parseHex(char *str);
uint32_t parseDec(char *str);

#endif
