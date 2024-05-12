#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "i8086.h"
#include "dev_timer.h"
#include "logfile.h"

#define GET_RW_MODE(c) (((c)&I8254_CONTROL_RW_MASK )>>I8254_CONTROL_RW_BIT )

void writeTimerReg(struct stMachineState *pM, uint8_t addr, uint8_t data){
    unsigned int target_ch, rwmode;

    logfile_printf(LOGCAT_IO_TIMER | LOGLV_WARNING, "Timer: addr %x is write 0x%x\n", addr, data);

    switch( addr ){
        case 3:
            target_ch = ((data&I8254_CONTROL_SEL_MASK)>>I8254_CONTROL_SEL_BIT);
            rwmode    = GET_RW_MODE(data);

            pM->pMemIo->ioTimer.control[ target_ch ] = (data&0x3f);
            pM->pMemIo->ioTimer.rwCount[ target_ch ] = 0;
            logfile_printf(LOGCAT_IO_TIMER | LOGLV_WARNING, "Timer: Timer%d control is configured as 0x%x\n", target_ch, pM->pMemIo->ioTimer.control[target_ch]);

            if( rwmode == 0 ){
                // Counter Latch Command (for Read Operation)
                pM->pMemIo->ioTimer.counter_shadow[ target_ch ] = pM->pMemIo->ioTimer.dummy_counter[ target_ch ];

                if( pM->pMemIo->ioTimer.dummy_counter[ target_ch ] == 0 ){
                    if( pM->pMemIo->ioTimer.counter[ target_ch ] == 0 ){
                        pM->pMemIo->ioTimer.dummy_counter[ target_ch ] = 0;
                    }else{
                        pM->pMemIo->ioTimer.dummy_counter[ target_ch ] = pM->pMemIo->ioTimer.counter[ target_ch ] - 1;
                    }
                }else{
                    pM->pMemIo->ioTimer.dummy_counter[ target_ch ] -= 1;
                }
            }
            break;
        case 0:
        case 1:
        case 2:
            rwmode = GET_RW_MODE(pM->pMemIo->ioTimer.control[addr]);

            if( rwmode == 1 ){
                // Read/Write least significant byte only
                pM->pMemIo->ioTimer.counter       [addr] = 
                pM->pMemIo->ioTimer.counter_shadow[addr] = (((uint16_t)data)<<0);
                
                logfile_printf(LOGCAT_IO_TIMER | LOGLV_WARNING, "Timer: Timer%d counter = %d\n", addr, pM->pMemIo->ioTimer.counter[addr]);
            }else if( rwmode == 2 ){
                // Read/Write most significant byte only
                pM->pMemIo->ioTimer.counter       [addr]  = 
                pM->pMemIo->ioTimer.counter_shadow[addr]  = (((uint16_t)data)<<8);

                logfile_printf(LOGCAT_IO_TIMER | LOGLV_INFO, "Timer: Timer%d counter = %d\n", addr, pM->pMemIo->ioTimer.counter[addr]);
            }else{
                // Read/Write least significant byte first, then most significant byte
                if( pM->pMemIo->ioTimer.rwCount[addr] == 0 ){
                    pM->pMemIo->ioTimer.counter_shadow[addr]  = (((uint16_t)data)<<0);
                    pM->pMemIo->ioTimer.rwCount[addr]++;
                }else{
                    pM->pMemIo->ioTimer.counter_shadow[addr] |= (((uint16_t)data)<<8);
                    pM->pMemIo->ioTimer.counter[addr]  = pM->pMemIo->ioTimer.counter_shadow[addr];
                    logfile_printf(LOGCAT_IO_TIMER | LOGLV_INFO, "Timer: Timer%d counter = %d\n", addr, pM->pMemIo->ioTimer.counter[addr]);
                    pM->pMemIo->ioTimer.rwCount[addr]=0;
                }
            }

            if( addr == 0 && pM->pMemIo->ioTimer.rwCount[addr] == 0 ){
                logfile_printf(LOGCAT_IO_TIMER | LOGLV_INFO, "Timer: Timer%d handler restart \n", addr);

                long new_interval = (pM->pMemIo->ioTimer.counter[0] * (1000000/100)) / (I8254_CLOCK_FREQ/100);
                setTimerInUSec(pM, new_interval);
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
            logfile_printf(LOGCAT_IO_TIMER | LOGLV_INFO, "Timer: Timer%d counter value is read (0x%x) \n", addr, pM->pMemIo->ioTimer.counter_shadow[addr]);
            rwmode = GET_RW_MODE(pM->pMemIo->ioTimer.control[addr]);

            if( rwmode == 1 ){
                // Read/Write least significant byte only
                return (pM->pMemIo->ioTimer.counter_shadow[addr] & 0xff);
            }else if( rwmode == 2 ){
                // Read/Write most significant byte only
                return ((pM->pMemIo->ioTimer.counter_shadow[addr]>>8) & 0xff);
            }else{
                // Read/Write least significant byte first, then most significant byte
                if( pM->pMemIo->ioTimer.rwCount[addr] == 0 ){
                    pM->pMemIo->ioTimer.rwCount[addr]++;
                    logfile_printf(LOGCAT_IO_TIMER | LOGLV_WARNING, "Timer: addr %d is read %x \n", addr, (pM->pMemIo->ioTimer.counter_shadow[addr] & 0xff));
                    return (pM->pMemIo->ioTimer.counter_shadow[addr] & 0xff);
                }else{
                    pM->pMemIo->ioTimer.rwCount[addr]=0;
                    logfile_printf(LOGCAT_IO_TIMER | LOGLV_WARNING, "Timer: addr %d is read %x \n", addr, ((pM->pMemIo->ioTimer.counter_shadow[addr]>>8) & 0xff));
                    return ((pM->pMemIo->ioTimer.counter_shadow[addr]>>8) & 0xff);
                }
            }

            break;
    }
    return 0;
}