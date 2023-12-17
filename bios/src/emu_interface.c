/*
Emulator interface routines.

Programs in emulators can communicate with the emulator through OUT instruction.

*/
#include <stdio.h>
#include "basicio.h"

#include "terminal.h"
#include "timer.h"
#include "system.h"

#include "emu_interface.h"



uint8_t execEmuCMD(uint8_t *buf){
    systemOutWord( (unsigned short)buf, IOADDR_EMU_INTERFACE);

    return buf[2];
}

uint8_t emuPrintMessage(uint8_t num){
    uint8_t buf[3+1];

    buf[0] = EMU_INTERFACE_MAGIC_NUM;
    buf[1] = EMU_INTERFACE_CMD_PRINT_MSG;
    buf[2] = EMU_INTERFACE_RESULT_IOERROR;
    buf[3] = num;

    return execEmuCMD(buf);
}

uint32_t emuGetMemoryCapacity(void){
    uint8_t buf[3+4];
    uint32_t cap;

    buf[0] = EMU_INTERFACE_MAGIC_NUM;
    buf[1] = EMU_INTERFACE_CMD_GET_MEM_CAPACITY;
    buf[2] = EMU_INTERFACE_RESULT_IOERROR;

    execEmuCMD(buf);

    cap = buf[3];
    cap|= (((uint32_t)buf[4]) <<  8);
    cap|= (((uint32_t)buf[5]) << 16);
    cap|= (((uint32_t)buf[6]) << 24);

    return cap;
}

uint16_t emuGetCPUType(void){
    uint8_t buf[3+2];
    uint16_t cpu;

    buf[0] = EMU_INTERFACE_MAGIC_NUM;
    buf[1] = EMU_INTERFACE_CMD_GET_CPU_TYPE;
    buf[2] = EMU_INTERFACE_RESULT_IOERROR;

    execEmuCMD(buf);

    cpu = buf[3];
    cpu|= (((uint32_t)buf[4]) <<  8);

    return cpu;
}

uint8_t emuLogMessage(uint16_t buf_seg, uint16_t buf_offset){
    uint8_t buf[3+4];

    buf[0] = EMU_INTERFACE_MAGIC_NUM;
    buf[1] = EMU_INTERFACE_CMD_LOG_MESSAGE;
    buf[2] = EMU_INTERFACE_RESULT_IOERROR;
    buf[3] = ((buf_seg>> 0)&0xff);
    buf[4] = ((buf_seg>> 8)&0xff);

    buf[5] = ((buf_offset>> 0)&0xff);
    buf[6] = ((buf_offset>> 8)&0xff);

    return execEmuCMD(buf);
}

uint8_t emuGetTime(uint16_t *pyear, uint8_t *pmonth, uint8_t *pday, uint8_t *phour, uint8_t *pmin, uint8_t *psec){
    uint8_t buf[3+6];

    buf[0] = EMU_INTERFACE_MAGIC_NUM;
    buf[1] = EMU_INTERFACE_CMD_GET_TIME;
    buf[2] = EMU_INTERFACE_RESULT_IOERROR;

    execEmuCMD(buf);

    if(pyear  != NULL){ *pyear  = buf[3] + 1900; }
    if(pmonth != NULL){ *pmonth = buf[4]; }
    if(pday   != NULL){ *pday   = buf[5]; }

    if(phour  != NULL){ *phour  = buf[6]; }
    if(pmin   != NULL){ *pmin   = buf[7]; }
    if(psec   != NULL){ *psec   = buf[8]; }

    return buf[2];
}

uint8_t emuGetDriveSize(uint8_t drive, uint32_t *size){
    uint8_t buf[3+5];

    buf[0] = EMU_INTERFACE_MAGIC_NUM;
    buf[1] = EMU_INTERFACE_CMD_GET_DRIVE_SIZE;
    buf[2] = EMU_INTERFACE_RESULT_IOERROR;
    buf[3] = drive;

    execEmuCMD(buf);

    *size = buf[7]; *size <<= 8;
    *size|= buf[6]; *size <<= 8;
    *size|= buf[5]; *size <<= 8;
    *size|= buf[4];

    return buf[2];
}

uint8_t emuReadDriveSector(uint8_t drive, uint32_t sector, uint16_t buf_seg, uint16_t buf_offset){
    uint8_t buf[3+9];

    buf[0] = EMU_INTERFACE_MAGIC_NUM;
    buf[1] = EMU_INTERFACE_CMD_READ_DRIVE_SECTOR;
    buf[2] = EMU_INTERFACE_RESULT_IOERROR;

    buf[3] = drive;

    buf[4] = ((sector>> 0)&0xff);
    buf[5] = ((sector>> 8)&0xff);
    buf[6] = ((sector>>16)&0xff);
    buf[7] = ((sector>>24)&0xff);

    buf[8] = ((buf_seg>> 0)&0xff);
    buf[9] = ((buf_seg>> 8)&0xff);

    buf[10] = ((buf_offset>> 0)&0xff);
    buf[11] = ((buf_offset>> 8)&0xff);

    return execEmuCMD(buf);
}

uint8_t emuWriteDriveSector(uint8_t drive, uint32_t sector, uint16_t buf_seg, uint16_t buf_offset){
    uint8_t buf[3+9];

    buf[0] = EMU_INTERFACE_MAGIC_NUM;
    buf[1] = EMU_INTERFACE_CMD_WRITE_DRIVE_SECTOR;
    buf[2] = EMU_INTERFACE_RESULT_IOERROR;

    buf[3] = drive;

    buf[4] = ((sector>> 0)&0xff);
    buf[5] = ((sector>> 8)&0xff);
    buf[6] = ((sector>>16)&0xff);
    buf[7] = ((sector>>24)&0xff);

    buf[8] = ((buf_seg>> 0)&0xff);
    buf[9] = ((buf_seg>> 8)&0xff);

    buf[10] = ((buf_offset>> 0)&0xff);
    buf[11] = ((buf_offset>> 8)&0xff);

    return execEmuCMD(buf);
}
