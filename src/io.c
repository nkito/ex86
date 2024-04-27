#include <stdio.h>
#include <stdint.h>

#include "i8086.h"
#include "dev_timer.h"
#include "dev_PIC.h"
#include "dev_video.h"
#include "dev_FDC.h"
#include "dev_DMAC.h"
#include "dev_UART.h"
#include "dev_CMOS.h"
#include "io.h"
#include "logfile.h"

#include "emu_interface.h"
#include "ExInst_common.h"


uint16_t readIOWord (struct stMachineState *pM, uint32_t addr){
    return (readIOByte(pM, addr) | (((uint16_t)readIOByte(pM, addr+1))<<8));
}

uint32_t readIODoubleWord(struct stMachineState *pM, uint32_t addr){
    return (readIOByte(pM, addr) | (((uint32_t)readIOByte(pM, addr+1))<<8) | (((uint32_t)readIOByte(pM, addr+2))<<16) | (((uint32_t)readIOByte(pM, addr+3))<<24));
}

void writeIOWord (struct stMachineState *pM, uint32_t addr, uint16_t data){
    if( addr == IOADDR_EMU_INTERFACE ){
        callEmuInterfacePort(pM, data);
    }else{
        writeIOByte(pM, addr  , data&0xff);
        writeIOByte(pM, addr+1, data>>8 );
    }
}

void writeIODoubleWord (struct stMachineState *pM, uint32_t addr, uint32_t data){
    if( addr == IOADDR_EMU_INTERFACE ){
        callEmuInterfacePort(pM, data);
    }else{
        writeIOByte(pM, addr  ,  data     &0xff);
        writeIOByte(pM, addr+1, (data>> 8)&0xff);
        writeIOByte(pM, addr+2, (data>>16)&0xff);
        writeIOByte(pM, addr+3, (data>>24)&0xff);
    }
}


#define ASCII_LF  0x0a
#define ASCII_CR  0x0d
#define ASCII_DEL 0x7f
#define ASCII_BS  0x08

#define LSR 5
#define TBR 0
#define RBR 0
#define SIO_TX_EMPTY     0x40
#define SIO_TX_BUF_EMPTY 0x20
#define SIO_RX_BUF_FULL  0x01

uint8_t  readIOByte (struct stMachineState *pM, uint32_t addr){
    static uint32_t unhandled = 0;

    if( MODE_PROTECTED ){
        if( ((pM->reg.cpl > IOPL(REG_EFLAGS)) || MODE_PROTECTEDVM)  && readTSSIOMapBit(pM, addr) ){
            ENTER_GP(0);
        }
    }

    // UART
    if(addr == 0 || addr==1){
        return readUARTReg(pM, &(pM->mem.ioUART1), addr);
    }

    // UART
    if(addr >= IOADDR_COM1_BASE && addr < IOADDR_COM1_BASE+IOADDR_UART_SIZE){
        return readUARTReg(pM, &(pM->mem.ioUART1), addr);
    }

    // Timer (i8254)
    if(addr >= IOADDR_TCU_BASE && addr <= IOADDR_TCU_BASE+3){
        return readTimerReg(pM, addr - IOADDR_TCU_BASE);
    }

    // FDC
    if(addr >= IOADDR_FDC_BASE && addr < IOADDR_FDC_BASE + IOADDR_FDC_SIZE ){
        return readFDCReg(pM, &(pM->mem.ioFDC), addr);
    }

    // DMAC
    if(addr >= IOADDR_DMAC0_BASE && addr < IOADDR_DMAC0_BASE + IOADDR_DMAC_SIZE ){
        return readDMACReg(pM, &(pM->mem.ioDMAC), addr);
    }

    // intterupt controller (i8259A x2)
    if(addr == IOADDR_PIC_MAIN_BASE || addr == IOADDR_PIC_MAIN_BASE+1){
        return readPICReg(pM, addr);

    }else if(addr == IOADDR_PIC_SUB_BASE || addr == IOADDR_PIC_SUB_BASE+1){
        return readPICReg(pM, addr);
    }

    // Video controller
    if(addr >= VIDEO_IOADDR_BASE && addr < VIDEO_IOADDR_BASE + VIDEO_IOADDR_SIZE){
        return readVideoReg(pM, addr);
    }

    // System Control Register A
    if( addr == IOADDR_SYSCTRL_A ){
        return pM->mem.ioSysCtrlA;
    }

    // System Control Register B
    if( addr == IOADDR_SYSCTRL_B ){
        return pM->mem.ioSysCtrlB;
    }

    // CMOS
    if( addr >= IOADDR_CMOS_BASE && addr < IOADDR_CMOS_BASE + IOADDR_CMOS_SIZE ){
        return readCMOSReg(pM, &(pM->mem.ioCMOS), addr);
    }

    if(unhandled != addr){
        logfile_printf(LOGCAT_IO_MISC | LOGLV_NOTICE, "Unhandled I/O access: port 0x%x was read\n", addr);
        unhandled = addr;
    }

    return addr;
    //return 0x0;
}


