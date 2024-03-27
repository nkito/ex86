#include <stdio.h>
#include <stdint.h>

#include "i8086.h"
#include "ALUop.h"
#include "ExInst_common.h"
//#include "ExInst86.h"
#include "ExInst386.h"
#include "decode.h"
#include "descriptor.h"
#include "misc.h"
#include "logfile.h"
#include "mem.h"


int exUD(struct stMachineState *pM, uint32_t pointer){
    uint32_t size;
    struct stOpl op;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = fetchCodeDataByte(pM, pointer+1);

    // 0f ff /r : UD0 r32, r/m32
    // 0f b9 /r : UD1 r32, r/m32
    // 0f 0b    : UD2

    if( inst0 != 0x0f || (inst1 != 0x0b && inst1 != 0xff && inst1 != 0xb9) ){
        return EX_RESULT_UNKNOWN;
    }

    if(inst1 == 0x0b){
        UPDATE_IP(2);
    }else{
        size = decode_mod_rm(pM,  pointer+2, INST_W_WORDACC, &op);
        UPDATE_IP(2+size);
    }

    ENTER_UD;

    return EX_RESULT_SUCCESS; // never reach here
}

int exShiftDouble(struct stMachineState *pM, uint32_t pointer){
    uint32_t size, val1, val2, shamt;
    struct stOpl op1, op2;

    uint16_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint16_t inst1 = fetchCodeDataByte(pM, pointer+1);
    uint16_t bit0 = ((inst1>>0) & 1);
    uint16_t bit3 = ((inst1>>3) & 1);

    int nbits  = (PREFIX_OP32 ? 32 : 16);

    if( inst0 != 0x0f || (inst1 & 0xf6) != 0xa4 ){
        return EX_RESULT_UNKNOWN;
    }

    size  = 2;
    size += decode_mod_rm(pM,  pointer+2, INST_W_WORDACC, &op1);
    decode_reg2          (pM,  pointer+2, INST_W_WORDACC, &op2);

    val1 = readOpl(pM,  &op1);
    val2 = readOpl(pM,  &op2);

    if( bit0 == 0 ){
        decode_imm(pM,  pointer+size, INST_W_BYTEACC, &shamt, INST_S_NOSIGNEX);
        size++;
    }else{
        shamt = REG_CX;
    }

    shamt &= 0x1f;
    UPDATE_IP(size);

    if(DEBUG){
        if(bit3 == 0) EXI_LOG_PRINTF("SHLD ");
        if(bit3 != 0) EXI_LOG_PRINTF("SHRD ");
        log_printOpl(EXI_LOGLEVEL, pM, &op1); EXI_LOG_PRINTF("(= %x),", val1); 
        log_printOpl(EXI_LOGLEVEL, pM, &op2); EXI_LOG_PRINTF("(= %x),", val2); 
        EXI_LOG_PRINTF(" %x\n", shamt);
    }

    if(shamt ==    0) return EX_RESULT_SUCCESS;
    if(shamt > nbits) return EX_RESULT_SUCCESS;

    if( bit3 == 0 ){ // SHLD

        if(val1 & (1<<(nbits - shamt))){ REG_FLAGS |=  (1<<FLAGS_BIT_CF); }
        else                           { REG_FLAGS &= ~(1<<FLAGS_BIT_CF); }

        if( (val1 ^ (val1<<1)) & (1 << (nbits-1)) ){ REG_FLAGS |=  (1<<FLAGS_BIT_OF); }
        else                                       { REG_FLAGS &= ~(1<<FLAGS_BIT_OF); }

        if( shamt == nbits ){
            val1 = val2;
        }else{
            val1 = (val1 << shamt) | ((val2>>(nbits-shamt)) & ((1<<shamt)-1) );
        }

        if( nbits == 16 ){
            val1 &= 0xffff;
        }
        if( val1 & (1<<(nbits-1)) ) REG_FLAGS |= (1<<FLAGS_BIT_SF);
        else                        REG_FLAGS &=~(1<<FLAGS_BIT_SF);

        if( val1 == 0 ) REG_FLAGS |= (1<<FLAGS_BIT_ZF);
        else            REG_FLAGS &=~(1<<FLAGS_BIT_ZF);

        if(calcParityWord(val1)) REG_FLAGS |= (1<<FLAGS_BIT_PF);
        else                     REG_FLAGS &=~(1<<FLAGS_BIT_PF);

    }else{ // SHRD
        if(val1 & (1<<(shamt-1))){ REG_FLAGS |=  (1<<FLAGS_BIT_CF); }
        else                     { REG_FLAGS &= ~(1<<FLAGS_BIT_CF); }

        if( ((val1>>(nbits-1)) ^ val2) & 1 ){ REG_FLAGS |=  (1<<FLAGS_BIT_OF); }
        else                                { REG_FLAGS &= ~(1<<FLAGS_BIT_OF); }

        if( shamt == nbits ){
            val1 = val2;
        }else{
            val1 = ((val1 >> shamt) & ((1<<(nbits-shamt))-1)) | (val2<<(nbits-shamt));
        }

        if( nbits == 16 ){
            val1 &= 0xffff;
        }
        if( val1 & (1<<(nbits-1)) ) REG_FLAGS |= (1<<FLAGS_BIT_SF);
        else                        REG_FLAGS &=~(1<<FLAGS_BIT_SF);

        if( val1 == 0 ) REG_FLAGS |= (1<<FLAGS_BIT_ZF);
        else            REG_FLAGS &=~(1<<FLAGS_BIT_ZF);

        if(calcParityWord(val1)) REG_FLAGS |= (1<<FLAGS_BIT_PF);
        else                     REG_FLAGS &=~(1<<FLAGS_BIT_PF);
    }

    //EXI_LOG_PRINTF(" %x\n", val1);

    writeOpl( pM,  &op1, val1 );

    return EX_RESULT_SUCCESS;
}


