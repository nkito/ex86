#include <stdio.h>
#include <stdint.h>

#include "i8086.h"
#include "dev_CMOS.h"
#include "logfile.h"

#include "ExInst_common.h"


#define LOGLEVEL_CMOS_INFO   (LOGCAT_IO_CMOS | LOGLV_INFO)
#define LOGLEVEL_CMOS_NOTICE (LOGCAT_IO_CMOS | LOGLV_NOTICE)

uint8_t int8toBCD(uint8_t val){
    return (((val/10)%10)<<4) + (val%10);
}

uint8_t readCMOSReg(struct stMachineState *pM, struct stIO_CMOS *pCMOS, uint32_t addr){
    uint8_t res;

    if( (addr&IOADDR_CMOS_MASK) != 1 ){
        logfile_printf(LOGLEVEL_CMOS_NOTICE,"CMOS: unknown register was read (addr: %x)\n", addr);
        return 0;
    }

    if( pCMOS->reg_addr == 0x10 ){
        // FDD
        // higher-nibble : 1st FDD (0x40 : 1.44MB 3.5 inch)
        // lower-nibble  : 2nd FDD (0x04 : 1.44MB 3.5 inch)
        return 0x40;
    }
    if( pCMOS->reg_addr == 0x12 ){
        // HDD
        // higher-nibble : 1st HDD (0x00 : none)
        // lower-nibble  : 2nd HDD (0x00 : none)
        return 0x10;
    }

    // RTC
    if( pCMOS->reg_addr == 0x00 ){ // Seconds
        res = int8toBCD(pCMOS->prevTM.tm_sec);
        return res;
    }
    if( pCMOS->reg_addr == 0x02 ){ // Minites
        res = int8toBCD(pCMOS->prevTM.tm_min);
        return res;
    }
    if( pCMOS->reg_addr == 0x04 ){ // Hours
        res =  int8toBCD(pCMOS->prevTM.tm_hour);
        return res;
    }
    if( pCMOS->reg_addr == 0x06 ){ // Weekday
        res =  int8toBCD(pCMOS->prevTM.tm_wday + 1); // 1=Sunday
        return res;
    }
    if( pCMOS->reg_addr == 0x07 ){ // Day of Month
        res =  int8toBCD(pCMOS->prevTM.tm_mday);
        return res;
    }
    if( pCMOS->reg_addr == 0x08 ){ // Month
        res =  int8toBCD(pCMOS->prevTM.tm_mon + 1);
        return res;
    }
    if( pCMOS->reg_addr == 0x09 ){ // Year
        res = int8toBCD(pCMOS->prevTM.tm_year % 100);
        return res;
    }

    if( pCMOS->reg_addr == 0x0A ){ // RTC status register A
        uint8_t flag = 0;
        uint8_t sec = pCMOS->prevTM.tm_sec;
        time_t  t;

        time(&t);
        localtime_r(&t, &(pCMOS->prevTM));
        if( sec != pCMOS->prevTM.tm_sec ){
            flag = 0x80; // update flag
        }
        res =  (flag | 2<<4 | 6);
        return res;
    }
    if( pCMOS->reg_addr == 0x0B ){ // RTC status register B
        res = (0x2); // 24 hour mode, BCD mode
        return res;
    }

    logfile_printf(LOGLEVEL_CMOS_NOTICE,"CMOS: unknown internal register was read (CMOS addr: %x)\n", pCMOS->reg_addr);

    return 0;
}

#define BCD2BIN(x) ( (((x)>>4)&0xf)*10 + (((x)>>0)&0xf) )

void writeCMOSReg(struct stMachineState *pM, struct stIO_CMOS *pCMOS, uint32_t addr, uint8_t data){
    time_t  t;
    struct tm reqTim;

    if( (addr&IOADDR_CMOS_MASK) == 1 ){
        logfile_printf(LOGLEVEL_CMOS_NOTICE,"CMOS: writing 0x%x for CMOS register (CMOS addr: %x)\n", data, pCMOS->reg_addr);

        time(&t);
        localtime_r(&t, &reqTim);

        switch( pCMOS->reg_addr ){
            case 0x00: reqTim.tm_sec  = BCD2BIN(data)    ; break;
            case 0x02: reqTim.tm_min  = BCD2BIN(data)    ; break;
            case 0x04: reqTim.tm_hour = BCD2BIN(data)    ; break;
            case 0x06: reqTim.tm_wday = BCD2BIN(data) - 1; break;
            case 0x07: reqTim.tm_mday = BCD2BIN(data)    ; break;
            case 0x08: reqTim.tm_mon  = BCD2BIN(data) - 1; break;
            case 0x09: reqTim.tm_year = (BCD2BIN(((int)data)) + 2000) - 1900; break;
            // case 0x32: reqTim.tm_sec = BCD2BIN(data); break;
        }

        setSystemTime(pM, &reqTim);
        return ;
    }

    pCMOS->reg_addr = data;

    time(&t);
    localtime_r(&t, &(pCMOS->prevTM));
}
