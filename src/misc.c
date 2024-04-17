#include <stdio.h>
#include <stdint.h>

#include "i8086.h"
#include "logfile.h"
#include "ExInst_common.h"

static void log_printOpl16(int loglevel, struct stOpl *pOp);
static void log_printOpl32(int loglevel, struct stOpl *pOp);


void log_printOpl(int loglevel, struct stMachineState *pM, struct stOpl *pOp){
	if( pM->prefix.data32 ){
		log_printOpl32(loglevel, pOp);
	}else{
		log_printOpl16(loglevel, pOp);
	}
}


static void log_printOpl16(int loglevel, struct stOpl *pOp){
    char *str;
    if( pOp->type == OpTypeReg ){
		switch((pOp->reg)&7){
			case 0:  str = pOp->width ? "AX" : "AL"; break;
			case 1:  str = pOp->width ? "CX" : "CL"; break;
			case 2:  str = pOp->width ? "DX" : "DL"; break;
			case 3:  str = pOp->width ? "BX" : "BL"; break;
			case 4:  str = pOp->width ? "SP" : "AH"; break;
			case 5:  str = pOp->width ? "BP" : "CH"; break;
			case 6:  str = pOp->width ? "SI" : "DH"; break;
			case 7:  str = pOp->width ? "DI" : "BH"; break;
        }
        logfile_printf_without_header(loglevel, str);
    }else if( pOp->type == OpTypeSegReg ){
		switch((pOp->reg)&7){
			case 0:  str = "ES"; break;
			case 1:  str = "CS"; break;
			case 2:  str = "SS"; break;
			case 3:  str = "DS"; break;
			case 4:  str = "FS"; break;
			case 5:  str = "GS"; break;
			default: str = "--"; break;
		}
        logfile_printf_without_header(loglevel, str);
    }else if( pOp->type == OpTypeMemWithSeg ){
		switch((pOp->reg)&3){
			case 0:  str = "ES"; break;
			case 1:  str = "CS"; break;
			case 2:  str = "SS"; break;
			case 3:  str = "DS"; break;
			case 4:  str = "FS"; break;
			case 5:  str = "GS"; break;
			default: str = "--"; break;		}
        logfile_printf_without_header(loglevel, "%s:[%x]", str, pOp->addr);
    }
}

static void log_printOpl32(int loglevel, struct stOpl *pOp){
    char *str;
    if( pOp->type == OpTypeReg ){
		switch((pOp->reg)&7){
			case 0:  str = pOp->width ? "EAX" : "AL"; break;
			case 1:  str = pOp->width ? "ECX" : "CL"; break;
			case 2:  str = pOp->width ? "EDX" : "DL"; break;
			case 3:  str = pOp->width ? "EBX" : "BL"; break;
			case 4:  str = pOp->width ? "ESP" : "AH"; break;
			case 5:  str = pOp->width ? "EBP" : "CH"; break;
			case 6:  str = pOp->width ? "ESI" : "DH"; break;
			case 7:  str = pOp->width ? "EDI" : "BH"; break;
        }
        logfile_printf_without_header(loglevel, str);
    }else if( pOp->type == OpTypeSegReg ){
		switch((pOp->reg)&7){
			case 0:  str = "ES"; break;
			case 1:  str = "CS"; break;
			case 2:  str = "SS"; break;
			case 3:  str = "DS"; break;
			case 4:  str = "FS"; break;
			case 5:  str = "GS"; break;
			default: str = "--"; break;
		}
        logfile_printf_without_header(loglevel, str);
    }else if( pOp->type == OpTypeMemWithSeg ){
		switch((pOp->reg)&7){
			case 0:  str = "ES"; break;
			case 1:  str = "CS"; break;
			case 2:  str = "SS"; break;
			case 3:  str = "DS"; break;
			case 4:  str = "FS"; break;
			case 5:  str = "GS"; break;
			default: str = "--"; break;
		}
        logfile_printf_without_header(loglevel, "%s:[%x]", str, pOp->addr);
    }
}

void log_printReg16(int loglevel, struct stMachineState *pM){
	logfile_printf(loglevel, "CS:IP = %04x:%04x\n", REG_CS, REG_IP);
	logfile_printf(loglevel, "AX=%04x BX=%04x CX=%04x DX=%04x \n", REG_AX, REG_BX, REG_CX, REG_DX);
	logfile_printf(loglevel, "SP=%04x BP=%04x SI=%04x DI=%04x \n", REG_SP, REG_BP, REG_SI, REG_DI);
	logfile_printf(loglevel, "ES=%04x CS=%04x SS=%04x DS=%04x FLAGS=%04x \n", REG_ES, REG_CS, REG_SS, REG_DS, REG_FLAGS);
}

void log_printReg32(int loglevel, struct stMachineState *pM){
	logfile_printf(loglevel, "CS:EIP = %04x:%08x\n", REG_CS, REG_EIP);
	logfile_printf(loglevel, "EAX=%08x EBX=%08x ECX=%08x EDX=%08x \n", REG_EAX, REG_EBX, REG_ECX, REG_EDX);
	logfile_printf(loglevel, "ESP=%08x EBP=%08x ESI=%08x EDI=%08x \n", REG_ESP, REG_EBP, REG_ESI, REG_EDI);
	logfile_printf(loglevel, "ES=%04x (base:%08x) CS=%04x (base:%08x) SS=%04x%c(base:%08x)\n",
		REG_ES, REG_ES_BASE, REG_CS, REG_CS_BASE, REG_SS, (pM->reg.descc_ss.flags & (1<<2)) ? 'D' : 'W', REG_SS_BASE);
	logfile_printf(loglevel, "DS=%04x (base:%08x) FS=%04x (base:%08x) GS=%04x (base:%08x)\n", 
		REG_DS, REG_DS_BASE, REG_FS, REG_FS_BASE, REG_GS, REG_GS_BASE);
	logfile_printf(loglevel, "CR0=%08x CR2=%08x CR3=%08x EFLAGS=%08x CPL=%x D=%d\n",
		REG_CR0, REG_CR2, REG_CR3, REG_EFLAGS, pM->reg.cpl, (pM->reg.descc_cs.flags & (1<<2)) ? 32 : 16);
}

uint32_t parseHex(char *str){
	uint32_t addr = 0, nd;
	int count = 0;
	if( str[0] == '0' && (str[1] == 'x' || str[1] == 'X') ){
		str = str + 2;
	}

	for(count=0; count<8 && str[count] != '\0'; count++ ){
		nd = 0x10;
		if( str[count] >= '0' && str[count] <= '9' ) nd = str[count] - '0';
		if( str[count] >= 'a' && str[count] <= 'f' ) nd = str[count] - 'a' + 0xa;
		if( str[count] >= 'A' && str[count] <= 'F' ) nd = str[count] - 'A' + 0xa;

		if(nd >= 0x10) break;

		addr = (addr<<4) + nd;
	}
	return addr;
}

uint32_t parseDec(char *str){
	uint32_t res = 0, nd;
	int count = 0;

	if( str[0] == '0' && (str[1] == 'x' || str[1] == 'X') ){
        return parseHex(str);
	}

	for(count=0; str[count] != '\0'; count++ ){
		nd = 10;
		if( str[count] >= '0' && str[count] <= '9' ) nd = str[count] - '0';

		if(nd >= 0x10) break;

		res = res * 10 + nd;
	}
	return res;
}