int exMOVSZX(struct stMachineState *pM, uint32_t pointer){
    uint32_t val;
    uint32_t size;
    uint32_t save_prefix = PREFIX_OP32;

    struct stOpl op1, op2;

    uint16_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint16_t inst1 = fetchCodeDataByte(pM, pointer+1);
    uint16_t bit0 = ((inst1>>0) & 1);

    // 0F B6, 0F B7 : MOVZX
    // 0F BE, 0F BF : MOVSX

    if( inst0 != 0x0f || !((inst1&0xfe) == 0xb6 || (inst1&0xfe) == 0xbe) ){
        return EX_RESULT_UNKNOWN;
    }


    //-----------------------------------------------------------
    // destination reg
    decode_reg2(pM, pointer+2, INST_W_WORDACC, &op2);

    if(DEBUG){
        EXI_LOG_PRINTF("MOV%cX ", (inst1&0xfe) == 0xb6 ? 'Z' : 'S');
        log_printOpl(EXI_LOGLEVEL, pM, &op2);
        EXI_LOG_PRINTF(", ");
    }

    //-----------------------------------------------------------
    // source
    size = decode_mod_rm(pM, pointer+2, INST_W_BIT, &op1);
    if( INST_W_BIT ) PREFIX_OP32 = 0;
    val  = readOpl(pM,  &op1);

    if(DEBUG){
        log_printOpl(EXI_LOGLEVEL, pM, &op1);
        EXI_LOG_PRINTF(" (=%x)\n", val);
    }
    PREFIX_OP32 = save_prefix;
    //-----------------------------------------------------------
    if( (inst1&0xfe) == 0xbe ){
        if( INST_W_BIT ) val = ((val&0x8000) ? (0xffff0000|val) : val);
        else             val = ((val&0x80)   ? (0xffffff00|val) : val);
    }

    UPDATE_IP( size + 2 );

    //if( INST_W_BIT && inst1 == 0xbf ) PREFIX_OP32 = 1;
    writeOpl( pM,  &op2, val );
    //PREFIX_OP32 = save_prefix;

    return EX_RESULT_SUCCESS;
}


int exCLTS(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = fetchCodeDataByte(pM, pointer+1);

    if( inst0 != 0x0f || inst1 != 0x06 ){
        return EX_RESULT_UNKNOWN;
    }

    UPDATE_IP( 2 );

    if(DEBUG){
        EXI_LOG_PRINTF("CLTS \n"); 
    }

    if( pM->reg.cpl != 0 ){
        ENTER_GP(0);
    }

    pM->reg.cr[0] &= (~(1<<CR0_BIT_TS));

    return EX_RESULT_SUCCESS;
}


