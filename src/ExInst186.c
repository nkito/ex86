#include <stdio.h>
#include <stdint.h>

#include "i8086.h"
//#include "ALUop.h"
#include "ExInst_common.h"
#include "ExInst86.h"
#include "ExInst186.h"
#include "decode.h"
#include "misc.h"
#include "logfile.h"
#include "mem.h"
#include "io.h"
#include "descriptor.h"


int exPUSHimm(struct stMachineState *pM, uint32_t pointer){
    uint32_t size, val;
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);

    if( inst0 == 0x68 ){
        // imm16/32
        size = decode_imm(pM, pointer+1, INST_W_WORDACC, &val, INST_S_NOSIGNEX);
    }else if( inst0 == 0x6a ){
        // imm8
        size = decode_imm(pM, pointer+1, INST_W_WORDACC, &val, INST_S_SIGNEX);
    }else{
        return EX_RESULT_UNKNOWN;
    }

    UPDATE_IP(1 + size);
    PUSH_TO_STACK( val );

    if(DEBUG) EXI_LOG_PRINTF("PUSH imm (=%x)\n", val);

    return EX_RESULT_SUCCESS;
}

int exPUSHA(struct stMachineState *pM, uint32_t pointer){
    uint32_t old_sp = REG_ESP;
    uint8_t inst0  = fetchCodeDataByte(pM, pointer);

    if( inst0 != 0x60 ) return EX_RESULT_UNKNOWN;

    UPDATE_IP(1);

    PUSH_TO_STACK(REG_EAX);
    PUSH_TO_STACK(REG_ECX);
    PUSH_TO_STACK(REG_EDX);
    PUSH_TO_STACK(REG_EBX);
    PUSH_TO_STACK(old_sp);
    PUSH_TO_STACK(REG_EBP);
    PUSH_TO_STACK(REG_ESI);
    PUSH_TO_STACK(REG_EDI);

    if(DEBUG) EXI_LOG_PRINTF("PUSHA\n");

    return EX_RESULT_SUCCESS;
}

int exPOPA(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);

    if( inst0 != 0x61 ) return EX_RESULT_UNKNOWN;

    UPDATE_IP(1);
    if( PREFIX_OP32 ){
        POP_FROM_STACK(REG_EDI);
        POP_FROM_STACK(REG_ESI);
        POP_FROM_STACK(REG_EBP);
        POP_FROM_STACK( inst0 ); // dummy
        POP_FROM_STACK(REG_EBX);
        POP_FROM_STACK(REG_EDX);
        POP_FROM_STACK(REG_ECX);
        POP_FROM_STACK(REG_EAX);
    }else{
        POP_FROM_STACK(REG_DI);
        POP_FROM_STACK(REG_SI);
        POP_FROM_STACK(REG_BP);
        POP_FROM_STACK( inst0 ); // dummy
        POP_FROM_STACK(REG_BX);
        POP_FROM_STACK(REG_DX);
        POP_FROM_STACK(REG_CX);
        POP_FROM_STACK(REG_AX);
    }

    if(DEBUG) EXI_LOG_PRINTF("POPA\n");

    return EX_RESULT_SUCCESS;
}

int exINSOUTS(struct stMachineState *pM, uint32_t pointer){
    uint32_t val;
    struct stOpl op1, op2;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t bit0  = ((inst0>>0) & 1);
    uint8_t bit1  = ((inst0>>1) & 1);
    uint16_t delta  = INST_W_BIT ? (PREFIX_OP32 ? 4 : 2) : 1;
    int32_t  ddelta = (REG_FLAGS & (1<<FLAGS_BIT_DF)) ? -delta : delta;

    if( (inst0&0xfc) != 0x6c ) return EX_RESULT_UNKNOWN;
    UPDATE_IP(1);

    if(DEBUG){
        if( INST_D_BIT == INST_D_FIRSTOP ){
            EXI_LOG_PRINTF("INS%s\n" , INST_W_BIT ? "W" : "B"); 
        }else{
            EXI_LOG_PRINTF("OUTS%s\n", INST_W_BIT ? "W" : "B");
        }
    }

    op2.type = OpTypeMemWithSeg_Reg;
    op2.reg  = SEGREG_NUM_ES; // no segment override is possible (See: iAPX286 Programmers Reference Manual B-44)
    op2.addr = REG_NUM_DI; // <- specify the register number (not the value of the register)
    op2.width= INST_W_BIT;

    op1.type = OpTypeMemWithSeg_Reg;
    op1.reg  = (PREFIX_SEG != PREF_SEG_UNSPECIFIED) ? PREFIX_SEG : SEGREG_NUM_DS; // DS could be altered with a prefix inst.
    op1.addr = REG_NUM_SI; // <- specify the register number (not the value of the register)
    op1.width= INST_W_BIT;

    do{
        if( (PREFIX_AD32 ? REG_ECX : REG_CX) == 0 && (PREFIX_REPZ == 0 || PREFIX_REPZ ==1) ){
            break;
        }

        if( INST_D_BIT == INST_D_FIRSTOP ){
            // INS 

            if( INST_W_BIT ){
                if( PREFIX_OP32 ){
                    val = readIODoubleWord(pM, REG_DX);
                }else{
                    val = readIOWord(pM, REG_DX);
                }
            }else{
                val = readIOByte(pM, REG_DX);
            }

            writeOpl(pM, &op2, val);

            if( PREFIX_AD32 ){ REG_EDI+= ddelta; }
            else             { REG_DI += ddelta; }
        }else{
            // OUTS

            if( INST_W_BIT ){
                if( PREFIX_OP32 ){
                    writeIODoubleWord(pM, REG_DX, readOpl(pM, &op1));
                }else{
                    writeIOWord(pM, REG_DX, readOpl(pM, &op1));
                }
            }else{
                writeIOByte(pM, REG_DX, readOpl(pM, &op1));
            }

            if( PREFIX_AD32 ){ REG_ESI+=ddelta; }
            else             { REG_SI +=ddelta; } 
        }

        if( PREFIX_REPZ == 0 || PREFIX_REPZ == 1 ){
            if( PREFIX_AD32 ){ REG_ECX--; }
            else             { REG_CX--;  }
        }else{
            break;
        }
    }while(1);

    return EX_RESULT_SUCCESS;
}


