#include <stdio.h>
#include <stdint.h>

#include "i8086.h"
#include "ALUop.h"
#include "decode.h"
#include "mem.h"
#include "descriptor.h"

#include "ExInst_common.h"

#include "logfile.h"

uint32_t decode_mod_rm(struct stMachineState *pM, uint32_t pointer, uint8_t width, struct stOpl *pOp1){
	unsigned char mod, rm;
	uint32_t size = 1;

	uint8_t code    = fetchCodeDataByte(pM, pointer);

	mod = ((code>>6) & 0x3);
	rm  = ((code>>0) & 0x7);

	pOp1->width = width;

	if( PREFIX_AD32 ){
		uint32_t disp = 0;

		if( mod == 3 ){
			pOp1->type = OpTypeReg; pOp1->reg = rm;
		}else{
			uint16_t seg_ss = (PREFIX_SEG != PREF_SEG_UNSPECIFIED) ? PREFIX_SEG : SEGREG_NUM_SS;
			uint16_t seg_ds = (PREFIX_SEG != PREF_SEG_UNSPECIFIED) ? PREFIX_SEG : SEGREG_NUM_DS;

			pOp1->type = OpTypeMemWithSeg;
			if( mod == 0 && rm == 5 ){
				size += 4;
				pOp1->addr  = ((uint32_t)fetchCodeDataByte(pM, pointer+1));
				pOp1->addr |= ((uint32_t)fetchCodeDataByte(pM, pointer+2))<< 8;
				pOp1->addr |= ((uint32_t)fetchCodeDataByte(pM, pointer+3))<<16;
				pOp1->addr |= ((uint32_t)fetchCodeDataByte(pM, pointer+4))<<24;
				pOp1->reg  = seg_ds;
			}else if( rm == 4 ){ // s-i-b byte
				size += 1;
				uint8_t sib = fetchCodeDataByte(pM, pointer+1);
				uint8_t ss  = ((sib>>6)&0x3);
				uint8_t idx = ((sib>>3)&0x7);
				uint8_t base= ( sib    &0x7);

				if( (mod==0 && base == 5) || (mod==2) ){
					decode_imm32(pM, pointer+size, &disp);
					size += 4;
				}else if( mod == 1){
					disp = ((uint32_t)fetchCodeDataByte(pM, pointer+size));
					if(disp&0x80) disp |=  0xffffff00;
					size += 1;
				}

				switch(idx){
					case 0: disp += (REG_EAX <<ss); break;
					case 1: disp += (REG_ECX <<ss); break;
					case 2: disp += (REG_EDX <<ss); break;
					case 3: disp += (REG_EBX <<ss); break;
					case 4: disp +=              0; break;
					case 5: disp += (REG_EBP <<ss); break;
					case 6: disp += (REG_ESI <<ss); break;
					case 7: disp += (REG_EDI <<ss); break;
				}

				switch(base){
					case 0: pOp1->addr = REG_EAX + disp; pOp1->reg = seg_ds; break;
					case 1: pOp1->addr = REG_ECX + disp; pOp1->reg = seg_ds; break;
					case 2: pOp1->addr = REG_EDX + disp; pOp1->reg = seg_ds; break;
					case 3: pOp1->addr = REG_EBX + disp; pOp1->reg = seg_ds; break;
					case 4: pOp1->addr = REG_ESP + disp; pOp1->reg = seg_ss; break;
					case 5: pOp1->addr =((mod==0)?0:REG_EBP)+disp; pOp1->reg = ((mod==0)?seg_ds :seg_ss); break;
					case 6: pOp1->addr = REG_ESI + disp; pOp1->reg = seg_ds; break;
					case 7: pOp1->addr = REG_EDI + disp; pOp1->reg = seg_ds; break;
				}

			}else{
				if( mod == 0 ){
					size+=0;
					disp = 0;
				}else if( mod == 1 ){
					size+=1;
					disp = ((uint32_t)fetchCodeDataByte(pM, pointer+1));
					if(disp&0x80) disp |= 0xffffff00;
				}else{ 
					size+=4;
					disp  = ((uint32_t)fetchCodeDataByte(pM, pointer+1));
					disp |=(((uint32_t)fetchCodeDataByte(pM, pointer+2))<< 8);
					disp |=(((uint32_t)fetchCodeDataByte(pM, pointer+3))<<16);
					disp |=(((uint32_t)fetchCodeDataByte(pM, pointer+4))<<24);
				}

				switch(rm){
					case 0: pOp1->addr = REG_EAX + disp; pOp1->reg = seg_ds; break;
					case 1: pOp1->addr = REG_ECX + disp; pOp1->reg = seg_ds; break;
					case 2: pOp1->addr = REG_EDX + disp; pOp1->reg = seg_ds; break;
					case 3: pOp1->addr = REG_EBX + disp; pOp1->reg = seg_ds; break;
					case 5: pOp1->addr = REG_EBP + disp; pOp1->reg = seg_ss; break;
					case 6: pOp1->addr = REG_ESI + disp; pOp1->reg = seg_ds; break;
					case 7: pOp1->addr = REG_EDI + disp; pOp1->reg = seg_ds; break;
				}
			}
		}
	}else{
		uint16_t disp = 0;
		if( mod == 3 ){
			pOp1->type = OpTypeReg; pOp1->reg = rm;
		}else{
			uint16_t seg_ss = (PREFIX_SEG != PREF_SEG_UNSPECIFIED) ? PREFIX_SEG : 2;
			uint16_t seg_ds = (PREFIX_SEG != PREF_SEG_UNSPECIFIED) ? PREFIX_SEG : 3;

			uint16_t disp_lo = fetchCodeDataByte(pM, pointer+1);
			uint16_t disp_hi = fetchCodeDataByte(pM, pointer+2);

			pOp1->type = OpTypeMemWithSeg;
			if( mod == 0 && rm == 6 ){
				size += 2;
				pOp1->addr = ((((uint16_t)disp_hi)<<8) | disp_lo);
				pOp1->reg  = seg_ds;
			}else{
				if( mod == 0 ){ size+=0; disp = 0; }
				if( mod == 1 ){ size+=1; disp = (disp_lo & 0x80) ? (0xff00 | disp_lo) : disp_lo; }
				if( mod == 2 ){ size+=2; disp = ((((uint16_t)disp_hi) << 8) | disp_lo); }

				switch(rm){
					case 0: pOp1->addr = REG_BX + REG_SI + disp; pOp1->reg = seg_ds; break;
					case 1: pOp1->addr = REG_BX + REG_DI + disp; pOp1->reg = seg_ds; break;
					case 2: pOp1->addr = REG_BP + REG_SI + disp; pOp1->reg = seg_ss; break;
					case 3: pOp1->addr = REG_BP + REG_DI + disp; pOp1->reg = seg_ss; break;
					case 4: pOp1->addr =          REG_SI + disp; pOp1->reg = seg_ds; break;
					case 5: pOp1->addr =          REG_DI + disp; pOp1->reg = seg_ds; break;
					case 6: pOp1->addr =          REG_BP + disp; pOp1->reg = seg_ss; break;
					case 7: pOp1->addr =          REG_BX + disp; pOp1->reg = seg_ds; break;
				}
				// because disp could be negative number, carry should be eliminated
				pOp1->addr &= 0xffff;
			}
		}
	}

	return size;
}