int exLARLSL(struct stMachineState *pM, uint32_t pointer){
    struct stOpl op1, op2;
    uint32_t size, result, invalid = 0;
    uint32_t data32 = PREFIX_OP32;
    uint16_t seg;
    struct stRawSegmentDesc RS;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = fetchCodeDataByte(pM, pointer+1);

    if( inst0 != 0x0f || (inst1 != 0x02 && inst1 != 0x03) ){
        return EX_RESULT_UNKNOWN;
    }

    size = decode_mod_rm(pM, pointer+2, INST_W_WORDACC, &op1); // source
    decode_reg2         (pM, pointer+2, INST_W_WORDACC, &op2); // destination

    UPDATE_IP( 2 + size );

    PREFIX_OP32 = 0;
    seg = readOpl(pM, &op1);
    PREFIX_OP32 = data32;
//    seg &= 0xffff;

    if(DEBUG){
        if(inst1 == 0x02 ) EXI_LOG_PRINTF("LAR "); 
        if(inst1 == 0x03 ) EXI_LOG_PRINTF("LSL "); 
        log_printOpl(EXI_LOGLEVEL, pM, &op2); EXI_LOG_PRINTF(", ");
        log_printOpl(EXI_LOGLEVEL, pM, &op1); EXI_LOG_PRINTF(" (=%x)\n", seg);
    }

    logfile_printf(LOGCAT_CPU_MEM | LOGLV_VERBOSE, "GDT limit %x  LDT limit %x\n", pM->reg.gdtr_limit, pM->reg.descc_ldt.limit);

    if( (seg&0x04) == 0 ){
        if( (seg & (~7)) > pM->reg.gdtr_limit ) invalid = 1;
    }else{
        if( (seg & (~7)) > pM->reg.descc_ldt.limit ) invalid = 1;
    }
    if( (seg & (~7)) == 0 ){
        invalid = 1;
    }

    if( ! invalid ){
        loadRawSegmentDesc(pM, seg, &RS);

        if( inst1 == 0x02 ){
            result  = (((uint32_t)RS.access) <<  8);
            result |= (((uint32_t)RS.flags)  << 20);
            result |= (RS.limit & 0xf0000);
        }else{
            if( RS.access & SEGACCESS_CODE_DATA_SEG ){
                // data or code degment
                if( RS.flags & SEGFLAGS_DSEG_GRAN ){
                    result  = ((RS.limit<<12) | 0xfff );
                }else{
                    result  = RS.limit;
                }
            }else{
                result  = RS.limit;
            }
        }
        writeOpl(pM, &op2, result);
    }

    if( ! invalid ){
        REG_FLAGS |= (1<<FLAGS_BIT_ZF);
    }else{
        REG_FLAGS &=~(1<<FLAGS_BIT_ZF);
    }

    return EX_RESULT_SUCCESS;
}


int exMOVCRDR(struct stMachineState *pM, uint32_t pointer){
    struct stOpl op;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = fetchCodeDataByte(pM, pointer+1);
    uint8_t inst2 = fetchCodeDataByte(pM, pointer+2);

    uint8_t idx;

    if( inst0 != 0x0f || (inst1&0xfc) != 0x20 || (inst2 & 0xc0) != 0xc0 ){
        return EX_RESULT_UNKNOWN;
    }

    PREFIX_OP32 = 1; //overwrite

    UPDATE_IP( 3 );

    decode_reg1(pM, pointer+2, INST_W_WORDACC, &op);
    idx = ((inst2>>3)&7);

    if( (inst1&1)==0 && idx > 3 ) return EX_RESULT_UNKNOWN;
    if( (inst1&1)==1 && idx == 4) return EX_RESULT_UNKNOWN;

    if( inst1 & 2 ){
        // CR/DR <- Reg
        if( (inst1&1)==0 ){
            // CR
            if( idx == 0 || idx == 3 ){ // DEBUG
                logfile_printf               (LOGCAT_CPU_EXE | LOGLV_INFO3, "MOV %s%d, ", ((inst1&1)==0) ? "CR" : "DR", idx); 
                log_printOpl                 (LOGCAT_CPU_EXE | LOGLV_INFO3, pM, &op);
                logfile_printf_without_header(LOGCAT_CPU_EXE | LOGLV_INFO3, "(= 0x%x)  (pointer: %x)\n", readOpl(pM, &op), pointer);
            }

            if( pM->reg.cpl != 0 ){
                ENTER_GP(0);
            }

            pM->reg.cr[idx] = readOpl(pM, &op);

            if( idx == 3 ){
                // TLB should be flushed
                flushTLB(pM);
            }

        }else{
            // DR
            pM->reg.dr[idx] = readOpl(pM, &op);
        }
        if(DEBUG){
            EXI_LOG_PRINTF("MOV %s%d, ", ((inst1&1)==0) ? "CR" : "DR", idx); 
            log_printOpl(EXI_LOGLEVEL, pM, &op);
            EXI_LOG_PRINTF("\n");
        }
    }else{
        // Reg <- CR/DR
        if( (inst1&1)==0 ){
            writeOpl(pM, &op, pM->reg.cr[idx]);
        }else{
            writeOpl(pM, &op, pM->reg.dr[idx]);
        }
        if(DEBUG){
            EXI_LOG_PRINTF("MOV "); 
            log_printOpl(EXI_LOGLEVEL, pM, &op);
            EXI_LOG_PRINTF(", %s%d\n", ((inst1&1)==0) ? "CR" : "DR", idx);
        }
    }

    return EX_RESULT_SUCCESS;
}