void writeIOByte (struct stMachineState *pM, uint32_t addr, uint8_t  data){

    if( MODE_PROTECTED ){
        if( ((pM->reg.cpl > IOPL(REG_EFLAGS)) || MODE_PROTECTEDVM)  && readTSSIOMapBit(pM, addr) ){
            ENTER_GP(0);
        }
    }

    if(addr == 0){ // addr==0 may conflict with DMA ch0, buf ch0 is not used (because it is reserved for DRAM)
        // UART
        writeUARTReg(pM, &(pM->mem.ioUART1), addr, data);
    }else if(addr >= IOADDR_COM1_BASE && addr < IOADDR_COM1_BASE+IOADDR_UART_SIZE){
        // UART
        writeUARTReg(pM, &(pM->mem.ioUART1), addr, data);

    }else if(addr >= IOADDR_TCU_BASE && addr <= IOADDR_TCU_BASE+3){
        writeTimerReg(pM, addr - IOADDR_TCU_BASE, data);

    }else if(addr == IOADDR_PIC_MAIN_BASE || addr == IOADDR_PIC_MAIN_BASE+1){
        writePICReg(pM, addr, data);

    }else if(addr == IOADDR_PIC_SUB_BASE || addr == IOADDR_PIC_SUB_BASE+1){
        writePICReg(pM, addr, data);

    }else if( addr >= VIDEO_IOADDR_BASE && addr < VIDEO_IOADDR_BASE + VIDEO_IOADDR_SIZE){
        writeVideoReg(pM, addr, data);

    }else if( addr >= IOADDR_FDC_BASE && addr < IOADDR_FDC_BASE + IOADDR_FDC_SIZE){
        writeFDCReg(pM, &(pM->mem.ioFDC), addr, data);

    }else if( addr >= IOADDR_DMAC0_BASE && addr < IOADDR_DMAC0_BASE + IOADDR_DMAC_SIZE){
        writeDMACReg(pM, &(pM->mem.ioDMAC), addr, data);

    }else if( addr != 0x80 && addr >= IOADDR_DMAC_PAGEADDR_BASE && addr < IOADDR_DMAC_PAGEADDR_BASE + IOADDR_DMAC_PAGEADDR_SIZE){
        setDMAPageAddr(pM, &(pM->mem.ioDMAPage), addr, data);

    }else if(addr == IOADDR_SYSCTRL_A ){
        pM->mem.ioSysCtrlA = data;

        // bit2 = 1: A20 is active (access above 1MB is possible)
        if( (data & 0x2) ){
            pM->mem.a20m = 0; // a20 line is enabled 
        }else{
            pM->mem.a20m = 1; // a20 line is disabled (masked)
        }

    }else if(addr == IOADDR_SYSCTRL_B ){
        pM->mem.ioSysCtrlB = data;
        if( (data & 0x3) == 0x03 ){
            printf("\a");
        }
    }else if( addr >= IOADDR_CMOS_BASE && addr < IOADDR_CMOS_BASE + IOADDR_CMOS_SIZE ){
        writeCMOSReg(pM, &(pM->mem.ioCMOS), addr, data);

        // pM->mem.ioCMOS.reg_addr = data;
    }else{
        if( addr != 0x80 ){
            logfile_printf(LOGCAT_IO_MISC | LOGLV_NOTICE, "Unhandled I/O access: port 0x%x was written with 0x%x [%x:%x]\n", addr, data, REG_CS, REG_EIP);
        }
    }
}
