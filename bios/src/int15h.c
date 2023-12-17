#include <stdio.h>
#include "basicio.h"

#include "system.h"
#include "asmfuncs.h"
#include "emu_interface.h"


unsigned int int15_reg_ax;
unsigned int int15_reg_bx;
unsigned int int15_reg_cx;
unsigned int int15_reg_dx;
unsigned int int15_reg_es;

unsigned int int15_reg_di;

unsigned int int15_reg_sp;
unsigned int int15_reg_ss;

unsigned int int15_reg_ss2;

#define PUSHA \
"mov %ss, %cs:int15_reg_ss2\n" \
"push %ax\n" \
"push %cx\n" \
"push %dx\n" \
"push %bx\n" \
"push %cs:int15_reg_ss2\n" \
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
"   .global int15_handler_asm        \n"
"int15_handler_asm:                  \n"
"	push %ds                         \n"
"	push %es                         \n"
PUSHA /*"	pusha                            \n"*/
"                                    \n"
"   mov %ax, %cs:int15_reg_ax        \n"
"   mov %bx, %cs:int15_reg_bx        \n"
"   mov %cx, %cs:int15_reg_cx        \n"
"   mov %dx, %cs:int15_reg_dx        \n"
"   mov %es, %cs:int15_reg_es        \n"
"   mov %di, %cs:int15_reg_di        \n"
"   mov %sp, %cs:int15_reg_sp        \n"
"   mov %ss, %cs:int15_reg_ss        \n"
"                                    \n"
"	mov	%cs, %ax                     \n"
"	mov	%ax, %ss                     \n"
"	mov	%ax, %ds                     \n"
"	mov	$0xffff, %sp                 \n"
"                                    \n"
"   mov int15_reg_ax, %ax            \n"
"	cmpw $0x2400, %ax                \n"
"	je	int15_handler_asm_ax2400     \n"
"	cmpw $0x2401, %ax                \n"
"	je	int15_handler_asm_ax2401     \n"
"	cmpw $0x2402, %ax                \n"
"	je	int15_handler_asm_ax2402     \n"
"	cmpw $0x2403, %ax                \n"
"	je	int15_handler_asm_ax2403     \n"
"	cmpw $0xe801, %ax                \n"
"	je	int15_handler_asm_axe801     \n"
"	cmpw $0xe820, %ax                \n"
"	je	int15_handler_asm_axe820     \n"
"	cmp  $0x86, %ah                  \n"
"	je	int15_handler_asm_ah86       \n"
"	cmp  $0x88, %ah                  \n"
"	je	int15_handler_asm_ah88       \n"
"	cmp  $0xc0, %ah                  \n"
"	je	int15_handler_asm_ahc0       \n"
"                                    \n"
"	call int15_default_handler       \n"
"                                    \n"
"   mov %cs:int15_reg_sp, %sp        \n"
"   mov %cs:int15_reg_ss, %ss        \n"
"                                    \n"
POPA/*"	popa                             \n"*/
"	pop %es                          \n"
"	pop %ds                          \n"
"	iret                             \n");

#define HEAD_OF_ASM_HANDLER(func)  \
"	call " func "                    \n" \
"                                    \n" \
"   mov %cs:int15_reg_sp, %sp        \n" \
"   mov %cs:int15_reg_ss, %ss        \n" \
"                                    \n" \
POPA/*"	popa                             \n"*/ \
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
"                                    \n" \
"   iret                             \n"


__asm__ (
"int15_handler_asm_ax2400:             \n"
HEAD_OF_ASM_HANDLER("int15_handler_ax2400")
"	movb    %cs:int15_reg_ax+1, %ah    \n"
IRET_WITHOUT_CARRYFLAG);

__asm__ (
"int15_handler_asm_ax2401:             \n"
HEAD_OF_ASM_HANDLER("int15_handler_ax2401")
"	movb    %cs:int15_reg_ax+1, %ah    \n"
IRET_WITHOUT_CARRYFLAG);

__asm__ (
"int15_handler_asm_ax2402:             \n"
HEAD_OF_ASM_HANDLER("int15_handler_ax2402")
"	mov     %cs:int15_reg_ax, %ax      \n"
"	mov     %cs:int15_reg_cx, %cx      \n"
IRET_WITHOUT_CARRYFLAG);

__asm__ (
"int15_handler_asm_ax2403:             \n"
HEAD_OF_ASM_HANDLER("int15_handler_ax2403")
"	movb    %cs:int15_reg_ax+1, %ah    \n"
"	mov     %cs:int15_reg_bx, %bx      \n"
IRET_WITHOUT_CARRYFLAG);


__asm__ (
"int15_handler_asm_axe801:             \n"
HEAD_OF_ASM_HANDLER("int15_handler_axe801")
"	mov	    %cs:int15_reg_ax, %ax    \n"
"	mov	    %cs:int15_reg_bx, %bx    \n"
"	mov	    %cs:int15_reg_cx, %cx    \n"
"	mov	    %cs:int15_reg_dx, %dx    \n"
IRET_WITHOUT_CARRYFLAG);


__asm__ (
"int15_handler_asm_axe820:             \n"
HEAD_OF_ASM_HANDLER("int15_handler_axe820")
"   iret                             \n");

__asm__ (
"int15_handler_asm_ah86:             \n"
HEAD_OF_ASM_HANDLER("int15_handler_ah86")
"	mov	    %cs:int15_reg_ax, %ax    \n"
IRET_WITHOUT_CARRYFLAG);