int exLSDesc(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = fetchCodeDataByte(pM, pointer+1);
    uint8_t inst2 = fetchCodeDataByte(pM, pointer+2);
    uint8_t funct = ((inst2>>3)&0x7);
    uint16_t size = 2;
    uint16_t segval = 0;
    struct stOpl op;
    uint8_t save_op32 = PREFIX_OP32;

    if( inst0 != 0x0f || (inst1 & 0xfe) != 0x00 ){
        return EX_RESULT_UNKNOWN;
    }

    if(inst1 == 0 && ((funct & 0xfc) == 0) ){
        size += decode_mod_rm(pM, pointer+2, INST_W_WORDACC, &op);
        UPDATE_IP( size );

        PREFIX_OP32 = 0;
        //------------------------------
        if(DEBUG){
            if( funct == 0 ) EXI_LOG_PRINTF("SLDT ");
            if( funct == 1 ) EXI_LOG_PRINTF("STR ");
            if( funct == 2 ) EXI_LOG_PRINTF("LLDT ");
            if( funct == 3 ) EXI_LOG_PRINTF("LTR ");

            log_printOpl(EXI_LOGLEVEL, pM, &op);
            if( funct & 2 ){
                EXI_LOG_PRINTF("(=%x)", readOpl(pM, &op));
            }
            EXI_LOG_PRINTF("\n");
        }

        if( funct & 2 ){
            segval = readOpl(pM, &op);
        }else{
            if( funct == 0 ) writeOpl( pM, &op, pM->reg.ldtr );
            if( funct == 1 ) writeOpl( pM, &op, pM->reg.tr   );
        }
        //------------------------------
        PREFIX_OP32 = save_op32;

        if( funct == 2 ){
            pM->reg.ldtr = segval;

            if( pM->reg.ldtr >= pM->reg.gdtr_limit ){
                logfile_printf(LOGCAT_CPU_EXE | LOGLV_ERROR, "LLDT : the ldtr (=0x%x) is out of GDTR limit. (pointer: %x)\n", pM->reg.ldtr, pointer);
            }

            loadRawSegmentDesc(pM, pM->reg.ldtr, &(pM->reg.descc_ldt));
            if( DEBUG ){
                logfile_printf(LOGCAT_CPU_EXE | LOGLV_NOTICE, "LDTR 0x%02x : LDT base 0x%08x, LDT limit 0x%05x (flags 0x%x, acc 0x%02x)\n", 
                pM->reg.ldtr, pM->reg.descc_ldt.base, pM->reg.descc_ldt.limit, pM->reg.descc_ldt.flags, pM->reg.descc_ldt.access);
            }
        }
        if( funct == 3 ){
            struct stRawSegmentDesc RS;
            loadTaskRegister(pM, segval, &RS);
            pM->reg.descc_tr = RS;
            pM->reg.tr       = segval;
            /*
            pM->reg.tr = segval;
            loadRawSegmentDesc(pM, pM->reg.tr, &(pM->reg.descc_tr));
            */

            logfile_printf(LOGCAT_CPU_EXE | LOGLV_INFO3, "LTR 0x%02x (pointer: %x)\n", segval, pointer);
        }

    }else if(inst1 == 0 && funct == 4){
        logfile_printf(LOGCAT_CPU_EXE | LOGLV_WARNING, "VERR \n");
        if(DEBUG) EXI_LOG_PRINTF("VERR \n");
        return EX_RESULT_UNKNOWN;
    }else if(inst1 == 0 && funct == 5){
        logfile_printf(LOGCAT_CPU_EXE | LOGLV_WARNING, "VERW \n");
        if(DEBUG) EXI_LOG_PRINTF("VERW \n");
        return EX_RESULT_UNKNOWN;
    }else if(inst1 == 1 && (funct == 0 || funct == 1) ){
        size += decode_mod_rm(pM, pointer+2, INST_W_WORDACC, &op);
        UPDATE_IP( size );

        if(DEBUG){
            EXI_LOG_PRINTF(funct == 0 ? "SGDT " : "SIDT ");
            log_printOpl(LOGCAT_CPU_EXE | LOGLV_INFO3, pM, &op);
            EXI_LOG_PRINTF("\n");
        }

        struct stOpl opd;
        opd.type = OpTypeMemWithSeg;
        opd.reg  = op.reg;
        opd.addr = readOplEA(pM, &op, 0);

        PREFIX_OP32 = 0;
        if( funct == 0 ){
            // SGDT 
            writeOpl(pM, &opd,  pM->reg.gdtr_limit           ); opd.addr+=2;
            writeOpl(pM, &opd,  pM->reg.gdtr_base     &0xffff); opd.addr+=2;
            writeOpl(pM, &opd, (pM->reg.gdtr_base>>16)&0xffff);
        }else{
            // SIDT
            writeOpl(pM, &opd,  pM->reg.idtr_limit           ); opd.addr+=2;
            writeOpl(pM, &opd,  pM->reg.idtr_base     &0xffff); opd.addr+=2;
            writeOpl(pM, &opd, (pM->reg.idtr_base>>16)&0xffff);
        }
        PREFIX_OP32 = save_op32;

    }else if(inst1 == 1 && (funct == 2 || funct == 3)){ // 2: LGDT, 3: LIDT
        size += decode_mod_rm(pM, pointer+2, INST_W_WORDACC, &op);
        UPDATE_IP( size );
        uint32_t addr = readOplEA(pM, &op, 1);

        uint32_t val16, val32;
        decode_imm16(pM, addr, &val16);
        decode_imm32(pM, addr+2, &val32);

        if(DEBUG){
            logfile_printf(LOGCAT_CPU_EXE | LOGLV_INFO3, funct == 2 ? "LGDT " : "LIDT "); 
            log_printOpl(LOGCAT_CPU_EXE | LOGLV_INFO3, pM, &op);
            logfile_printf_without_header(LOGCAT_CPU_EXE | LOGLV_INFO3, "(=%x) ", addr);
            logfile_printf_without_header(LOGCAT_CPU_EXE | LOGLV_INFO3, "pointer = 0x%x ", pointer);
            logfile_printf_without_header(LOGCAT_CPU_EXE | LOGLV_INFO3, "[base = 0x%x, limit = 0x%x]\n", val32, val16);
        }

        if( pM->reg.cpl != 0 ){
            ENTER_GP(0);
        }

        if( funct == 2 ){
            pM->reg.gdtr_limit = val16;
            pM->reg.gdtr_base  = val32;
        }else{
            pM->reg.idtr_limit = val16;
            pM->reg.idtr_base  = val32;
        }

/*
        if(DEBUG){
            struct stRawSegmentDesc RS;
            struct stGateDesc       GD;
            for(int k=0; k< val16; k+=8){
                if( funct == 2){
                    loadRawSegmentDesc(pM, k, &RS);
                    logfile_printf(LOGCAT_CPU_EXE | LOGLV_INFO3,  "LGDT offset 0x%02x : base 0x%08x, limit 0x%05x, flags 0x%x, acc 0x%02x\n",
                    k, RS.base, RS.limit, RS.flags, RS.access);
                }else{
                    loadIntDesc  (pM, k/8, &GD);
                    logfile_printf(LOGCAT_CPU_EXE | LOGLV_INFO3,  "LIDT offset 0x%02x : selector 0x%08x, offset 0x%05x, acc 0x%02x\n",
                    k, GD.selector, GD.offset, GD.access);
                }
            }
        }
*/

    }else if(inst1 == 1 && (funct == 4 || funct == 6) ){     // SMSW(4), LMSW(6)

        size += decode_mod_rm(pM, pointer+2, INST_W_WORDACC, &op);
        UPDATE_IP( size );

        if(DEBUG){
            PREFIX_OP32 = 0;
            logfile_printf(LOGCAT_CPU_EXE | LOGLV_INFO3, "%cMSW ", funct == 4 ? 'S': 'L');
            log_printOpl(LOGCAT_CPU_EXE | LOGLV_INFO3, pM, &op);
            logfile_printf_without_header(LOGCAT_CPU_EXE | LOGLV_INFO3, "(= 0x%x)\n", readOpl(pM, &op));
            logfile_printf_without_header(LOGCAT_CPU_EXE | LOGLV_INFO3, "addr 0x%x:0x%x pointer 0x%x\n", REG_CS, REG_EIP, pointer);
            PREFIX_OP32 = save_op32;
        }

        if( pM->reg.cpl != 0 && funct == 6 ){ // LMSW in CPL != 0 is prohibited
            ENTER_GP(0);
        }

        PREFIX_OP32 = 0;
        if(funct == 4){
            writeOpl(pM, &op, pM->reg.cr[0]);
        }else{
            uint32_t msw = readOpl(pM, &op);
            pM->reg.cr[0] &= 0xffff0001;      // PE bit should not be cleared
            pM->reg.cr[0] |= (msw & 0xffff);  // setting PE bit is possible
        }
        PREFIX_OP32 = save_op32;

    }else{
        return EX_RESULT_UNKNOWN;
    }

    return EX_RESULT_SUCCESS;
}


