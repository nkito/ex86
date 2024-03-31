#include <stdio.h>
#include <stdint.h>

#include "i8086.h"
#include "ALUop.h"
#include "decode.h"
#include "mem.h"
#include "descriptor.h"

#include "ExInst_common.h"

#include "logfile.h"


static void loadRawSegmentDescData(struct stMachineState *pM, uint32_t base, uint32_t limit, uint16_t selector, struct stRawSegmentDesc *pRS){
	uint16_t selector_body = (selector & (~7));

	if( selector & 7 ){
		logfile_printf(LOGCAT_CPU_MEM | LOGLV_INFO3, "%s: selector = %x (CS:EIP=%x:%x pointer %x)\n", __func__, selector, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
	}

	if( selector_body >= limit ){
		ENTER_GP(selector);
	}

	if( selector < 4 ){
		// invalid segments but loading them is valid.
		pRS->limit = pRS->base = pRS->access = pRS->flags = 0;
		return ;
	}


	uint32_t pointer = base + selector_body;

    pRS->limit  = readDataMemByteAsSV(pM, pointer+0);
    pRS->limit |= ((uint32_t) readDataMemByteAsSV(pM, pointer+1)      )<< 8;
    pRS->limit |= ((uint32_t)(readDataMemByteAsSV(pM, pointer+6)&0x0f))<<16;

    pRS->base  = readDataMemByteAsSV(pM, pointer+2);
    pRS->base |= ((uint32_t)readDataMemByteAsSV(pM, pointer+3))<< 8;
    pRS->base |= ((uint32_t)readDataMemByteAsSV(pM, pointer+4))<<16;
    pRS->base |= ((uint32_t)readDataMemByteAsSV(pM, pointer+7))<<24;

    pRS->access = readDataMemByteAsSV(pM, pointer+5);
    pRS->flags  = readDataMemByteAsSV(pM, pointer+6)>>4;

	uint8_t type;

	if( pRS->access & 0x10 ){
		// Code or Data Segment Descriptor
		type = ((pRS->access >>1)&0x7);

		if( ! (pRS->access & 0x80) ){
			logfile_printf(LOGCAT_CPU_MEM | LOGLV_WARNING, "%s: selector %x is not present (CS:EIP=%x:%x pointer %x)\n", __func__, selector, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
		}

		if( ! (pRS->access & 0x01) ){
			pRS->access |= 0x01; // access bit
			writeDataMemByteAsSV(pM, pointer+5, pRS->access);
		}

	}else{
		// System Descriptor
		type = (pRS->access &0xf);

		if( type != SYSDESC_TYPE_LDT         && 
		    type != SYSDESC_TYPE_TASKGATE    && 
		    type != SYSDESC_TYPE_TSS32_AVAIL && // 486 TSS
		    type != SYSDESC_TYPE_TSS32_BUSY  && // 486 TSS
			type != SYSDESC_TYPE_CALLGATE32  && // 486 call gate
			type != SYSDESC_TYPE_TRAPGATE32  && 
			type != SYSDESC_TYPE_INTGATE32
		){
			if( selector != 0 ){
				logfile_printf(LOGCAT_CPU_MEM | LOGLV_ERROR, "%s: selector type of %x is not valid (type: 0x%x) (CS:EIP=%x:%x pointer %x)\n", __func__, selector, type, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
			}
		}
	}
}

void loadRawSegmentDescFromGDT(struct stMachineState *pM, uint16_t selector, struct stRawSegmentDesc *pRS){
	loadRawSegmentDescData(pM, pM->reg.gdtr_base, pM->reg.gdtr_limit, selector, pRS);
}

void loadRawSegmentDescFromLDT(struct stMachineState *pM, uint16_t selector, struct stRawSegmentDesc *pRS){
	loadRawSegmentDescData(pM, pM->reg.descc_ldt.base, pM->reg.descc_ldt.limit, selector, pRS);
}

void loadRawSegmentDesc(struct stMachineState *pM, uint16_t selector, struct stRawSegmentDesc *pRS){
	if( selector & 0x4 ){
		loadRawSegmentDescData(pM, pM->reg.descc_ldt.base, pM->reg.descc_ldt.limit, selector, pRS);
	}else{
		loadRawSegmentDescData(pM, pM->reg.gdtr_base, pM->reg.gdtr_limit, selector, pRS);
	}
}

void loadDataSegmentDesc(struct stMachineState *pM, uint16_t selector, struct stDataDesc *pDD){
	struct stRawSegmentDesc RS;
	uint8_t RPL = (selector & 3);

	if( RPL < pM->reg.cpl ){
		logfile_printf(LOGCAT_CPU_MEM | LOGLV_NOTICE, "%s: CPL %x -> %x (CS:EIP=%x:%x pointer %x)\n", __func__, pM->reg.cpl, RPL, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
	}

	if( selector & 0x4 ){
		loadRawSegmentDescData(pM, pM->reg.descc_ldt.base, pM->reg.descc_ldt.limit, selector, &RS);
	}else{
		loadRawSegmentDescData(pM, pM->reg.gdtr_base, pM->reg.gdtr_limit, selector, &RS);
	}

	pDD->base   = RS.base;
	pDD->limit  = RS.limit;
	pDD->access = RS.access;
	pDD->flags  = RS.flags;

	pDD->gran     =  (pDD->flags  & SEGFLAGS_DSEG_GRAN     ) ? 1 : 0;
	pDD->big      =  (pDD->flags  & SEGFLAGS_DSEG_B_BIT    ) ? 1 : 0;
	pDD->writable =  (pDD->access & SEGACCESS_DSEG_WRITABLE) ? 1 : 0;
	pDD->DPL      = ((pDD->access>> SEGACCESS_BIT_DPL) & 3 );

	if( pDD->gran ){
		pDD->limit_min = (pDD->access & SEGACCESS_DSEG_EXDOWN) ? (pDD->limit+1)<<12 : 0;
		pDD->limit_max = (pDD->access & SEGACCESS_DSEG_EXDOWN) ? 0xffffffff         : (pDD->limit << 12) + 0xfff;
	}else{
		pDD->limit_min = (pDD->access & SEGACCESS_DSEG_EXDOWN) ? pDD->limit +1 : 0;
		pDD->limit_max = (pDD->access & SEGACCESS_DSEG_EXDOWN) ? 0xffffffff    : pDD->limit;
	}
}

void loadStackSegmentDesc(struct stMachineState *pM, uint16_t selector, struct stDataDesc *pDD){
	struct stRawSegmentDesc RS;
	uint8_t RPL = (selector & 3);

	if( RPL < pM->reg.cpl ){
		logfile_printf(LOGCAT_CPU_MEM | LOGLV_NOTICE, "%s: CPL %x -> %x (CS:EIP=%x:%x pointer %x)\n", __func__, pM->reg.cpl, RPL, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
	}

	if( selector & 0x4 ){
		loadRawSegmentDescData(pM, pM->reg.descc_ldt.base, pM->reg.descc_ldt.limit, selector, &RS);
	}else{
		loadRawSegmentDescData(pM, pM->reg.gdtr_base, pM->reg.gdtr_limit, selector, &RS);
	}

	pDD->base   = RS.base;
	pDD->limit  = RS.limit;
	pDD->access = RS.access;
	pDD->flags  = RS.flags;

	pDD->gran     =  (pDD->flags  & SEGFLAGS_DSEG_GRAN     ) ? 1 : 0;
	pDD->big      =  (pDD->flags  & SEGFLAGS_DSEG_B_BIT    ) ? 1 : 0;
	pDD->writable =  (pDD->access & SEGACCESS_DSEG_WRITABLE) ? 1 : 0;
	pDD->DPL      = ((pDD->access>> SEGACCESS_BIT_DPL) & 3 );

	if( pDD->gran ){
		pDD->limit_min = (pDD->access & SEGACCESS_DSEG_EXDOWN) ? ((pDD->limit+1)<<12) : 0;
		pDD->limit_max = (pDD->access & SEGACCESS_DSEG_EXDOWN) ? 0xffffffff           : ((pDD->limit << 12) + 0xfff);
	}else{
		pDD->limit_min = (pDD->access & SEGACCESS_DSEG_EXDOWN) ? (pDD->limit +1) : 0;
		pDD->limit_max = (pDD->access & SEGACCESS_DSEG_EXDOWN) ? 0xffffffff      : pDD->limit;
	}
}

void loadCodeSegmentDesc(struct stMachineState *pM, uint16_t selector, struct stCodeDesc *pCD){
	struct stRawSegmentDesc RS;
	uint8_t RPL = (selector & 3);

	if( selector & 0x4 ){
		loadRawSegmentDescData(pM, pM->reg.descc_ldt.base, pM->reg.descc_ldt.limit, selector, &RS);
	}else{
		loadRawSegmentDescData(pM, pM->reg.gdtr_base, pM->reg.gdtr_limit, selector, &RS);
	}

	pCD->base   = RS.base;
	pCD->limit  = RS.limit;
	pCD->access = RS.access;
	pCD->flags  = RS.flags;

	pCD->gran       =  (pCD->flags  & SEGFLAGS_CSEG_GRAN       ) ? 1 : 0;
	pCD->def        =  (pCD->flags  & SEGFLAGS_CSEG_D_BIT      ) ? 1 : 0;
	pCD->readable   =  (pCD->access & SEGACCESS_CSEG_READABLE  ) ? 1 : 0;
	pCD->conforming =  (pCD->access & SEGACCESS_CSEG_CONFORMING) ? 1 : 0;
	pCD->DPL        = ((pCD->access>> SEGACCESS_BIT_DPL) & 3 );


	if( ! (pCD->access & (1<<7))  ){
		// Present bit
		logfile_printf(LOGCAT_CPU_MEM | LOGLV_WARNING, "%s: selector %x is not present. (CS:EIP=%x:%x pointer %x)\n", __func__, selector, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
		pM->reg.fault = (1<<FAULTNUM_UNKNOWN);
	}
	if( ! (pCD->access & SEGACCESS_CODE_DATA_SEG)  ){
		// Descriptor type bit (0: system descriptor, 1: code/data segment)
		logfile_printf(LOGCAT_CPU_MEM | LOGLV_WARNING, "%s: selector %x is not code segment. (CS:EIP=%x:%x pointer %x)\n", __func__, selector, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
		pM->reg.fault = (1<<FAULTNUM_UNKNOWN);
	}
	if( ! (pCD->access & SEGACCESS_CSEG)  ){
		// Executable bit (0: data, 1: code)
		logfile_printf(LOGCAT_CPU_MEM | LOGLV_WARNING, "%s: selector %x is not executable. (CS:EIP=%x:%x pointer %x)\n", __func__, selector, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
		pM->reg.fault = (1<<FAULTNUM_UNKNOWN);
	}
	if( ! pCD->readable  ){
		// Readable bit (0: unreadable, 1: readable)
		logfile_printf(LOGCAT_CPU_MEM | LOGLV_WARNING, "%s: selector %x is not readable. (CS:EIP=%x:%x pointer %x)\n", __func__, selector, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
		pM->reg.fault = (1<<FAULTNUM_UNKNOWN);
	}

	pCD->limit_min = 0;
	pCD->limit_max = (pCD->gran) ? ((pCD->limit << 12) + 0xfff) : pCD->limit;

	if( DEBUG ){
		logfile_printf(LOGCAT_CPU_MEM | LOGLV_WARNING, "%s: selector %x is %d bit (CS:EIP=%x:%x pointer %x)\n", __func__, selector, (pCD->def) ? 32 : 16, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
	}

	if( MODE_PROTECTED && ! MODE_PROTECTEDVM ){
		if( pM->reg.cpl != RPL ){
//			logfile_printf(LOGLEVEL_INFO, "%s: CPL %x -> %x (CS:EIP=%x:%x pointer %x)\n", __func__, pM->reg.cpl, RPL, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
			pM->reg.cpl = RPL;
		}
	}
}

void loadTaskRegister    (struct stMachineState *pM, uint16_t selector, struct stRawSegmentDesc *pRS){
	uint16_t selector_body = (selector & (~7));
	uint32_t base, limit, pointer;

	if( selector & 0x4 ){
		base    = pM->reg.descc_ldt.base;
		limit   = pM->reg.descc_ldt.limit;
	}else{
		base    = pM->reg.gdtr_base;
		limit   = pM->reg.gdtr_limit;
	}

	if( selector      <      4 ) ENTER_GP(0);
	if( selector_body >= limit ) ENTER_GP(selector);

	pointer = base + selector_body;
	uint8_t access = readDataMemByteAsSV(pM, pointer+5);

	if( ! (SEGFLAGS_PRESENT & access) ) ENTER_GP(selector);

	writeDataMemByteAsSV(pM, pointer+5, (access & 0xf0) | SYSDESC_TYPE_TSS32_BUSY );

	if( selector & 0x4 ){
		loadRawSegmentDescData(pM, pM->reg.descc_ldt.base, pM->reg.descc_ldt.limit, selector, pRS);
	}else{
		loadRawSegmentDescData(pM, pM->reg.gdtr_base     , pM->reg.gdtr_limit     , selector, pRS);
	}
}



static void writeBackDWtoMem(struct stMachineState *pM, uint32_t addr, uint32_t data){
	writeDataMemByteAsSV(pM, addr+0, (data>> 0) & 0xff);
	writeDataMemByteAsSV(pM, addr+1, (data>> 8) & 0xff);
	writeDataMemByteAsSV(pM, addr+2, (data>>16) & 0xff);
	writeDataMemByteAsSV(pM, addr+3, (data>>24) & 0xff);
}

static uint32_t readDWfromMem(struct stMachineState *pM, uint32_t addr){
	uint32_t result;
	result =  readDataMemByteAsSV(pM, addr+0);
	result |= (readDataMemByteAsSV(pM, addr+1) << 8);
	result |= (readDataMemByteAsSV(pM, addr+2) <<16);
	result |= (readDataMemByteAsSV(pM, addr+3) <<24);
	return result;
}

#define TSS_LOC_PREVTSS 0x00
#define TSS_LOC_CR3		0x1c
#define TSS_LOC_EIP		0x20
#define TSS_LOC_EFLAGS	0x24
#define TSS_LOC_EAX		0x28
#define TSS_LOC_ECX		0x2c
#define TSS_LOC_EDX		0x30
#define TSS_LOC_EBX		0x34
#define TSS_LOC_ESP		0x38
#define TSS_LOC_EBP		0x3c
#define TSS_LOC_ESI		0x40
#define TSS_LOC_EDI		0x44
#define TSS_LOC_ES		0x48
#define TSS_LOC_CS		0x4c
#define TSS_LOC_SS		0x50
#define TSS_LOC_DS		0x54
#define TSS_LOC_FS		0x58
#define TSS_LOC_GS		0x5c
#define TSS_LOC_LDT		0x60
#define TSS_LOC_IOMBASE	0x66


/* Clear BUSY flag of current TSS descripter (specified by TR) and save state into TSS */
void unloadTaskRegister  (struct stMachineState *pM, uint32_t nextEIP){
	uint32_t base, limit;

	if( pM->reg.tr & 0x4 ){
		base    = pM->reg.descc_ldt.base;
		limit   = pM->reg.descc_ldt.limit;
	}else{
		base    = pM->reg.gdtr_base;
		limit   = pM->reg.gdtr_limit;
	}

	uint32_t pointer = base + (pM->reg.tr & (~7));
	writeDataMemByteAsSV(pM, pointer+5, (pM->reg.descc_tr.access & 0xf0) | SYSDESC_TYPE_TSS32_AVAIL );

	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_EAX, pM->reg.eax);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_ECX, pM->reg.ecx);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_EDX, pM->reg.edx);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_EBX, pM->reg.ebx);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_ESP, pM->reg.esp);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_EBP, pM->reg.ebp);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_ESI, pM->reg.esi);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_EDI, pM->reg.edi);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_ES,  pM->reg.es);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_CS,  pM->reg.cs);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_SS,  pM->reg.ss);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_DS,  pM->reg.ds);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_FS,  pM->reg.fs);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_GS,  pM->reg.gs);
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_EFLAGS, pM->reg.eflags);

	// It points the instruction after the one that caused the task switch.
	writeBackDWtoMem(pM, pM->reg.descc_tr.base + TSS_LOC_EIP, nextEIP);
}

