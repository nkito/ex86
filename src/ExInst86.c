#include <stdio.h>
#include <stdint.h>

#include "i8086.h"
#include "ALUop.h"
#include "ExInst_common.h"
#include "ExInst86.h"
#include "decode.h"
#include "misc.h"
#include "logfile.h"
#include "mem.h"
#include "io.h"
#include "descriptor.h"


uint32_t readFLAGS(struct stMachineState *pM){
    if( pM->pEmu->emu_cpu == EMU_CPU_8086 || pM->pEmu->emu_cpu == EMU_CPU_80186 ){
        return (0xf000 | (REG_FLAGS));
    }else if( pM->pEmu->emu_cpu == EMU_CPU_80286 ){
        return (0x0fd7 & (REG_FLAGS));
    }else{
        if( PREFIX_OP32 ){
            if( pM->pEmu->emu_cpu == EMU_CPU_80386 ){
                return ((0x00037fd7 & (REG_EFLAGS)) | 2);
            }else{
                return ((0x00077fd7 & (REG_EFLAGS)) | 2);
            }
        }else{
            return ((0x7fd7 & (REG_FLAGS)) | 2);
        }
    }
}


void enterINT32(struct stMachineState *pM, uint16_t int_num, uint16_t cs, uint32_t eip, int enable_error_code, uint32_t error_code, int isSoftInt){
    uint32_t save_data32 = PREFIX_OP32;
    uint32_t save_addr32 = PREFIX_AD32;
    uint32_t saved_eflag;
    struct stGateDesc GD;

    if(int_num == 0x80){
        // Linux system call
        logfile_printf((LOGCAT_CPU_EXE | LOGLV_INFO3), "INT %x: EAX=%x(%d) EBX=%x ECX=%x EDX=%x (CS:EIP=%x:%x)\n", int_num, REG_EAX, REG_EAX, REG_EBX, REG_ECX, REG_EDX, REG_CS, REG_EIP );
    }

    if( (int_num<<3) +7 > pM->reg.idtr_limit ){
        ENTER_GP( ECODE_SEGMENT_IDT(int_num, 1) );
    }

    loadIntDesc(pM, int_num, &GD);

    uint8_t CPL = pM->reg.cpl;
    //uint8_t RPL = (GD.selector&3);
    uint8_t DPL = SEGACCESS_DPL(GD.access);

    if( isSoftInt && (DPL < CPL) ){
        ENTER_GP( ECODE_SEGMENT_IDT(int_num, 0) );
    }

    if( ! (GD.access & SEGACCESS_PRESENT) ){
        logfile_printf((LOGCAT_CPU_EXE | LOGLV_ERROR), "Error : intterupt descriptor for int 0x%x is not present (CS:EIP %x:%x)\n", int_num, pM->reg.current_cs, pM->reg.current_eip);
        ENTER_NP( ECODE_SEGMENT_IDT(int_num, 1) );
    }

    // op size is determined by descriptors for interrupt
    switch( GD.access & 0xf ){
        case SYSDESC_TYPE_INTGATE32:
            PREFIX_OP32 = 1;
            PREFIX_AD32 = 1;
            saved_eflag = readFLAGS(pM);
            REG_EFLAGS &= ~((1<<FLAGS_BIT_IF)|(1<<FLAGS_BIT_TF)|(1<<FLAGS_BIT_NT)|(1<<EFLAGS_BIT_RF)|(1<<EFLAGS_BIT_VM));
            break;
        case SYSDESC_TYPE_TRAPGATE32:
            PREFIX_OP32 = 1;
            PREFIX_AD32 = 1;
            saved_eflag = readFLAGS(pM);
            REG_EFLAGS &= ~((1<<FLAGS_BIT_TF)|(1<<FLAGS_BIT_NT)|(1<<EFLAGS_BIT_RF)|(1<<EFLAGS_BIT_VM));
            break;
        case SYSDESC_TYPE_TASKGATE:
            // TODO: Task gate processing
            logfile_printf((LOGCAT_CPU_EXE | LOGLV_ERROR), "Error : intterupt descriptor for int 0x%x is type 0x%x  (CS:EIP %x:%x)\n", int_num, (GD.access & 0x0f), pM->reg.current_cs, pM->reg.current_eip);
        default:
        /*
            logfile_printf((LOGCAT_CPU_EXE | LOGLV_ERROR), "Error : intterupt descriptor for int 0x%x is type 0x%x  (CS:EIP %x:%x)\n", int_num, (GD.access & 0x0f), pM->reg.current_cs, pM->reg.current_eip);
            PREFIX_OP32 = 1;
            PREFIX_AD32 = 1;
            saved_eflag = readFLAGS(pM);
            REG_EFLAGS &= ~((1<<FLAGS_BIT_IF)|(1<<FLAGS_BIT_TF)|(1<<FLAGS_BIT_NT)|(1<<EFLAGS_BIT_RF)|(1<<EFLAGS_BIT_VM));
        */
            ENTER_NP( ECODE_SEGMENT_IDT(int_num, 1) );
            break;
    }

    struct stRawSegmentDesc RS;
    if( GD.selector < 4 ){
        // if the new code segment is pointed by a null selector
        ENTER_GP( 1 );
    }
    loadRawSegmentDesc(pM, GD.selector, &RS);

    uint8_t DPLC= SEGACCESS_DPL(RS.access);

    if( (!SEGACCESS_IS_CSEG(RS.access)) ||  (DPLC > CPL) ){
        ENTER_GP( ECODE_SEGMENT_GDT_LDT_EXT(GD.selector) );
    }

    if( 0 == (RS.access & SEGACCESS_PRESENT) ){
        ENTER_NP( ECODE_SEGMENT_GDT_LDT_EXT(GD.selector) );
    }

    if( (!SEGACCESS_CSEG_IS_CONFORMING(RS.access)) && DPLC < CPL ){
        uint32_t new_esp, old_esp;
        uint16_t new_ss,  old_ss;
        uint32_t TSSbase  = pM->reg.descc_tr.base;
        uint32_t TSSlimit = pM->reg.descc_tr.limit;
        
        if( TSSlimit < TSS_MINIMUM_LIMIT_VALUE_32BIT ){
            logfile_printf((LOGCAT_CPU_EXE | LOGLV_ERROR), "Error : intterupt descriptor for int 0x%x , PL %d -> %d  (CS:EIP %x:%x)\n", int_num, pM->reg.cpl, (GD.selector&3), pM->reg.current_cs, pM->reg.current_eip);
        }

        pM->reg.cpl = DPLC; // (GD.selector&3);
        GD.selector &= (~3);
        GD.selector |= DPLC;
        uint32_t TSS_SP_base = TSSbase + 4 + (pM->reg.cpl * 8);

        new_esp  = readDataMemByteAsSV(pM, TSS_SP_base);
        new_esp |= ((uint32_t)readDataMemByteAsSV(pM, TSS_SP_base + 1))<< 8;
        new_esp |= ((uint32_t)readDataMemByteAsSV(pM, TSS_SP_base + 2))<<16;
        new_esp |= ((uint32_t)readDataMemByteAsSV(pM, TSS_SP_base + 3))<<24;

        new_ss  = readDataMemByteAsSV(pM, TSS_SP_base + 4);
        new_ss |= ((uint16_t) readDataMemByteAsSV(pM, TSS_SP_base + 5))<< 8;

        old_ss  = REG_SS;
        old_esp = REG_ESP;

        updateSegReg(pM, SEGREG_NUM_SS, new_ss);
        REG_ESP = new_esp;

        if( saved_eflag & (1<<EFLAGS_BIT_VM) ){
            // from VM86 mode
            PUSH_TO_STACK( REG_GS );
            PUSH_TO_STACK( REG_FS );
            PUSH_TO_STACK( REG_DS );
            PUSH_TO_STACK( REG_ES );
            updateSegReg(pM, SEGREG_NUM_GS, 0);
            updateSegReg(pM, SEGREG_NUM_FS, 0);
            updateSegReg(pM, SEGREG_NUM_DS, 0);
            updateSegReg(pM, SEGREG_NUM_ES, 0);
        }

        PUSH_TO_STACK( old_ss );
        PUSH_TO_STACK( old_esp  );
    }else if( (!SEGACCESS_CSEG_IS_CONFORMING(RS.access)) && CPL != DPLC ){
        ENTER_GP( ECODE_SEGMENT_GDT_LDT_EXT( GD.selector ) );
    }

    PUSH_TO_STACK( saved_eflag );
    PUSH_TO_STACK( cs  );
    PUSH_TO_STACK( eip );
    if( enable_error_code ){
        PUSH_TO_STACK( error_code );
    }

    updateSegReg(pM, SEGREG_NUM_CS, GD.selector);
    REG_EIP = GD.offset;

    PREFIX_OP32 = save_data32;
    PREFIX_AD32 = save_addr32;
}

void enterINTwithECODE(struct stMachineState *pM, uint16_t int_num, uint16_t cs, uint32_t eip, uint32_t error_code){

    // Recover the ESP value.
    // Note that a fault may occur after incrementing/decrementing ESP value, e.g., POP instruction.
    // if( REG_ESP != pM->reg.current_esp ){ printf("[Simluator message] ESP value is recovered.\n"); }
    REG_ESP = pM->reg.current_esp; // TODO: This may cause other faulty behaviors. Careful check is required.

    if( REG_EFLAGS != pM->reg.current_eflags ){
        // REG_EFLAGS= pM->reg.current_eflags;
        logfile_printf((LOGCAT_CPU_EXE | LOGLV_WARNING), "Warning : modified EFLAGS is detected in a entry point of fault processing at SS:ESP=%x:%x. EFLAGS: %x -> %x.\n", pM->reg.current_cs, pM->reg.current_eip, pM->reg.current_eflags, REG_EFLAGS);
        logfile_printf((LOGCAT_CPU_EXE | LOGLV_WARNING), "          instruction cache %x %x \n", pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
    }

    enterINT32(pM, int_num, cs, eip, /* enableErroCode?*/ 1, error_code, 0);
}

void enterINT(struct stMachineState *pM, uint16_t int_num, uint16_t cs, uint32_t eip, int isSoftInt){
    if( MODE_PROTECTED ){
        enterINT32(pM, int_num, cs, eip, 0, 0, isSoftInt);
        return;
    }


    uint32_t save_data32, save_addr32;
    save_data32 = PREFIX_OP32;
    save_addr32 = PREFIX_AD32;

    PREFIX_OP32 = 0;
    PREFIX_AD32 = 0;

    PUSH_TO_STACK( readFLAGS(pM) );
    PUSH_TO_STACK( cs  );
    PUSH_TO_STACK( eip );

    REG_FLAGS &= ~((1<<FLAGS_BIT_IF)|(1<<FLAGS_BIT_TF));
    updateSegReg(pM, SEGREG_NUM_CS, readDataMemWord(pM, MEMADDR(0, 4*int_num +2)));
    REG_EIP = readDataMemWord(pM, MEMADDR(0, 4*int_num   ));

    PREFIX_OP32 = save_data32;
    PREFIX_AD32 = save_addr32;
}

int exPrefixDummy(struct stMachineState *pM, uint32_t pointer){ return EX_RESULT_UNKNOWN; }

int exESC(struct stMachineState *pM, uint32_t pointer){
    uint16_t size = 1;
    struct stOpl op;
    uint8_t wait = 0;

    if( (pM->reg.cr[0] & (1<<CR0_BIT_EM)) || (pM->reg.cr[0] & (1<<CR0_BIT_TS)) ){
        logfile_printf_without_header(LOGCAT_CPU_EXE | LOGLV_ERROR, "ESC and trap7 (%x:%x %x)\n", pM->reg.current_cs, pM->reg.current_eip, pointer);

        enterINT(pM, INTNUM_ESCOPCODE, pM->reg.current_cs, pM->reg.current_eip, 0);
        return EX_RESULT_SUCCESS;
    }


    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = pM->reg.fetchCache[1];
    uint8_t bit0 = ((inst0>>0) & 1);
    uint8_t bit1 = ((inst0>>1) & 1);

    if(inst0 == 0x9b){
        inst0 = inst1;
        inst1 = fetchCodeDataByte(pM, pointer+2);

        bit0 = ((inst0>>0) & 1);
        bit1 = ((inst0>>1) & 1);
        wait = 1;
        size++;
    }

    if( inst0 == 0xdb && inst1        == 0xe3 ) goto fpuop;    // F(N)INIT

    if( inst0 == 0xdd && (inst1&0x38) == 0x38 ) goto fpuop;    // F(N)STSW
    if( inst0 == 0xdf && inst1        == 0xe0 ) goto fpuop_ax; // F(N)STSW

    if( inst0 == 0xd9 && (inst1&0x38) == 0x38 ) goto fpuop;    // F(N)STCW

    return EX_RESULT_UNKNOWN;

fpuop:
    size +=  decode_mod_rm(pM, pointer+size, INST_W_BIT, &op);

    UPDATE_IP(size);

    if( INST_D_BIT == INST_D_FIRSTOP ){
        writeOpl(pM,  &op, 0xff);
    }else{
        // no operation
        // readOpl(pM,  pOpDest);
    }

    if(DEBUG){
        if( inst0 == 0xdb && inst1 == 0xe3        ) EXI_LOG_PRINTF("F%sINIT\n", wait ? "" : "N");
        if( inst0 == 0xdd && (inst1&0x38) == 0x38 ) EXI_LOG_PRINTF("F%sSTSW\n", wait ? "" : "N");
        if( inst0 == 0xd9 && (inst1&0x38) == 0x38 ) EXI_LOG_PRINTF("F%sSTCW\n", wait ? "" : "N");
    }

    return EX_RESULT_SUCCESS;

fpuop_ax:
    UPDATE_IP(size+1);

    REG_AX = 0xff;

    if(DEBUG){
        if( inst0 == 0xdf && inst1        == 0xe0 ) EXI_LOG_PRINTF("F%sSTSW AX\n", wait ? "" : "N");
    }
    return EX_RESULT_SUCCESS;
}

int exWAIT(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);

    if( inst0 != 0x9b ) return EX_RESULT_UNKNOWN;

    if(DEBUG){
        EXI_LOG_PRINTF("WAIT\n");
    }

    UPDATE_IP(1);

    if( (pM->reg.cr[0] & (1<<CR0_BIT_TS)) && (pM->reg.cr[0] & (1<<CR0_BIT_MP)) ){
        enterINT(pM, INTNUM_ESCOPCODE, pM->reg.current_cs, pM->reg.current_eip, 0);
        return EX_RESULT_SUCCESS;
    }

    return EX_RESULT_SUCCESS;
}


int exHLT(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);

    if( inst0 != 0xf4 ) return EX_RESULT_UNKNOWN;

    if(DEBUG){
        EXI_LOG_PRINTF("HLT");
    }

    UPDATE_IP(1);

    if( MODE_PROTECTED && (pM->reg.cpl != 0) ){
        ENTER_GP(0);
    }

    if( pM->pEmu->emu_cpu < EMU_CPU_80286 ){
        return EX_RESULT_UNKNOWN;  // stop the processor
    }else{
        return EX_RESULT_SUCCESS;
    }
}