int exSETcc(struct stMachineState *pM, uint32_t pointer){
    uint16_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer  );
    uint16_t inst1 = fetchCodeDataByte(pM, pointer+1);

    uint16_t cond = 0;
    uint16_t tmpval1, tmpval2, tmpval3;
    uint8_t  val = 0;
    uint32_t size;
    struct stOpl op;

    if( inst0 != 0x0f || (inst1 & 0xf0) != 0x90 ){
        return EX_RESULT_UNKNOWN;
    }

    size = decode_mod_rm(pM, pointer+2, INST_W_BYTEACC, &op);
    UPDATE_IP( 2 + size );
    cond = inst1&0x0f;

    const char *InstStr[] = 
        {"SETO ", "SETNO ", "SETB ", "SETNB ", "SETE ", "SETNE ", "SETBE ", "SETNBE ",
        "SETS ", "SETNS ", "SETP ", "SETNP ", "SETL ", "SETNL ", "SETLE ", "SETNLE "};

    if(DEBUG){
        EXI_LOG_PRINTF(InstStr[cond]);
        log_printOpl(EXI_LOGLEVEL, pM, &op);
        EXI_LOG_PRINTF("\n");
    }

    switch( cond ){
        case 0x0:
            if(  REG_FLAGS & (1<<FLAGS_BIT_OF) ){ val = 1; } break;
        case 0x1:
            if(!(REG_FLAGS & (1<<FLAGS_BIT_OF))){ val = 1; } break;
        case 0x2:
            if(  REG_FLAGS & (1<<FLAGS_BIT_CF) ){ val = 1; } break;
        case 0x3:
            if(!(REG_FLAGS & (1<<FLAGS_BIT_CF))){ val = 1; } break;
        case 0x4:
            if(  REG_FLAGS & (1<<FLAGS_BIT_ZF) ){ val = 1; } break;
        case 0x5:
            if(!(REG_FLAGS & (1<<FLAGS_BIT_ZF))){ val = 1; } break;
        case 0x6:
            if(  REG_FLAGS &((1<<FLAGS_BIT_CF)|(1<<FLAGS_BIT_ZF)) ){ val = 1; } break;
        case 0x7:
            if(!(REG_FLAGS &((1<<FLAGS_BIT_CF)|(1<<FLAGS_BIT_ZF)))){ val = 1; } break;
        case 0x8:
            if(  REG_FLAGS & (1<<FLAGS_BIT_SF) ){ val = 1; } break;
        case 0x9:
            if(!(REG_FLAGS & (1<<FLAGS_BIT_SF))){ val = 1; } break;
        case 0xa:
            if(  REG_FLAGS & (1<<FLAGS_BIT_PF) ){ val = 1; } break;
        case 0xb:
            if(!(REG_FLAGS & (1<<FLAGS_BIT_PF))){ val = 1; } break;
        case 0xc:
            tmpval1 = (REG_FLAGS & (1<<FLAGS_BIT_SF)) ? 1 : 0;
            tmpval2 = (REG_FLAGS & (1<<FLAGS_BIT_OF)) ? 1 : 0;
            if( tmpval1 != tmpval2 ){ val = 1; } break;
        case 0xd:
            tmpval1 = (REG_FLAGS & (1<<FLAGS_BIT_SF)) ? 1 : 0;
            tmpval2 = (REG_FLAGS & (1<<FLAGS_BIT_OF)) ? 1 : 0;
            if( tmpval1 == tmpval2 ){ val = 1; } break;
        case 0xe:
            tmpval1 = (REG_FLAGS & (1<<FLAGS_BIT_SF)) ? 1 : 0;
            tmpval2 = (REG_FLAGS & (1<<FLAGS_BIT_OF)) ? 1 : 0;
            tmpval3 = (REG_FLAGS & (1<<FLAGS_BIT_ZF)) ? 1 : 0;
            if( (tmpval1 != tmpval2) || (tmpval3 == 1) ){ val = 1; } break;
        case 0xf:
            tmpval1 = (REG_FLAGS & (1<<FLAGS_BIT_SF)) ? 1 : 0;
            tmpval2 = (REG_FLAGS & (1<<FLAGS_BIT_OF)) ? 1 : 0;
            tmpval3 = (REG_FLAGS & (1<<FLAGS_BIT_ZF)) ? 1 : 0;
            if( (tmpval1 == tmpval2) && (tmpval3 == 0) ){ val = 1; } break;
    }

    writeOpl(pM, &op, val);

    return EX_RESULT_SUCCESS;
}


