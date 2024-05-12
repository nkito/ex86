#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "i8086.h"
#include "dev_PIC.h"
#include "logfile.h"
#include "io.h"

void writePICReg(struct stMachineState *pM, uint8_t addr, uint8_t data){
    struct periPIC *pPIC;
    int isMain = 0;

    if(  addr == IOADDR_PIC_MAIN_BASE || addr == IOADDR_PIC_MAIN_BASE+1 ){
        pPIC = &(pM->pMemIo->ioPICmain);
        isMain = 1;
    }else if(  addr == IOADDR_PIC_SUB_BASE || addr == IOADDR_PIC_SUB_BASE+1 ){
        pPIC = &(pM->pMemIo->ioPICsub);
    }else{
        return;
    }

    if( 0 == (addr & 1) ){
        if( data & 0x10 ){
            pPIC->acc_idx = 2;
            pPIC->icw1 = data;
        }else if( (data&0x18) == 0x00 ){
            pPIC->acc_idx = 0;
            pPIC->ocw2 = data;
        }else if( (data&0x18) == 0x08 ){
            pPIC->acc_idx = 0;
            pPIC->ocw3 = data;
        }
    }else{
        if(pPIC->acc_idx == 0){
            pPIC->acc_idx = 0;
            pPIC->ocw1 = data;
        }else if(pPIC->acc_idx == 2){
            pPIC->acc_idx = 3;
            pPIC->icw2 = data;
        }else if(pPIC->acc_idx == 3){
            pPIC->acc_idx = ((pPIC->icw1) & 0x1) ? 4 : 0;
            pPIC->icw3 = data;
        }else if(pPIC->acc_idx == 4){
            pPIC->acc_idx = 0;
            pPIC->icw4 = data;
        }
    }

/*
    logfile_printf(LOGLEVEL_INFO,"PIC: %s is configured as  icw1..4: %02x %02x %02x %02x, ocw1..3: %02x %02x %02x,  (0x%x @ %x)\n",
     isMain ? "M":"S", pPIC->icw1, pPIC->icw2, pPIC->icw3, pPIC->icw4, pPIC->ocw1, pPIC->ocw2, pPIC->ocw3, data, addr&1);
*/

}

void updatePICReq(struct stMachineState *pM, uint8_t addr, uint8_t data){

}

uint8_t readPICReg(struct stMachineState *pM, uint8_t addr){
    struct periPIC *pPIC;
    int isMain = 0;

    if(  addr == IOADDR_PIC_MAIN_BASE || addr == IOADDR_PIC_MAIN_BASE+1 ){
        pPIC = &(pM->pMemIo->ioPICmain);
        isMain = 1;
    }else if(  addr == IOADDR_PIC_SUB_BASE || addr == IOADDR_PIC_SUB_BASE+1 ){
        pPIC = &(pM->pMemIo->ioPICsub);
    }else{
        return 0;
    }

//    logfile_printf(LOGLEVEL_INFO,"PIC: %s was read (addr:%x)\n", isMain ? "M":"S", addr&1);

    if( addr & 1){
        return pPIC->ocw1;
    }

    return 1;
}