int exMov(struct stMachineState *pM, uint32_t pointer){
    uint32_t val;
    uint32_t size;
    uint32_t data32 = PREFIX_OP32;

    struct stOpl *pOpDest = NULL;
    struct stOpl *pOpSrc  = NULL;
    struct stOpl opAcc, op1, op2;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = pM->reg.fetchCache[1];
    uint8_t bit0 = ((inst0>>0) & 1);
    uint8_t bit1 = ((inst0>>1) & 1);
    uint8_t bit3 = ((inst0>>3) & 1);

    opAcc.type  = OpTypeReg;
    opAcc.reg   = REG_NUM_AX;
    opAcc.width = INST_W_BIT;

    if( (inst0 & 0xfc) == 0x88 ){ 
        size = decode_mod_rm(pM, pointer+1, INST_W_BIT, &op1);
        decode_reg2         (pM, pointer+1, INST_W_BIT, &op2);
        UPDATE_IP(size + 1);
        pOpDest = (INST_D_BIT == INST_D_FIRSTOP) ? &op1 : &op2;
        pOpSrc  = (INST_D_BIT == INST_D_FIRSTOP) ? &op2 : &op1;
    }else if( (inst0 & 0xfe) == 0xc6 && (inst1 & 0x38) == 0x00 ){
        size =  decode_mod_rm(pM, pointer+1     , INST_W_BIT, &op1);
        size += decode_imm   (pM, pointer+1+size, INST_W_BIT, &val, INST_S_NOSIGNEX);
        UPDATE_IP(size + 1);

        pOpDest = &op1;
    }else if( (inst0 & 0xf0) == 0xb0 ){
        decode_reg1      (pM, pointer+0, INST_W_BIT_MOV_IMMREG, &op1);
        size = decode_imm(pM, pointer+1, INST_W_BIT_MOV_IMMREG, &val, INST_S_NOSIGNEX);
        UPDATE_IP(size + 1);

        pOpDest = &op1;
    }else if( (inst0 & 0xfe) == 0xa0 ){
        size = decode_immAddr(pM,  pointer+1, INST_W_BIT, &op1);
        UPDATE_IP(size + 1);

        pOpDest = &opAcc;
        pOpSrc  = &op1;
    }else if( (inst0 & 0xfe) == 0xa2 ){
        size = decode_immAddr(pM,  pointer+1, INST_W_BIT, &op1);
        UPDATE_IP(size + 1);

        pOpDest = &op1;
        pOpSrc  = &opAcc;
    }else if( inst0 == 0x8e ){
        // TODO: the word size is 16-bit even if operand-size is 32-bit 
        // segment reg <- r/m16
        size =  decode_mod_rm    (pM,  pointer+1, INST_W_WORDACC, &op1);
        val  =  decode_segReg3bit(pM,  pointer+1, &op2);
        if( val ) return EX_RESULT_UNKNOWN;
        UPDATE_IP(size + 1);

        pOpDest = &op2;
        pOpSrc  = &op1;
    }else if( inst0 == 0x8c ){
        // TODO: the word size is 16-bit even if operand-size is 32-bit 
        // r16/r32/m16 <- segment reg
        size =  decode_mod_rm(pM,  pointer+1, INST_W_WORDACC, &op1);
        val  =  decode_segReg3bit(pM,  pointer+1, &op2);
        if( val ) return EX_RESULT_UNKNOWN;
        UPDATE_IP(size + 1);

        pOpDest = &op1;
        pOpSrc  = &op2;
    }else{
        return EX_RESULT_UNKNOWN;
    }

    if(DEBUG){
        EXI_LOG_PRINTF("MOV "); log_printOpl(EXI_LOGLEVEL, pM, pOpDest); EXI_LOG_PRINTF(", ");
        if( pOpSrc == NULL ){
            EXI_LOG_PRINTF("%x", val);
        }else{
            log_printOpl(EXI_LOGLEVEL, pM, pOpSrc);
        }
        EXI_LOG_PRINTF("\n");
    }

    // Fetch the source value
    if( inst0 == 0x8e ){
        // moving a value to a segment reg should be done in 16-bit
        PREFIX_OP32 = 0;
    }
    val = (pOpSrc==NULL) ? val : readOpl(pM,  pOpSrc);
    PREFIX_OP32 = data32;


    // Write the value to the destination
    if( (inst0 == 0x8c) && pOpDest->type == OpTypeMemWithSeg ){
        // moving a segment reg value to memory should be done in 16-bit
        PREFIX_OP32 = 0;
    }
    writeOpl( pM,  pOpDest, val);
    PREFIX_OP32 = data32;

    return EX_RESULT_SUCCESS;
}

int exPUSH(struct stMachineState *pM, uint32_t pointer){
    struct stOpl op;

    uint32_t val;
    uint16_t size = 0;
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = pM->reg.fetchCache[1];

    /* -----------------------------------------------------------------
    NOTE THAT:

    The PUSH ESP instruction pushes the value of the ESP register as it existed before the instruction was executed.
    If a PUSH instruction uses a memory operand in which the ESP register is used for computing the operand address, 
    the address of the operand is computed before the ESP register is decremented.    

    from https://www.felixcloutier.com/x86/push.html
     ----------------------------------------------------------------- */

    if( inst0 == 0xff && (inst1 & 0x38) == 0x30 ){
        size = decode_mod_rm(pM,  pointer+1, INST_W_WORDACC, &op);
    }else if( (inst0&0xf8) == 0x50 ){
        decode_reg1(pM, pointer+0, INST_W_WORDACC, &op);
    }else if( (inst0&0xe7) == 0x06 ){
        decode_segReg(pM, pointer+0, &op);
    }else if( inst0 == 0x0f && (inst1 & 0xf7) == 0xa0 && pM->pEmu->emu_cpu >= EMU_CPU_80386 ){
        decode_segReg3bit(pM, pointer+1, &op);
        size++;
    }else{
        return EX_RESULT_UNKNOWN;
    }

    UPDATE_IP(1 + size);
    PUSH_TO_STACK( (val=readOpl(pM,  &op)) );

    if(DEBUG){ EXI_LOG_PRINTF("PUSH "); log_printOpl(EXI_LOGLEVEL, pM, &op); EXI_LOG_PRINTF("(=%x)\n", val); }

    return EX_RESULT_SUCCESS;
}

int exPOP(struct stMachineState *pM, uint32_t pointer){
    struct stOpl op;
    uint32_t val;

    uint16_t size = 0;
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = pM->reg.fetchCache[1];

    /* -----------------------------------------------------------------
    NOTE THAT:
    If the ESP register is used as a base register for addressing a destination operand in memory, 
    the POP instruction computes the effective address of the operand after it increments the ESP register. 
    For the case of a 16-bit stack where ESP wraps to 0H as a result of the POP instruction, the resulting location of the memory write is processor-family-specific.

    from https://www.felixcloutier.com/x86/pop
     ----------------------------------------------------------------- */
    if( pM->reg.descc_ss.flags & SEGFLAGS_DSEG_B_BIT ){
        REG_ESP += (PREFIX_OP32 ? 4 : 2);
    }else{
        REG_SP  += (PREFIX_OP32 ? 4 : 2);
    }


    if( inst0 == 0x8f && (inst1 & 0x38) == 0x00 ){
        // Warning: The following line may cause a PAGE FAULT! because "decode_mod_rm" may load a immidiate value from memory.
        // In such case, the program execution jumps to a simulator function calling a (guest) fault handler with a wrong (E)SP value 
        // and re-execution of this instruction after the handler will result in a wrong (E)SP value.
        // The original ESP value is saved in "current_esp" variable at mainloop and is recovered at "enterINTwithECODE".

        size = decode_mod_rm(pM,  pointer+1, INST_W_WORDACC, &op);
    }else if( (inst0&0xf8) == 0x58 ){
        decode_reg1(pM, pointer+0, INST_W_WORDACC, &op);
    }else if( (inst0&0xe7) == 0x07 && !(inst0 == 0x0f && pM->pEmu->emu_cpu >= EMU_CPU_80186) ){
        // POP a segment register
        // NOTE: "POP CS" is available before 80186
        decode_segReg(pM, pointer+0, &op);
    }else if( inst0 == 0x0f && (inst1 & 0xf7) == 0xa1 && pM->pEmu->emu_cpu >= EMU_CPU_80386 ){
        // POP FS, POP GS
        decode_segReg3bit(pM, pointer+1, &op);
        size++;
    }else{
        return EX_RESULT_UNKNOWN;
    }

    if( pM->reg.descc_ss.flags & SEGFLAGS_DSEG_B_BIT ){
        REG_ESP -= (PREFIX_OP32 ? 4 : 2);
    }else{
        REG_SP  -= (PREFIX_OP32 ? 4 : 2);
    }

    POP_FROM_STACK( val ); // THIS INCREMENTS ESP/SP
    writeOpl(pM,  &op, val);
    UPDATE_IP(1 + size);

    if(DEBUG){ EXI_LOG_PRINTF("POP "); log_printOpl(EXI_LOGLEVEL, pM, &op); EXI_LOG_PRINTF("(=%x)\n", val); }

    return EX_RESULT_SUCCESS;
}

int exXCHG(struct stMachineState *pM, uint32_t pointer){
    struct stOpl op1, op2;

    uint32_t val;
    uint16_t size = 0;
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t bit0 = ((inst0>>0) & 1);

    if( (inst0&0xfe) == 0x86 ){
        size = decode_mod_rm(pM, pointer+1, INST_W_BIT, &op1);
        decode_reg2         (pM, pointer+1, INST_W_BIT, &op2);
    }else if( (inst0&0xf8) == 0x90 ){
        op1.type  = OpTypeReg;
        op1.reg   = REG_NUM_AX;
        op1.width = INST_W_WORDACC;
        decode_reg1(pM, pointer+0, INST_W_WORDACC, &op2);
    }else{
        return EX_RESULT_UNKNOWN;
    }

    if(DEBUG){ 
        EXI_LOG_PRINTF("XCHG "); 
        log_printOpl(EXI_LOGLEVEL, pM, &op1); EXI_LOG_PRINTF(", "); 
        log_printOpl(EXI_LOGLEVEL, pM, &op2); EXI_LOG_PRINTF("\n");
    }

    val = readOpl(pM,  &op1);
    writeOpl(pM,  &op1, readOpl(pM,  &op2));
    writeOpl(pM,  &op2, val);

    UPDATE_IP(1 + size);

    return EX_RESULT_SUCCESS;
}


int exINOUT(struct stMachineState *pM, uint32_t pointer){
    uint32_t val;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t bit0 = ((inst0>>0) & 1);
    uint8_t bit1 = ((inst0>>1) & 1);

    if( (inst0&0xfc) == 0xe4 ){
        decode_imm(pM, pointer+1, INST_W_BYTEACC, &val, INST_S_NOSIGNEX);
        UPDATE_IP(2);
    }else if( (inst0&0xfc) == 0xec ){
        val = REG_DX;
        UPDATE_IP(1);
    }else{
        return EX_RESULT_UNKNOWN;
    }

    if( INST_D_BIT == INST_D_FIRSTOP ){
        if(DEBUG) EXI_LOG_PRINTF("IN %s, %x\n", INST_W_BIT ? ((PREFIX_OP32) ? "EAX" : "AX") : "AL", val); 
        if( INST_W_BIT ){
            if( PREFIX_OP32 ){
                REG_EAX = readIODoubleWord(pM, val);
            }else{
                REG_AX = readIOWord(pM, val);
            }
        }else{
            REG_AX &= 0xff00;
            REG_AX += readIOByte(pM, val);
        }
    }else{
        if(DEBUG) EXI_LOG_PRINTF("OUT %x, %s\n", val, INST_W_BIT ? ((PREFIX_OP32) ? "EAX" : "AX") : "AL");
        if( INST_W_BIT ){
            if( PREFIX_OP32 ){
                writeIODoubleWord(pM, val, REG_EAX);
            }else{
                writeIOWord(pM, val, REG_AX);
            }
        }else{
            writeIOByte(pM, val, REG_AX&0xff);
        }
    }

    return EX_RESULT_SUCCESS;
}


int exXLAT(struct stMachineState *pM, uint32_t pointer){
    struct stOpl op;
    uint16_t val;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);

    if(inst0 != 0xd7 ) return EX_RESULT_UNKNOWN;

    op.type  = OpTypeMemWithSeg;
    op.reg   = (PREFIX_SEG != PREF_SEG_UNSPECIFIED ? PREFIX_SEG : SEGREG_NUM_DS);
    op.addr  = (PREFIX_AD32 ? REG_EBX : REG_BX) + (REG_AX & 0x00ff);
    op.width = INST_W_BYTEACC;

    val      = readOpl(pM,  &op);
    REG_AX = ((REG_AX & 0xff00) | (val & 0xff));

    UPDATE_IP(1);

    if(DEBUG){
        EXI_LOG_PRINTF("XLAT \n");
    }

    return EX_RESULT_SUCCESS;
}

int exLEA(struct stMachineState *pM, uint32_t pointer){
    uint16_t size;
    struct stOpl op1, op2;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);

    if(inst0 != 0x8d ) return EX_RESULT_UNKNOWN;

    size = decode_mod_rm(pM,  pointer+1, INST_W_WORDACC, &op1);
    decode_reg2         (pM,  pointer+1, INST_W_WORDACC, &op2);

    uint32_t ea = readOplEA(pM,  &op1, 0); // the result of LEA is the offset part.

    writeOpl( pM,  &op2, ea );

    UPDATE_IP(1+size);

    if(DEBUG){
        EXI_LOG_PRINTF("LEA ");
        log_printOpl(EXI_LOGLEVEL, pM, &op2); EXI_LOG_PRINTF(", ");
        log_printOpl(EXI_LOGLEVEL, pM, &op1); EXI_LOG_PRINTF("\n");
    }

    return EX_RESULT_SUCCESS;
}


int exLDSLES(struct stMachineState *pM, uint32_t pointer){
    uint16_t size;
    uint32_t data32 = PREFIX_OP32;
    struct stOpl op1, op2;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = pM->reg.fetchCache[1];
    uint8_t inst2 = fetchCodeDataByte(pM, pointer+2);

    if(inst0 == 0xc5 || inst0 == 0xc4){
        if(inst1 == 0xc0) return EX_RESULT_UNKNOWN;
        size = 1;
        size+= decode_mod_rm(pM,  pointer+1, INST_W_WORDACC, &op1);
        decode_reg2         (pM,  pointer+1, INST_W_WORDACC, &op2);

    }else if(inst0 == 0x0f && (inst1 == 0xb2 || inst1 == 0xb4 || inst1 == 0xb5)){
        if(inst2 == 0xc0) return EX_RESULT_UNKNOWN;

        size = 2;
        size+= decode_mod_rm(pM,  pointer+2, INST_W_WORDACC, &op1);
        decode_reg2         (pM,  pointer+2, INST_W_WORDACC, &op2);
    }else{
        return EX_RESULT_UNKNOWN;
    }

    uint32_t d0 = readOpl(pM,  &op1); op1.addr = op1.addr + (data32 ? 4 : 2);
    PREFIX_OP32 = 0;
    uint16_t d2 = readOpl(pM,  &op1); // 16-bit access
    PREFIX_OP32 = data32;

    if(inst0 == 0xc5) updateSegReg(pM, SEGREG_NUM_DS, d2); // LDS
    if(inst0 == 0xc4) updateSegReg(pM, SEGREG_NUM_ES, d2); // LES

    if(inst0 == 0x0f){
        if(inst1 == 0xb2) updateSegReg(pM, SEGREG_NUM_SS, d2); // LSS
        if(inst1 == 0xb4) updateSegReg(pM, SEGREG_NUM_FS, d2); // LFS
        if(inst1 == 0xb5) updateSegReg(pM, SEGREG_NUM_GS, d2); // LGS
    }

    // common for LDS(0xc5), LES(0xc4), LSS(0x0f, 0xb2), LFS(0x0f, 0xb4), LGS(0x0f, 0xb5)
    writeOpl( pM,  &op2, d0 );

    UPDATE_IP(size);

    if(DEBUG){
        if(inst0 == 0xc5) EXI_LOG_PRINTF("LDS ");
        if(inst0 == 0xc4) EXI_LOG_PRINTF("LES ");
        if(inst0 == 0x0f){
            if(inst1 == 0xb2) EXI_LOG_PRINTF("LSS ");
            if(inst1 == 0xb4) EXI_LOG_PRINTF("LFS ");
            if(inst1 == 0xb5) EXI_LOG_PRINTF("LGS ");
        }

        log_printOpl(EXI_LOGLEVEL, pM, &op2); EXI_LOG_PRINTF(", "); 
        log_printOpl(EXI_LOGLEVEL, pM, &op1); EXI_LOG_PRINTF("\n");
    }

    return EX_RESULT_SUCCESS;
}