void loadTaskState(struct stMachineState *pM){

	// TLB should be flushed before loading a new value for CR3
	flushTLB(pM);

	pM->reg.eflags = readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_EFLAGS);
	pM->reg.cr[3]  = readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_CR3);

	pM->reg.ldtr = readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_LDT);
	loadRawSegmentDesc(pM, pM->reg.ldtr, &(pM->reg.descc_ldt));

	pM->reg.eax = readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_EAX);
	pM->reg.ecx = readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_ECX);
	pM->reg.edx = readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_EDX);
	pM->reg.ebx = readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_EBX);
	pM->reg.esp = readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_ESP);
	pM->reg.ebp = readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_EBP);
	pM->reg.esi = readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_ESI);
	pM->reg.edi = readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_EDI);
	updateSegReg(pM, SEGREG_NUM_ES, readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_ES));
	updateSegReg(pM, SEGREG_NUM_CS, readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_CS));
	updateSegReg(pM, SEGREG_NUM_SS, readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_SS));
	updateSegReg(pM, SEGREG_NUM_DS, readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_DS));
	updateSegReg(pM, SEGREG_NUM_FS, readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_FS));
	updateSegReg(pM, SEGREG_NUM_GS, readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_GS));

	pM->reg.eip = readDWfromMem(pM, pM->reg.descc_tr.base + TSS_LOC_EIP);
}