void decode_reg1(struct stMachineState *pM, uint32_t pointer, uint8_t width, struct stOpl *pOp1){
	uint8_t code = fetchCodeDataByte(pM, pointer);
	uint8_t reg;

	reg = ((code>>0) & 0x7);
	pOp1->type  = OpTypeReg;
	pOp1->reg   = reg;
	pOp1->width = width;
}

void decode_reg2(struct stMachineState *pM, uint32_t pointer, uint8_t width, struct stOpl *pOp2){
	uint8_t code = fetchCodeDataByte(pM, pointer);
	uint8_t reg;

	reg = ((code>>3) & 0x7);
	pOp2->type  = OpTypeReg;
	pOp2->reg   = reg;
	pOp2->width = width;
}

void decode_segReg(struct stMachineState *pM, uint32_t pointer, struct stOpl *pOp){
	uint8_t code = fetchCodeDataByte(pM, pointer);
	unsigned char reg;

	reg = ((code>>3) & 0x3);
	pOp->type  = OpTypeSegReg;
	pOp->reg   = reg;
	pOp->width = 1;
}

int decode_segReg3bit(struct stMachineState *pM, uint32_t pointer, struct stOpl *pOp){
	uint8_t code = fetchCodeDataByte(pM, pointer);
	unsigned char reg;

	reg = ((code>>3) & 0x7);
	if( pM->emu.emu_cpu == EMU_CPU_8086 || pM->emu.emu_cpu == EMU_CPU_80186 || pM->emu.emu_cpu == EMU_CPU_80286 ){
		reg &= 0x3;
	}else{
		if( reg >= SEGREG_NUMS32 ){
			return 1;
		}
	}
	pOp->type  = OpTypeSegReg;
	pOp->reg   = reg;
	pOp->width = 1;

	return 0;
}

