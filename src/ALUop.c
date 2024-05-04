#include <stdio.h>
#include <stdint.h>

#include "i8086.h"
#include "ALUop.h"
#include "ExInst_common.h"

uint8_t calcParityByte(uint8_t val){
    uint8_t p;
    p = ((val&0xf) ^ ((val>>4)&0xf));
    p = ((p  &0x3) ^ ((p  >>2)&0x3));
    p = ((p  &0x1) ^ ((p  >>1)&0x1));
    return (p^1);
}

static void updateFlags_OSZPC(struct stMachineState *pM, uint16_t of, uint16_t sf, uint16_t zf, uint16_t pf, uint16_t cf){
    REG_FLAGS &= ~((1<<FLAGS_BIT_OF) | (1<<FLAGS_BIT_SF) | (1<<FLAGS_BIT_ZF) | (1<<FLAGS_BIT_PF) | (1<<FLAGS_BIT_CF));
    REG_FLAGS |= ((of<<FLAGS_BIT_OF) |(sf<<FLAGS_BIT_SF) |(zf<<FLAGS_BIT_ZF) |(pf<<FLAGS_BIT_PF) |(cf<<FLAGS_BIT_CF));
}

static void updateFlags_OSZAPC(struct stMachineState *pM, uint16_t of, uint16_t sf, uint16_t zf, uint16_t af, uint16_t pf, uint16_t cf){
    REG_FLAGS &= ~((1<<FLAGS_BIT_OF) | (1<<FLAGS_BIT_SF) | (1<<FLAGS_BIT_ZF) | (1<<FLAGS_BIT_AF) | (1<<FLAGS_BIT_PF) | (1<<FLAGS_BIT_CF));
    REG_FLAGS |= ((of<<FLAGS_BIT_OF) |(sf<<FLAGS_BIT_SF) |(zf<<FLAGS_BIT_ZF) |(af<<FLAGS_BIT_AF) |(pf<<FLAGS_BIT_PF) |(cf<<FLAGS_BIT_CF));
}

uint32_t ALUOPAdd(struct stMachineState *pM, uint32_t op1, uint32_t op2, int isWord){
    if(!isWord)                  { op1 &=   0xff; op2 &=   0xff; }
    else if(! pM->prefix.data32 ){ op1 &= 0xffff; op2 &= 0xffff; }

    uint32_t xor = (op1 ^ op2);

    uint32_t result32 = op1 + op2;
    uint16_t result   = result32;
    uint8_t  result8  = result32;

    uint16_t ovf  = 0;
    uint16_t sign = 0;
    uint16_t zero = 0;
    uint16_t carry= 0;
    if(isWord){
        if( pM->prefix.data32 ){
            if( ((xor&(1<<31)) == 0) && ((op1^result32)&(1<<31)) == (1<<31) ) ovf = 1;
            if( result32 & (1<<31) ) sign = 1;
            if( result32 == 0      ) zero = 1;
            if( result32 < op1 || result32 < op2 ) carry = 1;
        }else{
            if( ((xor&0x8000) == 0) && ((op1^result)&0x8000) == 0x8000 ) ovf = 1;
            if( result & 0x8000    ) sign  = 1;
            if( result == 0        ) zero  = 1;
            if( result32 & 0x10000 ) carry = 1;
        }
    }else{
        if( ((xor&0x80) == 0) && ((op1^result8)&0x80 ) == 0x80 ) ovf = 1;
        if( result8 & 0x80   ) sign  = 1;
        if( result8 == 0     ) zero  = 1;
        if( result32 & 0x100 ) carry = 1;
    }

    updateFlags_OSZAPC(pM,
        ovf,       // overflow flag (signed overflow)
        sign,      // sign flag
        zero,      // zero flag
        (op1&0xf) + (op2&0xf) >= 0x10 ? 1 : 0, // auxillary carry flag
        calcParityByte(result8), // parity of the low byte
        carry ); // carry flag (unsigned overflow)

    return isWord ? ((pM->prefix.data32) ? result32 : result) : result8;
}


