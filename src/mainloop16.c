#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <sys/time.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>

#include "i8086.h"
#include "instmap16.h"

#include "misc.h"
#include "logfile.h"
#include "mem.h"
#include "terminal.h"

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
	pointerLog_end = (pointerLog_end + 1) % POINTER_LOG_SIZE;
	if( pointerLog_end == pointerLog_start ){
		pointerLog_start = (pointerLog_start + 1) % POINTER_LOG_SIZE;
	}
	pointerLog[pointerLog_end] = pointer;
}

int emuMonitor16_splitCmd(char *cmd, int *pargc, char **argv, int maxArg){
	int i, argc = 0;
	char *p = cmd;

	for(i = 0; i < maxArg; i++){
		while (*p == ' ' || *p == '\t' || *p == '\b' || *p == '\r' || *p == '\n')
			p++;
		argv[argc] = p;
		while (!(*p == ' ' || *p == '\t' || *p == '\b' || *p == '\r' || *p == '\n' || *p == '\0'))
			p++;
		if( *p == '\0' ){
			if( argv[argc] != p ){
				argc++;
			}
			break;
		}else{
			*p++ = '\0';
			argc++;
		}
	}
	*pargc = argc;

	return argc;
}

void emuMonitor16_help(void){
	printf(" d [addr] :  dump memory. \n");
	printf(" reg      :  show register values. \n");
	printf(" halt     :  halt the emulator and exit. \n");
	printf(" exit     :  exit this monitor and return to emulation. \n");
	printf(" help     :  show this message. \n\n");
}

int emuMonitor16_execCmd(struct stMachineState *pM, char *cmd){
	char *argv[20];
	int argc;
	//    static int start_address = 0;

	emuMonitor16_splitCmd(cmd, &argc, argv, sizeof(argv) / sizeof(char *));

	if( (!strcmp(argv[0], "d") && argc == 2) || (*argv[0] == 'd' && argc == 1) ){
		static unsigned int addr = 0;
		char memstr[17];
		unsigned char membyte;
		unsigned int i;

		memstr[16] = '\0';

		if( argc == 1 ){
			if( *(argv[0] + 1) != '\0' ){
				sscanf(argv[0] + 1, "%x", &addr);
			}
		}else{
			sscanf(argv[1], "%x", &addr);
		}
		for(i = (addr & 0xffff0); i <= ((addr + 0x7f) | 0xf) && i <= 0xfffff; i++){
			if( (i & 0xf) == 0 ){
				printf("%08x : ", i);
			}
			if( i < addr ){
				printf("   ");
				memstr[i & 0xf] = ' ';
			}else{
				membyte = readDataMemByte(pM, i);
				printf("%02x ", membyte);
				memstr[i & 0xf] = isprint(membyte) ? membyte : '.';
			}
			if( (i & 0xf) == 0xf ){
				printf(": %s \r\n", memstr);
			}
		}
		addr += 0x80;
	}else if( !strcmp(argv[0], "reg") ){
		printf("CS:IP = %04x:%04x\n", REG_CS, REG_IP);
		printf("AX=%04x BX=%04x CX=%04x DX=%04x \n", REG_AX, REG_BX, REG_CX, REG_DX);
		printf("SP=%04x BP=%04x SI=%04x DI=%04x \n", REG_SP, REG_BP, REG_SI, REG_DI);
		printf("ES=%04x CS=%04x SS=%04x DS=%04x FLAGS=%04x \n", REG_ES, REG_CS, REG_SS, REG_DS, REG_FLAGS);
	}else if( !strcmp(argv[0], "exit") ){
		return 0;
	}else if( !strcmp(argv[0], "halt") ){
		return -1;
	}else if( !strcmp(argv[0], "help") ){
		emuMonitor16_help();
	}else{
		char *p;
		for(p = argv[0]; *p != '\0'; p++){
			if (*p != '\n' && *p != '\r' && *p != '\t' && *p != ' ')
				break;
		}if( *p != '\0' ){
			printf("Syntax error\r\n");
			emuMonitor16_help();
		}
	}

	return 1;
}

extern struct termios term_original_settings;

/**
 * Emulator monitor for 16bit environment
 *
 * Return value is 0 or a negative value.
 * 0 requests the return to the emulation.
 * A negative value requests halting the emulator.
 */
int emuMonitor16(struct stMachineState *pM){
	int result;
	char cmd[128];
	struct termios settings;

	settings = term_original_settings;
	tcsetattr(0, TCSANOW, &settings);
	fcntl(0, F_SETFL, fcntl(0, F_GETFL) & (~O_NONBLOCK));
	termResetSettingForExit();

	termSetCharColor(charColorBrightRed);
	printf("\nEmulator monitor (16-bit)\n\n");
	printf("Use \"halt\" command to halt the emulator\n");
	printf("Use \"exit\" command to return the emulator\n");
	termSetCharColor(charColorReset);

	do{
		termSetCharColor(charColorBrightCyan);
		printf("> ");
		termSetCharColor(charColorReset);
		fflush(stdout);

		fgets(cmd, sizeof(cmd), stdin);
		result = emuMonitor16_execCmd(pM, cmd);
	}while( result > 0 );

	termResetSettingForExit();
	termClear();
	settings = term_original_settings;
	settings.c_lflag &= ~(ECHO | ICANON); /* without input echo, and unbuffered */
	settings.c_cc[VTIME] = 0;
	settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &settings);
	fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

	return result;
}