uint32_t decode_imm16(struct stMachineState *pM, uint32_t pointer, uint32_t *val){
    *val  = fetchCodeDataByte(pM, pointer+0);
    *val |= ((uint16_t)fetchCodeDataByte(pM, pointer+1))<<8;
    return 2;
}
uint32_t decode_imm32(struct stMachineState *pM, uint32_t pointer, uint32_t *val){
    *val  = fetchCodeDataByte(pM, pointer+0);
    *val |= ((uint32_t)fetchCodeDataByte(pM, pointer+1))<<8;
    *val |= ((uint32_t)fetchCodeDataByte(pM, pointer+2))<<16;
    *val |= ((uint32_t)fetchCodeDataByte(pM, pointer+3))<<24;
    return 4;
}

uint32_t decode_imm(struct stMachineState *pM, uint32_t pointer, uint8_t width, uint32_t *val, uint8_t signEx){
	*val = fetchCodeDataByte(pM, pointer);

	if(width == 0){
		;
	}else{
		if( signEx != INST_S_SIGNEX ){
			*val |= ((uint32_t)fetchCodeDataByte(pM, pointer+1))<< 8;

			if( PREFIX_OP32 ){
				*val |= ((uint32_t)fetchCodeDataByte(pM, pointer+2))<<16;
				*val |= ((uint32_t)fetchCodeDataByte(pM, pointer+3))<<24;
				return 4;
			}else{
				return 2;
			}
		}else{
			if( PREFIX_OP32 ){
				if(*val & 0x80) *val |= 0xffffff00;
			}else{
				if(*val & 0x80) *val |= 0xff00;
			}
		}
	}
	return 1;
}

uint32_t decode_immAddr(struct stMachineState *pM, uint32_t pointer, uint8_t width, struct stOpl *pOp1){
	uint8_t data_lo = fetchCodeDataByte(pM, pointer);
	uint8_t data_hi = fetchCodeDataByte(pM, pointer+1);

	pOp1->type = OpTypeMemWithSeg;
	pOp1->reg  = SEGREG_NUM_DS;
	pOp1->width= width;
	if( PREFIX_SEG != PREF_SEG_UNSPECIFIED ) pOp1->reg = PREFIX_SEG;

	pOp1->addr = ((((uint16_t)data_hi) << 8) | data_lo);

	if( PREFIX_AD32 ){
		pOp1->addr |= ((uint32_t)fetchCodeDataByte(pM, pointer+2)) << 16;
		pOp1->addr |= ((uint32_t)fetchCodeDataByte(pM, pointer+3)) << 24;
		return 4;
	}

	return 2;
}