int exBOUND(struct stMachineState *pM, uint32_t pointer){
    int32_t val, lower, upper; // signed values
    uint16_t size;
    struct stOpl op1, op2;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);

    if( inst0 != 0x62 ) return EX_RESULT_UNKNOWN;


    size = decode_mod_rm(pM, pointer+1, INST_W_WORDACC, &op1); // bounds (RSRC)
    decode_reg2         (pM, pointer+1, INST_W_WORDACC, &op2); // array index (LSRC)

    UPDATE_IP(size + 1);

    if( PREFIX_OP32 ){
        lower = readOpl(pM, &op1); op1.addr += 4;
        upper = readOpl(pM, &op1);
        val   = readOpl(pM, &op2);
    }else{
        lower = (int32_t)((int16_t)readOpl(pM, &op1)); op1.addr += 2;
        upper = (int32_t)((int16_t)readOpl(pM, &op1));
        val   = (int32_t)((int16_t)readOpl(pM, &op2));
    }

    if(DEBUG){
        EXI_LOG_PRINTF("BOUND ");
        log_printOpl(EXI_LOGLEVEL, pM, &op1); EXI_LOG_PRINTF(", ");
        log_printOpl(EXI_LOGLEVEL, pM, &op2); EXI_LOG_PRINTF("(0x%x, 0x%x)\n", lower, upper);
    }

    if( val < lower || val > upper+((PREFIX_OP32) ? 4 : 2) ){
        // TODO: check the address (this address is correct for 486, but correctness for 80186 is unclear)
        enterINT(pM, INTNUM_BOUNDS, pM->reg.current_cs, pM->reg.current_eip, 0);
    }
    return EX_RESULT_SUCCESS;
}



int exIMULimm(struct stMachineState *pM, uint32_t pointer){
    uint16_t size;
    uint32_t lsrc, rsrc;
    uint64_t rsrcs, lsrcs;
    struct stOpl op, op2;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);

    if( inst0 != 0x69 && inst0 != 0x6b ) return EX_RESULT_UNKNOWN;

    if( inst0 == 0x69 ){
        // rw <- ew x dw [word imm]
        size = decode_mod_rm(pM, pointer+1     , INST_W_WORDACC, &op);  // ew
        decode_reg2         (pM, pointer+1     , INST_W_WORDACC, &op2); // rw
        size += decode_imm  (pM, pointer+1+size, INST_W_WORDACC, &lsrc, INST_S_NOSIGNEX);
    }else{
        // rw <- ew x db [byte imm]
        size = decode_mod_rm(pM, pointer+1     , INST_W_WORDACC, &op);
        decode_reg2         (pM, pointer+1     , INST_W_WORDACC, &op2);
        size += decode_imm  (pM, pointer+1+size, INST_W_WORDACC, &lsrc, INST_S_SIGNEX); // byte access (a signed word with sign extention is the result)
    }
    UPDATE_IP(size + 1);

    rsrc = readOpl(pM, &op);

    if(DEBUG){
        EXI_LOG_PRINTF("IMUL ");
        log_printOpl(EXI_LOGLEVEL, pM, &op2); EXI_LOG_PRINTF(",");
        log_printOpl(EXI_LOGLEVEL, pM, &op);  EXI_LOG_PRINTF("(=%x), ", rsrc);
        EXI_LOG_PRINTF("0x%x\n", lsrc);
    }

    if( PREFIX_OP32 ){
        rsrcs = (rsrc & 0x80000000) ? (rsrc|0xffffffff00000000ULL) : rsrc;
        lsrcs = (lsrc & 0x80000000) ? (lsrc|0xffffffff00000000ULL) : lsrc;
    }else{
        rsrcs = (rsrc & 0x8000) ? (rsrc|0xffffffffffff0000ULL) : rsrc;
        lsrcs = (lsrc & 0x8000) ? (lsrc|0xffffffffffff0000ULL) : lsrc;
    }
    int64_t  muls = ((int64_t)rsrcs) * ((int64_t)lsrcs);

    if( PREFIX_OP32 ){
        writeOpl(pM, &op2, (uint32_t)(muls & 0xffffffff));
        if( (muls&0xffffffff80000000ULL) == 0 || (muls&0xffffffff80000000ULL) == 0xffffffff80000000ULL ){
            REG_FLAGS &=~((1<<FLAGS_BIT_CF)|(1<<FLAGS_BIT_OF));
        }else{
            REG_FLAGS |= ((1<<FLAGS_BIT_CF)|(1<<FLAGS_BIT_OF));
        }
    }else{
        writeOpl(pM, &op2, (uint32_t)(muls & 0xffff));
        if( (muls&0xffffffffffff8000ULL) == 0 || (muls&0xffffffffffff8000ULL) == 0xffffffffffff8000ULL ){
            REG_FLAGS &=~((1<<FLAGS_BIT_CF)|(1<<FLAGS_BIT_OF));
        }else{
            REG_FLAGS |= ((1<<FLAGS_BIT_CF)|(1<<FLAGS_BIT_OF));
        }
    }

    return EX_RESULT_SUCCESS;
}


