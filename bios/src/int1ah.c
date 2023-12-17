#include <stdio.h>
#include "basicio.h"

#include "terminal.h"
#include "system.h"

#include "asmfuncs.h"
#include "timer.h"
#include "emu_interface.h"

unsigned int int1a_reg_ax;
unsigned int int1a_reg_bx;
unsigned int int1a_reg_cx;
unsigned int int1a_reg_dx;
unsigned int int1a_reg_es;

unsigned int int1a_reg_sp;
unsigned int int1a_reg_ss;

unsigned int int1a_reg_ss2;

#define PUSHA \
"mov %ss, %cs:int1a_reg_ss2\n" \
"push %ax\n" \
"push %cx\n" \
"push %dx\n" \
"push %bx\n" \
"push %cs:int1a_reg_ss2\n" \
"push %bp\n" \
"push %si\n" \
"push %di\n"

#define POPA \
"pop %di\n" \
"pop %si\n" \
"pop %bp\n" \
"pop %bx\n"  /* skip next 2 bytes of stack */ \
"pop %bx\n" \
"pop %dx\n" \
"pop %cx\n" \
"pop %ax\n" 

__asm__ (
"   .global int1a_handler_asm        \n"
"int1a_handler_asm:                  \n"
"	push %ds                         \n"
"	push %es                         \n"
PUSHA /*"	pusha                            \n"*/
"                                    \n"
"   mov %ax, %cs:int1a_reg_ax        \n"
"   mov %bx, %cs:int1a_reg_bx        \n"
"   mov %cx, %cs:int1a_reg_cx        \n"
"   mov %dx, %cs:int1a_reg_dx        \n"
"   mov %es, %cs:int1a_reg_es        \n"
"   mov %sp, %cs:int1a_reg_sp        \n"
"   mov %ss, %cs:int1a_reg_ss        \n"
"                                    \n"
"	mov	%cs, %ax                     \n"
"	mov	%ax, %ss                     \n"
"	mov	%ax, %ds                     \n"
"	mov	$0xffff, %sp                 \n"
"                                    \n"
"   mov int1a_reg_ax, %ax            \n"
"	cmp	$0x00, %ah                   \n"
"	je	int1a_handler_asm_ah00       \n"
"	cmp	$0x01, %ah                   \n"
"	je	int1a_handler_asm_ah01       \n"
"	cmp	$0x02, %ah                   \n"
"	je	int1a_handler_asm_ah02       \n"
"	cmp	$0x03, %ah                   \n"
"	je	int1a_handler_asm_ah03       \n"
"	cmp	$0x04, %ah                   \n"
"	je	int1a_handler_asm_ah04       \n"
"	cmp	$0x05, %ah                   \n"
"	je	int1a_handler_asm_ah05       \n"
"                                    \n"
"	call int1a_default_handler       \n"
"                                    \n"
"   mov %cs:int1a_reg_sp, %sp        \n"
"   mov %cs:int1a_reg_ss, %ss        \n"
"                                    \n"
POPA /*"	popa                             \n"*/
"	pop %es                          \n"
"	pop %ds                          \n"
"	iret                             \n");

#define HEAD_OF_ASM_HANDLER(func)  \
"	call " func "                    \n" \
"                                    \n" \
"   mov %cs:int1a_reg_sp, %sp        \n" \
"   mov %cs:int1a_reg_ss, %ss        \n" \
"                                    \n" \
POPA /*"	popa                             \n"*/ \
"	pop %es                          \n" \
"	pop %ds                          \n" 

#define IRET_WITHOUT_CARRYFLAG  \
"   push %ax                         \n" \
"   push %bp                         \n" \
"   mov  %sp, %bp                    \n" \
"   movw 8(%bp), %ax                 \n" /* clear the carry flag */ \
"   andw $0xfffe, %ax                \n" \
"   movw %ax, 8(%bp)                 \n" \
"   pop  %bp                         \n" \
"   pop  %ax                         \n" \
"   iret                             \n"

__asm__ (
"int1a_handler_asm_ah00:             \n"
HEAD_OF_ASM_HANDLER("int1a_handler_ah00")
"	mov	    %cs:int1a_reg_cx, %cx    \n"
"	mov	    %cs:int1a_reg_dx, %dx    \n"
"	movb	%cs:int1a_reg_ax, %al    \n"
"	iret                             \n");