int exBT(struct stMachineState *pM, uint32_t pointer){
    uint16_t size;
    struct stOpl op1, op2;
    uint32_t val2;
    int32_t bitpos;
    uint32_t targetword, targetbit;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer  );
    uint8_t inst1 = fetchCodeDataByte(pM, pointer+1);
    uint8_t inst2 = fetchCodeDataByte(pM, pointer+2);
    uint8_t funct = 0;

    if( inst0 != 0x0f ) return EX_RESULT_UNKNOWN;

    size = 2;
    if( (inst1 & 0xc7) == 0x83 ){
        funct = ((inst1>>3) & 0x7);
        if( (funct&4) == 0) return EX_RESULT_UNKNOWN;

        size+= decode_mod_rm(pM,  pointer+2, INST_W_WORDACC, &op1);
        decode_reg2         (pM,  pointer+2, INST_W_WORDACC, &op2);
        val2 = readOpl(pM,  &op2);
    }else if( inst1 == 0xba ){
        funct = ((inst2>>3) & 0x7);
        if( (funct&4) == 0) return EX_RESULT_UNKNOWN;

        size+= decode_mod_rm(pM, pointer+size, INST_W_WORDACC, &op1);
        size+= decode_imm   (pM, pointer+size, INST_W_WORDACC, &val2, INST_S_SIGNEX);
    }else{
        return EX_RESULT_UNKNOWN;
    }
    UPDATE_IP(size);

    if(DEBUG){
        if(funct == 4) EXI_LOG_PRINTF("BT ");
        if(funct == 5) EXI_LOG_PRINTF("BTS ");
        if(funct == 6) EXI_LOG_PRINTF("BTR ");
        if(funct == 7) EXI_LOG_PRINTF("BTC ");
        log_printOpl(EXI_LOGLEVEL, pM, &op1);
        EXI_LOG_PRINTF(", 0x%x\n", val2); 
    }

    if( PREFIX_OP32 ){
        bitpos    = (val2&0x1f);
        op1.addr += (((val2 >> 5) * 4) | ((val2&(1<<31)) ? 0xf0000000 : 0) );
    }else{
        bitpos    = (val2&0x0f);
        op1.addr += (((val2 >> 4) * 2) | ((val2&(1<<31)) ? 0xf0000000 : 0) );
    }
    targetword = readOpl(pM, &op1);
    targetbit  = (targetword & (1<<bitpos));

    if(funct == 5){ // BTS
        targetword |= (1<<bitpos);
    }else if(funct == 6){ // BTR
        targetword &= ~(1<<bitpos);
    }else if(funct == 7){ // BTC
        targetword ^= (1<<bitpos);
        // targetbit  ^= (1<<bitpos);
    }

    if( targetbit ){
        REG_FLAGS |= (1<<FLAGS_BIT_CF);
    }else{
        REG_FLAGS &=~(1<<FLAGS_BIT_CF);
    }

    if( funct != 4 ){ // write is not necessary for BT
        writeOpl( pM, &op1, targetword );
    }

    return EX_RESULT_SUCCESS;
}

