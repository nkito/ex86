#include <stdio.h>
#include <stdint.h>
#include <signal.h>

#include "i8086.h"
#include "instmap16.h"

#include "misc.h"
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
	pointerLog_end = (pointerLog_end+1) % POINTER_LOG_SIZE;
	if( pointerLog_end == pointerLog_start ){
		pointerLog_start = (pointerLog_start+1) % POINTER_LOG_SIZE;
	}
	pointerLog[pointerLog_end] = pointer;
}


void mainloop16(struct stMachineState *pM){
	uint64_t nExecInsts = 0;
    int stop = 0;
	int result;
	uint16_t prev_flags;
	uint32_t pointer;
	// uint8_t inst0, inst1;
	int (*exFunc)(struct stMachineState *pM, uint32_t pointer);

	pointer = MEMADDR(REG_CS, REG_IP); 
	pM->reg.fetchCache[0] = fetchCodeDataByte(pM, pointer);
	pM->reg.fetchCache[1] = fetchCodeDataByte(pM, pointer+1);

	PREFIX_AD32 = 0;
	PREFIX_OP32 = 0;
	while( (stop == 0 || (stop > 0 && stop >= nExecInsts)) && eflag == 0 ){
		pointer = MEMADDR(REG_CS, REG_IP);
		pM->reg.current_cs  = REG_CS;
		pM->reg.current_eip = REG_EIP;

		PREFIX_SEG  = PREF_SEG_UNSPECIFIED;
		PREFIX_REPZ = PREF_REP_UNSPECIFIED;
		
		pM->reg.fetchCache[0] = fetchCodeDataByte(pM, pointer);
		pM->reg.fetchCache[1] = fetchCodeDataByte(pM, pointer+1);

		if(DEBUG){
			logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
			logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x  insts = %02x, %02x \n", pointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
			log_printReg16(LOGLEVEL_EMU_NOTICE, pM);
		}
		if( (pointer == pM->emu.breakPoint && stop==0) || (pM->emu.breakCounter != 0 && pM->emu.breakCounter == nExecInsts) ){
			logfile_printf(LOGLEVEL_EMU_NOTICE, "Breakpoint\n");
			logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
			logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x  insts = %02x, %02x \n", pointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
			log_printReg16(LOGLEVEL_EMU_NOTICE, pM);

			DEBUG=1;
			stop = nExecInsts + pM->emu.runAfterBreak;
		}

		/*
		if( (inst0 & 0xe7 ) == 0x26 ){
			// Segment Prefix 
			seg = ( (inst0 >> 3) & 3);
			pointer++; (*reg.p_ip)++;

			inst0 = fetchCodeDataByte(&mem, pointer);
			inst1 = fetchCodeDataByte(&mem, pointer+1);
			if(DEBUG){
				logfile_printf(LOGLEVEL_INFO, "Segment prefix: %x\n", seg);
				logfile_printf(LOGLEVEL_INFO, "new pointer: %05x  insts = %02x, %02x \n", pointer, inst0, inst1);
			}
		}
		*/
		if( (pM->reg.fetchCache[0] & 0xfe) == 0xf2 ){
			// REP
			PREFIX_REPZ = (pM->reg.fetchCache[0] & 1);
			pointer++; REG_IP++;

			pM->reg.fetchCache[0] = fetchCodeDataByte(pM, pointer);
			pM->reg.fetchCache[1] = fetchCodeDataByte(pM, pointer+1);
			if(DEBUG){
				logfile_printf(LOGLEVEL_EMU_INFO, "REP prefix (z: %x)\n", PREFIX_REPZ);
				logfile_printf(LOGLEVEL_EMU_INFO, "new pointer: %05x  insts = %02x, %02x \n", pointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
			}
		}
		if( (pM->reg.fetchCache[0] & 0xe7 ) == 0x26 ){
			// Segment Prefix 
			PREFIX_SEG = ( (pM->reg.fetchCache[0] >> 3) & 3);
			pointer++; REG_IP++;

			pM->reg.fetchCache[0] = fetchCodeDataByte(pM, pointer);
			pM->reg.fetchCache[1] = fetchCodeDataByte(pM, pointer+1);
			if(DEBUG){
				logfile_printf(LOGLEVEL_EMU_INFO, "Segment prefix: %x\n", PREFIX_SEG);
				logfile_printf(LOGLEVEL_EMU_INFO, "new pointer: %05x  insts = %02x, %02x \n", pointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
			}
		}

		saveInstPointer(pointer);

		// table lookup of the processing function for the instruction
		if( pM->emu.emu_cpu == EMU_CPU_8086 ){
			exFunc = instCodeFunc8086[pM->reg.fetchCache[0]];
		}else if( pM->emu.emu_cpu == EMU_CPU_80186 ){
			exFunc = instCodeFunc80186[pM->reg.fetchCache[0]];
		}else{
			exFunc = 0;
		}

		if(exFunc == 0){
			if( (pM->reg.fetchCache[0] & 0xfe) == 0xfe ){
				exFunc = instCodeFunc_0xfe_0xff[(pM->reg.fetchCache[1]>>3)&7];
			}else if( (pM->reg.fetchCache[0] & 0xfe) == 0xf6 ){
				exFunc = instCodeFunc_0xf6_0xf7[(pM->reg.fetchCache[1]>>3)&7];
			}else{
				/* HLT instruction */
				if( pM->reg.fetchCache[0] == 0xf4 ){
					fprintf(stderr, "\n\nProcessor halted\n");
					logfile_printf(LOGLEVEL_EMU_NOTICE, "Processor halted\n");
			        goto mainloop16_exit;
				}
			}
		}

		if( exFunc != 0 ){
			if(DEBUG) logfile_printf(LOGLEVEL_EMU_ERROR, "cnt:%06ld PC:%x  ", nExecInsts, pointer);
			prev_flags = REG_FLAGS;
			result = exFunc(pM, pointer);
		}

		if( exFunc == 0 ){
			fprintf(stderr, "Unknown instruction\n");
			logfile_printf(LOGLEVEL_EMU_ERROR, "Unknown instruction\n");
			goto mainloop16_exit;
		}
		if( result != EX_RESULT_SUCCESS ){
			fprintf(stderr, "Instruction execusion error %d\n", result);
			logfile_printf(LOGLEVEL_EMU_ERROR, "Instruction execusion error %d\n", result);
			goto mainloop16_exit;
		}

		if( prev_flags & REG_FLAGS & (1<<FLAGS_BIT_TF) ){
			enterINT(pM, 1, REG_CS, REG_IP);

		}else if( (nExecInsts&0x0f)==0 && pM->mem.ioTimer.counter[0] != 0 && ((REG_FLAGS & (1<<FLAGS_BIT_IF)) != 0) ){
			if( tflag ){
				tflag = 0;
				enterINT(pM, 0x08, REG_CS, REG_IP);

				// resetTimerCounter(0);
			}
		}

		nExecInsts++;
	}

mainloop16_exit:

	if( eflag ){
		logfile_printf(LOGLEVEL_EMU_NOTICE, "Interrupt...\n");
	}

	logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
	logfile_printf(LOGLEVEL_EMU_NOTICE, "history:\n");
	for(int i=pointerLog_start; pointerLog_end >= 0 && pointerLog_end < POINTER_LOG_SIZE; i = (i+1)%POINTER_LOG_SIZE){
		logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x\n", pointerLog[i]);
		if(i == pointerLog_end) break;
	}
	logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
	logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x  insts = %02x, %02x \n", pointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
	log_printReg16(LOGLEVEL_EMU_NOTICE, pM);

}
