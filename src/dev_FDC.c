#include <stdio.h>
#include <stdint.h>

#include "i8086.h"
#include "dev_FDC.h"
#include "logfile.h"

#include "ExInst_common.h"

#define LOGLEVEL_FDC_INFO   (LOGCAT_IO_FDC | LOGLV_INFO)
#define LOGLEVEL_FDC_NOTICE (LOGCAT_IO_FDC | LOGLV_NOTICE)


void readFDD(uint8_t *pCylinder, uint8_t *pHead, uint8_t *pSector, uint8_t *buf512){
    FILE *fp = NULL;
    size_t count;

    unsigned int nsector   = 18;
    unsigned int nhead     = 2;
//    unsigned int ncylinder = 80;

    unsigned long pos;

    pos = ((*pCylinder*2 + *pHead)*18UL + *pSector - 1);

    fp = fopen("driveRoot.img", "rb");

    if( fp == NULL ) return ;
    if( 0 != fseek(fp, ((unsigned long)pos)*512, SEEK_SET) ){
        fclose(fp);
        logfile_printf(LOGLEVEL_FDC_NOTICE,"FDC read: cannot read a sector\n");
        return ;
    }
    count = fread(buf512, 1, 512, fp);
    fclose(fp);
    if( count != 512 ){
        logfile_printf(LOGLEVEL_FDC_NOTICE,"FDC read: cannot read a sector\n");
    }

    (*pSector)++;
    if(*pSector >  nsector){ *pSector = 1; (*pHead)++; }
    if(*pHead   >= nhead  ){ *pHead = 0; (*pCylinder)++; }

    return;
}

uint8_t readFDCReg(struct stMachineState *pM, struct periFDC *pFDC, uint32_t addr){

    if( (addr&IOADDR_FDC_MASK) ==  FDC_STATUS_A ){
        logfile_printf(LOGLEVEL_FDC_INFO,"FDC I/O access: status A was read\n");
        return 0x00;
    }
    if( (addr&IOADDR_FDC_MASK) == FDC_STATUS_B ){
        logfile_printf(LOGLEVEL_FDC_INFO,"FDC I/O access: status B was read\n");
        return 0x00;
    }
    if( (addr&IOADDR_FDC_MASK) == FDC_DIR ){
        logfile_printf(LOGLEVEL_FDC_INFO,"FDC I/O access: digital input register (DIR) was read\n");
        return 0x00;
    }

    if( (addr&IOADDR_FDC_MASK) == FDC_MSR ){
        uint8_t result = 0x80; // STATUS_READY
        if( pM->mem.ioFDC.resp_cnt < pM->mem.ioFDC.resp_len ){
            result |= 0x40; // STATUS_DIR
            result |= 0x10; // STATUS_BUSY
        }
        logfile_printf(LOGLEVEL_FDC_INFO, "FDC I/O access: main status register (MSR) was read: %x\n", result);
        return result;
    }
    if( (addr&IOADDR_FDC_MASK) == FDC_DATA_FIFO ){
        logfile_printf(LOGLEVEL_FDC_INFO,"FDC I/O access: FIFO was read\n");
        pM->mem.ioFDC.resp_cnt++;
        if( pM->mem.ioFDC.resp_cnt > pM->mem.ioFDC.resp_len ){
            pM->mem.ioFDC.resp_cnt = pM->mem.ioFDC.resp_len;
        }
        return pM->mem.ioFDC.fifo[ pM->mem.ioFDC.resp_cnt - 1 ];
    }

    return 0;
}