__asm__ (
"int1a_handler_asm_ah01:             \n"
HEAD_OF_ASM_HANDLER("int1a_handler_ah01")
"	iret                             \n");

__asm__ (
"int1a_handler_asm_ah02:             \n"
HEAD_OF_ASM_HANDLER("int1a_handler_ah02")
"	mov	    %cs:int1a_reg_cx, %cx    \n"
"	mov	    %cs:int1a_reg_dx, %dx    \n"
IRET_WITHOUT_CARRYFLAG);

__asm__ (
"int1a_handler_asm_ah03:             \n"
HEAD_OF_ASM_HANDLER("int1a_handler_ah03")
"	iret                             \n");

__asm__ (
"int1a_handler_asm_ah04:             \n"
HEAD_OF_ASM_HANDLER("int1a_handler_ah04")
"	mov	    %cs:int1a_reg_cx, %cx    \n"
"	mov	    %cs:int1a_reg_dx, %dx    \n"
IRET_WITHOUT_CARRYFLAG);

__asm__ (
"int1a_handler_asm_ah05:             \n"
HEAD_OF_ASM_HANDLER("int1a_handler_ah05")
"	iret                             \n");



void int1a_default_handler(void){
    char buf[128];
	unsigned int reg_ss;

    asm volatile("mov  %%ss, %w0" : "=a"(reg_ss) : );

    s_snprintf(buf, sizeof(buf), "INT 0x1A\n");
    emuLogMessage(reg_ss, (uint16_t)buf);

    s_snprintf(buf, sizeof(buf), "AX: %04x BX: %04x CX: %04x DX: %04x ES: %04x \n", 
        int1a_reg_ax, int1a_reg_bx, int1a_reg_cx, int1a_reg_dx, int1a_reg_es);
    emuLogMessage(reg_ss, (uint16_t)buf);
}

void int1a_handler_ah00(void){
    // get tick
    unsigned char clearRollOverFlag = 0;

    fetch_data(0x40, (unsigned char *)0x6c, (unsigned char *)&int1a_reg_dx, 2); // low
    fetch_data(0x40, (unsigned char *)0x6e, (unsigned char *)&int1a_reg_cx, 2); // high

    int1a_reg_ax = 0x0000;
    fetch_data(0x40, (unsigned char *)0x70, (unsigned char *)&int1a_reg_ax, 1); // RollOverFlag
    copy_data(&clearRollOverFlag, 0x40, (unsigned char *)0x70, 1);
}

extern unsigned char  RTC_available;

void int1a_handler_ah01(void){
    // set tick

    long tick = (((long)int1a_reg_dx) | (((long)int1a_reg_cx)<<16));
    updateTickOffset(tick);

    copy_data((unsigned char *)&int1a_reg_dx, 0x40, (unsigned char *)0x6c, 2);
    copy_data((unsigned char *)&int1a_reg_cx, 0x40, (unsigned char *)0x6e, 2);
}

void int1a_handler_ah02(void){
    // get TIME

    // Time comes from the emulator 
    uint8_t hour, min, sec;
    emuGetTime(NULL, NULL, NULL, &hour, &min, &sec);

    unsigned int bcdHour = ((hour/10)<<4) + (hour%10);
    unsigned int bcdMin  = ((min /10)<<4) + (min %10);
    unsigned int bcdSec  = ((sec /10)<<4) + (sec %10);

    int1a_reg_cx = (bcdHour<<8) + bcdMin;
    int1a_reg_dx = (bcdSec <<8);
}

void int1a_handler_ah03(void){
    // set TIME

    // ignore the request
}

void int1a_handler_ah04(void){
    // get DATE

    // Date comes from the emulator 
    uint16_t year;
    uint8_t  month, day;
    emuGetTime(&year, &month, &day, NULL, NULL, NULL);

    unsigned int bcdYear = ((year/1000)<<12) + (((year%1000)/100)<<8) + (((year%100)/10)<<4) + (year%10);
    unsigned int bcdMon  = ((month/10)<<4)   + (month %10);
    unsigned int bcdDay  = ((day  /10)<<4)   + (day %10);

    int1a_reg_cx = bcdYear;
    int1a_reg_dx = (bcdMon<<8) + bcdDay;
}

void int1a_handler_ah05(void){
    // set DATE

    // ignore the request
}