uint32_t readOplEA(struct stMachineState *pM, struct stOpl *pOp, uint8_t withSegBase){
	uint32_t op1 = 0;
	uint32_t addr = 0;

	if(pOp->type == OpTypeReg){
		switch(pOp->reg){
			case 0:  op1 = (pOp->width ? REG_EAX : (REG_AX&0xff)); break;
			case 1:  op1 = (pOp->width ? REG_ECX : (REG_CX&0xff)); break;
			case 2:  op1 = (pOp->width ? REG_EDX : (REG_DX&0xff)); break;
			case 3:  op1 = (pOp->width ? REG_EBX : (REG_BX&0xff)); break;
			case 4:  op1 = (pOp->width ? REG_ESP : ((REG_AX>>8)&0xff)); break;
			case 5:  op1 = (pOp->width ? REG_EBP : ((REG_CX>>8)&0xff)); break;
			case 6:  op1 = (pOp->width ? REG_ESI : ((REG_DX>>8)&0xff)); break;
			case 7:  op1 = (pOp->width ? REG_EDI : ((REG_BX>>8)&0xff)); break;
			default: printf("Invalid reg number \n");
		}
		if( PREFIX_OP32 ){
			addr = op1;
		}else{
			addr =(op1 & 0xffff);
		}
	}else if(pOp->type == OpTypeSegReg){
		switch(pOp->reg){
			case 0:  op1 = REG_ES; break;
			case 1:  op1 = REG_CS; break;
			case 2:  op1 = REG_SS; break;
			case 3:  op1 = REG_DS; break;
			case 4:  op1 = REG_FS; break;
			case 5:  op1 = REG_GS; break;
			default: printf("Invalid reg number \n");
		}
		addr = op1;
	}else if(pOp->type == OpTypeMemWithSeg){
		if( withSegBase ){
			switch(pOp->reg){
				case 0:  addr = REG_ES_BASE; break;
				case 1:  addr = REG_CS_BASE; break;
				case 2:  addr = REG_SS_BASE; break;
				case 3:  addr = REG_DS_BASE; break;
				case 4:  addr = REG_FS_BASE; break;
				case 5:  addr = REG_GS_BASE; break;
				default: printf("Invalid reg number \n");
			}
		}
		addr += pOp->addr;
	}else if(pOp->type == OpTypeMemDirect){
        addr = pOp->addr;
	}
	return addr;
}



#define LOAD_DATA_DESCRIPTOR_IN_REAL(seg, val)                              \
	do{                                                                     \
		pM->reg.descc_##seg.base = ((val)<<4);                              \
		pM->reg.descc_##seg.limit = pM->reg.descc_##seg.limit_max = 0xffff; \
		pM->reg.descc_##seg.flags = pM->reg.descc_##seg.limit_min = 0;      \
		pM->reg.descc_##seg.access   = 0;                                   \
		pM->reg.descc_##seg.gran     = 0;                                   \
		pM->reg.descc_##seg.big      = 0;                                   \
		pM->reg.descc_##seg.writable = 1;                                   \
		pM->reg.descc_##seg.DPL      = 0;                                   \
	}while(0)

#define LOAD_CODE_DESCRIPTOR_IN_REAL(seg, val)                              \
	do{                                                                     \
		pM->reg.descc_##seg.base = ((val)<<4);                              \
		pM->reg.descc_##seg.limit = pM->reg.descc_##seg.limit_max = 0xffff; \
		pM->reg.descc_##seg.flags = pM->reg.descc_##seg.limit_min = 0;      \
		pM->reg.descc_##seg.access     = 0;                                 \
		pM->reg.descc_##seg.gran       = 0;                                 \
		pM->reg.descc_##seg.def        = 0;                                 \
		pM->reg.descc_##seg.readable   = 1;                                 \
		pM->reg.descc_##seg.conforming = 0;                                 \
		pM->reg.descc_##seg.DPL        = 0;                                 \
	}while(0)

#define SEG_ERR_MSG(seg, oft, min, max) \
	logfile_printf(LOGCAT_CPU_MEM | LOGLV_ERROR, "%s: access violation in reading/writing data in segment %s offset %x : min %x max %x (CS:EIP=%x:%x pointer %x)\n", __func__, (seg), (oft), (min), (max), REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);

