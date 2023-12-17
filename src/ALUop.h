#ifndef __ALUOP_H__ 
#define __ALUOP_H__ 

uint32_t calcParityDoubleWord(uint32_t val);
uint16_t calcParityWord(uint16_t val);
uint8_t  calcParityByte(uint8_t val);

uint32_t ALUOPAdd(struct stMachineState *pM, uint32_t op1, uint32_t op2, int isWord);
uint32_t ALUOPSub(struct stMachineState *pM, uint32_t op1, uint32_t op2, int isWord);

uint32_t ALUOPAdd3(struct stMachineState *pM, uint32_t op1, uint32_t op2, uint32_t op3, int isWord);
uint32_t ALUOPSub3(struct stMachineState *pM, uint32_t op1, uint32_t op2, uint32_t op3, int isWord);

uint32_t ALUOPand(struct stMachineState *pM, uint32_t op1, uint32_t op2, int isWord);
uint32_t ALUOPor (struct stMachineState *pM, uint32_t op1, uint32_t op2, int isWord);
uint32_t ALUOPxor(struct stMachineState *pM, uint32_t op1, uint32_t op2, int isWord);

#endif