int exENTER(struct stMachineState *pM, uint32_t pointer){
    struct stOpl op;
    uint32_t allocsize, nestlevel, frametmp;
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint32_t saved_ad32 = PREFIX_AD32;


    if(inst0 != 0xc8) return EX_RESULT_UNKNOWN;

    decode_imm16(pM, pointer+1, &allocsize);
    decode_imm  (pM, pointer+3, INST_W_BYTEACC, &nestlevel, INST_S_NOSIGNEX); nestlevel &= (32-1);
    UPDATE_IP(4);

    if(DEBUG) EXI_LOG_PRINTF("ENTER %d %d\n", allocsize, nestlevel); 

    if( PREFIX_OP32 ){ // According to i386 and recent Intel IA-32 manuals, this condition is operand size and is not address nor stack size.
        PUSH_TO_STACK( REG_EBP );
    }else{
        PUSH_TO_STACK( REG_BP );
    }

    if( PREFIX_OP32 ){ // According to a recent Intel IA-32 manual, this condition is operand size. But it is unclear.
        frametmp = REG_ESP;
    }else{
        frametmp = REG_SP;
    }

    op.type = OpTypeMemWithSeg_Reg;
    op.reg  = SEGREG_NUM_SS; // TODO: check possibility of segment override with a prefix instruction
    op.addr = REG_NUM_BP; // <- specify the register number (not the value of the register)
    op.width= INST_W_WORDACC;

    if( nestlevel > 0 ){
        // TODO: the startpoint(i=0) is correct? ("i=1" is correct. Ref. IA32 Intel Archtecture Software Developpers Mannual, Japanese Ed., 2004.)
        for(int i=1; i < nestlevel; i++){  
            if( PREFIX_OP32 ){
                if( pM->reg.descc_ss.big ){
                    REG_EBP -= 4;
                    PREFIX_AD32 = 1; // use EBP for address
                    PUSH_TO_STACK( readOpl(pM, &op) );
                    PREFIX_AD32 = saved_ad32;
                }else{
                    REG_BP  -= 4;
                    PREFIX_AD32 = 0; // use BP for address
                    PUSH_TO_STACK( readOpl(pM, &op) );
                    PREFIX_AD32 = saved_ad32;
                }
            }else{
                if( pM->reg.descc_ss.big ){
                    REG_EBP -= 2;
                    PREFIX_AD32 = 1; // use EBP for address
                    PUSH_TO_STACK( readOpl(pM, &op) );
                    PREFIX_AD32 = saved_ad32;
                }else{
                    REG_BP  -= 2;
                    PREFIX_AD32 = 0; // use BP for address
                    PUSH_TO_STACK( readOpl(pM, &op)  );
                    PREFIX_AD32 = saved_ad32;
                }
            }
        }
        PUSH_TO_STACK( frametmp );
    }

    if( PREFIX_OP32 ){
        REG_EBP = frametmp;
    }else{
        REG_BP = (frametmp & 0xfffe);
    }

    // Note that i386 manual uses "Stack address size" here
    if( pM->reg.descc_ss.big ){
        REG_ESP -= allocsize;
    }else{
        REG_SP -= allocsize;
    }

    return EX_RESULT_SUCCESS;
}

int exLEAVE(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);

    if(inst0 != 0xc9) return EX_RESULT_UNKNOWN;

    if(DEBUG) EXI_LOG_PRINTF("LEAVE \n"); 

    UPDATE_IP(1);

    if( pM->reg.descc_ss.big ){
        REG_ESP = REG_EBP;
    }else{
        REG_SP = REG_BP;
    }

    if( PREFIX_OP32 ){
        POP_FROM_STACK( REG_EBP );
    }else{
        POP_FROM_STACK( REG_BP );
    }

    return EX_RESULT_SUCCESS;
}