uint32_t ALUOPSub(struct stMachineState *pM, uint32_t op1, uint32_t op2, int isWord){
    if(!isWord)                  { op1 &=   0xff; op2 &=   0xff; }
    else if(! pM->prefix.data32 ){ op1 &= 0xffff; op2 &= 0xffff; }

    uint32_t nop2= -op2;
    uint32_t xor = (op1 ^ nop2);

    uint32_t result32 = op1 + nop2;
    uint16_t result   = result32;
    uint8_t  result8  = result32;

    uint16_t ovf  = 0;
    uint16_t sign = 0;
    uint16_t zero = 0;
    uint16_t carry= 0;
    if(isWord){
        if( pM->prefix.data32 ){
            if( ((xor&(1<<31)) == 0) && op2 != (1<<31) && ((op1^result32)&(1<<31)) == (1<<31) ) ovf = 1;
            if( result32&(1<<31) ) sign  = 1;
            if( result32 == 0    ) zero  = 1;
            if( op1 < op2        ) carry = 1;
        }else{
            if( ((xor&0x8000) == 0) && op2 != 0x8000 && ((op1^result)&0x8000) == 0x8000 ) ovf = 1;
            if( result&0x8000 ) sign  = 1;
            if( result == 0   ) zero  = 1;
            if( op1 < op2     ) carry = 1;
        }
    }else{
        if( ((xor&0x80  ) == 0) && op2 != 0x80 && ((op1^result8)&0x80 ) == 0x80 ) ovf = 1;
        if( result8&0x80 ) sign = 1;
        if( result8== 0  ) zero = 1;
        if( ((uint8_t)op1) <  ((uint8_t)op2) ) carry = 1;
    }

    updateFlags_OSZAPC(pM, 
        ovf,       // overflow flag (signed overflow)
        sign,      // sign flag
        zero,      // zero flag
        (op1&0xf) < (nop2&0xf) ? 1 : 0, // auxillary carry flag
        calcParityByte(result8),
        carry ); // carry flag (unsigned overflow)


    return isWord ? ((pM->prefix.data32) ? result32 : result) : result8;
}


/*
Note: 
  op3 should be 0 or 1
*/
uint32_t ALUOPAdd3(struct stMachineState *pM, uint32_t op1, uint32_t op2, uint32_t op3, int isWord){
    uint32_t result_msb2;
    uint32_t result32 = op1 + op2 + op3;
    uint16_t result   = result32;
    uint8_t  result8  = result32;

    uint16_t ovf  = 0;
    uint16_t sign = 0;
    uint16_t zero = 0;
    uint16_t carry= 0;
    if(isWord){
        if( pM->prefix.data32 ){
            result_msb2 = (op1&0x7fffffff) + (op2&0x7fffffff) + (op3&0x7fffffff);

            if( result32 < op1 || result32 < op2 || result32 < op3 ) carry = 1;
            if( (!carry && ((result_msb2&0x80000000) !=0)) ||
                ( carry && ((result_msb2&0x80000000) ==0)) ) ovf = 1;
            if( result32&(1<<31) ) sign  = 1;
            if( result32 == 0    ) zero  = 1;
        }else{
            result_msb2 = (op1&0x7fff) + (op2&0x7fff) + (op3&0x7fff);
            if( ((result32 & 0x10000) == 0 &&  ((result_msb2&0x8000) !=0)) ||
                ((result32 & 0x10000) != 0 &&  ((result_msb2&0x8000) ==0)) ) ovf = 1;
            if( result & 0x8000    ) sign  = 1;
            if( result == 0        ) zero  = 1;
            if( result32 & 0x10000 ) carry = 1;
        }
    }else{
        result_msb2 = (op1&0x7f) + (op2&0x7f) + (op3&0x7f);
        if( ((result32 & 0x100) == 0 &&  ((result_msb2&0x80) !=0)) ||
            ((result32 & 0x100) != 0 &&  ((result_msb2&0x80) ==0)) ) ovf = 1;
        if( result8 & 0x80   ) sign  = 1;
        if( result8 == 0     ) zero  = 1;
        if( result32 & 0x100 ) carry = 1;
    }

    updateFlags_OSZAPC(pM, 
        ovf,       // overflow flag
        sign,      // sign flag
        zero,      // zero flag
        (op1&0xf) + (op2&0xf) + (op3&0xf) >= 0x10 ? 1 : 0, // auxillary carry flag
        calcParityByte(result8),
        carry );   // carry flag


    return isWord ? ((pM->prefix.data32) ? result32 : result) : result8;
}

