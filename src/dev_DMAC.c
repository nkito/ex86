#include <stdio.h>
#include <stdint.h>

#include "i8086.h"
#include "dev_DMAC.h"
#include "logfile.h"

#include "ExInst_common.h"


void setDMAPageAddr(struct stMachineState *pM, struct periDMAPageAddr *pPage, uint32_t addr, uint8_t data){
    switch(addr){
        case DMA_PAGEADDR_CH0: pPage->addr[0] = data; break;
        case DMA_PAGEADDR_CH1: pPage->addr[1] = data; break;
        case DMA_PAGEADDR_CH2: pPage->addr[2] = data; break;
        case DMA_PAGEADDR_CH3: pPage->addr[3] = data; break;
        case DMA_PAGEADDR_CH4: pPage->addr[4] = data; break;
        case DMA_PAGEADDR_CH5: pPage->addr[5] = data; break;
        case DMA_PAGEADDR_CH6: pPage->addr[6] = data; break;
        case DMA_PAGEADDR_CH7: pPage->addr[7] = data; break;
        default: break;
    }

    logfile_printf(LOGCAT_IO_DMAC | LOGLV_INFO, "DMA Page: ch0 %x, ch1 %x, ch2 %x, ch3 %x, ch4 %x, ch5 %x, ch6 %x, ch7 %x. (addr %x, data %x)\n",
        pPage->addr[0], pPage->addr[1], pPage->addr[2], pPage->addr[3],
        pPage->addr[4], pPage->addr[5], pPage->addr[6], pPage->addr[7], addr, data);

}

uint8_t readDMACReg(struct stMachineState *pM, struct periDMAC *pDMAC, uint32_t addr){

    if( (addr&IOADDR_DMAC_MASK) ==  DMAC_REG_STATUS ){
        logfile_printf(LOGCAT_IO_DMAC | LOGLV_INFO, "DMAC: status register was read\n");
        return 0x00;
    }
    if( (addr&IOADDR_DMAC_MASK) ==  DMAC_REG_TEMP ){
        logfile_printf(LOGCAT_IO_DMAC | LOGLV_INFO, "DMAC: temporary register was read\n");
        return 0x00;
    }

    logfile_printf(LOGCAT_IO_DMAC | LOGLV_INFO, "DMAC: unknown register was read (addr: %x)\n", addr);

    return 0;
}

void writeDMACReg  (struct stMachineState *pM, struct periDMAC *pDMAC, uint32_t addr, uint8_t data){

    if( (addr&IOADDR_DMAC_MASK) ==  DMAC_REG_CMD ){
        logfile_printf(LOGCAT_IO_DMAC | LOGLV_INFO, "DMAC: command register was wrote %x\n", data);

    }else if( (addr&IOADDR_DMAC_MASK) ==  DMAC_REG_MODE ){
        logfile_printf(LOGCAT_IO_DMAC | LOGLV_INFO, "DMAC: mode register was wrote %x\n", data);

    }else if( (addr&IOADDR_DMAC_MASK) ==  DMAC_REG_REQ ){
        logfile_printf(LOGCAT_IO_DMAC | LOGLV_INFO, "DMAC: request register was wrote %x\n", data);

    }else if( (addr&IOADDR_DMAC_MASK) ==  DMAC_REG_MASK ){
        logfile_printf(LOGCAT_IO_DMAC | LOGLV_INFO, "DMAC: mask register was wrote %x\n", data);

    }else if( (addr&IOADDR_DMAC_MASK) ==  DMAC_REG_MASK_SR ){
        logfile_printf(LOGCAT_IO_DMAC | LOGLV_INFO, "DMAC: mask s/r register was wrote %x\n", data);

    }else if( (addr&IOADDR_DMAC_MASK) ==  4 ){
        if( pDMAC->cnt[2] == 0 ){
            pDMAC->addr[2] = data;
        }else{
            pDMAC->addr[2] |= (data << 8);
            pDMAC->addr[2] &= 0xffff;
        }

        if( pDMAC->cnt[2] != 0 ){
            logfile_printf(LOGCAT_IO_DMAC | LOGLV_INFO, "DMAC: CH2 addr was refreshed %x, %x (data %x)\n", pDMAC->addr[2], pDMAC->cnt[2], data);
        }
        pDMAC->cnt[2] = ((pDMAC->cnt[2]+1)&1);
    }else{
        logfile_printf(LOGCAT_IO_DMAC | LOGLV_WARNING, "DMAC: write for unknown register (addr: %x, data %x)\n", addr, data);
    }

}
