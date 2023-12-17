#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>

#include "i8086.h"
#include "dev_timer.h"
#include "logfile.h"


volatile sig_atomic_t tflag = 0;
void timer_handler(int signum){
  switch (signum) {
  case SIGALRM:
    tflag = 1;
    break;

  default:
    break;
  }
}

#define GET_RW_MODE(c) (((c)&I8254_CONTROL_RW_MASK )>>I8254_CONTROL_RW_BIT )

void writeTimerReg(struct stMachineState *pM, uint8_t addr, uint8_t data){
    struct sigaction action, old_action;
    struct itimerval timer, old_timer;
    unsigned int target_ch, rwmode;

    logfile_printf(LOGCAT_IO_TIMER | LOGLV_WARNING, "Timer: addr %x is write 0x%x\n", addr, data);

    switch( addr ){
        case 3:
            target_ch = ((data&I8254_CONTROL_SEL_MASK)>>I8254_CONTROL_SEL_BIT);
            rwmode    = GET_RW_MODE(data);

            pM->mem.ioTimer.control[ target_ch ] = (data&0x3f);
            pM->mem.ioTimer.rwCount[ target_ch ] = 0;
            logfile_printf(LOGCAT_IO_TIMER | LOGLV_WARNING, "Timer: Timer%d control is configured as 0x%x\n", target_ch, pM->mem.ioTimer.control[target_ch]);

            if( rwmode == 0 ){
                // Counter Latch Command (for Read Operation)
                pM->mem.ioTimer.counter_shadow[ target_ch ] =
                    ( pM->mem.ioTimer.counter[ target_ch ] != 0 ) ? 
                        ( pM->mem.ioTimer.dummy_counter[ target_ch ] % pM->mem.ioTimer.counter[ target_ch ] ) : 0;
                pM->mem.ioTimer.dummy_counter[ target_ch ]+=10;
            }
            break;
        case 0:
        case 1:
        case 2:
            rwmode = GET_RW_MODE(pM->mem.ioTimer.control[addr]);

            if( rwmode == 1 ){
                // Read/Write least significant byte only
                pM->mem.ioTimer.counter       [addr] = 
                pM->mem.ioTimer.counter_shadow[addr] = (((uint16_t)data)<<0);
                
                logfile_printf(LOGCAT_IO_TIMER | LOGLV_WARNING, "Timer: Timer%d counter = %d\n", addr, pM->mem.ioTimer.counter[addr]);
            }else if( rwmode == 2 ){
                // Read/Write most significant byte only
                pM->mem.ioTimer.counter       [addr]  = 
                pM->mem.ioTimer.counter_shadow[addr]  = (((uint16_t)data)<<8);

                logfile_printf(LOGCAT_IO_TIMER | LOGLV_INFO, "Timer: Timer%d counter = %d\n", addr, pM->mem.ioTimer.counter[addr]);
            }else{
                // Read/Write least significant byte first, then most significant byte
                if( pM->mem.ioTimer.rwCount[addr] == 0 ){
                    pM->mem.ioTimer.counter_shadow[addr]  = (((uint16_t)data)<<0);
                    pM->mem.ioTimer.rwCount[addr]++;
                }else{
                    pM->mem.ioTimer.counter_shadow[addr] |= (((uint16_t)data)<<8);
                    pM->mem.ioTimer.counter[addr]  = pM->mem.ioTimer.counter_shadow[addr];
                    logfile_printf(LOGCAT_IO_TIMER | LOGLV_INFO, "Timer: Timer%d counter = %d\n", addr, pM->mem.ioTimer.counter[addr]);
                    pM->mem.ioTimer.rwCount[addr]=0;
                }
            }

            if( addr == 0 && pM->mem.ioTimer.rwCount[addr] == 0 ){
                logfile_printf(LOGCAT_IO_TIMER | LOGLV_INFO, "Timer: Timer%d handler restart \n", addr);

                memset(&    action, 0, sizeof(    action));
                memset(&old_action, 0, sizeof(old_action));

                action.sa_handler = timer_handler;
                action.sa_flags = SA_RESTART;

                if (sigaction(SIGALRM, &action, &old_action) == -1) {
                    logfile_printf(LOGCAT_IO_TIMER | LOGLV_ERROR, "Timer: timer setting error (failed to set sigaction for %d)\n", addr);
                    logfile_close();
                    exit(2);
                }

                timer.it_value.tv_sec = 0;
                timer.it_value.tv_usec = pM->mem.ioTimer.counter[0];
                timer.it_interval.tv_sec = 0;
                timer.it_interval.tv_usec = pM->mem.ioTimer.counter[0];
                if (setitimer(ITIMER_REAL, &timer, &old_timer) == -1) {
                    logfile_printf(LOGCAT_IO_TIMER | LOGLV_ERROR, "Timer: timer setting error (%d)\n", addr);
                    logfile_close();
                    exit(2);
                }
            }

            break;
    }
}

uint8_t readTimerReg(struct stMachineState *pM, uint8_t addr){
    unsigned int rwmode;

    logfile_printf(LOGCAT_IO_TIMER | LOGLV_WARNING, "Timer: addr %d is read\n", addr);

    switch( addr ){
        case 3:
            return 0xff;
            break;
        case 0:
        case 1:
        case 2:
            logfile_printf(LOGCAT_IO_TIMER | LOGLV_INFO, "Timer: Timer%d counter value is read (0x%x) \n", addr, pM->mem.ioTimer.counter_shadow[addr]);
            rwmode = GET_RW_MODE(pM->mem.ioTimer.control[addr]);

            if( rwmode == 1 ){
                // Read/Write least significant byte only
                return (pM->mem.ioTimer.counter_shadow[addr] & 0xff);
            }else if( rwmode == 2 ){
                // Read/Write most significant byte only
                return ((pM->mem.ioTimer.counter_shadow[addr]>>8) & 0xff);
            }else{
                // Read/Write least significant byte first, then most significant byte
                if( pM->mem.ioTimer.rwCount[addr] == 0 ){
                    pM->mem.ioTimer.rwCount[addr]++;
                    logfile_printf(LOGCAT_IO_TIMER | LOGLV_WARNING, "Timer: addr %d is read %x \n", addr, (pM->mem.ioTimer.counter_shadow[addr] & 0xff));
                    return (pM->mem.ioTimer.counter_shadow[addr] & 0xff);
                }else{
                    pM->mem.ioTimer.rwCount[addr]=0;
                    logfile_printf(LOGCAT_IO_TIMER | LOGLV_WARNING, "Timer: addr %d is read %x \n", addr, ((pM->mem.ioTimer.counter_shadow[addr]>>8) & 0xff));
                    return ((pM->mem.ioTimer.counter_shadow[addr]>>8) & 0xff);
                }
            }

            break;
    }
    return 0;
}