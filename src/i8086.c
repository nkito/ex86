#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#include "i8086.h"
#include "misc.h"
#include "logfile.h"
#include "terminal.h"
#include "mem.h"

#include "mainloop.h"

#ifndef DEBUG
int DEBUG = 0;
#endif

//-----------------------------------------------------
//   signal handler                      
//-----------------------------------------------------
volatile sig_atomic_t eflag = 0;
extern volatile sig_atomic_t tflag;

void sig_handler_SIGINT(int signum) {
	eflag = 1;
}
//-----------------------------------------------------


void printUsage(void){
		fprintf(stderr, "i8086 emulator\n\n");
		fprintf(stderr, "Usage: \n");
		fprintf(stderr, " i8086emu [Options...] [Memory image file]\n\n");
		fprintf(stderr, "Options:\n");
		fprintf(stderr, " -l [loadaddr] : specifies load address of memory image\n");
		fprintf(stderr, " -b [addr]     : enables breakpoint \n");
		fprintf(stderr, " -bm [mask]    : enables breakpoint mask \n");
		fprintf(stderr, " -c [count]    : enables break after executing the specified number of instructions\n");
		fprintf(stderr, " -r [#insts]   : specifies number of instruction execusions after break (in hexadecimal)\n");
		fprintf(stderr, " -w [addr]     : enables address write watch \n");
		fprintf(stderr, " -8086         : emulates i8086\n");
		fprintf(stderr, " -186          : \n");
		fprintf(stderr, " -80186        : emulates i80186 (default)\n");
		fprintf(stderr, " -386          : \n");
		fprintf(stderr, " -80386        : emulates i386\n");
		fprintf(stderr, " -dm           : dump memory image to dump.txt during exiting\n");
		fprintf(stderr, " -h            : prints this message\n");
		fprintf(stderr, " -debug        : enables debug mode\n");
		fprintf(stderr, "\n");
}