void writeFDCReg(struct stMachineState *pM, struct periFDC *pFDC, uint32_t addr, uint8_t data){

    if( (addr&IOADDR_FDC_MASK) == FDC_DATARATE_SEL ){
        /*
        pM->mem.ioFDC.irq = 1;
        pM->mem.ioFDC.fifo_pointer = 0;
        */
        logfile_printf(LOGLEVEL_FDC_NOTICE,"FDC I/O access: datarate selector register (DSR) was wrote %x\n", data);
        if( data & 0x80 ){
            // reset
            pM->mem.ioFDC.irq = 1;
            pM->mem.ioFDC.fifo_pointer = 0;            
        }
    }else if( (addr&IOADDR_FDC_MASK) == FDC_DOR ){
        if( (pM->mem.ioFDC.dor & 0x08) &&    // IRQ/DMA is enabled
            (data & 0x04)==0                 // reset state
        ){
            pM->mem.ioFDC.irq = 1;
            pM->mem.ioFDC.fifo_pointer = 0;
        }

        if( data == 0x0c && pM->mem.ioFDC.fifo_pointer != 0 ){
            pM->mem.ioFDC.fifo_pointer = 0;
        }

        pM->mem.ioFDC.resp_len = 0;
        pM->mem.ioFDC.resp_cnt = 0;
        pM->mem.ioFDC.dor      = data;
        logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC I/O access: digital output register (DOR) was wrote %x\n", data);
    }else if( (addr&IOADDR_FDC_MASK) == FDC_DATA_FIFO ){
        pM->mem.ioFDC.fifo[ pM->mem.ioFDC.fifo_pointer % sizeof(pM->mem.ioFDC.fifo) ] = data;
        pM->mem.ioFDC.fifo_pointer ++;

        logfile_printf(LOGLEVEL_FDC_INFO, "FDC I/O access: FIFO was wrote %x\n", data);

        if( pM->mem.ioFDC.fifo[0] == FDC_CMD_SENSEI ){
            if( pM->mem.ioFDC.fifo_pointer == 1 ){
                pM->mem.ioFDC.resp_cnt = 0;
                pM->mem.ioFDC.resp_len = 2;
                pM->mem.ioFDC.fifo_pointer = 0;
                pM->mem.ioFDC.fifo[0] = 0x20;
                pM->mem.ioFDC.fifo[1] = pM->mem.ioFDC.cylinder;
                logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC CMD: SENSE\n");
            }
        }else if(pM->mem.ioFDC.fifo[0] == FDC_CMD_DUMPREGS){
            if( pM->mem.ioFDC.fifo_pointer == 1 ){
                pM->mem.ioFDC.resp_cnt = 0;
                pM->mem.ioFDC.resp_len = 10;
                pM->mem.ioFDC.fifo_pointer = 0;
                pM->mem.ioFDC.fifo[0] = 0;
                logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC CMD: DUMPREG\n");
            }
        }else if( pM->mem.ioFDC.fifo[0] == FDC_CMD_CONFIGURE ){
            if( pM->mem.ioFDC.fifo_pointer == 4 ){
                pM->mem.ioFDC.resp_cnt = 0;
                pM->mem.ioFDC.resp_len = 0; // no response
                pM->mem.ioFDC.fifo_pointer = 0;
                pM->mem.ioFDC.fifo[0] = 0;
                logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC CMD: CONFIGURE\n");
            }
        }else if( pM->mem.ioFDC.fifo[0] == FDC_CMD_PERPENDICULAR ){
            if( pM->mem.ioFDC.fifo_pointer == 2 ){
                pM->mem.ioFDC.resp_cnt = 0;
                pM->mem.ioFDC.resp_len = 0; // no response ?
                pM->mem.ioFDC.fifo_pointer = 0;
                pM->mem.ioFDC.fifo[0] = 0;
                logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC CMD: PERPENDICULAR\n");
            }
        }else if( pM->mem.ioFDC.fifo[0] == FDC_CMD_UNLOCK ){
            if( pM->mem.ioFDC.fifo_pointer == 1 ){
                pM->mem.ioFDC.resp_cnt = 0;
                pM->mem.ioFDC.resp_len = 1;
                pM->mem.ioFDC.fifo_pointer = 0;
                pM->mem.ioFDC.fifo[0] = 0;
                logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC CMD: UNLOCK\n");
            }
        }else if( pM->mem.ioFDC.fifo[0] == FDC_CMD_PARTID ){
            if( pM->mem.ioFDC.fifo_pointer == 1 ){
                pM->mem.ioFDC.resp_cnt = 0;
                pM->mem.ioFDC.resp_len = 1;
                pM->mem.ioFDC.fifo_pointer = 0;
                pM->mem.ioFDC.fifo[0] = 0;  // 82078
                logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC CMD: PARTID\n");
            }
        }else if( pM->mem.ioFDC.fifo[0] == FDC_CMD_SPECIFY ){
            if( pM->mem.ioFDC.fifo_pointer == 3 ){
                pM->mem.ioFDC.resp_cnt = 0;
                pM->mem.ioFDC.resp_len = 0; // no response
                pM->mem.ioFDC.fifo_pointer = 0;
                logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC CMD: SPECIFY\n");
            }
        }else if( pM->mem.ioFDC.fifo[0] == FDC_CMD_RECALIBRATE ){
            if( pM->mem.ioFDC.fifo_pointer == 2 ){
                pM->mem.ioFDC.resp_cnt = 0;
                pM->mem.ioFDC.resp_len = 0; // no response
                pM->mem.ioFDC.fifo_pointer = 0;
                pM->mem.ioFDC.irq = 1; // interrupt request
                logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC CMD: RECALIBRATE\n");
            }
        }else if( pM->mem.ioFDC.fifo[0] == FDC_CMD_SEEK ){
            if( pM->mem.ioFDC.fifo_pointer == 3 ){
                pM->mem.ioFDC.resp_cnt = 0;
                pM->mem.ioFDC.resp_len = 0; // no response
                pM->mem.ioFDC.fifo_pointer = 0;
                pM->mem.ioFDC.irq = 1; // interrupt request

                pM->mem.ioFDC.cylinder = pM->mem.ioFDC.fifo[2]; // 

                logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC CMD: SEEK\n");
            }
        }else if( pM->mem.ioFDC.fifo[0] == FDC_CMD_READ ){
            if( pM->mem.ioFDC.fifo_pointer == 9 ){
                pM->mem.ioFDC.resp_cnt = 0;
                pM->mem.ioFDC.resp_len = 7;
                pM->mem.ioFDC.fifo_pointer = 0;

                logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC CMD: READ %x cyl:%x head:%x sec:%x ssize:%x eot:%x gpl:%x dtl:%x\n", 
                pM->mem.ioFDC.fifo[1], pM->mem.ioFDC.fifo[2], pM->mem.ioFDC.fifo[3], pM->mem.ioFDC.fifo[4],
                pM->mem.ioFDC.fifo[5], pM->mem.ioFDC.fifo[6], pM->mem.ioFDC.fifo[7], pM->mem.ioFDC.fifo[8]);

                pM->mem.ioFDC.sector_size = pM->mem.ioFDC.fifo[5];
                pM->mem.ioFDC.sector      = pM->mem.ioFDC.fifo[4]; // 
                pM->mem.ioFDC.head        = pM->mem.ioFDC.fifo[3]; // 
                pM->mem.ioFDC.cylinder    = pM->mem.ioFDC.fifo[2]; // 

                uint8_t buf[512];
                readFDD(&(pM->mem.ioFDC.cylinder), &(pM->mem.ioFDC.head), &(pM->mem.ioFDC.sector), buf);
                uint32_t DMABASE = (pM->mem.ioDMAPage.addr[2]<<16) + (pM->mem.ioDMAC.addr[2]);
                if( DMABASE + 512 < EMU_MEM_SIZE){
                    for(int ad=0; ad<512;ad++){
                        pM->mem.mem[ DMABASE + ad ] = buf[ad];
                    }
                }
                logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC CMD: READ (DMA addr %x)\n", DMABASE);

                pM->mem.ioFDC.fifo[6] = pM->mem.ioFDC.sector_size;
                pM->mem.ioFDC.fifo[5] = pM->mem.ioFDC.sector; 
                pM->mem.ioFDC.fifo[4] = pM->mem.ioFDC.head; 
                pM->mem.ioFDC.fifo[3] = pM->mem.ioFDC.cylinder; 

                pM->mem.ioFDC.fifo[0] =                     // Status Register 0
                    (1<<5) +                                // SEEK end
                    ((pM->mem.ioFDC.head==0) ? 0 : 0x04) +  // Head address
                    (pM->mem.ioFDC.fifo[1] & 3);            // selected drive
                pM->mem.ioFDC.fifo[1] = 0;                  // Status Register 1
                pM->mem.ioFDC.fifo[2] = 0;                  // Status Register 2

                pM->mem.ioFDC.irq = 1; // interrupt request
            }
        }else if( pM->mem.ioFDC.fifo[0] == FDC_CMD_WRITE ){
            if( pM->mem.ioFDC.fifo_pointer == 9 ){
                pM->mem.ioFDC.resp_cnt = 0;
                pM->mem.ioFDC.resp_len = 7;
                pM->mem.ioFDC.fifo_pointer = 0;
                logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC CMD: WRITE\n");
            }
        }else if( pM->mem.ioFDC.fifo[0] == FDC_CMD_GETSTATUS ){
            if( pM->mem.ioFDC.fifo_pointer == 2 ){
                pM->mem.ioFDC.resp_cnt = 0;
                pM->mem.ioFDC.resp_len = 1;
                pM->mem.ioFDC.fifo_pointer = 0;
                logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC CMD: GETSTATUS\n");
            }
        }else{
            logfile_printf(LOGLEVEL_FDC_NOTICE, "FDC I/O access: unknown command %x\n", data);
        }
    }
}