// -----------------------------------------------------------------------------------------
//   Code for Gate Descriptors
//     
//      - Call gates
//      - Trap gates
//      - Interrupt gates
//      - Task gates
// -----------------------------------------------------------------------------------------


static void loadGateDescData(struct stMachineState *pM, uint32_t base, uint32_t limit, uint16_t selector, struct stGateDesc *pGD){
	if( selector & 7 ){
		logfile_printf(LOGCAT_CPU_MEM | LOGLV_INFO3, "%s: selector = %x CS:EIP = %x:%x (%x)\n", __func__, selector, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
	}

	selector &= (~7);

	if( selector >= limit ){
		pM->reg.fault = (1<<FAULTNUM_UNKNOWN);
		return ;
	}

	uint32_t pointer = base + selector;

    pGD->selector  = readDataMemByteAsSV(pM, pointer+2);
    pGD->selector |= ((uint16_t) readDataMemByteAsSV(pM, pointer+3))<< 8;

    pGD->offset  = readDataMemByteAsSV(pM, pointer+0);
    pGD->offset |= ((uint32_t)readDataMemByteAsSV(pM, pointer+1))<< 8;
    pGD->offset |= ((uint32_t)readDataMemByteAsSV(pM, pointer+6))<<16;
    pGD->offset |= ((uint32_t)readDataMemByteAsSV(pM, pointer+7))<<24;

    pGD->access = readDataMemByteAsSV(pM, pointer+5);
    pGD->len    = readDataMemByteAsSV(pM, pointer+4);

	uint8_t type = (pGD->access & 0xf);

	logfile_printf(LOGCAT_CPU_MEM | LOGLV_INFO3, "%s: selector = %x => selector=%x, offset=%x, access=%x, len=%x (CS:EIP = %x:%x (%x))\n", 
	__func__, selector, pGD->selector, pGD->offset, pGD->access, pGD->len, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);

}

void loadGateDesc(struct stMachineState *pM, uint16_t selector, struct stGateDesc *pGD){
	if( selector & 0x4 ){
		loadGateDescData(pM, pM->reg.descc_ldt.base, pM->reg.descc_ldt.limit, selector, pGD);
	}else{
		loadGateDescData(pM, pM->reg.gdtr_base, pM->reg.gdtr_limit, selector, pGD);
	}
}


void loadIntDesc(struct stMachineState *pM, uint8_t int_num, struct stGateDesc *pGD){

	if( (((uint32_t)int_num)<<3) >= pM->reg.idtr_limit ){
		pM->reg.fault = (1<<FAULTNUM_UNKNOWN);
		return ;
	}

	uint32_t pointer = pM->reg.idtr_base + (((uint32_t)int_num)<<3);

    pGD->selector  = readDataMemByteAsSV(pM, pointer+2);
    pGD->selector |= ((uint32_t) readDataMemByteAsSV(pM, pointer+3)) << 8;

    pGD->offset  = readDataMemByteAsSV(pM, pointer+0);
    pGD->offset |= ((uint32_t)readDataMemByteAsSV(pM, pointer+1))<< 8;
    pGD->offset |= ((uint32_t)readDataMemByteAsSV(pM, pointer+6))<<16;
    pGD->offset |= ((uint32_t)readDataMemByteAsSV(pM, pointer+7))<<24;

    pGD->access = readDataMemByteAsSV(pM, pointer+5);
}


uint8_t getDescType(struct stMachineState *pM, uint16_t selector){
	uint16_t selector_body = (selector & (~7));
	uint32_t base, limit, pointer;

	if( selector < 4 ){
		// invalid segments but loading them is valid.
		return 0;
	}

	if( selector & 0x4 ){
		base    = pM->reg.descc_ldt.base;
		limit   = pM->reg.descc_ldt.limit;
	}else{
		base    = pM->reg.gdtr_base;
		limit   = pM->reg.gdtr_limit;
	}

	if( selector_body >= limit ) ENTER_GP(selector);

	pointer = base + selector_body;

	return readDataMemByteAsSV(pM, pointer+5);
}