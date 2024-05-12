#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <sys/time.h>


#include "i8086.h"
#include "misc.h"
#include "logfile.h"
#include "terminal.h"
#include "mem.h"
#include "time.h"

//-----------------------------------------------------
//   signal handler                      
//-----------------------------------------------------
volatile sig_atomic_t intflag = 0;
volatile sig_atomic_t timerflag;

void sig_handler_SIGINT(int signum) {
    intflag = intflag + 1;
}

void timer_handler(int signum){
  switch (signum) {
  case SIGALRM:
    timerflag = 1;
    break;

  default:
    break;
  }
}

//-----------------------------------------------------
// Terminal setting
//-----------------------------------------------------
struct termios termOrigSettings;
//-----------------------------------------------------


int getIntFlag(struct stMachineState *pM){
    return intflag;
}
int getTimerFlag(struct stMachineState *pM){
    return timerflag;
}
void resetTimerFlag(struct stMachineState *pM){
    timerflag = 0;
}

uint64_t getTimeInMs(struct stMachineState *pM){
    struct timeval TimevalTime;

    gettimeofday(&TimevalTime, NULL);
    return (((uint64_t)TimevalTime.tv_sec) * 1000) + (((uint64_t)TimevalTime.tv_usec) / 1000);
}

void setTimerInUSec(struct stMachineState *pM, uint32_t interval_in_Usec){
    struct sigaction action, old_action;
    struct itimerval timer, old_timer;

    memset(&    action, 0, sizeof(    action));
    memset(&old_action, 0, sizeof(old_action));

    action.sa_handler = timer_handler;
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGALRM, &action, &old_action) == -1) {
        logfile_printf(LOGCAT_IO_TIMER | LOGLV_ERROR, "Timer: timer setting error (failed to set sigaction)\n");
        logfile_close();
        shutdownEmulator(pM);
    }

    timer.it_value.tv_sec     = 0;
    timer.it_value.tv_usec    = interval_in_Usec;
    timer.it_interval.tv_sec  = 0;
    timer.it_interval.tv_usec = interval_in_Usec;
    if (setitimer(ITIMER_REAL, &timer, &old_timer) == -1) {
        logfile_printf(LOGCAT_IO_TIMER | LOGLV_ERROR, "Timer: timer setting error\n");
        logfile_close();
        shutdownEmulator(pM);
    }
}


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
    fprintf(stderr, " -log          : generates the log file of the emulator \n");
    fprintf(stderr, " -loglevel [lv]: generates the log file of the emulator with specified log level 0-15 (lower value is more verbose) \n");
    fprintf(stderr, " -8086         : emulates i8086\n");
    fprintf(stderr, " -186          : \n");
    fprintf(stderr, " -80186        : emulates i80186 (default)\n");
    fprintf(stderr, " -386          : \n");
    fprintf(stderr, " -80386        : emulates i386\n");
    fprintf(stderr, " -h            : prints this message\n");
    fprintf(stderr, " -debug        : enables debug mode\n");
    fprintf(stderr, "\n");
}

int initEmulator(struct stMachineState *pM, int argc, char *argv[]){
    FILE    *fp         = NULL;
    char    *filename   = NULL;
    uint32_t load_pos   = 0;


    if( argc < 2 ){
        printUsage();
        return 0;
    }

    pM->pMemIo->stMem.mem = malloc(EMU_MEM_SIZE);
    if( pM->pMemIo->stMem.mem == NULL ){
        fprintf(stderr, "Memory allocation for a virtual machine was failed.\n");
        exit(1);        
    }
//    memset(pM->pMemIo->stMem.mem, 0, EMU_MEM_SIZE);

    for(uint32_t i = 0; i<EMU_MEM_SIZE; i++){
        writeDataMemByte(pM, i, 0);
    }


    for(int i=1; i<argc; i++){
        if(!strcmp("-debug", argv[i])){
#ifndef DEBUG
            DEBUG = 1;
#endif
        }else if(!strcmp("-l", argv[i])){
            load_pos = (i+1 < argc) ? parseHex( argv[i+1] ) : 0;
            i++;
        }else if(!strcmp("-b", argv[i])){
            pM->pEmu->breakPoint = (i+1 < argc) ? parseHex( argv[i+1] ) : 0;
            i++;
        }else if(!strcmp("-bm", argv[i])){
            pM->pEmu->breakMask = (i+1 < argc) ? parseHex( argv[i+1] ) : 0;
            pM->pEmu->breakMask = ~pM->pEmu->breakMask;
            i++;
        }else if(!strcmp("-c", argv[i])){
            pM->pEmu->breakCounter = (i+1 < argc) ? parseDec( argv[i+1] ) : 0;
            i++;
        }else if(!strcmp("-r", argv[i])){
            pM->pEmu->runAfterBreak = (i+1 < argc) ? parseDec( argv[i+1] ) : 0;
            i++;
        }else if(!strcmp("-w", argv[i])){
            pM->pEmu->watchAddr = (i+1 < argc) ? parseHex( argv[i+1] ) : 0;
            i++;
        }else if(!strcmp("-h", argv[i])){
            printUsage();
            return 0;
        }else if(!strcmp("-loglevel", argv[i])){
            pM->pEmu->log_level = (i+1 < argc) ? (LOGLV_MASK & parseHex( argv[i+1] )) : LOGLV_NOTICE;
            i++;
        }else if(!strcmp("-log", argv[i])){
            pM->pEmu->log_level = LOGLV_NOTICE; 
        }else if(!strcmp("-8086", argv[i])){
            pM->pEmu->emu_cpu = EMU_CPU_8086;
        }else if(!strcmp("-80186", argv[i]) || !strcmp("-186", argv[i])){
            pM->pEmu->emu_cpu = EMU_CPU_80186;
        }else if(!strcmp("-80386", argv[i]) || !strcmp("-386", argv[i])){
            pM->pEmu->emu_cpu = EMU_CPU_80386;
        }else{
            filename = argv[i];
            fp = fopen(filename, "rb");
            if( fp == NULL ){
                fprintf(stderr, " Failed to open \"%s\" \n", filename);
                exit(1);
            }
            if( fread(pM->pMemIo->stMem.mem+load_pos, 1, EMU_MEM_SIZE-load_pos, fp) < 1 ){
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

    //-----------------------------------------------------
    // Register a signal handler for handling Ctrl+C
    //-----------------------------------------------------
    if ( signal(SIGINT, sig_handler_SIGINT) == SIG_ERR ) {
        logfile_printf(LOGCAT_EMU | LOGLV_ERROR, "failed to register a signal hander.\n");
        logfile_close();
        shutdownEmulator(pM);
    }
    //-----------------------------------------------------


    return 0;
}



void shutdownEmulator( struct stMachineState *pM ){
    restoreTerminalSetting(pM);
    termResetSettingForExit();

    exit(1);
}

void initTerminalSetting(struct stMachineState *pM){
    struct termios settings;

    settings = termOrigSettings;
    settings.c_lflag &= ~(ECHO|ICANON);  /* without input echo, and unbuffered */
    settings.c_cc[VTIME] = 0;
    settings.c_cc[VMIN] = 1;
    tcsetattr(0,TCSANOW,&settings);
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
}

void saveTerminalSetting(struct stMachineState *pM){
    tcgetattr(0, &termOrigSettings);
}

void restoreTerminalSetting(struct stMachineState *pM){
    tcsetattr(0, TCSANOW, &termOrigSettings);
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) & (~O_NONBLOCK));
}
