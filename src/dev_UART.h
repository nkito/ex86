#ifndef __DEV_UART_H__ 
#define __DEV_UART_H__ 


#define IOADDR_COM0_BASE  0x3F8
#define IOADDR_COM1_BASE  0x2F8

#define IOADDR_UART_SIZE  0x008
#define IOADDR_UART_MASK  0x007

#define UART_REG_RXBUF     0x0 /* read       */
#define UART_REG_TXBUF     0x0 /* write      */
#define UART_REG_INTEN     0x1 /* read/write */
#define UART_REG_INTID     0x2 /* read       */
#define UART_REG_LINECTL   0x3 /* read/write */
#define UART_REG_MODEMCTL  0x4 /* read/write */
#define UART_REG_LINESTAT  0x5 /* read/write */
#define UART_REG_MODEMSTAT 0x6 /* read/write */
#define UART_REG_SCRATCH   0x7 /* write      */

#define UART_TX_EMPTY     0x40
#define UART_TX_BUF_EMPTY 0x20
#define UART_RX_BUF_FULL  0x01

uint8_t readUARTReg(struct stMachineState *pM, struct stIO_UART *pUART, uint32_t addr);
void writeUARTReg  (struct stMachineState *pM, struct stIO_UART *pUART, uint32_t addr, uint8_t data);

#endif
