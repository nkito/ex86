#ifndef __DECODE_H__ 
#define __DECODE_H__ 

uint32_t decode_imm16(struct stMachineState *pM, uint32_t pointer, uint32_t *val);
uint32_t decode_imm32(struct stMachineState *pM, uint32_t pointer, uint32_t *val);

uint32_t decode_mod_rm (struct stMachineState *pM, uint32_t pointer, uint8_t width, struct stOpl *pOp1);
uint32_t decode_immAddr(struct stMachineState *pM, uint32_t pointer, uint8_t width, struct stOpl *pOp1);
uint32_t decode_imm    (struct stMachineState *pM, uint32_t pointer, uint8_t width, uint32_t *val, uint8_t signEx);

/* using the bottom 3 bits (bits[2:0]) */
void decode_reg1       (struct stMachineState *pM, uint32_t pointer, uint8_t width, struct stOpl *pOp1);

/* using the middle 3 bits (bits[5:3]) */
void decode_reg2       (struct stMachineState *pM, uint32_t pointer, uint8_t width, struct stOpl *pOp2); 

void decode_segReg     (struct stMachineState *pM, uint32_t pointer, struct stOpl *pOp);
int  decode_segReg3bit (struct stMachineState *pM, uint32_t pointer, struct stOpl *pOp);

void updateSegReg      (struct stMachineState *pM, uint8_t segReg, uint32_t val);

void     writeOpl      (struct stMachineState *pM, struct stOpl *pOp, uint32_t val);
uint32_t readOpl       (struct stMachineState *pM, struct stOpl *pOp);
uint32_t readOplEA(struct stMachineState *pM, struct stOpl *pOp, uint8_t withSegBase);

#endif