int exAHF(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);

    if( inst0 == 0x9f ){
        if(DEBUG) EXI_LOG_PRINTF("LAHF\n"); 

        uint16_t tmp_flag = ((readFLAGS(pM) & 0x00ff) << 8);
        REG_AX = ((REG_AX & 0x00ff) | tmp_flag);
    }else if( inst0 == 0x9e ){
        if(DEBUG) EXI_LOG_PRINTF("SAHF\n"); 

        REG_FLAGS &= 0xff00;
        REG_FLAGS |= ((REG_AX>>8) & ((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_PF) | (1<<FLAGS_BIT_AF) | (1<<FLAGS_BIT_ZF) | (1<<FLAGS_BIT_SF)));
        REG_FLAGS |= 2;
    }else{
        return EX_RESULT_UNKNOWN;
    }
    UPDATE_IP(1);

    return EX_RESULT_SUCCESS;
}


int exPUSHFPOPF(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint32_t flag;
    uint32_t mask;

    if( inst0 == 0x9c ){
        if(DEBUG) EXI_LOG_PRINTF("PUSHF\n");

        if( MODE_PROTECTEDVM && IOPL(REG_EFLAGS) < 3 ){
            logfile_printf(LOGCAT_CPU_EXE | LOGLV_ERROR, "GP(0) at PUSHF (pointer %x)\n", pointer);
            ENTER_GP(0);
        }
        mask = ~( (1<<EFLAGS_BIT_VM) | (1<<EFLAGS_BIT_RF));
        PUSH_TO_STACK( readFLAGS(pM) & mask );

    }else if( inst0 == 0x9d ){
        if(DEBUG) EXI_LOG_PRINTF("POPF ");

        POP_FROM_STACK( flag );

        if(DEBUG) EXI_LOG_PRINTF("(0x%x) \n", flag);

        if( MODE_PROTECTEDVM ){
            if( (PREFIX_OP32 ? IOPL(REG_EFLAGS) : IOPL(REG_FLAGS)) == 3  ){
                if( PREFIX_OP32 ){
                    mask  = (FLAGS_BIT_IOPL_MASK | (1<<EFLAGS_BIT_VM));

                    REG_EFLAGS &= mask;
                    REG_EFLAGS |= (flag & (~mask));
                    REG_EFLAGS &= ~(1<<EFLAGS_BIT_RF);
                }else{
                    REG_FLAGS &= FLAGS_BIT_IOPL_MASK;
                    REG_FLAGS |= (flag & (~FLAGS_BIT_IOPL_MASK));
                }
            }else{
                ENTER_GP(0);
            }
        }else if( (! MODE_PROTECTED) || pM->reg.cpl == 0 ){
            if( PREFIX_OP32 ){
                REG_EFLAGS &= (1<<EFLAGS_BIT_VM);
                REG_EFLAGS |= (flag & (~(1<<EFLAGS_BIT_VM)));
                REG_EFLAGS &= ~(1<<EFLAGS_BIT_RF);
            }else{
                REG_FLAGS  = flag;
            }
        }else{
            // protected and cpl > 0
            if( PREFIX_OP32 ){
                if( pM->reg.cpl > IOPL(REG_EFLAGS) ){
                    // VM bit should not be changed with POPF
                    mask = ((1<<FLAGS_BIT_IF) | FLAGS_BIT_IOPL_MASK | (1<<EFLAGS_BIT_VM));

                    REG_EFLAGS &= mask;
                    REG_EFLAGS |= (flag & (~mask));
                    REG_EFLAGS &= ~(1<<EFLAGS_BIT_RF);
                }else{
                    mask  = (FLAGS_BIT_IOPL_MASK | (1<<EFLAGS_BIT_VM));

                    REG_EFLAGS &= mask;
                    REG_EFLAGS |= (flag & (~mask));
                    REG_EFLAGS &= ~(1<<EFLAGS_BIT_RF);
                }
            }else{
                REG_FLAGS &= FLAGS_BIT_IOPL_MASK;
                REG_FLAGS |= (flag & (~FLAGS_BIT_IOPL_MASK));
            }
        }
    }else{
        return EX_RESULT_UNKNOWN;
    }
    UPDATE_IP(1);

    return EX_RESULT_SUCCESS;
}


#define ALU2OP_FUNCT_ADD (0x0<<3)
#define ALU2OP_FUNCT_ADC (0x2<<3)
#define ALU2OP_FUNCT_SUB (0x5<<3)
#define ALU2OP_FUNCT_SBB (0x3<<3)
#define ALU2OP_FUNCT_CMP (0x7<<3)
#define ALU2OP_FUNCT_AND (0x4<<3)
#define ALU2OP_FUNCT_OR  (0x1<<3)
#define ALU2OP_FUNCT_XOR (0x6<<3)

int exALU2OP(struct stMachineState *pM, uint32_t pointer){
    uint32_t val, val2;
    uint16_t size;

    struct stOpl *pOpDest = NULL;
    struct stOpl *pOpSrc  = NULL;
    struct stOpl opAcc, op1, op2;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = pM->reg.fetchCache[1];
    uint8_t bit0 = ((inst0>>0) & 1);
    uint8_t bit1 = ((inst0>>1) & 1);

    opAcc.type  = OpTypeReg;
    opAcc.reg   = REG_NUM_AX;
    opAcc.width = INST_W_BIT;

    uint8_t funct = ((inst0 & 0x80) ? (inst1 & 0x38) : (inst0 & 0x38));

    if( (inst0 & 0xfc) == funct ){ 
        size = decode_mod_rm(pM, pointer+1, INST_W_BIT, &op1);
        decode_reg2         (pM, pointer+1, INST_W_BIT, &op2);

        pOpDest = (INST_D_BIT == INST_D_FIRSTOP) ? &op1 : &op2;
        pOpSrc  = (INST_D_BIT == INST_D_FIRSTOP) ? &op2 : &op1;
        UPDATE_IP(size + 1);
    }else if( (((inst0 & 0xfe) == 0x80) || (inst0 == 0x83)) && (inst1 & 0x38) == funct ){
        size =  decode_mod_rm(pM, pointer+1     , INST_W_BIT, &op1);
        size += decode_imm   (pM, pointer+1+size, INST_W_BIT, &val2, INST_S_BIT);

        pOpDest = &op1;
        UPDATE_IP(size + 1);
    }else if( (inst0 & 0xfe) == (funct | 0x04) ){
        size = decode_imm(pM, pointer+1, INST_W_BIT, &val2, INST_S_NOSIGNEX);

        pOpDest = &opAcc;
        UPDATE_IP(size + 1);
    }else{
        EXI_LOG_ERR_PRINTF("Failed %x %x\n", inst0, inst1);
        return EX_RESULT_UNKNOWN;
    }

    val = readOpl(pM,  pOpDest);

    if(DEBUG){
        switch(funct){
            case ALU2OP_FUNCT_ADD: EXI_LOG_PRINTF("ADD "); break;
            case ALU2OP_FUNCT_ADC: EXI_LOG_PRINTF("ADC "); break;
            case ALU2OP_FUNCT_SUB: EXI_LOG_PRINTF("SUB "); break;
            case ALU2OP_FUNCT_SBB: EXI_LOG_PRINTF("SBB "); break;
            case ALU2OP_FUNCT_CMP: EXI_LOG_PRINTF("CMP "); break;
            case ALU2OP_FUNCT_AND: EXI_LOG_PRINTF("AND "); break;
            case ALU2OP_FUNCT_OR : EXI_LOG_PRINTF("OR ");  break;
            case ALU2OP_FUNCT_XOR: EXI_LOG_PRINTF("XOR "); break;
        }
        log_printOpl(EXI_LOGLEVEL, pM, pOpDest); EXI_LOG_PRINTF("(=%x), ", val);
        if( pOpSrc == NULL ){
            EXI_LOG_PRINTF("%x", val2);
        }else{
            log_printOpl(EXI_LOGLEVEL, pM, pOpSrc);
            EXI_LOG_PRINTF("(=%x)", readOpl(pM, pOpSrc));
        }
        EXI_LOG_PRINTF("\n");
    }

    uint16_t carry = (((REG_FLAGS)&(1<<FLAGS_BIT_CF)) ? 1 : 0);

    switch(funct){
        case ALU2OP_FUNCT_ADD: 
            val = ALUOPAdd(pM, val, (pOpSrc==NULL) ? val2 : readOpl(pM,  pOpSrc), INST_W_BIT);
            break;
        case ALU2OP_FUNCT_ADC:
            val = ALUOPAdd3(pM, val, (pOpSrc==NULL) ? val2 : readOpl(pM,  pOpSrc), carry, INST_W_BIT);
            break;
        case ALU2OP_FUNCT_SUB:
            val = ALUOPSub(pM, val, (pOpSrc==NULL) ? val2 : readOpl(pM,  pOpSrc), INST_W_BIT);
            break;
        case ALU2OP_FUNCT_SBB:
            val = ALUOPSub3(pM, val, (pOpSrc==NULL) ? val2 : readOpl(pM,  pOpSrc), carry, INST_W_BIT);
            break;
        case ALU2OP_FUNCT_CMP:
            val = ALUOPSub(pM, val, (pOpSrc==NULL) ? val2 : readOpl(pM,  pOpSrc), INST_W_BIT);
            break;
        case ALU2OP_FUNCT_AND:
            val = ALUOPand(pM, val, (pOpSrc==NULL) ? val2 : readOpl(pM,  pOpSrc), INST_W_BIT);
            break;
        case ALU2OP_FUNCT_OR :
            val = ALUOPor (pM, val, (pOpSrc==NULL) ? val2 : readOpl(pM,  pOpSrc), INST_W_BIT);
            break;
        case ALU2OP_FUNCT_XOR:
            val = ALUOPxor(pM, val, (pOpSrc==NULL) ? val2 : readOpl(pM,  pOpSrc), INST_W_BIT);
            break;
    }

    if( funct != ALU2OP_FUNCT_CMP ){
        writeOpl( pM,  pOpDest, val );
    }

    return EX_RESULT_SUCCESS;
}



int exINCDEC(struct stMachineState *pM, uint32_t pointer){
    uint32_t size, val;
    struct stOpl op;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = pM->reg.fetchCache[1];
    uint8_t bit0 = ((inst0>>0) & 1);

    int isWordAcc = INST_W_BIT;

    uint8_t funct = ((inst0 & 0x80) ? (inst1 & 0x38) : (inst0 & 0x38));
    // funct 
    // 0x00: INC
    // 0x08: DEC

    if( funct != 0x00 && funct != 0x08 ) return EX_RESULT_UNKNOWN;

    if( (inst0 & 0xfe) == 0xfe && (inst1 & 0x38) == funct ){ 
        size = decode_mod_rm(pM,  pointer+1, INST_W_BIT, &op);
        UPDATE_IP(size + 1);

    }else if( (inst0 & 0xf8) == (0x40 | funct) ){
        decode_reg1(pM, pointer, INST_W_WORDACC, &op);
        UPDATE_IP(1);
        isWordAcc = INST_W_WORDACC;
    }

    if(DEBUG){
        if( funct == 0x00 ) EXI_LOG_PRINTF("INC ");
        if( funct == 0x08 ) EXI_LOG_PRINTF("DEC ");
        log_printOpl(EXI_LOGLEVEL, pM, &op);
        EXI_LOG_PRINTF(" (=%x)\n", readOpl(pM, &op));
    }

    // CF should not be affected
    int8_t cf = ((REG_FLAGS & (1<<FLAGS_BIT_CF)) ? 1 : 0);

    val = readOpl(pM,  &op);
    if( funct == 0x00 ){
        val = ALUOPAdd(pM, val, 1, isWordAcc);
    }else{
        val = ALUOPSub(pM, val, 1, isWordAcc);
    }
    if(cf){ REG_FLAGS |= (1<<FLAGS_BIT_CF); }
    else  { REG_FLAGS &=~(1<<FLAGS_BIT_CF); }

    writeOpl( pM,  &op, val );

    return EX_RESULT_SUCCESS;
}


int exNEGNOT(struct stMachineState *pM, uint32_t pointer){
    uint32_t size, val, origval;
    struct stOpl op;

    uint8_t inst0 = pM->reg.fetchCache[0];
    uint8_t inst1 = pM->reg.fetchCache[1];
    uint8_t bit0 = ((inst0>>0) & 1);

    uint8_t funct = (inst1 & 0x38);
    // funct 
    // 0x18: NEG
    // 0x10: NOT

    if( (inst0&0xfe) != 0xf6 || (funct != 0x18 && funct != 0x10) ) return EX_RESULT_UNKNOWN;

    size = decode_mod_rm(pM,  pointer+1, INST_W_BIT, &op);
    UPDATE_IP(size + 1);

    val = readOpl(pM,  &op);

    if( funct == 0x18 ){ // NEG
        origval = val;
        val = ALUOPSub(pM, 0, origval, INST_W_BIT);
        if(origval == 0){ REG_FLAGS &=~(1<<FLAGS_BIT_CF); }
        else            { REG_FLAGS |= (1<<FLAGS_BIT_CF); }
    }else{               // NOT
        // flag bits are not affected
        val = ~val;
        val = (INST_W_BIT ? (PREFIX_OP32 ? val : (val & 0xffff)) : (val & 0xff));
    }
    writeOpl( pM,  &op, val );

    if(DEBUG){
        if( funct == 0x18 ) EXI_LOG_PRINTF("NEG ");
        if( funct == 0x10 ) EXI_LOG_PRINTF("NOT ");
        log_printOpl(EXI_LOGLEVEL, pM, &op);
        EXI_LOG_PRINTF("\n");
    }

    return EX_RESULT_SUCCESS;
}