void mainloop16(struct stMachineState *pM){
	uint64_t nExecInsts = 0;
	int stop = 0;
	int result;
	uint16_t prev_flags;
	uint32_t pointer;
	int (*exFunc)(struct stMachineState *pM, uint32_t pointer);

	struct timeval TimevalTime;
	volatile sig_atomic_t prev_eflag = 0;
	time_t prevSIGINTtime = 0;

	pointer = MEMADDR(REG_CS, REG_IP);
	pM->reg.fetchCache[0] = fetchCodeDataByte(pM, pointer);
	pM->reg.fetchCache[1] = fetchCodeDataByte(pM, pointer + 1);

	if( eflag ){
		prev_eflag = eflag;
		gettimeofday(&TimevalTime, NULL);
		prevSIGINTtime = (TimevalTime.tv_sec * 1000) + (TimevalTime.tv_usec / 1000);
	}

	PREFIX_AD32 = 0;
	PREFIX_OP32 = 0;
	while( (stop == 0 || (stop > 0 && stop >= nExecInsts)) ){

		// Checking the SIGINT status, treatment of Ctrl+C.
		if( eflag != prev_eflag ){
			prev_eflag = eflag;
			struct timeval time_now;
			gettimeofday(&time_now, NULL);
			time_t currentSIGINTtime = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);

			// time between two Ctrl+C keyins is shorter than 1000ms, then enter the monitor
			if( currentSIGINTtime - prevSIGINTtime < 1000 ){
				if( emuMonitor16(pM) < 0 ){
					logfile_printf(LOGLEVEL_EMU_NOTICE, "Interrupt...\n");
					break;
				}
			}
			prevSIGINTtime = currentSIGINTtime;
		}

		pointer = MEMADDR(REG_CS, REG_IP);
		pM->reg.current_cs  = REG_CS;    // To save the instruction pointer including prefix
		pM->reg.current_eip = REG_IP;    // To save the instruction pointer including prefix

		PREFIX_SEG  = PREF_SEG_UNSPECIFIED;
		PREFIX_REPZ = PREF_REP_UNSPECIFIED;

		pM->reg.fetchCache[0] = fetchCodeDataByte(pM, pointer);
		pM->reg.fetchCache[1] = fetchCodeDataByte(pM, pointer + 1);

		if(DEBUG){
			logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
			logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x  insts = %02x, %02x \n", pointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
			log_printReg16(LOGLEVEL_EMU_NOTICE, pM);
		}
		if( (pointer == pM->emu.breakPoint && stop == 0) || (pM->emu.breakCounter != 0 && pM->emu.breakCounter == nExecInsts) ){
			logfile_printf(LOGLEVEL_EMU_NOTICE, "Breakpoint\n");
			logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
			logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x  insts = %02x, %02x \n", pointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
			log_printReg16(LOGLEVEL_EMU_NOTICE, pM);

			DEBUG = 1;
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
			pointer++;
			REG_IP++;

			pM->reg.fetchCache[0] = fetchCodeDataByte(pM, pointer);
			pM->reg.fetchCache[1] = fetchCodeDataByte(pM, pointer + 1);
			if(DEBUG){
				logfile_printf(LOGLEVEL_EMU_INFO, "REP prefix (z: %x)\n", PREFIX_REPZ);
				logfile_printf(LOGLEVEL_EMU_INFO, "new pointer: %05x  insts = %02x, %02x \n", pointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
			}
		}
		if( (pM->reg.fetchCache[0] & 0xe7) == 0x26 ){
			// Segment Prefix
			PREFIX_SEG = ((pM->reg.fetchCache[0] >> 3) & 3);
			pointer++;
			REG_IP++;

			pM->reg.fetchCache[0] = fetchCodeDataByte(pM, pointer);
			pM->reg.fetchCache[1] = fetchCodeDataByte(pM, pointer + 1);
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
				exFunc = instCodeFunc_0xfe_0xff[(pM->reg.fetchCache[1] >> 3) & 7];
			}else if( (pM->reg.fetchCache[0] & 0xfe) == 0xf6 ){
				exFunc = instCodeFunc_0xf6_0xf7[(pM->reg.fetchCache[1] >> 3) & 7];
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
			if (DEBUG) 
				logfile_printf(LOGLEVEL_EMU_ERROR, "cnt:%06ld PC:%x  ", nExecInsts, pointer);
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

		if( prev_flags & REG_FLAGS & (1 << FLAGS_BIT_TF) ){
			enterINT(pM, 1, REG_CS, REG_IP);
		}else if( (nExecInsts & 0x0f) == 0 && pM->mem.ioTimer.counter[0] != 0 && ((REG_FLAGS & (1 << FLAGS_BIT_IF)) != 0) ){
			if( tflag ){
				tflag = 0;
				enterINT(pM, 0x08, REG_CS, REG_IP);

				// resetTimerCounter(0);
			}
		}

		nExecInsts++;
	}

mainloop16_exit:

	logfile_printf(LOGLEVEL_EMU_NOTICE, "Exit...\n");

	logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
	logfile_printf(LOGLEVEL_EMU_NOTICE, "history:\n");
	for(int i = pointerLog_start; pointerLog_end >= 0 && pointerLog_end < POINTER_LOG_SIZE; i = (i + 1) % POINTER_LOG_SIZE){
		logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x\n", pointerLog[i]);
		if( i == pointerLog_end )
			break;
	}
	logfile_printf(LOGLEVEL_EMU_NOTICE, "================================== \n");
	logfile_printf(LOGLEVEL_EMU_NOTICE, "pointer: %05x  insts = %02x, %02x \n", pointer, pM->reg.fetchCache[0], pM->reg.fetchCache[1]);
	log_printReg16(LOGLEVEL_EMU_NOTICE, pM);
}