int exBitScan(struct stMachineState *pM, uint32_t pointer){
    uint16_t size;
    struct stOpl op1, op2;
    uint32_t d0, i=0, b;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer  );
    uint8_t inst1 = fetchCodeDataByte(pM, pointer+1);

    if(inst0 != 0x0f                 ) return EX_RESULT_UNKNOWN;
    if(inst1 != 0xbc && inst1 != 0xbd) return EX_RESULT_UNKNOWN; // 0xbc: BSF, 0xbd: BSR

    size = decode_mod_rm(pM,  pointer+2, INST_W_WORDACC, &op1);
    decode_reg2         (pM,  pointer+2, INST_W_WORDACC, &op2);

    d0 = readOpl(pM,  &op1);
    if( d0 == 0 ){
        REG_FLAGS |= (1<<FLAGS_BIT_ZF);
    }else{
        REG_FLAGS &=~(1<<FLAGS_BIT_ZF);
        if( 0xbc == inst1 ){
            // BSF
            for(i= 0, b=      1; (d0 & b) == 0 ; i++, b=b<<1) ;
        }else{
            // BSR
            for(i=31, b=(1<<31); (d0 & b) == 0 ; i--, b=b>>1) ;
        }
    }

    writeOpl( pM, &op2, i );

    UPDATE_IP(2+size);

    if(DEBUG){
        if(inst1 == 0xbc) EXI_LOG_PRINTF("BSF ");
        if(inst1 == 0xbd) EXI_LOG_PRINTF("BSR ");
        log_printOpl(EXI_LOGLEVEL, pM, &op2); EXI_LOG_PRINTF(", "); 
        log_printOpl(EXI_LOGLEVEL, pM, &op1); EXI_LOG_PRINTF("\n");
    }

    return EX_RESULT_SUCCESS;
}


