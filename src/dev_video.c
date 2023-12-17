#include <stdio.h>
#include <stdint.h>

#include "i8086.h"
#include "dev_video.h"
#include "logfile.h"

void writeVideoReg(struct stMachineState *pM, uint16_t addr, uint8_t data){
    static uint8_t control_addr = 0;
    static uint16_t cursor_pos = 0;

    switch( addr & 0x0f ){
        case 4:
            control_addr = data;
            break;
        case 5:
            switch( control_addr ){
                case 0x0e: 
                    cursor_pos = (cursor_pos&0x00ff) + (((uint16_t)data)<<8);
                    break;
                case 0x0f: 
                    cursor_pos = (cursor_pos&0xff00) + ((uint16_t)data);
                    break;
                default:
                    logfile_printf(LOGCAT_IO_VIDEO | LOGLV_INFO, "Video: writing reg 0x%02x with data = 0x%02x\n", addr, data);
            }
            break;
        default:
            logfile_printf(LOGCAT_IO_VIDEO | LOGLV_INFO, "Video: addr 0x%02x data = 0x%02x\n", addr, pM->mem.ioTimer.counter[addr]);
            break;
    }
}

uint8_t readVideoReg(struct stMachineState *pM, uint16_t addr){
   return 0;
}