uint32_t ALUOPSub3(struct stMachineState *pM, uint32_t op1, uint32_t op2, uint32_t op3, int isWord){
    uint32_t nop2= -op2;
    uint32_t xor = (op1 ^ (op3 ? (~op2) : (nop2)));

    uint32_t result32 = op1 + nop2 -op3;
    uint16_t result   = result32;
    uint8_t  result8  = result32;

    uint16_t ovf  = 0;
    uint16_t sign = 0;
    uint16_t zero = 0;
    uint16_t carry= 0;
    if(isWord){
        if( pM->prefix.data32 ){
            if( ((xor&(1<<31)) == 0) && ((op1^result32)&(1<<31)) == (1<<31) ) ovf = 1;
            if(result32&(1<<31) ) sign  = 1;
            if(result32 == 0    ) zero  = 1;
            if( op1 < (op2+op3) ) carry = 1;
        }else{
            if( ((xor&0x8000) == 0) && ((op1^result)&0x8000) == 0x8000 ) ovf = 1;
            // when op2 == 0x80000 (checked with a real intel chip (386EX))
            // 0x8000 - 0x8000 - 1 -> 0xffff and ovf=0
            // 0x8001 - 0x8000 - 1 -> 0x0000 and ovf=1
            if( result&0x8000   ) sign  = 1;
            if( result == 0     ) zero  = 1;
            if( op1 < (op2+op3) ) carry = 1;
        }
    }else{
        if( ((xor&0x80  ) == 0) && ((op1^result8)&0x80 ) == 0x80 ) ovf = 1;
        if(result8&0x80 ) sign = 1;
        if(result8== 0) zero = 1;
        if( (op1&0xff) < (op2&0xff) + (op3&0xff) ) carry = 1;
    }

    updateFlags_OSZAPC(pM, 
        ovf,       // overflow flag (signed overflow)
        sign,      // sign flag
        zero,      // zero flag
        (op1&0xf) < ((op2&0xf) + (op3&0xf)) ? 1 : 0, // auxillary carry flag
        calcParityByte(result8),
        carry );   // carry flag (unsigned overflow)


    return isWord ? ((pM->prefix.data32) ? result32 : result) : result8;
}

uint32_t ALUOPand(struct stMachineState *pM, uint32_t op1, uint32_t op2, int isWord){
    uint32_t land   = (op1&op2);
    uint32_t result = isWord ? ((pM->prefix.data32) ? land : (land&0xffff)) : (land & 0xff);
    uint32_t sign   = isWord ? ((pM->prefix.data32) ? ((result>>31)&1) : ((result>>15)&1)) : ((result>>7)&1);

    updateFlags_OSZPC(pM, 
        0,                     // overflow
        sign,                  // sign flag
        result == 0 ? 1 : 0,   // zero flag
        calcParityByte(result),// parity flag
        0 );                   // carry flag
    return result;
}

uint32_t ALUOPor(struct stMachineState *pM, uint32_t op1, uint32_t op2, int isWord){
    uint32_t lor    = (op1|op2);
    uint32_t result = isWord ? ((pM->prefix.data32) ? lor : (lor&0xffff)) : (lor & 0xff);
    uint32_t sign   = isWord ? ((pM->prefix.data32) ? ((result>>31)&1) : ((result>>15)&1)) : ((result>>7)&1);

    updateFlags_OSZPC(pM, 
        0,                     // overflow
        sign,                  // sign flag
        result == 0 ? 1 : 0,   // zero flag
        calcParityByte(result),// parity flag
        0 );                   // carry flag
    return result;
}

uint32_t ALUOPxor(struct stMachineState *pM, uint32_t op1, uint32_t op2, int isWord){
    uint32_t xor    = (op1^op2);
    uint32_t result = isWord ? ((pM->prefix.data32) ? xor : (xor&0xffff)) : (xor & 0xff);
    uint32_t sign   = isWord ? ((pM->prefix.data32) ? ((result>>31)&1) : ((result>>15)&1)) : ((result>>7)&1);

    updateFlags_OSZPC(pM, 
        0,                     // overflow
        sign,                  // sign flag
        result == 0 ? 1 : 0,   // zero flag
        calcParityByte(result),// parity flag
        0 );                   // carry flag
    return result;
}

