#include <stdio.h>
#include <stdint.h>
#include <signal.h>

#include "i8086.h"
#include "instmap32.h"

#include "misc.h"
#include "dev_UART.h"
#include "logfile.h"
#include "mem.h"



#define LOGLEVEL_EMU_INFO   (LOGCAT_EMU | LOGLV_INFO)
#define LOGLEVEL_EMU_INFO3  (LOGCAT_EMU | LOGLV_INFO3)
#define LOGLEVEL_EMU_NOTICE (LOGCAT_EMU | LOGLV_NOTICE)
#define LOGLEVEL_EMU_ERROR  (LOGCAT_EMU | LOGLV_ERROR)

extern volatile sig_atomic_t eflag;
extern volatile sig_atomic_t tflag;


#define POINTER_LOG_SIZE 30
static uint32_t pointerLog[POINTER_LOG_SIZE];
static int pointerLog_start = 0;
static int pointerLog_end   =-1;
static void saveInstPointer(uint32_t pointer){
	int next_end = (pointerLog_end+1) % POINTER_LOG_SIZE;
	if( pointerLog_end >= 0 && next_end == pointerLog_start ){
		pointerLog_start = (pointerLog_start+1) % POINTER_LOG_SIZE;
	}
	pointerLog_end = next_end;
	pointerLog[pointerLog_end] = pointer;
}