#define CHECK_WRITE_CONDITION_FOR_DATA(seg, offset)                                                     \
	do{                                                                                                 \
		if( (offset) < pM->reg.descc_##seg.limit_min || (offset) > pM->reg.descc_##seg.limit_max){      \
			SEG_ERR_MSG(#seg, pOp->addr, pM->reg.descc_##seg.limit_min, pM->reg.descc_##seg.limit_max); \
			ENTER_GP(0);                                                                                \
		}                                                                                               \
	}while(0)

#define CHECK_WRITE_CONDITION_FOR_CODE(seg, offset)                                                     \
	do{                                                                                                 \
		if( (offset) < pM->reg.descc_##seg.limit_min || (offset) > pM->reg.descc_##seg.limit_max){      \
			SEG_ERR_MSG(#seg, pOp->addr, pM->reg.descc_##seg.limit_min, pM->reg.descc_##seg.limit_max); \
			ENTER_GP(0);                                                                                \
		}                                                                                               \
	}while(0)

#define CHECK_READ_CONDITION_FOR_DATA(seg, offset)                                                     \
	do{                                                                                                 \
		if( (offset) < pM->reg.descc_##seg.limit_min || (offset) > pM->reg.descc_##seg.limit_max){      \
			SEG_ERR_MSG(#seg, pOp->addr, pM->reg.descc_##seg.limit_min, pM->reg.descc_##seg.limit_max); \
			ENTER_GP(0);                                                                                \
		}                                                                                               \
	}while(0)

#define CHECK_READ_CONDITION_FOR_CODE(seg, offset)                                                     \
	do{                                                                                                 \
		if( (offset) < pM->reg.descc_##seg.limit_min || (offset) > pM->reg.descc_##seg.limit_max){      \
			SEG_ERR_MSG(#seg, pOp->addr, pM->reg.descc_##seg.limit_min, pM->reg.descc_##seg.limit_max); \
			ENTER_GP(0);                                                                                \
		}                                                                                               \
	}while(0)


uint32_t readOpl(struct stMachineState *pM, struct stOpl *pOp){
	uint32_t op1 = 0;

	if(pOp->type == OpTypeReg){
		switch(pOp->reg){
			case 0:  op1 = (pOp->width ? REG_EAX : (REG_AX&0xff)); break;
			case 1:  op1 = (pOp->width ? REG_ECX : (REG_CX&0xff)); break;
			case 2:  op1 = (pOp->width ? REG_EDX : (REG_DX&0xff)); break;
			case 3:  op1 = (pOp->width ? REG_EBX : (REG_BX&0xff)); break;
			case 4:  op1 = (pOp->width ? REG_ESP : ((REG_AX>>8)&0xff)); break;
			case 5:  op1 = (pOp->width ? REG_EBP : ((REG_CX>>8)&0xff)); break;
			case 6:  op1 = (pOp->width ? REG_ESI : ((REG_DX>>8)&0xff)); break;
			case 7:  op1 = (pOp->width ? REG_EDI : ((REG_BX>>8)&0xff)); break;
			default: printf("Invalid reg number \n");
		}
		if( ! PREFIX_OP32 ) op1 &= 0xffff;
	}else if(pOp->type == OpTypeSegReg){
		switch(pOp->reg){
			case 0:  op1 = REG_ES; break;
			case 1:  op1 = REG_CS; break;
			case 2:  op1 = REG_SS; break;
			case 3:  op1 = REG_DS; break;
			case 4:  op1 = REG_FS; break;
			case 5:  op1 = REG_GS; break;
			default: printf("Invalid reg number \n");
		}
	}else if(pOp->type == OpTypeMemWithSeg){
		uint32_t addr = 0;
		switch(pOp->reg){
			case 0:  addr = REG_ES_BASE; CHECK_READ_CONDITION_FOR_DATA(es, pOp->addr); break;
			case 1:  addr = REG_CS_BASE; CHECK_READ_CONDITION_FOR_CODE(cs, pOp->addr); break;
			case 2:  addr = REG_SS_BASE; CHECK_READ_CONDITION_FOR_DATA(ss, pOp->addr); break;
			case 3:  addr = REG_DS_BASE; CHECK_READ_CONDITION_FOR_DATA(ds, pOp->addr); break;
			case 4:  addr = REG_FS_BASE; CHECK_READ_CONDITION_FOR_DATA(fs, pOp->addr); break;
			case 5:  addr = REG_GS_BASE; CHECK_READ_CONDITION_FOR_DATA(gs, pOp->addr); break;
			default: printf("Invalid reg number \n");
		}
		addr += pOp->addr;
		if(pOp->width){
			if( pM->prefix.data32 ){
				op1 = readDataMemDoubleWord(pM, addr);
			}else{
				op1 = readDataMemWord(pM, addr);
			}
		}else{
			op1 = readDataMemByte(pM, addr);
		}
	}else if(pOp->type == OpTypeMemWithSeg_Reg){
		uint32_t addr   = 0;
		uint32_t offset = 0;
		switch(pOp->addr){
			case 0:  offset = PREFIX_AD32 ? REG_EAX : REG_AX; break;
			case 1:  offset = PREFIX_AD32 ? REG_ECX : REG_CX; break;
			case 2:  offset = PREFIX_AD32 ? REG_EDX : REG_DX; break;
			case 3:  offset = PREFIX_AD32 ? REG_EBX : REG_BX; break;
			case 4:  offset = PREFIX_AD32 ? REG_ESP : REG_SP; break;
			case 5:  offset = PREFIX_AD32 ? REG_EBP : REG_BP; break;
			case 6:  offset = PREFIX_AD32 ? REG_ESI : REG_SI; break;
			case 7:  offset = PREFIX_AD32 ? REG_EDI : REG_DI; break;
			default: printf("Invalid reg number \n");
		}
		switch(pOp->reg){
			case 0:  addr = REG_ES_BASE+offset; CHECK_READ_CONDITION_FOR_DATA(es, offset); break;
			case 1:  addr = REG_CS_BASE+offset; CHECK_READ_CONDITION_FOR_CODE(cs, offset); break;
			case 2:  addr = REG_SS_BASE+offset; CHECK_READ_CONDITION_FOR_DATA(ss, offset); break;
			case 3:  addr = REG_DS_BASE+offset; CHECK_READ_CONDITION_FOR_DATA(ds, offset); break;
			case 4:  addr = REG_FS_BASE+offset; CHECK_READ_CONDITION_FOR_DATA(fs, offset); break;
			case 5:  addr = REG_GS_BASE+offset; CHECK_READ_CONDITION_FOR_DATA(gs, offset); break;
			default: printf("Invalid reg number \n");
		}
		if(pOp->width){
			if( PREFIX_OP32 ){
				op1 = readDataMemDoubleWord(pM, addr);
			}else{
				op1 = readDataMemWord(pM, addr);
			}
		}else{
			op1 = readDataMemByte(pM, addr);
		}
	}else if(pOp->type == OpTypeMemDirect){
		if(pOp->width){
			if( PREFIX_OP32 ){
				op1 = readDataMemDoubleWord(pM, pOp->addr);
			}else{
				op1 = readDataMemWord(pM, pOp->addr);
			}
		}else{
			op1 = readDataMemByte(pM, pOp->addr);
		}
	}
	return op1;
}

void updateSegReg(struct stMachineState *pM, uint8_t segReg, uint32_t val){
	struct stOpl op;

	op.type = OpTypeSegReg;
	op.reg  = segReg;
	writeOpl(pM, &op, val);
}


void writeOpl(struct stMachineState *pM, struct stOpl *pOp, uint32_t val){

	if(pOp->type == OpTypeReg){
		if( PREFIX_OP32 ){
			switch(pOp->reg){
				case 0:  REG_EAX = (pOp->width ? val : ((REG_EAX & 0xffffff00) | (val&0xff))); break;
				case 1:  REG_ECX = (pOp->width ? val : ((REG_ECX & 0xffffff00) | (val&0xff))); break;
				case 2:  REG_EDX = (pOp->width ? val : ((REG_EDX & 0xffffff00) | (val&0xff))); break;
				case 3:  REG_EBX = (pOp->width ? val : ((REG_EBX & 0xffffff00) | (val&0xff))); break;

				case 4:  if(pOp->width){ REG_ESP = val; }else{ REG_EAX = ((REG_EAX & 0xffff00ff) | ((val&0xff)<<8)); }; break;
				case 5:  if(pOp->width){ REG_EBP = val; }else{ REG_ECX = ((REG_ECX & 0xffff00ff) | ((val&0xff)<<8)); }; break;
				case 6:  if(pOp->width){ REG_ESI = val; }else{ REG_EDX = ((REG_EDX & 0xffff00ff) | ((val&0xff)<<8)); }; break;
				case 7:  if(pOp->width){ REG_EDI = val; }else{ REG_EBX = ((REG_EBX & 0xffff00ff) | ((val&0xff)<<8)); }; break;
				default: printf("Invalid reg number \n");
			}
		}else{
			switch(pOp->reg){
				case 0:  REG_AX = (pOp->width ? val : ((REG_AX & 0xff00) | (val&0xff))); break;
				case 1:  REG_CX = (pOp->width ? val : ((REG_CX & 0xff00) | (val&0xff))); break;
				case 2:  REG_DX = (pOp->width ? val : ((REG_DX & 0xff00) | (val&0xff))); break;
				case 3:  REG_BX = (pOp->width ? val : ((REG_BX & 0xff00) | (val&0xff))); break;

				case 4:  if(pOp->width){ REG_SP = val; }else{ REG_AX = ((REG_AX & 0x00ff) | ((val&0xff)<<8)); }; break;
				case 5:  if(pOp->width){ REG_BP = val; }else{ REG_CX = ((REG_CX & 0x00ff) | ((val&0xff)<<8)); }; break;
				case 6:  if(pOp->width){ REG_SI = val; }else{ REG_DX = ((REG_DX & 0x00ff) | ((val&0xff)<<8)); }; break;
				case 7:  if(pOp->width){ REG_DI = val; }else{ REG_BX = ((REG_BX & 0x00ff) | ((val&0xff)<<8)); }; break;
				default: printf("Invalid reg number \n");
			}
		}
	}else if(pOp->type == OpTypeSegReg){
		val &= 0xffff;

		if( MODE_PROTECTED32 ){
			// protected mode (without VM86 mode)
			switch(pOp->reg){
				case 0: REG_ES = val; loadDataSegmentDesc (pM, val, &(pM->reg.descc_es)); break;
				case 1: REG_CS = val; loadCodeSegmentDesc (pM, val, &(pM->reg.descc_cs)); break;
				case 2: REG_SS = val; loadStackSegmentDesc(pM, val, &(pM->reg.descc_ss)); break;
				case 3: REG_DS = val; loadDataSegmentDesc (pM, val, &(pM->reg.descc_ds)); break;
				case 4: REG_FS = val; loadDataSegmentDesc (pM, val, &(pM->reg.descc_fs)); break;
				case 5: REG_GS = val; loadDataSegmentDesc (pM, val, &(pM->reg.descc_gs)); break;
				default: printf("Invalid reg number \n");
			}
		}else{
			// real mode or VM86 mode
			switch(pOp->reg){
				case 0:  REG_ES = val; LOAD_DATA_DESCRIPTOR_IN_REAL(es, val); break;
				case 1:  REG_CS = val; LOAD_CODE_DESCRIPTOR_IN_REAL(cs, val); break;
				case 2:  REG_SS = val; LOAD_DATA_DESCRIPTOR_IN_REAL(ss, val); break;
				case 3:  REG_DS = val; LOAD_DATA_DESCRIPTOR_IN_REAL(ds, val); break;
				case 4:  REG_FS = val; LOAD_DATA_DESCRIPTOR_IN_REAL(fs, val); break;
				case 5:  REG_GS = val; LOAD_DATA_DESCRIPTOR_IN_REAL(gs, val); break;
				default: printf("Invalid reg number \n");
			}
		}
	}else if(pOp->type == OpTypeMemWithSeg){
		uint32_t addr = 0;
		switch(pOp->reg){
			case 0:  addr = REG_ES_BASE; CHECK_WRITE_CONDITION_FOR_DATA(es, pOp->addr); break;
			case 1:  addr = REG_CS_BASE; CHECK_WRITE_CONDITION_FOR_CODE(cs, pOp->addr);/*ENTER_UD;*/ break;
			case 2:  addr = REG_SS_BASE; CHECK_WRITE_CONDITION_FOR_DATA(ss, pOp->addr); break;
			case 3:  addr = REG_DS_BASE; CHECK_WRITE_CONDITION_FOR_DATA(ds, pOp->addr); break;
			case 4:  addr = REG_FS_BASE; CHECK_WRITE_CONDITION_FOR_DATA(fs, pOp->addr); break;
			case 5:  addr = REG_GS_BASE; CHECK_WRITE_CONDITION_FOR_DATA(gs, pOp->addr); break;
			default: printf("Invalid reg number \n");
		}
		addr+= pOp->addr;

		if(pOp->width){
			if( PREFIX_OP32 ){
				writeDataMemDoubleWord(pM, addr, val);
			}else{
				writeDataMemWord(pM, addr, val);
			}
		}else{
			writeDataMemByte(pM, addr, val);
		}

	}else if(pOp->type == OpTypeMemWithSeg_Reg){
		uint32_t addr   = 0;
		uint32_t offset = 0;
		switch(pOp->addr){
			case 0:  offset = PREFIX_AD32 ? REG_EAX : REG_AX; break;
			case 1:  offset = PREFIX_AD32 ? REG_ECX : REG_CX; break;
			case 2:  offset = PREFIX_AD32 ? REG_EDX : REG_DX; break;
			case 3:  offset = PREFIX_AD32 ? REG_EBX : REG_BX; break;
			case 4:  offset = PREFIX_AD32 ? REG_ESP : REG_SP; break;
			case 5:  offset = PREFIX_AD32 ? REG_EBP : REG_BP; break;
			case 6:  offset = PREFIX_AD32 ? REG_ESI : REG_SI; break;
			case 7:  offset = PREFIX_AD32 ? REG_EDI : REG_DI; break;
			default: printf("Invalid reg number \n");
		}
		switch(pOp->reg){
			case 0:  addr = REG_ES_BASE+offset; CHECK_WRITE_CONDITION_FOR_DATA(es, offset); break;
			case 1:  addr = REG_CS_BASE+offset; CHECK_WRITE_CONDITION_FOR_CODE(cs, offset); break;
			case 2:  addr = REG_SS_BASE+offset; CHECK_WRITE_CONDITION_FOR_DATA(ss, offset); break;
			case 3:  addr = REG_DS_BASE+offset; CHECK_WRITE_CONDITION_FOR_DATA(ds, offset); break;
			case 4:  addr = REG_FS_BASE+offset; CHECK_WRITE_CONDITION_FOR_DATA(fs, offset); break;
			case 5:  addr = REG_GS_BASE+offset; CHECK_WRITE_CONDITION_FOR_DATA(gs, offset); break;
			default: printf("Invalid reg number \n");
		}
		if(pOp->width){
			if( PREFIX_OP32 ){
				writeDataMemDoubleWord(pM, addr, val);
			}else{
				writeDataMemWord(pM, addr, val);
			}
		}else{
			writeDataMemByte(pM, addr, val);
		}

	}else if(pOp->type == OpTypeMemDirect){
		if(pOp->width){
			if( PREFIX_OP32 ){
				writeDataMemDoubleWord(pM, pOp->addr, val);
			}else{
				writeDataMemWord(pM, pOp->addr, val);
			}
		}else{
			writeDataMemByte(pM, pOp->addr, val);
		}
	}
}