int exMUL32(struct stMachineState *pM, uint32_t pointer){
    uint16_t size;
    uint64_t lsrc, rsrc;
    struct stOpl op;

    uint8_t inst0 = pM->reg.fetchCache[0];
    uint8_t inst1 = pM->reg.fetchCache[1];
    uint8_t bit0 = ((inst0>>0) & 1);

    uint8_t funct = (inst1 & 0x38);
    // funct 
    // 0x20: MUL  (unsigned)
    // 0x28: IMUL (signed)

    if( (inst0&0xfe) != 0xf6 || (funct != 0x20 && funct != 0x28) ) return EX_RESULT_UNKNOWN;

    size = decode_mod_rm(pM,  pointer+1, INST_W_BIT, &op);
    UPDATE_IP(size + 1);

    rsrc = readOpl(pM,  &op);
    if( INST_W_BIT ){
        lsrc = REG_EAX;
    }else{
        lsrc = (REG_AX&0xff);
        rsrc&= 0xff;
    }


    if(DEBUG){
        if( funct == 0x20 ) EXI_LOG_PRINTF("MUL%c ",  INST_W_BIT ? 'w' : 'b');
        if( funct == 0x28 ) EXI_LOG_PRINTF("IMUL%c ",  INST_W_BIT ? 'w' : 'b');
        log_printOpl(EXI_LOGLEVEL, pM, &op);
        EXI_LOG_PRINTF("(=%x)\n", rsrc);
    }

    if( funct == 0x20 ){ // MUL (unsigned)
        uint64_t mulu = ((uint64_t)rsrc) * ((uint64_t)lsrc);

        if( INST_W_BIT ){
            REG_EAX = ( mulu     &0xffffffff);
            REG_EDX = ((mulu>>32)&0xffffffff);
            if(REG_EDX !=0) REG_FLAGS |= ((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
            else            REG_FLAGS &=~((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
        }else{
            REG_AX = ( mulu     &0xffff);

            if(REG_AX&0xff00) REG_FLAGS |= ((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
            else              REG_FLAGS &=~((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
        }
    }else{ // IMUL (signed)
        uint64_t rsrcs, lsrcs;
        if( INST_W_BIT ){
            rsrcs = ((rsrc & 0x80000000ULL) ? (rsrc|0xffffffff00000000ULL) : rsrc);
            lsrcs = ((lsrc & 0x80000000ULL) ? (lsrc|0xffffffff00000000ULL) : lsrc);
        }else{
            rsrcs = ((rsrc & 0x80ULL)       ? (rsrc|0xffffffffffffff00ULL) : rsrc);
            lsrcs = ((lsrc & 0x80ULL)       ? (lsrc|0xffffffffffffff00ULL) : lsrc);
        }

        int64_t  muls = ((int64_t)rsrcs) * ((int64_t)lsrcs);

        if( INST_W_BIT ){
            REG_EAX = ( muls     &0xffffffff);
            REG_EDX = ((muls>>32)&0xffffffff);

            if( (muls&0xffffffff80000000ULL) == 0 || (muls&0xffffffff80000000ULL) == 0xffffffff80000000ULL ){
                REG_FLAGS &=~((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
            }else{
                REG_FLAGS |= ((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
            }
        }else{
            REG_AX = ( muls     &0xffff);

            if( (muls&0xff80) == 0 || (muls&0xff80) == 0xff80 ){
                REG_FLAGS &=~((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
            }else{
                REG_FLAGS |= ((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
            }
        }
    }


    return EX_RESULT_SUCCESS;
}


int exMUL(struct stMachineState *pM, uint32_t pointer){
    uint16_t size;
    uint32_t lsrc, rsrc;
    struct stOpl op;

    if( PREFIX_OP32 ) return exMUL32(pM, pointer);

    uint8_t inst0 = pM->reg.fetchCache[0];
    uint8_t inst1 = pM->reg.fetchCache[1];
    uint8_t bit0 = ((inst0>>0) & 1);

    uint8_t funct = (inst1 & 0x38);
    // funct 
    // 0x20: MUL  (unsigned)
    // 0x28: IMUL (signed)

    if( (inst0&0xfe) != 0xf6 || (funct != 0x20 && funct != 0x28) ) return EX_RESULT_UNKNOWN;

    size = decode_mod_rm(pM,  pointer+1, INST_W_BIT, &op);
    UPDATE_IP(size + 1);

    rsrc = readOpl(pM,  &op);
    if( INST_W_BIT ){
        lsrc = REG_AX;
    }else{
        lsrc = (REG_AX&0xff);
        rsrc&= 0xff;
    }


    if(DEBUG){
        if( funct == 0x20 ) EXI_LOG_PRINTF("MUL%c ",  INST_W_BIT ? 'w' : 'b');
        if( funct == 0x28 ) EXI_LOG_PRINTF("IMUL%c ",  INST_W_BIT ? 'w' : 'b');
        log_printOpl(EXI_LOGLEVEL, pM, &op);
        EXI_LOG_PRINTF("(=%x)\n", rsrc);
    }

    if( funct == 0x20 ){ // MUL (unsigned)
        uint32_t mulu = ((uint32_t)rsrc) * ((uint32_t)lsrc);

        if( INST_W_BIT ){
            REG_AX = ( mulu     &0xffff);
            REG_DX = ((mulu>>16)&0xffff);
            if(REG_DX !=0) REG_FLAGS |= ((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
            else           REG_FLAGS &=~((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
        }else{
            REG_AX = ( mulu     &0xffff);

            if(REG_AX&0xff00) REG_FLAGS |= ((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
            else              REG_FLAGS &=~((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
        }
    }else{ // IMUL (signed)
        uint32_t rsrcs, lsrcs;
        if( INST_W_BIT ){
            rsrcs = ((rsrc & 0x8000) ? (rsrc|0xffff0000) : rsrc);
            lsrcs = ((lsrc & 0x8000) ? (lsrc|0xffff0000) : lsrc);
        }else{
            rsrcs = ((rsrc & 0x80)   ? (rsrc|0xffffff00) : rsrc);
            lsrcs = ((lsrc & 0x80)   ? (lsrc|0xffffff00) : lsrc);
        }

        int32_t  muls = ((int32_t)rsrcs) * ((int32_t)lsrcs);

        if( INST_W_BIT ){
            REG_AX = ( muls     &0xffff);
            REG_DX = ((muls>>16)&0xffff);

            if( (muls&0xffff8000) == 0 || (muls&0xffff8000) == 0xffff8000 ){
                REG_FLAGS &=~((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
            }else{
                REG_FLAGS |= ((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
            }
        }else{
            REG_AX = ( muls     &0xffff);

            if( (muls&0xff80) == 0 || (muls&0xff80) == 0xff80 ){
                REG_FLAGS &=~((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
            }else{
                REG_FLAGS |= ((1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_OF));
            }
        }
    }


    return EX_RESULT_SUCCESS;
}


int exDIV32(struct stMachineState *pM, uint32_t pointer){
    uint16_t size;
    uint64_t numr, divr;
    struct stOpl op;

    uint8_t inst0 = pM->reg.fetchCache[0];
    uint8_t inst1 = pM->reg.fetchCache[1];
    uint8_t bit0 = ((inst0>>0) & 1);

    uint8_t funct = (inst1 & 0x38);
    // funct 
    // 0x30: DIV  (unsigned)
    // 0x38: IDIV (signed)

    if( (inst0&0xfe) != 0xf6 || (funct != 0x30 && funct != 0x38) ) return EX_RESULT_UNKNOWN;

    size = decode_mod_rm(pM,  pointer+1, INST_W_BIT, &op);
    UPDATE_IP(size + 1);

    divr = readOpl(pM,  &op);
    if( INST_W_BIT ){
        numr = REG_EDX;
        numr = (numr << 16);
        numr = (numr << 16);
        numr|= REG_EAX;
        if(funct == 0x38 && (divr&0x80000000ULL) ){
            divr |= 0xffffffff00000000ULL;
        }
    }else{
        numr = REG_AX;
        divr&= 0xff;
        if(funct == 0x38 && (numr&0x8000)){
            numr |= 0xffffffffffff0000ULL;
        }
        if(funct == 0x38 && (divr&0x80)){
            divr |= 0xffffffffffffff00ULL;
        }
    }

    if(DEBUG){
        if( funct == 0x30 ) EXI_LOG_PRINTF("DIV%c ",  INST_W_BIT ? 'w' : 'b');
        if( funct == 0x38 ) EXI_LOG_PRINTF("IDIV%c ",  INST_W_BIT ? 'w' : 'b');
        log_printOpl(EXI_LOGLEVEL, pM, &op);
        EXI_LOG_PRINTF("(=%x)\n", divr);
    }

    if( funct == 0x30 ){ // DIV (unsigned)
        uint64_t quou = (divr==0) ? 0xfffffffffULL : ((uint64_t)numr) / ((uint64_t)divr);
        uint64_t remu = (divr==0) ? 0xfffffffffULL : ((uint64_t)numr) % ((uint64_t)divr);

        if( (INST_W_BIT != 0 && quou > 0xffffffffULL) || 
            (INST_W_BIT == 0 && quou > 0xff) ){
            // overflow
            logfile_printf((LOGCAT_CPU_EXE | LOGLV_ERROR), "Error : divide 0 (3) %lx/%lx (CS:EIP %x:%x)\n", numr, divr, pM->reg.current_cs, pM->reg.current_eip);
            enterINT(pM,  INTNUM_DIVIDE_ERROR, pM->reg.current_cs, pM->reg.current_eip, 0);
        }else{
            if( INST_W_BIT ){
                REG_EAX = quou;
                REG_EDX = remu;
            }else{
                REG_AX = ((remu&0xff)<<8);
                REG_AX|=  (quou&0xff);
            }
        }
    }else{ // IDIV (signed)
        int64_t  quos = (divr==0) ? 0xfffffffffLL : (( int64_t)numr) / (( int64_t)divr);
        int64_t  rems = (divr==0) ? 0xfffffffffLL : (( int64_t)numr) % (( int64_t)divr);

        if( (INST_W_BIT != 0 && quos > 0 && quos > 0x7fffffffLL) || 
            (INST_W_BIT != 0 && quos < 0 && quos <-0x80000000LL) ||
            (INST_W_BIT == 0 && quos > 0 && quos > 0x7f) ||
            (INST_W_BIT == 0 && quos < 0 && quos <-0x80) ){
            // overflow
            enterINT(pM,  INTNUM_DIVIDE_ERROR, pM->reg.current_cs, pM->reg.current_eip, 0);
        }else{
            if( INST_W_BIT ){
                REG_EAX = quos;
                REG_EDX = rems;
            }else{
                REG_AX = ((((uint32_t)rems)&0xff)<<8);
                REG_AX|=  (((uint32_t)quos)&0xff);
            }
        }
    }

    return EX_RESULT_SUCCESS;
}

int exDIV(struct stMachineState *pM, uint32_t pointer){
    uint16_t size;
    uint32_t numr, divr;
    struct stOpl op;

    if( PREFIX_OP32 ) return exDIV32(pM, pointer);

    uint8_t inst0 = pM->reg.fetchCache[0];
    uint8_t inst1 = pM->reg.fetchCache[1];
    uint8_t bit0 = ((inst0>>0) & 1);

    uint8_t funct = (inst1 & 0x38);
    // funct 
    // 0x30: DIV  (unsigned)
    // 0x38: IDIV (signed)

    if( (inst0&0xfe) != 0xf6 || (funct != 0x30 && funct != 0x38) ) return EX_RESULT_UNKNOWN;

    size = decode_mod_rm(pM,  pointer+1, INST_W_BIT, &op);
    UPDATE_IP(size + 1);

    divr = readOpl(pM,  &op);
    if( INST_W_BIT ){
        numr = REG_DX;
        numr = (numr << 16);
        numr|= REG_AX;
        if(funct == 0x38 && (divr&0x8000) ){
            divr |= 0xffff0000;
        }
    }else{
        numr = REG_AX;
        divr&= 0xff;
        if(funct == 0x38 && (numr&0x8000)){
            numr |= 0xffff0000;
        }
        if(funct == 0x38 && (divr&0x80)){
            divr |= 0xffffff00;
        }
    }

    if(DEBUG){
        if( funct == 0x30 ) EXI_LOG_PRINTF("DIV%c ",  INST_W_BIT ? 'w' : 'b');
        if( funct == 0x38 ) EXI_LOG_PRINTF("IDIV%c ",  INST_W_BIT ? 'w' : 'b');
        log_printOpl(EXI_LOGLEVEL, pM, &op);
        EXI_LOG_PRINTF("(=%x)\n", divr);
    }

    if( funct == 0x30 ){ // DIV (unsigned)
        uint32_t quou = (divr==0) ? 0xfffff : ((uint32_t)numr) / ((uint32_t)divr);
        uint32_t remu = (divr==0) ? 0xfffff : ((uint32_t)numr) % ((uint32_t)divr);

        if( (INST_W_BIT != 0 && quou > 0xffff) || 
            (INST_W_BIT == 0 && quou > 0xff) ){
            // overflow
            if( pM->pEmu->emu_cpu == EMU_CPU_8086 ){
                // i8086 pushes the address of the next instruction.
                // See: https://stackoverflow.com/questions/71070990/x86-division-exception-return-address
                enterINT(pM,  INTNUM_DIVIDE_ERROR, REG_CS, REG_IP, 0);
            }else{
                enterINT(pM,  INTNUM_DIVIDE_ERROR, pM->reg.current_cs, pM->reg.current_eip, 0);
            }
        }else{
            if( INST_W_BIT ){
                REG_AX = quou;
                REG_DX = remu;
            }else{
                REG_AX = ((remu&0xff)<<8);
                REG_AX|=  (quou&0xff);
            }
        }
    }else{ // IDIV (signed)
        int32_t  quos = (divr==0) ? 0xfffff : (( int32_t)numr) / (( int32_t)divr);
        int32_t  rems = (divr==0) ? 0xfffff : (( int32_t)numr) % (( int32_t)divr);

        if( (INST_W_BIT != 0 && quos > 0 && quos > 0x7fff) || 
            (INST_W_BIT != 0 && quos < 0 && quos <-0x8000) ||
            (INST_W_BIT == 0 && quos > 0 && quos > 0x7f) ||
            (INST_W_BIT == 0 && quos < 0 && quos <-0x80) ){
            // overflow
            if( pM->pEmu->emu_cpu == EMU_CPU_8086 ){
                // i8086 pushes the address of the next instruction.
                // See: https://stackoverflow.com/questions/71070990/x86-division-exception-return-address
                enterINT(pM,  INTNUM_DIVIDE_ERROR, REG_CS, REG_IP, 0);
            }else{
                enterINT(pM,  INTNUM_DIVIDE_ERROR, pM->reg.current_cs, pM->reg.current_eip, 0);
            }
        }else{
            if( INST_W_BIT ){
                REG_AX = quos;
                REG_DX = rems;
            }else{
                REG_AX = ((((uint32_t)rems)&0xff)<<8);
                REG_AX|=  (((uint32_t)quos)&0xff);
            }
        }
    }

    return EX_RESULT_SUCCESS;
}

int exDASAAS(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = fetchCodeDataByte(pM, pointer);
    uint8_t old_al, old_cf, al;

    if( inst0 != 0x2F && inst0 != 0x3F ) return EX_RESULT_UNKNOWN;

    UPDATE_IP(1);

    if(inst0 == 0x2F){
        if(DEBUG) EXI_LOG_PRINTF("DAS\n"); 

        old_al = ((REG_AX)  & 0xff);
        old_cf =((REG_FLAGS & (1<<FLAGS_BIT_CF)) ? 1 : 0);
        REG_FLAGS &= ~(1<<FLAGS_BIT_CF);

        if( (old_al & 0x0f) > 9 || (REG_FLAGS & (1<<FLAGS_BIT_AF))  ){
            REG_AX = ((REG_AX & 0xff00) | ((old_al - 6) & 0xff));
            if( old_cf || (old_al < 6) ){
                REG_FLAGS |= (1<<FLAGS_BIT_CF);
            }
            REG_FLAGS |= (1<<FLAGS_BIT_AF);
        }else{
            REG_FLAGS &=~(1<<FLAGS_BIT_AF);
        }
        if( old_al > 0x99 || old_cf  ){
            REG_AX     = ((REG_AX & 0xff00) | (((REG_AX & 0xff) - 0x60) & 0xff));
            REG_FLAGS |= (1<<FLAGS_BIT_CF);
        }

        al = ((REG_AX) & 0xff);
        if(al == 0) REG_FLAGS |= (1<<FLAGS_BIT_ZF);
        else        REG_FLAGS &=~(1<<FLAGS_BIT_ZF);

        if(al&0x80) REG_FLAGS |= (1<<FLAGS_BIT_SF);
        else        REG_FLAGS &=~(1<<FLAGS_BIT_SF);

        if(calcParityByte(al)) REG_FLAGS |= (1<<FLAGS_BIT_PF);
        else                   REG_FLAGS &=~(1<<FLAGS_BIT_PF);

    }else if(inst0 == 0x3F){
        if(DEBUG){ 
            EXI_LOG_PRINTF("AAS\n");
        }
        if( (REG_AX & 0xf) > 9 || (REG_FLAGS & (1<<FLAGS_BIT_AF))  ){
            REG_AX     = REG_AX - 6;
            REG_AX     = (((REG_AX - 0x100) & 0xff00) | (REG_AX & 0x0f));
            REG_FLAGS |= (1<<FLAGS_BIT_CF);
            REG_FLAGS |= (1<<FLAGS_BIT_AF);
        }else{
            REG_FLAGS &=~(1<<FLAGS_BIT_CF);
            REG_FLAGS &=~(1<<FLAGS_BIT_AF);
            REG_AX     = (REG_AX & 0xff0f);
        }
    }

    return EX_RESULT_SUCCESS;
}

int exAADAAM(struct stMachineState *pM, uint32_t pointer){
    uint16_t al;
    uint32_t val;
    uint8_t inst0 = fetchCodeDataByte(pM, pointer);

    if( inst0 != 0xD5 && inst0 != 0xd4 ) return EX_RESULT_UNKNOWN;

    decode_imm(pM, pointer+1, INST_W_BYTEACC, &val, INST_S_NOSIGNEX);
    UPDATE_IP(2);

    if(inst0 == 0xd5){
        if(DEBUG) EXI_LOG_PRINTF("AAD %x\n", val); 

        REG_AX  = ((REG_AX>>8)&0xff) * val + (REG_AX & 0xff);
        REG_AX &= 0xff;

        al = REG_AX;
    }else if(inst0 == 0xd4){
        if(DEBUG){ 
            EXI_LOG_PRINTF("AAM %x\n", val);
            if(val!=10) EXI_LOG_PRINTF("AAM without 0xa value (%x) at [%x:%x]\n", val, REG_CS, REG_EIP-2);
        }

        if(val == 0){
            EXI_LOG_ERR_PRINTF("ERROR: operand of AAM instruction is zero.\n");
            return EX_RESULT_UNKNOWN;
        }

        al = (REG_AX & 0xff);
        REG_AX = ((al / val)<<8);  // AH
        al = al % val;
        REG_AX|= al;  // AL
    }

    if(al == 0) REG_FLAGS |= (1<<FLAGS_BIT_ZF);
    else        REG_FLAGS &=~(1<<FLAGS_BIT_ZF);

    if(al&0x80) REG_FLAGS |= (1<<FLAGS_BIT_SF);
    else        REG_FLAGS &=~(1<<FLAGS_BIT_SF);

    if(calcParityByte(al)) REG_FLAGS |= (1<<FLAGS_BIT_PF);
    else                   REG_FLAGS &=~(1<<FLAGS_BIT_PF);

    return EX_RESULT_SUCCESS;
}


int exDAA(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = fetchCodeDataByte(pM, pointer);

    if( inst0 != 0x27 ) return EX_RESULT_UNKNOWN;

    if(DEBUG) EXI_LOG_PRINTF("DAA\n"); 

    UPDATE_IP(1);

    uint8_t old_al = (REG_AX&0xff);
    uint8_t old_cf =((REG_FLAGS & (1<<FLAGS_BIT_CF)) ? 1 : 0);
    REG_FLAGS &= ~(1<<FLAGS_BIT_CF);

    if( (old_al & 0x0f) > 9 || (REG_FLAGS & (1<<FLAGS_BIT_AF))  ){
        REG_AX = (REG_AX&0xff00) | ((old_al + 6) & 0xff);
        if(old_cf || (old_al > 255-6) ){
            REG_FLAGS |= (1<<FLAGS_BIT_CF);
        }
        REG_FLAGS |=  (1<<FLAGS_BIT_AF);
    }else{
        REG_FLAGS &= ~(1<<FLAGS_BIT_AF);
    }
    if((old_al&0xff)>0x99 || old_cf){
        REG_AX = ((REG_AX&0xff00) | ((REG_AX+0x60)&0xff));
        REG_FLAGS |=  (1<<FLAGS_BIT_CF);
    }else{
        REG_FLAGS &= ~(1<<FLAGS_BIT_CF);
    }

    uint8_t al = (REG_AX&0xff);
    if(al == 0) REG_FLAGS |= (1<<FLAGS_BIT_ZF);
    else        REG_FLAGS &=~(1<<FLAGS_BIT_ZF);

    if(al&0x80) REG_FLAGS |= (1<<FLAGS_BIT_SF);
    else        REG_FLAGS &=~(1<<FLAGS_BIT_SF);

    if(calcParityByte(al)) REG_FLAGS |= (1<<FLAGS_BIT_PF);
    else                   REG_FLAGS &=~(1<<FLAGS_BIT_PF);

    return EX_RESULT_SUCCESS;
}

int exConvSiz(struct stMachineState *pM, uint32_t pointer){
    uint32_t a;
    uint8_t inst0 = fetchCodeDataByte(pM, pointer);

    if( inst0 == 0x98 ){
        if( PREFIX_OP32 ){
            if(DEBUG) EXI_LOG_PRINTF("CWDE\n"); 
            a = (REG_EAX & 0xffff);
            REG_EAX= ((a & 0x8000) ? (a | 0xffff0000) : a);
        }else{
            if(DEBUG) EXI_LOG_PRINTF("CBW\n"); 
            a = (REG_AX & 0xff);
            REG_AX = ((a &   0x80) ? (a |     0xff00) : a);
        }
    }else if( inst0 == 0x99 ){
        if( PREFIX_OP32 ){
            if(DEBUG) EXI_LOG_PRINTF("CDQ\n"); 
            REG_EDX= ((REG_EAX & 0x80000000) ? 0xffffffff : 0);
        }else{
            if(DEBUG) EXI_LOG_PRINTF("CWD\n"); 
            REG_DX = ((REG_AX & 0x8000) ? 0xffff : 0);
        }
    }else{
        return EX_RESULT_UNKNOWN;
    }
    UPDATE_IP(1);


    return EX_RESULT_SUCCESS;
}


#define SHUFT_FUNCT_SHL  (0x4<<3)
#define SHUFT_FUNCT_SHR  (0x5<<3)
#define SHUFT_FUNCT_SAR  (0x7<<3)
#define SHUFT_FUNCT_ROL  (0x0<<3)
#define SHUFT_FUNCT_ROR  (0x1<<3)
#define SHUFT_FUNCT_RCL  (0x2<<3)
#define SHUFT_FUNCT_RCR  (0x3<<3)


int exShift(struct stMachineState *pM, uint32_t pointer){
    uint32_t size, val, shamt;
    struct stOpl op;

    const char *InstStr[8] = 
        {"ROL", "ROR",  "RCL", "RCR", "SHL", "SHR", NULL, "SAR"};

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = pM->reg.fetchCache[1];
    uint8_t bit0 = ((inst0>>0) & 1);
    uint8_t bit1 = ((inst0>>1) & 1);

    uint8_t funct = (inst1 & 0x38);

    if( ((inst0 & 0xfc) != 0xd0 && (inst0 & 0xfc) != 0xc0) || funct == (6<<3) ){
        return EX_RESULT_UNKNOWN;
    }

    if( (inst0 & 0xfc) == 0xd0 ){
        // 8086 original instructions
        size  = decode_mod_rm(pM,  pointer+1, INST_W_BIT, &op);
        val   = readOpl(pM,  &op);
        shamt = (INST_V_BIT==INST_V_CNT1) ? 1 : (REG_CX & 0xff);
    }else{
        // 186 instructions
        size = decode_mod_rm(pM,  pointer+1     , INST_W_BIT    , &op);
        decode_imm          (pM,  pointer+1+size, INST_W_BYTEACC, &shamt, INST_S_NOSIGNEX);
        size += 1;
        val   = readOpl(pM,  &op);
    }
    UPDATE_IP(size + 1);

    uint32_t msb_pat, op_mask;

    if( pM->pEmu->emu_cpu >= EMU_CPU_80386){ // TODO: check the border
        shamt &= 0x1f; 
    }

    if( INST_W_BIT ){
        if( PREFIX_OP32 ){
            msb_pat = 0x80000000;
            op_mask = 0xffffffff;
        }else{
            msb_pat = 0x8000;
            op_mask = 0xffff;
        }
    }else{
        msb_pat = 0x80;
        op_mask = 0xff;
    }


    if( funct == SHUFT_FUNCT_SHL ){
        uint8_t temp = shamt, msb, cf;
        while( temp != 0 ){
            msb = (val & msb_pat) ? 1 : 0;

            if(msb){ REG_FLAGS |=  (1<<FLAGS_BIT_CF); cf =1; }
            else   { REG_FLAGS &= ~(1<<FLAGS_BIT_CF); cf =0; }

            val = ((val * 2) & op_mask);
            temp--;

            msb = (val & msb_pat) ? 1 : 0;

            if( msb^cf ) REG_FLAGS |= (1<<FLAGS_BIT_OF);
            else         REG_FLAGS &=~(1<<FLAGS_BIT_OF);
        }

        msb = (val & msb_pat) ? 1 : 0;

        if( msb != 0 ) REG_FLAGS |= (1<<FLAGS_BIT_SF);
        else           REG_FLAGS &=~(1<<FLAGS_BIT_SF);

        if( val == 0 ) REG_FLAGS |= (1<<FLAGS_BIT_ZF);
        else           REG_FLAGS &=~(1<<FLAGS_BIT_ZF);

        val  = (val & op_mask);
        if(calcParityByte(val)) REG_FLAGS |= (1<<FLAGS_BIT_PF); // parity of the low byte
        else                    REG_FLAGS &=~(1<<FLAGS_BIT_PF);

    }else if( funct == SHUFT_FUNCT_SHR ){
        uint8_t temp = shamt, msb;
        while( temp != 0 ){
            msb = (val & msb_pat) ? 1 : 0;

            if(val&1) REG_FLAGS |=  (1<<FLAGS_BIT_CF);
            else      REG_FLAGS &= ~(1<<FLAGS_BIT_CF);

            val = val / 2;
            temp--;

            if( msb ) REG_FLAGS |= (1<<FLAGS_BIT_OF);
            else      REG_FLAGS &=~(1<<FLAGS_BIT_OF);
        }
        msb = (val & msb_pat) ? 1 : 0;

        if( msb != 0 ) REG_FLAGS |= (1<<FLAGS_BIT_SF);
        else           REG_FLAGS &=~(1<<FLAGS_BIT_SF);

        if( val == 0 ) REG_FLAGS |= (1<<FLAGS_BIT_ZF);
        else           REG_FLAGS &=~(1<<FLAGS_BIT_ZF);

        val  = (val & op_mask);
        if(calcParityByte(val)) REG_FLAGS |= (1<<FLAGS_BIT_PF);
        else                    REG_FLAGS &=~(1<<FLAGS_BIT_PF);

    }else if( funct == SHUFT_FUNCT_SAR ){
        uint8_t temp = shamt, msb;
        while( temp != 0 ){
            msb = (val & msb_pat) ? 1 : 0;

            if(val&1) REG_FLAGS |=  (1<<FLAGS_BIT_CF);
            else      REG_FLAGS &= ~(1<<FLAGS_BIT_CF);

            val = val / 2;
            temp--;

            if(msb) val |= msb_pat;

            REG_FLAGS &=~(1<<FLAGS_BIT_OF);
        }

        msb = (val & msb_pat) ? 1 : 0;

        if( msb != 0 ) REG_FLAGS |= (1<<FLAGS_BIT_SF);
        else           REG_FLAGS &=~(1<<FLAGS_BIT_SF);

        if( val == 0 ) REG_FLAGS |= (1<<FLAGS_BIT_ZF);
        else           REG_FLAGS &=~(1<<FLAGS_BIT_ZF);

        val  = (val & op_mask);
        if(calcParityByte(val)) REG_FLAGS |= (1<<FLAGS_BIT_PF);
        else                    REG_FLAGS &=~(1<<FLAGS_BIT_PF);
    }else if( funct == SHUFT_FUNCT_ROL ){
        // affects only CF, OF
        uint8_t temp = shamt, cf, msb;
        while( temp != 0 ){
            msb = (val & msb_pat) ? 1 : 0;

            if( msb ){
                REG_FLAGS |= (1<<FLAGS_BIT_CF); cf = 1;
            }else{
                REG_FLAGS &=~(1<<FLAGS_BIT_CF); cf = 0;
            }
            val = val*2 + cf;
            temp--;

            msb = (val & msb_pat) ? 1 : 0;

            if( msb^cf ){
                REG_FLAGS |= (1<<FLAGS_BIT_OF);
            }else{
                REG_FLAGS &=~(1<<FLAGS_BIT_OF);
            }
        }
    }else if( funct == SHUFT_FUNCT_ROR ){
        // affects only CF, OF
        uint8_t temp = shamt, cf, msb2, msb;
        while( temp != 0 ){
            msb  = (val &  msb_pat    ) ? 1 : 0;
            msb2 = (val & (msb_pat>>1)) ? 1 : 0;

            if(val&1){ REG_FLAGS |=  (1<<FLAGS_BIT_CF); cf=1;}
            else     { REG_FLAGS &= ~(1<<FLAGS_BIT_CF); cf=0;}

            val = val / 2;
            if(cf) val |= msb_pat;
            temp--;

            msb  = (val &  msb_pat    ) ? 1 : 0;
            msb2 = (val & (msb_pat>>1)) ? 1 : 0;

            if( msb^msb2 ) REG_FLAGS |= (1<<FLAGS_BIT_OF);
            else           REG_FLAGS &=~(1<<FLAGS_BIT_OF);
        }
    }else if( funct == SHUFT_FUNCT_RCL ){
        // affects only CF, OF
        uint8_t temp = shamt, tempcf, cf, msb;
        while( temp != 0 ){
            tempcf = ((REG_FLAGS & (1<<FLAGS_BIT_CF)) ? 1 : 0);

            msb = (val & msb_pat) ? 1 : 0;

            if( msb ){
                REG_FLAGS |= (1<<FLAGS_BIT_CF); cf = 1;
            }else{
                REG_FLAGS &=~(1<<FLAGS_BIT_CF); cf = 0;
            }
            val = val*2 + tempcf;
            temp--;

            msb = (val & msb_pat) ? 1 : 0;

            if( msb^cf ){
                REG_FLAGS |= (1<<FLAGS_BIT_OF);
            }else{
                REG_FLAGS &=~(1<<FLAGS_BIT_OF);
            }
        }
    }else if( funct == SHUFT_FUNCT_RCR ){
        // affects only CF, OF
        uint8_t temp = shamt, tempcf, msb2, msb;
        while( temp != 0 ){
            tempcf = ((REG_FLAGS & (1<<FLAGS_BIT_CF)) ? 1 : 0);

            msb  = (val &  msb_pat    ) ? 1 : 0;
            msb2 = (val & (msb_pat>>1)) ? 1 : 0;

            if(val&1) REG_FLAGS |=  (1<<FLAGS_BIT_CF);
            else      REG_FLAGS &= ~(1<<FLAGS_BIT_CF);

            val = val / 2;
            if(tempcf) val |= msb_pat;
            temp--;

            msb  = (val &  msb_pat    ) ? 1 : 0;
            msb2 = (val & (msb_pat>>1)) ? 1 : 0;

            if( msb^msb2 ) REG_FLAGS |= (1<<FLAGS_BIT_OF);
            else           REG_FLAGS &=~(1<<FLAGS_BIT_OF);
        }
    }else{
        return EX_RESULT_UNKNOWN;
    }

    writeOpl( pM,  &op, val );

    if(DEBUG){
        EXI_LOG_PRINTF(InstStr[(funct>>3) & 0x0f]); 
        EXI_LOG_PRINTF(" "); log_printOpl(EXI_LOGLEVEL, pM, &op);
        EXI_LOG_PRINTF(", %d\n", shamt);
    }

    return EX_RESULT_SUCCESS;
}


int exTEST(struct stMachineState *pM, uint32_t pointer){
    uint32_t val, val2;
    uint32_t size;

    struct stOpl *pOpDest = NULL;
    struct stOpl *pOpSrc  = NULL;
    struct stOpl opAcc, op1, op2;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = pM->reg.fetchCache[1];
    uint8_t bit0 = ((inst0>>0) & 1);

    opAcc.type  = OpTypeReg;
    opAcc.reg   = REG_NUM_AX;
    opAcc.width = INST_W_BIT;

    if( (inst0 & 0xfe) == 0x84 ){ 
        size = decode_mod_rm(pM,  pointer+1, INST_W_BIT, &op1);
        decode_reg2         (pM,  pointer+1, INST_W_BIT, &op2);

        pOpDest = &op1;
        pOpSrc  = &op2;
    }else if( (inst0 & 0xfe) == 0xf6 && (inst1 & 0x38) == 0x00 ){
        size  = decode_mod_rm(pM,  pointer+1     , INST_W_BIT, &op1);
        size += decode_imm   (pM,  pointer+1+size, INST_W_BIT, &val2, INST_S_NOSIGNEX);

        pOpDest = &op1;
    }else if( (inst0 & 0xfe) == 0xa8 ){
        size = decode_imm(pM, pointer+1, INST_W_BIT, &val2, INST_S_NOSIGNEX);

        pOpDest = &opAcc;
    }else{
        EXI_LOG_PRINTF("Failed %x %x\n", inst0, inst1);
        return EX_RESULT_UNKNOWN;
    }
    UPDATE_IP( size + 1 );

    if(DEBUG){
        EXI_LOG_PRINTF("TEST "); log_printOpl(EXI_LOGLEVEL, pM, pOpDest); EXI_LOG_PRINTF(", ");
        if( pOpSrc == NULL ){
            EXI_LOG_PRINTF("%x", val2);
        }else{
            log_printOpl(EXI_LOGLEVEL, pM, pOpSrc);
        }
        EXI_LOG_PRINTF("\n");
    }

    val = readOpl(pM,  pOpDest);

    ALUOPand(pM, val, (pOpSrc==NULL) ? val2 : readOpl(pM,  pOpSrc), INST_W_BIT);

    return EX_RESULT_SUCCESS;
}


int exStringInst(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    struct stOpl opax, op1, op2;

    const char *InstStr[16] = 
        {NULL, NULL,   NULL,   NULL, "MOVS", "MOVS", "CMPS", "CMPS",
         NULL, NULL, "STOS", "STOS", "LODS", "LODS", "SCAS", "SCAS"};

    if( (inst0 & 0xf0) != 0xa0 || InstStr[inst0&0x0f] == NULL ){
        return EX_RESULT_UNKNOWN;
    }

    if(DEBUG){
        EXI_LOG_PRINTF(InstStr[inst0&0x0f]); EXI_LOG_PRINTF("\n");
    }

    uint8_t bit0 = ((inst0>>0) & 1);
    uint16_t delta  = INST_W_BIT ? (PREFIX_OP32 ? 4 : 2) : 1;
    int32_t  ddelta = (REG_FLAGS & (1<<FLAGS_BIT_DF)) ? -delta : delta;

    op1.type = OpTypeMemWithSeg_Reg;
    op1.reg  = (PREFIX_SEG != PREF_SEG_UNSPECIFIED) ? PREFIX_SEG : SEGREG_NUM_DS; // DS could be altered with a prefix inst.
    op1.addr = REG_NUM_SI;
    op1.width= INST_W_BIT;

    op2.type = OpTypeMemWithSeg_Reg;
    op2.reg  = SEGREG_NUM_ES;
    op2.addr = REG_NUM_DI;
    op2.width= INST_W_BIT;

    opax.type = OpTypeReg;
    opax.reg  = REG_NUM_AX;
    opax.width= INST_W_BIT;

    do{
        if( (PREFIX_AD32 ? REG_ECX : REG_CX) == 0 && (PREFIX_REPZ == 0 || PREFIX_REPZ ==1) ){
            break;
        }

        if( (inst0&0xfe) == 0xa4 ){ // MOVS
            writeOpl(pM,  &op2, readOpl(pM,  &op1) );

            if( PREFIX_AD32 ){ REG_ESI+=ddelta; REG_EDI+=ddelta; }
            else             { REG_SI +=ddelta; REG_DI +=ddelta; }

        }else if( (inst0&0xfe) == 0xa6 ){ //CMPS
            ALUOPSub(pM, readOpl(pM,  &op1), readOpl(pM,  &op2), INST_W_BIT);

            if( PREFIX_AD32 ){ REG_ESI+=ddelta; REG_EDI+=ddelta; }
            else             { REG_SI +=ddelta; REG_DI +=ddelta; }

        }else if( (inst0&0xfe) == 0xae ){ // SCAS
            ALUOPSub(pM, readOpl(pM, &opax), readOpl(pM,  &op2), INST_W_BIT);

            if( PREFIX_AD32 ){ REG_EDI+=ddelta; }
            else             { REG_DI +=ddelta; }

        }else if( (inst0&0xfe) == 0xac ){ // LODS
            writeOpl(pM,  &opax, readOpl(pM,  &op1) );

            if( PREFIX_AD32 ){ REG_ESI+=ddelta; }
            else             { REG_SI +=ddelta; }            

        }else if( (inst0&0xfe) == 0xaa ){ // STOS
            writeOpl(pM,  &op2, REG_EAX );

            if( PREFIX_AD32 ){ REG_EDI+=ddelta; }
            else             { REG_DI +=ddelta; }
        }

        if( PREFIX_REPZ == 0 || PREFIX_REPZ == 1 ){
            if( PREFIX_AD32 ){ REG_ECX--; }
            else             { REG_CX--;  }

            if( (inst0&0xfe) == 0xa6 || (inst0&0xfe) == 0xae ){ // CMPS, SCAS
                // REPE (REPZ)
                if( PREFIX_REPZ == 1 && ((REG_FLAGS&(1<<FLAGS_BIT_ZF))==0) ) break;
                // REPNE (REPNZ)
                if( PREFIX_REPZ == 0 && ((REG_FLAGS&(1<<FLAGS_BIT_ZF))!=0) ) break;
            }
        }else{
            break;
        }
    }while(1);

    UPDATE_IP(1);

    return EX_RESULT_SUCCESS;
}


static int exCALL_interSegment(struct stMachineState *pM, uint32_t pointer, uint32_t seg, uint32_t offset, uint32_t next_eip){
    uint8_t access;

    if( (pM->pEmu->emu_cpu < EMU_CPU_80286) || (!MODE_PROTECTED32) ){
        if(PREFIX_OP32){ 
            PUSH_TO_STACK( REG_CS  );
            PUSH_TO_STACK( REG_EIP ); 
            updateSegReg(pM, SEGREG_NUM_CS, seg);
            REG_EIP = offset;
        }else{
            PUSH_TO_STACK( REG_CS );
            PUSH_TO_STACK( REG_IP );
            updateSegReg(pM, SEGREG_NUM_CS, seg);
            REG_EIP = (offset & 0xffff);
        }

        return EX_RESULT_SUCCESS;
    }

    // protected mode (non-VM86)
    if( ! getDescType(pM, seg, &access) ){
        ENTER_GP( ECODE_SEGMENT_GDT_LDT(seg) );
    }

    if( SEGACCESS_IS_TSS32_AVAIL(access) ){
        struct stRawSegmentDesc RS;
        uint8_t RPL = (seg & 3);

        // load TSS descriptor pointed by selector value "seg"
        // (do NOT load register values from TSS here)
        // Set busy bits and link field
        loadTaskRegister(pM, seg, &RS, 1, pM->reg.tr);

        if((RS.limit < TSS_MINIMUM_LIMIT_VALUE_32BIT) || 
            pM->reg.cpl > SEGACCESS_DPL(RS.access)    ||
            RPL         > SEGACCESS_DPL(RS.access) ){
            ENTER_GP( ECODE_SEGMENT_GDT_LDT(seg) );
        }

        // Save current register values, and keep busy bit of the TSS and keep NT flag
        unloadTaskRegister(pM, next_eip, 0, 0);

        // Change the task register and load register value from TSS
        pM->reg.tr       = seg;
        pM->reg.descc_tr = RS;
        pM->reg.cr[0]   |= (1<<CR0_BIT_TS);
        loadTaskState(pM);

        // set NT flag of the new task in CALL
        REG_FLAGS |= (1<<FLAGS_BIT_NT);

    }else if( SEGACCESS_IS_CALLGATE32(access) ){
        logfile_printf_without_header((LOGCAT_CPU_EXE | LOGLV_ERROR), "CALL %x:%x   (CALLGATE32, %x)\n", seg, offset, pointer);

        PREFIX_OP32 = 1;
        PREFIX_AD32 = 1;

        struct stGateDesc CG;
        struct stRawSegmentDesc RS;

        uint8_t CPL = pM->reg.cpl;
        uint8_t DPL = SEGACCESS_DPL(access);

        logfile_printf_without_header((LOGCAT_CPU_EXE | LOGLV_ERROR), "test1 \n");

        if( (DPL < CPL) || (DPL < (seg&3)) ){
            ENTER_GP( ECODE_SEGMENT_GDT_LDT(seg) );
        }

        if( ! (access & SEGACCESS_PRESENT) ){
            ENTER_NP( ECODE_SEGMENT_GDT_LDT(seg) );
        }

        loadGateDesc(pM, seg, &CG);
        loadRawSegmentDesc(pM, CG.selector, &RS);

        uint8_t DPLC= SEGACCESS_DPL(RS.access);

        if( (!SEGACCESS_IS_CSEG(RS.access)) || (DPLC > CPL) ){
            ENTER_GP( ECODE_SEGMENT_GDT_LDT_EXT(CG.selector) );
        }

        if( 0 == (RS.access & SEGACCESS_PRESENT) ){
            ENTER_NP( ECODE_SEGMENT_GDT_LDT_EXT(CG.selector) );
        }

        uint32_t stack_top = pM->reg.descc_ss.base + REG_ESP;

        if( (!SEGACCESS_CSEG_IS_CONFORMING(RS.access)) && DPLC < CPL ){
            uint32_t new_esp, old_esp;
            uint16_t new_ss,  old_ss;

            uint32_t TSSbase     = pM->reg.descc_tr.base;
            uint32_t TSS_SP_base = TSSbase + 4 + (DPLC * 8);

            new_esp  = readDataMemByteAsSV(pM, TSS_SP_base);
            new_esp |= ((uint32_t)readDataMemByteAsSV(pM, TSS_SP_base + 1))<< 8;
            new_esp |= ((uint32_t)readDataMemByteAsSV(pM, TSS_SP_base + 2))<<16;
            new_esp |= ((uint32_t)readDataMemByteAsSV(pM, TSS_SP_base + 3))<<24;

            new_ss  = readDataMemByteAsSV(pM, TSS_SP_base + 4);
            new_ss |= ((uint16_t) readDataMemByteAsSV(pM, TSS_SP_base + 5))<< 8;

            old_ss  = REG_SS;
            old_esp = REG_ESP;

            CG.selector &= (~3);
            CG.selector |= DPLC;

            // without this line, access violation occurs
            pM->reg.cpl = DPLC;

            updateSegReg(pM, SEGREG_NUM_SS, new_ss);
            REG_ESP = new_esp;

            PUSH_TO_STACK( old_ss );
            PUSH_TO_STACK( old_esp  );
        }

        for(int t=0; t < CG.len; t++){
            // TODO: check this code !!!
            uint32_t arg = readDataMemDoubleWord(pM, stack_top - 4 - 4*t);
            logfile_printf_without_header((LOGCAT_CPU_EXE | LOGLV_ERROR), "Args: 0x%x/0x%x \n", arg, RS.access);
            PUSH_TO_STACK( arg );
        }

        PUSH_TO_STACK( REG_CS  );
        PUSH_TO_STACK( REG_EIP );

        updateSegReg(pM, SEGREG_NUM_CS, CG.selector);
        REG_EIP = CG.offset;

    }else if( SEGACCESS_IS_CALLGATE16(access) ){
        logfile_printf_without_header((LOGCAT_CPU_EXE | LOGLV_ERROR), "CALL %x:%x   (CALLGATE16, %x)\n", seg, offset, pointer);
        return EX_RESULT_UNKNOWN;
    }else{
        if( !SEGACCESS_IS_CSEG(access) ){
            return EX_RESULT_UNKNOWN;
        }

        if( SEGACCESS_CSEG_IS_CONFORMING(access) ){
            // conforming case
            if( SEGACCESS_DPL(access) > pM->reg.cpl ) ENTER_GP( ECODE_SEGMENT_GDT_LDT(seg) );
        }else{
            // nonconforming case
            if( (seg&3)               >  pM->reg.cpl ) ENTER_GP( ECODE_SEGMENT_GDT_LDT(seg) );
            if( SEGACCESS_DPL(access) != pM->reg.cpl ) ENTER_GP( ECODE_SEGMENT_GDT_LDT(seg) );
        }

        if( !(SEGACCESS_PRESENT & access) ){
            ENTER_NP( ECODE_SEGMENT_GDT_LDT(seg) );
        }

        seg = ((seg&(~3)) | pM->reg.cpl);

        if(PREFIX_OP32){ 
            PUSH_TO_STACK( REG_CS  );
            PUSH_TO_STACK( REG_EIP ); 
            updateSegReg(pM, SEGREG_NUM_CS, seg);
            REG_EIP = offset;
        }else{
            PUSH_TO_STACK( REG_CS );
            PUSH_TO_STACK( REG_IP );
            updateSegReg(pM, SEGREG_NUM_CS, seg);
            REG_EIP = (offset & 0xffff);
        }
    }

    return EX_RESULT_SUCCESS;
}

int exCALL(struct stMachineState *pM, uint32_t pointer){
    uint32_t val, val2, size;
    struct stOpl op;

    uint8_t inst0 = pM->reg.fetchCache[0]; //fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = pM->reg.fetchCache[1];

    uint32_t data32 = PREFIX_OP32;

    if( inst0 == 0xe8 ){
        // Direct within segment
        size = decode_imm(pM, pointer+1, INST_W_WORDACC, &val, INST_S_NOSIGNEX);
        if(PREFIX_OP32){ 
            REG_EIP += (1+size); 
            PUSH_TO_STACK( REG_EIP );
            REG_EIP += val;
        }else{
            REG_IP += (1+size);
            PUSH_TO_STACK( REG_IP );
            REG_IP += val; REG_EIP &= 0xffff;
        }
        if(DEBUG) EXI_LOG_PRINTF("CALL %x\n", val);

    }else if( inst0 == 0xff && (inst1 & 0x38) == 0x10 ){
        // Indirect within segment
        size = decode_mod_rm(pM,  pointer+1, INST_W_WORDACC, &op);
        uint32_t new_ip = readOpl(pM,  &op); // read the destination at first, then push (E)IP
        if(PREFIX_OP32){ 
            REG_EIP += (1+size); 
            PUSH_TO_STACK( REG_EIP );
            REG_EIP = new_ip;
        }else{
            REG_IP += (1+size);
            PUSH_TO_STACK( REG_IP );
            REG_EIP = (new_ip & 0xffff);
        }
        if(DEBUG){ EXI_LOG_PRINTF("CALL ("); log_printOpl(EXI_LOGLEVEL, pM, &op); EXI_LOG_PRINTF(")\n"); }

    }else if( inst0 == 0x9a ){
        // Direct intersegment
        size = decode_imm(pM, pointer+1, INST_W_WORDACC, &val,  INST_S_NOSIGNEX);

        PREFIX_OP32 = 0;
        decode_imm(pM, pointer+1+size, INST_W_WORDACC, &val2, INST_S_NOSIGNEX); // 16-bit access
        PREFIX_OP32 = data32;

        if(DEBUG) EXI_LOG_PRINTF("LCALL %x:%x\n", val2, val);

        UPDATE_IP( 1+size+2 );

        return exCALL_interSegment(pM, pointer, val2, val, REG_EIP);

    }else if( inst0 == 0xff && (inst1 & 0x38) == 0x18 ){
        // Indirect intersegment
        size = decode_mod_rm(pM,  pointer+1, INST_W_WORDACC, &op);
        UPDATE_IP( 1+size );

        // this data should be read before pushing code segment
        // (segment prefix could be used for this instruction)
        uint32_t new_ip = readOpl(pM,  &op);

        PREFIX_OP32 = 0;
        op.addr += (data32 ? 4 : 2);
        uint16_t new_cs = readOpl(pM,  &op); // 16-bit access
        PREFIX_OP32 = data32;

        if(DEBUG){ EXI_LOG_PRINTF("LCALL %x:%x\n", new_cs, new_ip); }

        return exCALL_interSegment(pM, pointer, new_cs, new_ip, REG_EIP);
    }else{
        return EX_RESULT_UNKNOWN;
    }

    return EX_RESULT_SUCCESS;
}


static int exJMP_interSegment(struct stMachineState *pM, uint32_t pointer, uint32_t seg, uint32_t offset, uint32_t next_eip){
    uint8_t access;

    if( PREFIX_OP32 ){
        REG_EIP = offset;
    }else{
        REG_EIP = (offset & 0xffff);
    }

    if( pM->pEmu->emu_cpu >= EMU_CPU_80286 && MODE_PROTECTED32 ){
        if( ! getDescType(pM, seg, &access) ){
            ENTER_GP( ECODE_SEGMENT_GDT_LDT(seg) );
        }
        if( SEGACCESS_IS_TSS32_AVAIL(access) ){
            struct stRawSegmentDesc RS;
            uint8_t RPL = (seg & 3);

            if(DEBUG) EXI_LOG_PRINTF("LJMP %x:%x   (TSS)\n", seg, offset, pointer);

            // load TSS descriptor pointed by selector value "seg"
            // (do NOT load register values from TSS here)
            // Set busy bits, and keep link field
            loadTaskRegister(pM, seg, &RS, 1, 0);

            if((RS.limit < TSS_MINIMUM_LIMIT_VALUE_32BIT) || 
                pM->reg.cpl > SEGACCESS_DPL(RS.access)    ||
                RPL         > SEGACCESS_DPL(RS.access) ){
                ENTER_GP( ECODE_SEGMENT_GDT_LDT(seg) );
            }

            // Save current register values, clear busy bit of the TSS and keep NT flag
            unloadTaskRegister(pM, next_eip, 1, 0);

            // Change the task register and load register value from TSS
            pM->reg.tr       = seg;
            pM->reg.descc_tr = RS;
            pM->reg.cr[0]   |= (1<<CR0_BIT_TS);
            loadTaskState(pM);

            // clear NT flag of the new task in JMP
            REG_FLAGS &= ~(1<<FLAGS_BIT_NT);

        }else if( SEGACCESS_IS_CALLGATE32(access) ){
            logfile_printf_without_header((LOGCAT_CPU_EXE | LOGLV_ERROR), "LJMP %x:%x   (TRAPGATE, %x)\n", seg, offset, pointer);
            return EX_RESULT_UNKNOWN;
        }else{
            if( SEGACCESS_CSEG_IS_CONFORMING(access) ){
                // conforming case
                if( SEGACCESS_DPL(access) > pM->reg.cpl ) ENTER_GP( ECODE_SEGMENT_GDT_LDT(seg) );
            }else{
                // nonconforming case
                if( (seg&3)               >  pM->reg.cpl ) ENTER_GP( ECODE_SEGMENT_GDT_LDT(seg) );
                if( SEGACCESS_DPL(access) != pM->reg.cpl ) ENTER_GP( ECODE_SEGMENT_GDT_LDT(seg) );
            }

            seg = ((seg&(~3)) | pM->reg.cpl);
            updateSegReg(pM, SEGREG_NUM_CS, seg);
            if(DEBUG) EXI_LOG_PRINTF("LJMP %x:%x\n", seg, offset);
        }
    }else{
        updateSegReg(pM, SEGREG_NUM_CS, seg);
        if(DEBUG) EXI_LOG_PRINTF("LJMP %x:%x\n", seg, offset);
    }

    return EX_RESULT_SUCCESS;
}

int exJMP(struct stMachineState *pM, uint32_t pointer){
    uint32_t val, val2, size;
    struct stOpl op;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t inst1 = pM->reg.fetchCache[1];

    if( inst0 == 0xe9 ){
        // Direct within segment
        size = decode_imm(pM, pointer+1, INST_W_WORDACC, &val, INST_S_NOSIGNEX);
        UPDATE_IP( 1 + size );
        UPDATE_IP( val );
        if(DEBUG) EXI_LOG_PRINTF("JMP %x\n", val);
 
    }else if( inst0 == 0xeb ){
        // Direct within segment-short
        decode_imm(pM, pointer+1, INST_W_WORDACC, &val,  INST_S_SIGNEX);
        UPDATE_IP( val+2 );
        if(DEBUG) EXI_LOG_PRINTF("JMP %x\n", val);

    }else if( inst0 == 0xff && (inst1 & 0x38) == 0x20 ){
        // Indirect within segment
        decode_mod_rm(pM, pointer+1, INST_W_WORDACC, &op);
        if(DEBUG){ EXI_LOG_PRINTF("JMP ("); log_printOpl(EXI_LOGLEVEL, pM, &op); EXI_LOG_PRINTF(")\n"); }
        if( PREFIX_OP32 ){
            REG_EIP = readOpl(pM, &op);
        }else{
            REG_EIP =(readOpl(pM, &op) & 0xffff);
        }

    }else if( inst0 == 0xea ){
        // Direct intersegment
        uint32_t size;
        uint32_t next_eip;
        size = decode_imm(pM, pointer+1     , INST_W_WORDACC, &val,  INST_S_NOSIGNEX);
        decode_imm16     (pM, pointer+1+size,                 &val2);

        next_eip = REG_EIP + 1 + size + 2;
        return exJMP_interSegment(pM, pointer, val2, val, next_eip);

    }else if( inst0 == 0xff && (inst1 & 0x38) == 0x28 ){
        // Indirect intersegment

        uint32_t size;
        uint32_t next_eip;
        size = decode_mod_rm(pM,  pointer+1, INST_W_WORDACC, &op);
        if(DEBUG){ EXI_LOG_PRINTF("LJMP ("); log_printOpl(EXI_LOGLEVEL, pM, &op); EXI_LOG_PRINTF(")\n"); }

        if( PREFIX_OP32 ){
            val = readOpl(pM, &op); op.addr += 4;
        }else{
            val = readOpl(pM, &op); op.addr += 2;
        }
        val2  = readOpl(pM,  &op);
        val2 &= 0xffff;

        next_eip = REG_EIP + 1 + size;
        return exJMP_interSegment(pM, pointer, val2, val, next_eip);

    }else{
        return EX_RESULT_UNKNOWN;
    }

    return EX_RESULT_SUCCESS;
}


int exRET(struct stMachineState *pM, uint32_t pointer){
    uint32_t val, newseg;

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);

    if( inst0 == 0xc3 || inst0 == 0xcb ){
        if( PREFIX_OP32 ){ POP_FROM_STACK( REG_EIP );                  }
        else             { POP_FROM_STACK( REG_EIP ); REG_EIP&=0xffff; }

        if(inst0 == 0xc3){
            // within segment
            if(DEBUG) EXI_LOG_PRINTF("RET \n"); 
        }else{
            // inter segment
            POP_FROM_STACK(newseg);
            updateSegReg(pM, SEGREG_NUM_CS, newseg);
            if( MODE_PROTECTED && ! MODE_PROTECTEDVM ){
                if( pM->reg.cpl > SEGACCESS_DPL(pM->reg.descc_es.access) ){ updateSegReg(pM, SEGREG_NUM_ES, 0); }
                if( pM->reg.cpl > SEGACCESS_DPL(pM->reg.descc_ds.access) ){ updateSegReg(pM, SEGREG_NUM_DS, 0); }
                if( pM->reg.cpl > SEGACCESS_DPL(pM->reg.descc_fs.access) ){ updateSegReg(pM, SEGREG_NUM_FS, 0); }
                if( pM->reg.cpl > SEGACCESS_DPL(pM->reg.descc_gs.access) ){ updateSegReg(pM, SEGREG_NUM_GS, 0); }
            }

            if(DEBUG) EXI_LOG_PRINTF("LRET \n"); 
        }

    }else if( inst0 == 0xc2 || inst0 == 0xca ){
        // adding immed to SP

        // imm16 is a signed value according to the recent Intel 64 and IA-32 architecture software developer's manual
        decode_imm16(pM, pointer+1, &val); 
        if( (val & 0x8000) && PREFIX_OP32 ) val |= 0xffff0000;

        if( PREFIX_OP32 ){ POP_FROM_STACK( REG_EIP );                  }
        else             { POP_FROM_STACK( REG_EIP ); REG_EIP&=0xffff; }

        if(inst0 == 0xc2){
            // within segment
            if(DEBUG) EXI_LOG_PRINTF("RET %x\n", val); 
        }else{
            // inter segment
            POP_FROM_STACK(newseg);

            updateSegReg(pM, SEGREG_NUM_CS, newseg);
            if( MODE_PROTECTED && ! MODE_PROTECTEDVM ){
                if( pM->reg.cpl > SEGACCESS_DPL(pM->reg.descc_es.access) ){ updateSegReg(pM, SEGREG_NUM_ES, 0); }
                if( pM->reg.cpl > SEGACCESS_DPL(pM->reg.descc_ds.access) ){ updateSegReg(pM, SEGREG_NUM_DS, 0); }
                if( pM->reg.cpl > SEGACCESS_DPL(pM->reg.descc_fs.access) ){ updateSegReg(pM, SEGREG_NUM_FS, 0); }
                if( pM->reg.cpl > SEGACCESS_DPL(pM->reg.descc_gs.access) ){ updateSegReg(pM, SEGREG_NUM_GS, 0); }
            }

            if(DEBUG) EXI_LOG_PRINTF("LRET %x\n", val); 
        }
        if( pM->reg.descc_ss.big ){
            REG_ESP += val;
        }else{
            REG_SP  += val;
        }

    }else{
        return EX_RESULT_UNKNOWN;
    }

    return EX_RESULT_SUCCESS;
}

int exCondJump(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint16_t cond = 0;
    uint16_t tmpval1, tmpval2, tmpval3;
    uint32_t val;
    uint32_t size;

    if( (inst0 & 0xf0) == 0x70 ){
        decode_imm(pM, pointer+1, INST_W_WORDACC, &val, INST_S_SIGNEX);
        UPDATE_IP(2);
        cond = (inst0 & 0x0f);
    }else{
        uint16_t inst1 = pM->reg.fetchCache[1];
        if( inst0 == 0x0f && (inst1 & 0xf0) == 0x80 && pM->pEmu->emu_cpu >= EMU_CPU_80386 ){
            size = decode_imm(pM, pointer+2, INST_W_WORDACC, &val, INST_S_NOSIGNEX);
            UPDATE_IP(2 + size);
            cond = (inst1 & 0x0f);
        }else{
            return EX_RESULT_UNKNOWN;
        }
    }


    const char *InstStr[] = 
        {"JO", "JNO", "JB", "JNB", "JE", "JNE", "JBE", "JNBE",
        "JS", "JNS", "JP", "JNP", "JL", "JNL", "JLE", "JNLE"};

    if(DEBUG){
        EXI_LOG_PRINTF(InstStr[cond]); EXI_LOG_PRINTF(" %x\n", val);
    }

    switch( cond ){
        case 0x0:
            if(  REG_FLAGS & (1<<FLAGS_BIT_OF) ){ UPDATE_IP( val ); } break;
        case 0x1:
            if(!(REG_FLAGS & (1<<FLAGS_BIT_OF))){ UPDATE_IP( val ); } break;
        case 0x2:
            if(  REG_FLAGS & (1<<FLAGS_BIT_CF) ){ UPDATE_IP( val ); } break;
        case 0x3:
            if(!(REG_FLAGS & (1<<FLAGS_BIT_CF))){ UPDATE_IP( val ); } break;
        case 0x4:
            if(  REG_FLAGS & (1<<FLAGS_BIT_ZF) ){ UPDATE_IP( val ); } break;
        case 0x5:
            if(!(REG_FLAGS & (1<<FLAGS_BIT_ZF))){ UPDATE_IP( val ); } break;
        case 0x6:
            if(  REG_FLAGS &((1<<FLAGS_BIT_CF)|(1<<FLAGS_BIT_ZF)) ){ UPDATE_IP( val ); } break;
        case 0x7:
            if(!(REG_FLAGS &((1<<FLAGS_BIT_CF)|(1<<FLAGS_BIT_ZF)))){ UPDATE_IP( val ); } break;
        case 0x8:
            if(  REG_FLAGS & (1<<FLAGS_BIT_SF) ){ UPDATE_IP( val ); } break;
        case 0x9:
            if(!(REG_FLAGS & (1<<FLAGS_BIT_SF))){ UPDATE_IP( val ); } break;
        case 0xa:
            if(  REG_FLAGS & (1<<FLAGS_BIT_PF) ){ UPDATE_IP( val ); } break;
        case 0xb:
            if(!(REG_FLAGS & (1<<FLAGS_BIT_PF))){ UPDATE_IP( val ); } break;
        case 0xc:
            tmpval1 = (REG_FLAGS & (1<<FLAGS_BIT_SF)) ? 1 : 0;
            tmpval2 = (REG_FLAGS & (1<<FLAGS_BIT_OF)) ? 1 : 0;
            if( tmpval1 != tmpval2 ){ UPDATE_IP( val ); } break;
        case 0xd:
            tmpval1 = (REG_FLAGS & (1<<FLAGS_BIT_SF)) ? 1 : 0;
            tmpval2 = (REG_FLAGS & (1<<FLAGS_BIT_OF)) ? 1 : 0;
            if( tmpval1 == tmpval2 ){ UPDATE_IP( val ); } break;
        case 0xe:
            tmpval1 = (REG_FLAGS & (1<<FLAGS_BIT_SF)) ? 1 : 0;
            tmpval2 = (REG_FLAGS & (1<<FLAGS_BIT_OF)) ? 1 : 0;
            tmpval3 = (REG_FLAGS & (1<<FLAGS_BIT_ZF)) ? 1 : 0;
            if( (tmpval1 != tmpval2) || (tmpval3 == 1) ){ UPDATE_IP( val ); } break;
        case 0xf:
            tmpval1 = (REG_FLAGS & (1<<FLAGS_BIT_SF)) ? 1 : 0;
            tmpval2 = (REG_FLAGS & (1<<FLAGS_BIT_OF)) ? 1 : 0;
            tmpval3 = (REG_FLAGS & (1<<FLAGS_BIT_ZF)) ? 1 : 0;
            if( (tmpval1 == tmpval2) && (tmpval3 == 0) ){ UPDATE_IP( val ); } break;
    }

    return EX_RESULT_SUCCESS;
}


int exLOOP(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint32_t val;

    if( inst0 != 0xe0 && inst0 != 0xe1 && inst0 != 0xe2 ){
        return EX_RESULT_UNKNOWN;
    }

    decode_imm(pM, pointer+1, INST_W_WORDACC, &val, INST_S_SIGNEX);

    // Condition for the register selection is "ADDRESS SIZE"
    if(PREFIX_AD32){ REG_ECX--; }else{ REG_CX--; }
    UPDATE_IP(2);

    if( inst0 == 0xe2 ){ // LOOP
        if(DEBUG) EXI_LOG_PRINTF("LOOP %x\n", val);

        if( (PREFIX_AD32 ? REG_ECX : REG_CX) != 0 ){
            UPDATE_IP( val );
        }
    }else if( inst0 == 0xe1 ){ // LOOPE/LOOPZ
        if(DEBUG) EXI_LOG_PRINTF("LOOPZ %x\n", val);

        if( (PREFIX_AD32 ? REG_ECX : REG_CX) != 0 && (REG_FLAGS & (1<<FLAGS_BIT_ZF)) ){
            UPDATE_IP( val );
        }
    }else if( inst0 == 0xe0 ){ // LOOPNE/LOOPNZ
        if(DEBUG) EXI_LOG_PRINTF("LOOPNZ %x\n", val);

        if( (PREFIX_AD32 ? REG_ECX : REG_CX) != 0 && 0 == (REG_FLAGS & (1<<FLAGS_BIT_ZF)) ){
            UPDATE_IP( val );
        }
    }

    return EX_RESULT_SUCCESS;
}


int exJCXZ(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint32_t val;

    if( inst0 != 0xe3 ){
        return EX_RESULT_UNKNOWN;
    }

    decode_imm(pM, pointer+1, INST_W_WORDACC, &val, INST_S_SIGNEX);
    UPDATE_IP(2);

    if(DEBUG) EXI_LOG_PRINTF("JCXZ %x\n", val);

    // Condition for the register selection is "ADDRESS SIZE"
    if( (PREFIX_AD32 ? REG_ECX : REG_CX) == 0 ){
        UPDATE_IP( val );
    }

    return EX_RESULT_SUCCESS;
}

int exINT(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint8_t bit0  = ((inst0>>0) & 1);
    uint32_t val;

    if( (inst0&0xfe)  != 0xcc ){
        return EX_RESULT_UNKNOWN;
    }

    // perform INT3 if bit0 is 0
    val = 3;
    UPDATE_IP(1);
    if( bit0 ){
        decode_imm(pM, pointer+1, INST_W_BYTEACC, &val, INST_S_NOSIGNEX);
        UPDATE_IP(1);
    }
    if(DEBUG) EXI_LOG_PRINTF("INT %x (AX=%x)\n", val, REG_AX);

    enterINT(pM, val, REG_CS, REG_EIP /*PREFIX_OP32 ? REG_EIP : REG_IP*/, 1);
    return EX_RESULT_SUCCESS;
}

int exINTO(struct stMachineState *pM, uint32_t pointer){

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);

    if( inst0 != 0xce ){
        return EX_RESULT_UNKNOWN;
    }

    UPDATE_IP(1);

    if(DEBUG) EXI_LOG_PRINTF("INTO\n");

    if( ! (REG_FLAGS & (1<<FLAGS_BIT_OF)) ){
        return EX_RESULT_SUCCESS;
    }

    enterINT(pM,  INTNUM_OVERFLOW, REG_CS, REG_EIP /*PREFIX_OP32 ? REG_EIP : REG_IP*/, 1);

    return EX_RESULT_SUCCESS;
}


int exIRET_real(struct stMachineState *pM, uint32_t pointer){
    uint32_t val1, val2, val3;

    POP_FROM_STACK( val1 ); // (E)IP
    POP_FROM_STACK( val2 ); // CS
    POP_FROM_STACK( val3 ); // (E)FLAGS

    if( PREFIX_OP32 ){
        REG_EIP = val1;

        // this mask is based on "Intel 64 and IA-32 Architectures Software Developer’s Manual," order# 325462-083US, March 2024.
        REG_EFLAGS &= 0x1a0000;
        REG_EFLAGS |= (val3 & 0x257FD5);
    }else{
        REG_EIP = (val1&0xffff);
        REG_FLAGS  = val3;
    }

    updateSegReg(pM, SEGREG_NUM_CS, val2);

    return EX_RESULT_SUCCESS;    
}

int exIRET_VM86(struct stMachineState *pM, uint32_t pointer){
    uint32_t val1, val2, val3;

    if( IOPL(REG_EFLAGS) < 3 ){
        ENTER_GP(0);
    }

    POP_FROM_STACK( val1 ); // (E)IP
    POP_FROM_STACK( val2 ); // CS
    POP_FROM_STACK( val3 ); // (E)FLAGS

    if( PREFIX_OP32 ){
        REG_EIP = val1;

        uint32_t mask  = (FLAGS_BIT_IOPL_MASK | (1<<EFLAGS_BIT_VM));

        REG_EFLAGS &= mask;
        REG_EFLAGS |= (val3 & (~mask));
//        REG_EFLAGS &= ~(1<<EFLAGS_BIT_RF);
    }else{
        REG_EIP = (val1&0xffff);

        REG_FLAGS &= FLAGS_BIT_IOPL_MASK;
        REG_FLAGS |= (val3 & (~FLAGS_BIT_IOPL_MASK));
    }

    updateSegReg(pM, SEGREG_NUM_CS, val2);

    return EX_RESULT_SUCCESS;    
}

int exIRET_NT(struct stMachineState *pM, uint32_t pointer){
    // ---------------------------
    // This code is not checked
    // ---------------------------

    struct stRawSegmentDesc RS;
    uint16_t next_tr;
    next_tr  =             readDataMemByteAsSV(pM, pM->reg.descc_tr.base+0);
    next_tr |= (((uint16_t)readDataMemByteAsSV(pM, pM->reg.descc_tr.base+1))<<8);
    uint8_t  RPL     = (next_tr & 3);

    uint8_t access;
    if( ! getDescType(pM, next_tr, &access) ){
        ENTER_GP( ECODE_SEGMENT_GDT_LDT(next_tr) );
    }

    if( ! SEGACCESS_IS_TSS32(access) ){
        ENTER_TS( ECODE_SEGMENT_GDT_LDT(next_tr) );
    }

    // load a TSS descriptor pointed by "next_tr"
    // loading values from TSS is done after checking the descriptor
    // Keep busy bits and link field
    loadTaskRegister(pM, next_tr, &RS, 0, 0);

    if((RS.limit < TSS_MINIMUM_LIMIT_VALUE_32BIT) || 
        pM->reg.cpl > SEGACCESS_DPL(RS.access)    ||
        RPL         > SEGACCESS_DPL(RS.access) ){
        ENTER_TS( ECODE_SEGMENT_GDT_LDT(next_tr) );
    }

    // Save current register values and clear busy bit of the TSS and NT flag
    unloadTaskRegister(pM, pM->reg.current_eip + 1, 1, 1);

    // Change the task register and load register value from TSS
    pM->reg.tr       = next_tr;
    pM->reg.descc_tr = RS;
    pM->reg.cr[0]   |= (1<<CR0_BIT_TS);
    loadTaskState(pM);

    // keep NT flag of the new task in IRET

    return EX_RESULT_SUCCESS;
}

int exIRET(struct stMachineState *pM, uint32_t pointer){
    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);
    uint32_t val1, val2, val3, val4, val5;
    int goOuter = 0;

    if( inst0  != 0xcf ) return EX_RESULT_UNKNOWN; 
    if( DEBUG ) EXI_LOG_PRINTF("IRET\n");

    //--------------------------------------------
    // Processing for real and virtual 8086 modes
    //--------------------------------------------
    if( !MODE_PROTECTED   ) return exIRET_real(pM, pointer);
    if(  MODE_PROTECTEDVM ) return exIRET_VM86(pM, pointer);

    //--------------------------------------------
    // Processing for protected mode
    //--------------------------------------------
    if( REG_EFLAGS & (1<<FLAGS_BIT_NT) ) return exIRET_NT(pM, pointer);


    uint32_t saved_eflags = REG_EFLAGS;

    POP_FROM_STACK( val1 ); // (E)IP
    POP_FROM_STACK( val2 ); // CS
    POP_FROM_STACK( val3 ); // (E)FLAGS

    if( pM->reg.cpl == 0 && (val3 & (1<<EFLAGS_BIT_VM)) ){
        // return to VM86 mode

        logfile_printf(LOGCAT_CPU_EXE | LOGLV_INFO3, "Entering VM86 mode (pointer %x)\n", pointer);

        uint32_t vala, valb, valc, vald;

        REG_EFLAGS = val3;

        updateSegReg(pM, SEGREG_NUM_CS, val2);
        REG_EIP = (PREFIX_OP32 ? val1 : (val1&0xffff));

        int saved_op32 = PREFIX_OP32;
        PREFIX_OP32 = 1;
        POP_FROM_STACK( val4 ); // ESP
        POP_FROM_STACK( val5 ); // SS
        POP_FROM_STACK( vala ); // ES
        POP_FROM_STACK( valb ); // DS
        POP_FROM_STACK( valc ); // FS
        POP_FROM_STACK( vald ); // GS

        updateSegReg(pM, SEGREG_NUM_SS, val5);
        REG_ESP = val4;

        updateSegReg(pM, SEGREG_NUM_ES, vala);
        updateSegReg(pM, SEGREG_NUM_DS, valb);
        updateSegReg(pM, SEGREG_NUM_FS, valc);
        updateSegReg(pM, SEGREG_NUM_GS, vald);

        pM->reg.cpl = 3;

        PREFIX_OP32 = saved_op32;

        return EX_RESULT_SUCCESS;
    }
    
    
    if( (val2&0x3) > pM->reg.cpl ){ // is going to an outer level
        POP_FROM_STACK( val4 ); // ESP
        POP_FROM_STACK( val5 ); // SS

        goOuter = 1;
    }

    uint32_t flag_mask = 
        ( (1<<FLAGS_BIT_CF) | (1<<FLAGS_BIT_PF) | (1<<FLAGS_BIT_AF) | 
          (1<<FLAGS_BIT_ZF) | (1<<FLAGS_BIT_SF) | (1<<FLAGS_BIT_TF) | 
          (1<<FLAGS_BIT_DF) | (1<<FLAGS_BIT_OF) | (1<<FLAGS_BIT_NT) );

    if( PREFIX_OP32 ){
        REG_EIP     = val1;
        REG_EFLAGS &= (~flag_mask);
        REG_EFLAGS |= (val3 & flag_mask);

        REG_EFLAGS &= ~((1<<EFLAGS_BIT_AC)|(1<<EFLAGS_BIT_RF));
        REG_EFLAGS |= (val3 & ((1<<EFLAGS_BIT_AC)|(1<<EFLAGS_BIT_RF)));
    }else{
        REG_EIP     = (val1&0xffff);
        REG_EFLAGS &= (~flag_mask);
        REG_EFLAGS |= (val3 & flag_mask);
    }

    if( pM->reg.cpl <= IOPL(saved_eflags) ){
        REG_EFLAGS &= ~(1<<FLAGS_BIT_IF);
        REG_EFLAGS |= (val3 & (1<<FLAGS_BIT_IF));
    }
    if( pM->reg.cpl == 0 ){
        REG_EFLAGS &= ~FLAGS_BIT_IOPL_MASK;
        REG_EFLAGS |= (val3 & FLAGS_BIT_IOPL_MASK);
    }

    // This line may cause a fault. Therefore, loading the values from the stack was done before it
    updateSegReg(pM, SEGREG_NUM_CS, val2);

    if( goOuter ){
        updateSegReg(pM, SEGREG_NUM_SS, val5);
        REG_ESP = val4;

        if( pM->reg.cpl > SEGACCESS_DPL(pM->reg.descc_es.access) ){ updateSegReg(pM, SEGREG_NUM_ES, 0); }
        if( pM->reg.cpl > SEGACCESS_DPL(pM->reg.descc_ds.access) ){ updateSegReg(pM, SEGREG_NUM_DS, 0); }
        if( pM->reg.cpl > SEGACCESS_DPL(pM->reg.descc_fs.access) ){ updateSegReg(pM, SEGREG_NUM_FS, 0); }
        if( pM->reg.cpl > SEGACCESS_DPL(pM->reg.descc_gs.access) ){ updateSegReg(pM, SEGREG_NUM_GS, 0); }
    }

    return EX_RESULT_SUCCESS;
}


int exSetClearFlag(struct stMachineState *pM, uint32_t pointer){

    uint8_t inst0 = pM->reg.fetchCache[0]; // fetchCodeDataByte(pM, pointer);

    const char *InstStr[16] = 
        {NULL,  NULL,  NULL,  NULL,  NULL, "CMC",  NULL, NULL,
        "CLC", "STC", "CLI", "STI", "CLD", "STD", NULL, NULL};

    if( (inst0 & 0xf0) != 0xf0 || InstStr[inst0&0x0f] == NULL ){
        return EX_RESULT_UNKNOWN;
    }

    if(DEBUG){
        EXI_LOG_PRINTF(InstStr[inst0&0x0f]); EXI_LOG_PRINTF("\n");
    }

    if( inst0 == 0xfa || inst0 == 0xfb ){ // CLI, STI
        if( MODE_PROTECTED && (IOPL(REG_EFLAGS) < pM->reg.cpl) ){
            ENTER_GP(0);
        }
    }


    if( inst0 == 0xf8 ){ // CLC
        REG_FLAGS &= (0xffff ^ (1<<FLAGS_BIT_CF));

    }else if( inst0 == 0xf5 ){ //CMC
        REG_FLAGS ^= (1<<FLAGS_BIT_CF);

    }else if( inst0 == 0xf9 ){ // STC
        REG_FLAGS |= (1<<FLAGS_BIT_CF);

    }else if( inst0 == 0xfc ){ // CLD
        REG_FLAGS &= (0xffff ^ (1<<FLAGS_BIT_DF));

    }else if( inst0 == 0xfd ){ // STD
        REG_FLAGS |= (1<<FLAGS_BIT_DF);

    }else if( inst0 == 0xfa ){ // CLI
        REG_FLAGS &= (0xffff ^ (1<<FLAGS_BIT_IF));

    }else if( inst0 == 0xfb ){ // STI
        REG_FLAGS |= (1<<FLAGS_BIT_IF);

    }else{
        return EX_RESULT_UNKNOWN;
    }

    UPDATE_IP(1);

    return EX_RESULT_SUCCESS;
}