__asm__ (
"int15_handler_asm_ah88:             \n"
HEAD_OF_ASM_HANDLER("int15_handler_ah88")
"	mov	    %cs:int15_reg_ax, %ax    \n"
IRET_WITHOUT_CARRYFLAG);

__asm__ (
"int15_handler_asm_ahc0:             \n"
HEAD_OF_ASM_HANDLER("int15_handler_ahc0")
"	mov	    %cs:int15_reg_es, %es    \n"
"	mov	    %cs:int15_reg_bx, %bx    \n"
"	movb    %cs:int15_reg_ax+1, %ah  \n"
IRET_WITHOUT_CARRYFLAG);


void int15_default_handler(void){
    if( (int15_reg_ax & 0xff00) == 0xbf00 ){
        // DOS4GW related, not BIOS call
        return;
    }

    char buf[128];
	unsigned int reg_ss;

    asm volatile("mov  %%ss, %w0" : "=a"(reg_ss) : );

    s_snprintf(buf, sizeof(buf), "INT 0x15\n");
    emuLogMessage(reg_ss, (uint16_t)buf);

    s_snprintf(buf, sizeof(buf), "AX: %04x BX: %04x CX: %04x DX: %04x ES: %04x DI: %04x \n", 
        int15_reg_ax, int15_reg_bx, int15_reg_cx, int15_reg_dx, int15_reg_es, int15_reg_di);
    emuLogMessage(reg_ss, (uint16_t)buf);
}

void int15_handler_ah86(void){
    //  INT 15h / AH = 86h - BIOS wait function. 

    unsigned long delay = (((unsigned long)int15_reg_cx)<<16) + int15_reg_dx;

    // TODO: it is temporaly code
    volatile int i;
    for(i=0; i<delay; i++) ;
}


unsigned int int15_a20_status = 0;

void int15_handler_ax2400(void){
    // Disable A20 GATE

    /*
    if( processor_is_486_or_newer ){
        asm volatile(".byte 0x0f, 0x09\n");
    }
    */

    // This BIOS call intends to disables A20. But a DOS app uses this to enable A20 line...
    systemOutByte(0x02, 0x92); // SystemControlPortA

    int15_a20_status = 0;
    
    int15_reg_ax = 0;
    return ;
}

void int15_handler_ax2401(void){
    // Enable A20 GATE

    /*
    if( processor_is_486_or_newer ){
        asm volatile(".byte 0x0f, 0x09\n");
    }
    */

    systemOutByte(0x02, 0x92); // SystemControlPortA

    int15_a20_status = 1;

    int15_reg_ax = 0;
    return ;
}

void int15_handler_ax2402(void){
    // Return A20 GATE status

    int15_a20_status = ( systemInByte(0x92) & 2 ) ? 1 : 0;

    int15_reg_ax = int15_a20_status;
    int15_reg_cx = 0;
    return ;
}

void int15_handler_ax2403(void){
    // Return A20 GATE support

    int15_reg_ax = 0;
    int15_reg_bx = (1); // supported on keyboard controller
    return ;
}

void int15_handler_axe801(void){
    /*
        Refer: http://mirror.cs.msu.ru/oldlinux.org/Linux.old/docs/interrupts/int-html/rb-1739.htm

        Return:CF clear if successful
        AX = extended memory between 1M and 16M, in K (max 3C00h = 15MB)
        BX = extended memory above 16M, in 64K blocks
        CX = configured memory 1M to 16M, in K
        DX = configured memory above 16M, in 64K blocks
    */
    uint32_t memsize = (emuGetMemoryCapacity()>>10); // capacity in KiB

    int15_reg_ax = (memsize <= 16*1024) ? (memsize - (1 << 10))    : 0x3c00;
    int15_reg_bx = (memsize  > 16*1024) ? (memsize - 16*1024) / 64 : 0;
    int15_reg_cx = (memsize <= 16*1024) ? (memsize - (1 << 10)) : 0x3c00;
    int15_reg_dx = (memsize  > 16*1024) ? (memsize - 16*1024) / 64 : 0;
}


void int15_handler_axe820(void){
    // This feature uses 32-bit regs
    // It is disabled
    /*
    s_printf("\n* INT 0x15\n");
    s_printf("AX: %04x BX: %04x CX: %04x DX: %04x ES: %04x DI: %04x \n", 
        int15_reg_ax, int15_reg_bx, int15_reg_cx, int15_reg_dx, int15_reg_es, int15_reg_di);
    */
    return ;
}

void int15_handler_ah88(void){
    // Return memory size

    // Obtain memory size from the emulator
    uint32_t memsize = (emuGetMemoryCapacity()>>10); // capacity in KiB

    int15_reg_ax = (memsize <= 16*1024) ? (memsize - (1 << 10)) : 0x3c00;
}

unsigned char int15_c0_buf[10];

void int15_handler_ahc0(void){
	unsigned int reg_cs;
	asm volatile("mov  %%cs, %w0" : "=a"(reg_cs) : );

    int15_reg_ax = 0;
    int15_reg_bx = (unsigned int)int15_c0_buf;
    int15_reg_es = reg_cs;

    int15_c0_buf[0] =  8;
    int15_c0_buf[1] =  0;
    int15_c0_buf[2] =  1; // model
    int15_c0_buf[3] =  1; // model
    int15_c0_buf[4] =  1; // BIOS revision

    int15_c0_buf[5] =  0; // feature byte 1
    int15_c0_buf[6] =  0; // feature byte 2
    int15_c0_buf[7] =  0; // feature byte 3
    int15_c0_buf[8] =  0; // feature byte 4
    int15_c0_buf[9] =  0; // feature byte 5
}