int main(int argc, char *argv[]){
	struct stMachineState ms;

	FILE    *fp         = NULL;
	char    *filename   = NULL;
	uint32_t load_pos   = 0;
	int     dump_memory = 0;

	if( argc < 2 ){
		printUsage();
		return 0;
	}


	memset(&ms, 0, sizeof(ms));


	ms.mem.mem = malloc(EMU_MEM_SIZE);
	if( ms.mem.mem == NULL ){
		fprintf(stderr, "Memory allocation for a virtual machine was failed.\n");
		exit(1);		
	}
	memset(ms.mem.mem, 0, EMU_MEM_SIZE);


	ms.emu.breakPoint    = EMU_MEM_SIZE;
	ms.emu.breakMask     = ~0;
	ms.emu.runAfterBreak = 10;
	ms.emu.breakCounter  = 0;
	ms.emu.emu_cpu       = EMU_CPU_80186;

	//-----------------------------------------------------
	// Reset the CPU register and set pointers for CPU registers
	//-----------------------------------------------------
	ms.reg.eip           = 0x0000fff0;
	ms.reg.cs            = 0xf000;
	ms.reg.descc_cs.base = 0xf0000;
	ms.reg.eflags        = 0x00000000;

	ms.reg.descc_es.limit = ms.reg.descc_es.limit_max = 0xffff; ms.reg.descc_es.writable = 1;
	ms.reg.descc_cs.limit = ms.reg.descc_cs.limit_max = 0xffff; ms.reg.descc_cs.readable = 1;
	ms.reg.descc_ss.limit = ms.reg.descc_ss.limit_max = 0xffff; ms.reg.descc_ss.writable = 1;
	ms.reg.descc_ds.limit = ms.reg.descc_ds.limit_max = 0xffff; ms.reg.descc_ds.writable = 1;
	ms.reg.descc_fs.limit = ms.reg.descc_fs.limit_max = 0xffff; ms.reg.descc_fs.writable = 1;
	ms.reg.descc_gs.limit = ms.reg.descc_gs.limit_max = 0xffff; ms.reg.descc_gs.writable = 1;

	ms.reg.eax  = 0x12345678;
	ms.reg.p_ax = (uint16_t *)(&(ms.reg.eax)); ms.reg.p_bx = (uint16_t *)(&(ms.reg.ebx));
	ms.reg.p_cx = (uint16_t *)(&(ms.reg.ecx)); ms.reg.p_dx = (uint16_t *)(&(ms.reg.edx));
	ms.reg.p_sp = (uint16_t *)(&(ms.reg.esp)); ms.reg.p_bp = (uint16_t *)(&(ms.reg.ebp));
	ms.reg.p_si = (uint16_t *)(&(ms.reg.esi)); ms.reg.p_di = (uint16_t *)(&(ms.reg.edi));
	ms.reg.p_ip = (uint16_t *)(&(ms.reg.eip)); ms.reg.p_flags = (uint16_t *)(&(ms.reg.eflags));

	// This line is necessary (to avoid compiler optimization)
	// Compiler seems to guess the value of *reg.p_ax is not initialized and 
	// omits the following two "if" statements for checking endianness if this function call (printf) does not exist.
	printf(" ");

	if( *(ms.reg.p_ax) == 0x5678 ){
		// little endian
		printf("This computer is little-endian.\n");
	}else if( *(ms.reg.p_ax) == 0x1234 ){
		// big endian
		printf("This computer is big-endian.\n");
		ms.reg.p_ax++; ms.reg.p_bx++;
		ms.reg.p_cx++; ms.reg.p_dx++;
		ms.reg.p_sp++; ms.reg.p_bp++;
		ms.reg.p_si++; ms.reg.p_di++;
		ms.reg.p_ip++; ms.reg.p_flags++;
	}else{
		fprintf(stderr, "failed to reset CPU registers.\n");
		exit(1);
	}
	ms.reg.eax  = 0x0;

	//-----------------------------------------------------
	// initializing the memory system and debugging feature
	//-----------------------------------------------------
	ms.mem.watchAddr = EMU_MEM_SIZE; // watch is disabled

	ms.emu.log_enabled_cat = (LOGCAT_EMU | LOGCAT_CPU_EXE);
	ms.emu.log_level       = LOGLV_NOTICE;

	ms.mem.ioSysCtrlB = 0;
	/*
	ms.mem.ioPICmain.init = 0;
	ms.mem.ioPICsub .init = 0;
	ms.mem.ioPICmain.seqn = 0;
	ms.mem.ioPICsub .seqn = 0;
	*/

	for(uint32_t i = 0; i<EMU_MEM_SIZE; i++){
		writeDataMemByte(&ms, i, 0);
	}

	// clear display area (it clears the screen immidiately)
	for(uint32_t i = 0; i< 80*25; i++){
		writeDataMemByte(&ms, 0xb8000 + 2*i+0, ' ');
		writeDataMemByte(&ms, 0xb8000 + 2*i+1, 0x08); // color
	}

	//-----------------------------------------------------
	// Reset the terminal color for serial console programs such as monitors
	//-----------------------------------------------------
	termResetColor();
	termResetBlink();
	//-----------------------------------------------------


	for(int i=1; i<argc; i++){
		if(!strcmp("-debug", argv[i])){
#ifndef DEBUG
			DEBUG = 1;
#endif
		}else if(!strcmp("-l", argv[i])){
			load_pos = (i+1 < argc) ? parseHex( argv[i+1] ) : 0;
			i++;
		}else if(!strcmp("-b", argv[i])){
			ms.emu.breakPoint = (i+1 < argc) ? parseHex( argv[i+1] ) : 0;
			i++;
		}else if(!strcmp("-bm", argv[i])){
			ms.emu.breakMask = (i+1 < argc) ? parseHex( argv[i+1] ) : 0;
			ms.emu.breakMask = ~ms.emu.breakMask;
			i++;
		}else if(!strcmp("-c", argv[i])){
			ms.emu.breakCounter = (i+1 < argc) ? parseDec( argv[i+1] ) : 0;
			i++;
		}else if(!strcmp("-r", argv[i])){
			ms.emu.runAfterBreak = (i+1 < argc) ? parseDec( argv[i+1] ) : 0;
			i++;
		}else if(!strcmp("-w", argv[i])){
			ms.mem.watchAddr = (i+1 < argc) ? parseHex( argv[i+1] ) : 0;
			i++;
		}else if(!strcmp("-dm", argv[i])){
			dump_memory = 1;
		}else if(!strcmp("-h", argv[i])){
			printUsage();
			return 0;
		}else if(!strcmp("-8086", argv[i])){
			ms.emu.emu_cpu = EMU_CPU_8086;
		}else if(!strcmp("-80186", argv[i]) || !strcmp("-186", argv[i])){
			ms.emu.emu_cpu = EMU_CPU_80186;
		}else if(!strcmp("-80386", argv[i]) || !strcmp("-386", argv[i])){
			ms.emu.emu_cpu = EMU_CPU_80386;
		}else{
			filename = argv[i];
			fp = fopen(filename, "rb");
			if( fp == NULL ){
				fprintf(stderr, " Failed to open \"%s\" \n", filename);
				exit(1);
			}
			if( fread(ms.mem.mem+load_pos, 1, EMU_MEM_SIZE-load_pos, fp) < 1 ){
				fprintf(stderr, " Read error \"%s\" \n", filename);
				exit(1);
			}
			printf("File \"%s\" is loading to 0x%x\n", filename, load_pos);
			fclose(fp);
		}
	}

	if( fp == NULL ){
		fprintf(stderr, "no image file is loaded...\n\n");
		printUsage();
		exit(1);
	}

	logfile_init( ms.emu.log_enabled_cat, ms.emu.log_level);
	logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "------------------------\n");
	logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "Starting the emulator...\n");

	if(DEBUG){
		logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "Debug mode\n");
	}

	if(ms.emu.breakPoint < EMU_MEM_SIZE){
		logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "Breakpoint        : 0x%x\n", ms.emu.breakPoint);
		logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "#insts after break: %d\n",   ms.emu.runAfterBreak);
	}


	//-----------------------------------------------------
	// Register a signal handler for handling Ctrl+C
	//-----------------------------------------------------
	if ( signal(SIGINT, sig_handler_SIGINT) == SIG_ERR ) {
		logfile_printf(LOGCAT_EMU | LOGLV_ERROR, "failed to register a signal hander.\n");
		logfile_close();
		exit(1);
	}
	//-----------------------------------------------------


	struct termios save_settings;
	struct termios settings;

	tcgetattr(0, &save_settings);
	settings = save_settings;
	settings.c_lflag &= ~(ECHO|ICANON);  /* without input echo, and unbuffered */
	settings.c_cc[VTIME] = 0;
	settings.c_cc[VMIN] = 1;
	tcsetattr(0,TCSANOW,&settings);
	fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

	if( ms.emu.emu_cpu == EMU_CPU_80186 || ms.emu.emu_cpu == EMU_CPU_8086 ){
		mainloop16(&ms);
	}else if( ms.emu.emu_cpu == EMU_CPU_80386 ){
		mainloop32(&ms);
	}

	tcsetattr(0, TCSANOW, &save_settings);
	termResetSettingForExit();

	logfile_close();

	if( eflag ){
		printf("Interrupt...\n");
	}

	if( dump_memory ){
		if( ms.reg.cr[0] & (1<<CR0_BIT_PE) ){
			printf("writing memory image to dump.txt...\n");
			filename = "dump.txt";
			fp = fopen(filename, "w");
			if( fp == NULL ){
				fprintf(stderr, " Failed to open \"%s\" \n", filename);
				exit(1);
			}
//			for(uint32_t i=0; i< EMU_MEM_SIZE; i+=16){
			for(uint32_t i=0xc1ff0000; i< 0xc2000000; i+=16){
				fprintf(fp, "%05x: ", i);
				for(uint32_t j=0; j< 16; j++){
					if( checkLinearAccessible(&ms, i+j) ){
						fprintf(fp, "%02x ", readDataMemByteAsSV(&ms, i+j) );
					}else{
						fprintf(fp, "-- ");
					}
				}
				fprintf(fp, " | ");
				for(uint32_t j=0; j< 16; j++){
					if( checkLinearAccessible(&ms, i+j) ){
						char b = readDataMemByteAsSV(&ms, i+j);
						fprintf(fp, "%c", isprint( b ) ? b : '.' );
					}else{
						fprintf(fp, ".");
					}
				}
				fprintf(fp, "\n");
			}
			fclose(fp);
			printf("Done\n");
		}

		printf("writing memory image to dump_phy.txt...\n");
		filename = "dump_phy.txt";
		fp = fopen(filename, "w");
		if( fp == NULL ){
			fprintf(stderr, " Failed to open \"%s\" \n", filename);
			exit(1);
		}
		for(uint32_t i=0; i< EMU_MEM_SIZE; i+=16){
			fprintf(fp, "%05x: ", i);
			for(uint32_t j=0; j< 16; j++){
				fprintf(fp, "%02x ", ms.mem.mem[i+j] );
			}
			fprintf(fp, " | ");
			for(uint32_t j=0; j< 16; j++){
				char b = ms.mem.mem[i+j];
				fprintf(fp, "%c", isprint( b ) ? b : '.' );
			}
			fprintf(fp, "\n");
		}
		fclose(fp);
		printf("Done\n");
	}

	return 0;
}

