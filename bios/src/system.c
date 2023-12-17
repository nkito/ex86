#include "system.h"


void systemOutByte(unsigned char value, unsigned int port) {
    asm volatile("outb %b0, %w1" : : "a"(value), "Nd"(port));
}

unsigned char systemInByte(unsigned int port) {
    unsigned char value;
    asm volatile("inb  %w1, %b0" : "=a"(value) : "d"(port));
    return value;
}

void systemOutWord(unsigned short value, unsigned int port) {
    asm volatile("outw %w0, %w1" : : "a"(value), "Nd"(port));
}

unsigned short systemInWord(unsigned int port) {
    unsigned short value;
    asm volatile("inw  %w1, %w0" : "=a"(value) : "d"(port));
    return value;
}

void systemReset(void){
	asm volatile("out %0, $0x96" : : "a"(0x01));
}

#define LSR 5
#define TBR 0
#define RBR 0
#define SIO_TX_BUF_EMPTY 0x20
#define SIO_RX_BUF_FULL  0x01


void systemSerialPutc(char c){
	while( (systemInByte(IOADDR_SCU_BASE+LSR) & SIO_TX_BUF_EMPTY) == 0 );
	systemOutByte(c, IOADDR_SCU_BASE + TBR);
}

char systemSerialGetc(void){
	while( (systemInByte(IOADDR_SCU_BASE+LSR) & SIO_RX_BUF_FULL) == 0 );
	return systemInByte(IOADDR_SCU_BASE + RBR);
}

int systemIsUSARTDataAvailable(void){
	return (systemInByte(IOADDR_SCU_BASE+LSR) & SIO_RX_BUF_FULL) == 0 ? 0 : 1;
}