int exIMUL2Op(struct stMachineState *pM, uint32_t pointer){
    uint16_t size;
    uint32_t lsrc, rsrc;
    uint64_t rsrcs, lsrcs;
    struct stOpl opl, opr;

    uint16_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint16_t inst1 = fetchCodeDataByte(pM, pointer+1);

    if( inst0 != 0x0f || inst1 != 0xaf ){
        return EX_RESULT_UNKNOWN;
    }

    size = decode_mod_rm(pM,  pointer+2, INST_W_WORDACC, &opr);
    decode_reg2         (pM,  pointer+2, INST_W_WORDACC, &opl); // destination
    UPDATE_IP( size + 2 );

    rsrc = readOpl(pM, &opr);
    lsrc = readOpl(pM, &opl);

    if(DEBUG){
        EXI_LOG_PRINTF("IMUL ");
        log_printOpl(EXI_LOGLEVEL, pM, &opl);
        EXI_LOG_PRINTF(", ");
        log_printOpl(EXI_LOGLEVEL, pM, &opr);
        EXI_LOG_PRINTF("(=%x)\n", rsrc);
    }

    if( PREFIX_OP32 ){
        rsrcs = (rsrc & 0x80000000) ? (rsrc|0xffffffff00000000ULL) : rsrc;
        lsrcs = (lsrc & 0x80000000) ? (lsrc|0xffffffff00000000ULL) : lsrc;
    }else{
        rsrcs = (rsrc & 0x8000) ? (rsrc|0xffffffffffff0000ULL) : rsrc;
        lsrcs = (lsrc & 0x8000) ? (lsrc|0xffffffffffff0000ULL) : lsrc;
    }

    int64_t  muls = ((int64_t)rsrcs) * ((int64_t)lsrcs);

    writeOpl(pM, &opl, (uint32_t)(muls & 0xffffffff));

    if( PREFIX_OP32 ){
        if( (muls&0xffffffff80000000ULL) == 0 || (muls&0xffffffff80000000ULL) == 0xffffffff80000000ULL ){
            REG_FLAGS &=~((1<<FLAGS_BIT_CF)|(1<<FLAGS_BIT_OF));
        }else{
            REG_FLAGS |= ((1<<FLAGS_BIT_CF)|(1<<FLAGS_BIT_OF));
        }
    }else{
        if( (muls&0xffffffffffff8000ULL) == 0 || (muls&0xffffffffffff8000ULL) == 0xffffffffffff8000ULL ){
            REG_FLAGS &=~((1<<FLAGS_BIT_CF)|(1<<FLAGS_BIT_OF));
        }else{
            REG_FLAGS |= ((1<<FLAGS_BIT_CF)|(1<<FLAGS_BIT_OF));
        }
    }

    return EX_RESULT_SUCCESS;
}