#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#define _MK_FP(seg, offset) (void __far *)(((unsigned long)(seg) << 16) \
    + (unsigned long)(unsigned)(offset))


#define IOADDR_PCCOM2_BASE      0x2F8
#define IOADDR_PCTIMER_BASE     0x40

#define  IOADDR_ICUMST_BASE 0x20
#define  IOADDR_ICUSLV_BASE 0xA0
#define  IOADDR_TCU_BASE    0x40
#define  IOADDR_SCU_BASE    IOADDR_PCCOM2_BASE


void systemReset(void);
void systemOutByte(unsigned char value, unsigned int port);
void systemOutWord(unsigned short value, unsigned int port);
unsigned char  systemInByte(unsigned int port);
unsigned short systemInWord(unsigned int port);

void systemSerialPutc(char c);
char systemSerialGetc(void);
int  systemIsUSARTDataAvailable(void);

#endif