int prefixCheck(struct stMachineState *pM, uint32_t *pPointer){
	int nPrefix = 0;

	if( (pM->reg.fetchCache[0] & 0xfe) == 0xf2 ){
		// REP
		PREFIX_REPZ = (pM->reg.fetchCache[0] & 1);
		(*pPointer)++;
		if(PREFIX_OP32){ REG_EIP++; }else{ REG_IP++; }

		pM->reg.fetchCache[0] = pM->reg.fetchCache[1];
		pM->reg.fetchCache[1] = fetchCodeDataByte(pM, (*pPointer)+1);
		if(DEBUG){
			logfile_printf(LOGLEVEL_EMU_INFO, "REP prefix (z: %x)\n", PREFIX_REPZ);
			logfile_printf(LOGLEVEL_EMU_INFO, "new pointer: %05x  insts = %02x, %02x \n", *pPointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
		}
		nPrefix++;
	}
	if( pM->reg.fetchCache[0] == 0x67 ){
		// Address-size Prefix 
		PREFIX_AD32 = (PREFIX_AD32 ? 0 : 1);
		(*pPointer)++;
		if(PREFIX_OP32){ REG_EIP++; }else{ REG_IP++; }

		pM->reg.fetchCache[0] = pM->reg.fetchCache[1];
		pM->reg.fetchCache[1] = fetchCodeDataByte(pM, (*pPointer)+1);
		if(DEBUG){
			logfile_printf(LOGLEVEL_EMU_INFO3, "Address-size Prefix\n");
			logfile_printf(LOGLEVEL_EMU_INFO3, "new pointer: %05x  insts = %02x, %02x \n", *pPointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
		}
		nPrefix++;
	}
	if( pM->reg.fetchCache[0] == 0x66 ){
		// Operand-size Prefix 
		PREFIX_OP32 = (PREFIX_OP32 ? 0 : 1);
		(*pPointer)++;
		if(PREFIX_OP32){ REG_EIP++; }else{ REG_IP++; }

		pM->reg.fetchCache[0] = pM->reg.fetchCache[1];
		pM->reg.fetchCache[1] = fetchCodeDataByte(pM, (*pPointer)+1);

		if(DEBUG){
			logfile_printf(LOGLEVEL_EMU_INFO, "Operand-size prefix\n");
			logfile_printf(LOGLEVEL_EMU_INFO, "new pointer: %05x  insts = %02x, %02x \n", *pPointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
		}
		nPrefix++;
	}

	if( (pM->reg.fetchCache[0] & 0xe7 ) == 0x26 ){
		// Segment Prefix 
		PREFIX_SEG = ( (pM->reg.fetchCache[0] >> 3) & 3);
		(*pPointer)++;
		if(PREFIX_OP32){ REG_EIP++; }else{ REG_IP++; }

		pM->reg.fetchCache[0] = pM->reg.fetchCache[1];
		pM->reg.fetchCache[1] = fetchCodeDataByte(pM, (*pPointer)+1);
		if(DEBUG){
			logfile_printf(LOGLEVEL_EMU_INFO, "Segment prefix: %x\n", PREFIX_SEG);
			logfile_printf(LOGLEVEL_EMU_INFO, "new pointer: %05x  insts = %02x, %02x \n", *pPointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
		}
		nPrefix++;
	}
	if( (pM->reg.fetchCache[0] & 0xfe ) == 0x64 ){
		// Segment Prefix 
		PREFIX_SEG = ( pM->reg.fetchCache[0] == 0x64 ? SEGREG_NUM_FS : SEGREG_NUM_GS );
		(*pPointer)++;
		if(PREFIX_OP32){ REG_EIP++; }else{ REG_IP++; }

		pM->reg.fetchCache[0] = pM->reg.fetchCache[1];
		pM->reg.fetchCache[1] = fetchCodeDataByte(pM, (*pPointer)+1);
		if(DEBUG){
			logfile_printf(LOGLEVEL_EMU_INFO, "Segment prefix: %x\n", PREFIX_SEG);
			logfile_printf(LOGLEVEL_EMU_INFO, "new pointer: %05x  insts = %02x, %02x \n", *pPointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
		}
		nPrefix++;
	}

	if( pM->reg.fetchCache[0] == 0xf0 ){
		// LOCK Prefix 
		(*pPointer)++; 
		if(PREFIX_OP32){ REG_EIP++; }else{ REG_IP++; }

		pM->reg.fetchCache[0] = pM->reg.fetchCache[1];
		pM->reg.fetchCache[1] = fetchCodeDataByte(pM, (*pPointer)+1);
		if(DEBUG){
			logfile_printf(LOGLEVEL_EMU_INFO, "LOCK prefix\n");
			logfile_printf(LOGLEVEL_EMU_INFO, "new pointer: %05x  insts = %02x, %02x \n", *pPointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
		}
		nPrefix++;
	}

	return nPrefix;
}

void mainloop32_inner(struct stMachineState *pM);

void mainloop32(struct stMachineState *pM){

	pM->emu.nExecInsts = 0;
	pM->emu.stop       = 0;

	do{
		if ( sigsetjmp(pM->emu.env, 1) != 0 ) {
			if( pM->reg.fault & (1<<FAULTNUM_PAGEFAULT) ){
				logfile_printf(LOGLEVEL_EMU_INFO3, "Page fault detected (error code: %x). CS:EIP=%x:%x (pointer %x)\n", pM->reg.error_code, pM->reg.current_cs, pM->reg.current_eip, REG_CS_BASE + pM->reg.current_eip);
			}else if( pM->reg.fault & (1<<FAULTNUM_GP) ){
				logfile_printf(LOGLEVEL_EMU_INFO3, "General protection fault detected (error code: %x). CS:EIP=%x:%x (pointer %x)\n", pM->reg.error_code, pM->reg.current_cs, pM->reg.current_eip, REG_CS_BASE + pM->reg.current_eip);
			}else if( pM->reg.fault & (1<<FAULTNUM_STACKFAULT) ){
				logfile_printf(LOGLEVEL_EMU_INFO3, "Stack fault detected (error code: %x). CS:EIP=%x:%x (pointer %x)\n", pM->reg.error_code, pM->reg.current_cs, pM->reg.current_eip, REG_CS_BASE + pM->reg.current_eip);
			}
		}else{
			break;
		}
	}while( 1 );

	mainloop32_inner(pM);
}


void mainloop32_inner(struct stMachineState *pM){
	int result;
	uint16_t saved_ss;
	uint32_t prev_eflags;
	uint32_t pointer = REG_CS_BASE + REG_EIP;
	int (*exFunc)(struct stMachineState *pM, uint32_t pointer);


	if( eflag != 0 ){
		goto mainloop32_exit;
	}

	if( pM->reg.fault ){
		/*
		logfile_printf(LOGCAT_CPU_EXE | LOGLV_ERROR, 
		"%s: fault %x detected (CS:EIP=%x:%x %x:%x pointer %x)\n", __func__, pM->reg.fault, pM->reg.current_cs, pM->reg.current_eip, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
		*/

		if( pM->reg.fault & (1<<FAULTNUM_PAGEFAULT) ){
			pM->reg.fault = 0;
			enterINTwithECODE(pM, FAULTNUM_PAGEFAULT, pM->reg.current_cs, pM->reg.current_eip, pM->reg.error_code);
		}
		if( pM->reg.fault & (1<<FAULTNUM_STACKFAULT) ){
			pM->reg.fault = 0;
			enterINTwithECODE(pM, FAULTNUM_STACKFAULT, pM->reg.current_cs, pM->reg.current_eip, pM->reg.error_code);
		}
		if( pM->reg.fault & (1<<FAULTNUM_GP) ){
			pM->reg.fault = 0;
			enterINTwithECODE(pM, FAULTNUM_GP, pM->reg.current_cs, pM->reg.current_eip, pM->reg.error_code);
		}
		if( pM->reg.fault & (1<<FAULTNUM_SEGNOTP) ){
			pM->reg.fault = 0;
			enterINTwithECODE(pM, FAULTNUM_SEGNOTP, pM->reg.current_cs, pM->reg.current_eip, pM->reg.error_code);
		}
		if( pM->reg.fault & (1<<FAULTNUM_INVALIDTSS) ){
			pM->reg.fault = 0;
			enterINTwithECODE(pM, FAULTNUM_INVALIDTSS, pM->reg.current_cs, pM->reg.current_eip, pM->reg.error_code);
		}
		if( pM->reg.fault & (1<<INTNUM_UDOPCODE) ){
			pM->reg.fault = 0;
			enterINTwithECODE(pM, INTNUM_UDOPCODE, pM->reg.current_cs, pM->reg.current_eip, pM->reg.error_code);
		}
	}

	while( (pM->emu.stop == 0 || (pM->emu.stop > 0 && pM->emu.stop >= pM->emu.nExecInsts)) && eflag == 0 ){

		PREFIX_SEG  = PREF_SEG_UNSPECIFIED;
		PREFIX_REPZ = PREF_REP_UNSPECIFIED;
		PREFIX_AD32 = CODESEG_D_BIT && !(pM->reg.eflags & (1<<EFLAGS_BIT_VM)) ? 1 : 0;
		PREFIX_OP32 = CODESEG_D_BIT && !(pM->reg.eflags & (1<<EFLAGS_BIT_VM)) ? 1 : 0;
		saved_ss = REG_SS;

		pointer = REG_CS_BASE + REG_EIP;
		pM->reg.current_cs  = REG_CS;    // To save the instruction pointer including prefix
		pM->reg.current_eip = REG_EIP;   // To save the instruction pointer including prefix
		pM->reg.fault = 0;

		pM->reg.fetchCache[0] = fetchCodeDataByte(pM, pointer);
		pM->reg.fetchCache[1] = fetchCodeDataByte(pM, pointer+1);

		if(DEBUG){
			logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
			logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x  insts = %02x, %02x \n", pointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
			log_printReg32(LOGLEVEL_EMU_NOTICE, pM);
		}
		if( ((pointer&pM->emu.breakMask) == (pM->emu.breakPoint&pM->emu.breakMask) && pM->emu.stop==0) || (pM->emu.breakCounter != 0 && pM->emu.breakCounter == pM->emu.nExecInsts) ){
			logfile_printf(LOGLEVEL_EMU_NOTICE, "Breakpoint\n");
			logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
			logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x  insts = %02x, %02x \n", pointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
			log_printReg32(LOGLEVEL_EMU_NOTICE, pM);

			DEBUG=1;
			pM->emu.stop = pM->emu.nExecInsts + pM->emu.runAfterBreak;
		}

		saveInstPointer(pointer);

		if( pM->reg.fetchCache[0] == 0xeb && pM->reg.fetchCache[1] == 0xfe && !(REG_FLAGS & ((1<<FLAGS_BIT_TF)|(1<<FLAGS_BIT_IF))) ){
			fprintf(stderr, "Infinite loop detected\n");
			logfile_printf(LOGLEVEL_EMU_NOTICE, "Infinite loop detected\n");
			goto mainloop32_exit;
		}

		// DEBUG CODE
		if( pM->reg.fetchCache[0] == 0x00 && pM->reg.fetchCache[1] == 0x00 && 00 == fetchCodeDataByte(pM, pointer+2)){
			fprintf(stderr, "Here may not be code region \n");
			logfile_printf(LOGLEVEL_EMU_NOTICE, "Here may not be code region \n");
			goto mainloop32_exit;
		}


		// table lookup of the processing function for the instruction
		while( (exFunc = instCodeFunc386[pM->reg.fetchCache[0]]) == exPrefixDummy ){
			prefixCheck(pM, &pointer);
		}

		if(exFunc == 0){
			if( pM->reg.fetchCache[0] == 0x0f ){
				exFunc = instCodeFunc386_0x0f[pM->reg.fetchCache[1]];
			}else if( (pM->reg.fetchCache[0]&0xfe) == 0xfe ){
				exFunc = instCodeFunc386_0xfe_0xff[(pM->reg.fetchCache[1]>>3)&7];
			}else if( (pM->reg.fetchCache[0]&0xfe) == 0xf6 ){
				exFunc = instCodeFunc386_0xf6_0xf7[(pM->reg.fetchCache[1]>>3)&7];
			}
		}

		if( exFunc != 0 ){
			if(DEBUG) logfile_printf(LOGLEVEL_EMU_ERROR, "cnt:%06ld PC:%x  ", pM->emu.nExecInsts, pointer);
			prev_eflags = REG_FLAGS;
			result = exFunc(pM, pointer);
		}

		if( exFunc == 0 ){
			fprintf(stderr, "Unknown instruction\n");
			logfile_printf(LOGLEVEL_EMU_ERROR, "Unknown instruction\n");
			goto mainloop32_exit;
		}
		if( result != EX_RESULT_SUCCESS ){
			fprintf(stderr, "Instruction execusion error %d\n", result);
			logfile_printf(LOGLEVEL_EMU_ERROR, "Instruction execusion error %d\n", result);
			goto mainloop32_exit;
		}

		if( saved_ss != REG_SS && (exFunc == exMov || exFunc == exPOP) ){
			// do not process interrupts after "mov ss" or "pop ss" instruction
			// to prevent inconsistency of ss and (e)sp in following code
			//
			// mov ss, ...
			// mov sp, ...
		}else if( prev_eflags & REG_FLAGS & (1<<FLAGS_BIT_TF) ){
			enterINT(pM, INTNUM_SINGLE_STEP, REG_CS, REG_EIP);


		}else if( REG_FLAGS & (1<<FLAGS_BIT_IF) ){
			if( pM->mem.ioTimer.counter[0] != 0 && 0 == ((pM->mem.ioPICmain.ocw1) & 1) && tflag ){
				tflag = 0;
				if( ! DEBUG ){
					enterINT(pM, (pM->mem.ioPICmain.icw2)&0xf8, REG_CS, REG_EIP);
				}
			}else if( pM->mem.ioFDC.irq && 0 == ((pM->mem.ioPICmain.ocw1) & (1<<6)) ){
				logfile_printf(LOGLEVEL_EMU_INFO, "FDC interrupt \n");
				enterINT(pM, ((pM->mem.ioPICmain.icw2)&0xf8)+6, REG_CS, REG_EIP);
				pM->mem.ioFDC.irq = 0;

			}else if( pM->mem.ioUART1.int_enable && 0 == ((pM->mem.ioPICmain.ocw1) & (1<<3)) ){
				if( (pM->emu.nExecInsts&0x0f) == 0 && pM->mem.ioUART1.int_enable & 0x2 ){
					// Once this interrupt is enabled, its handler will be called continuously and
					// cannot be stop in interrupt-enabled conditions.
					// Thus, the former condition is introduced.
					logfile_printf(LOGLEVEL_EMU_INFO, "UART interrupt (tx ready) \n");
					enterINT(pM, ((pM->mem.ioPICmain.icw2)&0xf8)+3, REG_CS, REG_EIP);

				}else if( (pM->mem.ioUART1.int_enable & 0x1) ){
					if( ++(pM->mem.ioUART1.chkCntForInt) > 100 ){
						readUARTReg(pM, &(pM->mem.ioUART1), IOADDR_COM1_BASE + UART_REG_LINESTAT);
					}
					if( pM->mem.ioUART1.buffered ){
						logfile_printf(LOGLEVEL_EMU_INFO, "UART interrupt (rx ready) \n");
						enterINT(pM, ((pM->mem.ioPICmain.icw2)&0xf8)+3, REG_CS, REG_EIP);
					}
				}
			}
		}

		if( pM->reg.fault ){
			logfile_printf(LOGLEVEL_EMU_ERROR, "Unhandled fault ... (CS:EIP %x:%x)\n", pM->reg.current_cs, pM->reg.current_eip);
			goto mainloop32_exit;
		}

		pM->emu.nExecInsts++;
	}

mainloop32_exit:

	printf("\n\n");

	if( eflag ){
		logfile_printf(LOGLEVEL_EMU_NOTICE, "Interrupt...\n");
	}

	struct periPIC *pPIC;
	pPIC = &(pM->mem.ioPICmain);

	logfile_printf(LOGLEVEL_EMU_INFO,"PIC: %s is configured as  icw1..4: %02x %02x %02x %02x, ocw1..3: %02x %02x %02x\n",
     "M", pPIC->icw1, pPIC->icw2, pPIC->icw3, pPIC->icw4, pPIC->ocw1, pPIC->ocw2, pPIC->ocw3);

	pPIC = &(pM->mem.ioPICsub);
	logfile_printf(LOGLEVEL_EMU_INFO,"PIC: %s is configured as  icw1..4: %02x %02x %02x %02x, ocw1..3: %02x %02x %02x\n",
     "S", pPIC->icw1, pPIC->icw2, pPIC->icw3, pPIC->icw4, pPIC->ocw1, pPIC->ocw2, pPIC->ocw3);

	logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
	logfile_printf(LOGLEVEL_EMU_NOTICE, "history:\n");
	for(int i=pointerLog_start; pointerLog_end >= 0 && pointerLog_end < POINTER_LOG_SIZE; i = (i+1)%POINTER_LOG_SIZE){
		logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x\n", pointerLog[i]);
		if(i == pointerLog_end) break;
	}
	logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
	logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x, instruction cache = %02x, %02x \n", pointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
	logfile_printf(LOGLEVEL_EMU_NOTICE, "Instruction: ");
	for(int i=0; i<16; i++){
		if( checkLinearAccessible(pM, pointer+i) ){
			logfile_printf_without_header(LOGLEVEL_EMU_NOTICE, " %02x", readDataMemByteAsSV(pM, pointer+i));	
		}else{
			logfile_printf_without_header(LOGLEVEL_EMU_NOTICE, " --");	
		}
	}
	logfile_printf_without_header(LOGLEVEL_EMU_NOTICE, "\n");

	log_printReg32(LOGLEVEL_EMU_NOTICE, pM);

}
