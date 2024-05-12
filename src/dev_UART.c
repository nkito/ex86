#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>

#include "i8086.h"
#include "dev_UART.h"
#include "logfile.h"
#include "ExInst_common.h"


#define ASCII_LF  0x0a
#define ASCII_CR  0x0d
#define ASCII_DEL 0x7f
#define ASCII_BS  0x08

#define LOGLEVEL_UART_INFO   (LOGCAT_IO_UART | LOGLV_INFO)
#define LOGLEVEL_UART_NOTICE (LOGCAT_IO_UART | LOGLV_NOTICE)


uint8_t readUARTReg(struct stMachineState *pM, struct stIO_UART *pUART, uint32_t addr){

    if( (addr&IOADDR_UART_MASK) == UART_REG_RXBUF ){
        if( pUART->buffered ){
            pUART->buffered = 0;
        }

        if(pUART->buf[0] == ASCII_LF ) pUART->buf[0] = ASCII_CR;
        if(pUART->buf[0] == ASCII_DEL) pUART->buf[0] = ASCII_BS;

        return pUART->buf[0];
    }
    if( (addr&IOADDR_UART_MASK) == UART_REG_LINESTAT ){
        // status reg
        // bit0: receive buffer empty if this bit is 0
        // bit1: transmitter idle if this bit is 0

        if( ! pUART->buffered){
            pUART->buffered = read(0, pUART->buf, 1);
            if( pUART->buffered < 0){
                pUART->buffered = 0;
            }
        }
        if( addr == IOADDR_COM0_BASE + UART_REG_LINESTAT || addr == IOADDR_COM1_BASE + UART_REG_LINESTAT ){
            return (pUART->buffered ? (UART_TX_BUF_EMPTY|UART_TX_EMPTY|UART_RX_BUF_FULL) : (UART_TX_BUF_EMPTY|UART_TX_EMPTY));
        }
        return (pUART->buffered ? 1 : 0);
    }
    if( (addr&IOADDR_UART_MASK) == UART_REG_INTEN ){
        logfile_printf(LOGLEVEL_UART_INFO, "UART: int enable register was read\n");
        return pUART->int_enable;
    }
    if( (addr&IOADDR_UART_MASK) == UART_REG_MODEMSTAT ){
        return ((1<<4)|(1<<5)); // CTS (Clear To Send) and DSR (Data Set Ready) bits are set
    }
    if( (addr&IOADDR_UART_MASK) == UART_REG_SCRATCH ){
        logfile_printf(LOGLEVEL_UART_INFO, "UART: scratch register was read\n");
        return pUART->scratch;
    }
    if( (addr&IOADDR_UART_MASK) == UART_REG_INTID ){

        if( (pUART->int_enable & 0x01) && pUART->buffered ){
            return (4 << 1) + 0;
        }
        if( (pUART->int_enable & 0x02) ){
            return (2 << 1) + 0;
        }
        return 1;
    }

    logfile_printf(LOGLEVEL_UART_NOTICE, "UART: unknown register was read (addr: %x)\n", addr);

    return 0;
}

void writeUARTReg  (struct stMachineState *pM, struct stIO_UART *pUART, uint32_t addr, uint8_t data){

    if( (addr&IOADDR_UART_MASK) == UART_REG_TXBUF ){
        if(DEBUG){
            PRINTF("SERIAL OUT: %c\n", data);
        }else{
            if(data == '\n'){
                PRINTF("\033[39m");
                PRINTF("\033[49m");
            }
            PRINTF("%c", data);
            FLUSH_STDOUT();

            logfile_printf(LOGLEVEL_UART_NOTICE, "UART OUT(port %x) : %x \'(%c)\' \n", addr, data, isprint(data) ? data : '?');
        }
    }else if( (addr&IOADDR_UART_MASK) == UART_REG_INTEN ){
        logfile_printf(LOGLEVEL_UART_INFO, "UART: int enable register was wrote %x\n", data);
        pUART->int_enable = data;
    }else if( (addr&IOADDR_UART_MASK) == UART_REG_SCRATCH ){
        logfile_printf(LOGLEVEL_UART_INFO, "UART: scratch register was wrote %x\n", data);
        pUART->scratch = data;
    }else{
        logfile_printf(LOGLEVEL_UART_NOTICE, "UART: write for unknown register (addr: %x, data %x)\n", addr, data);
    }
}
