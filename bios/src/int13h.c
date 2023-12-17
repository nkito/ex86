#include <stdio.h>
#include "basicio.h"

#include "system.h"
#include "asmfuncs.h"
#include "emu_interface.h"


unsigned int int13_reg_ax;
unsigned int int13_reg_bx;
unsigned int int13_reg_cx;
unsigned int int13_reg_dx;
unsigned int int13_reg_ds;
unsigned int int13_reg_es;
unsigned int int13_reg_si;

unsigned int int13_reg_sp;
unsigned int int13_reg_ss;

unsigned int int13_reg_ss2;

#define PUSHA \
"mov %ss, %cs:int13_reg_ss2\n" \
"push %ax\n" \
"push %cx\n" \
"push %dx\n" \
"push %bx\n" \
"push %cs:int13_reg_ss2\n" \
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
"   .global int13_handler_asm        \n"
"int13_handler_asm:                  \n"
"	push %ds                         \n"
"	push %es                         \n"
PUSHA /*"	pusha                            \n"*/
"                                    \n"
"   mov %ax, %cs:int13_reg_ax        \n"
"   mov %bx, %cs:int13_reg_bx        \n"
"   mov %cx, %cs:int13_reg_cx        \n"
"   mov %dx, %cs:int13_reg_dx        \n"
"   mov %ds, %cs:int13_reg_ds        \n"
"   mov %es, %cs:int13_reg_es        \n"
"   mov %si, %cs:int13_reg_si        \n"
"   mov %sp, %cs:int13_reg_sp        \n"
"   mov %ss, %cs:int13_reg_ss        \n"
"                                    \n"
"	mov	%cs, %ax                     \n"
"	mov	%ax, %ss                     \n"
"	mov	%ax, %ds                     \n"
"	mov	$0xffff, %sp                 \n"
"                                    \n"
"   mov int13_reg_ax, %ax            \n"
"	cmp	$0x00, %ah                   \n"
"	je	int13_handler_asm_ah00       \n"
"	cmp	$0x02, %ah                   \n"
"	je	int13_handler_asm_ah02       \n"
"	cmp	$0x03, %ah                   \n"
"	je	int13_handler_asm_ah03       \n"
"	cmp	$0x08, %ah                   \n"
"	je	int13_handler_asm_ah08       \n"
"	cmp	$0x15, %ah                   \n"
"	je	int13_handler_asm_ah15       \n"
"	cmp	$0x41, %ah                   \n"
"	je	int13_handler_asm_ah41       \n"
"	cmp	$0x42, %ah                   \n"
"	je	int13_handler_asm_ah42       \n"
"                                    \n"
"	call int13_default_handler       \n"
"                                    \n"
"   mov %cs:int13_reg_sp, %sp        \n"
"   mov %cs:int13_reg_ss, %ss        \n"
"                                    \n"
POPA /*"	popa                             \n"*/
"	pop %es                          \n"
"	pop %ds                          \n"
"	iret                             \n");

#define HEAD_OF_ASM_HANDLER(func)  \
"	call " func "                    \n" \
"                                    \n" \
"   mov %cs:int13_reg_sp, %sp        \n" \
"   mov %cs:int13_reg_ss, %ss        \n" \
"                                    \n" \
POPA /*"	popa                             \n"*/ \
"	pop %es                          \n" \
"	pop %ds                          \n" 

#define IRET_WITH_CARRYFLAG  \
"   push %ax                         \n" \
"   push %bp                         \n" \
"   mov  %sp, %bp                    \n" \
"   movw 8(%bp), %ax                 \n" /* set the carry flag */ \
"   orw  $1, %ax                     \n" \
"   movw %ax, 8(%bp)                 \n" \
"   pop  %bp                         \n" \
"   pop  %ax                         \n" \
"   iret                             \n" 

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
"int13_handler_asm_ah00:             \n"
HEAD_OF_ASM_HANDLER("int13_handler_ah00")
"   mov $0, %ax                      \n"
IRET_WITHOUT_CARRYFLAG);

__asm__ (
"int13_handler_asm_ah02:             \n"
HEAD_OF_ASM_HANDLER("int13_handler_ah02")
"                                    \n"
"   mov %cs:int13_reg_ax, %ax        \n"
"   cmp $0, %ah                      \n"
"   je 1f                            \n"
"                                    \n"
    IRET_WITH_CARRYFLAG
"1:                                  \n"
    IRET_WITHOUT_CARRYFLAG);

__asm__ (
"int13_handler_asm_ah03:             \n"
HEAD_OF_ASM_HANDLER("int13_handler_ah03")
"                                    \n"
"   mov %cs:int13_reg_ax, %ax        \n"
"   cmp $0, %ah                      \n"
"   je 2f                            \n"
"                                    \n"
    IRET_WITH_CARRYFLAG
"2:                                  \n"
    IRET_WITHOUT_CARRYFLAG);

__asm__ (
"int13_handler_asm_ah08:             \n"
HEAD_OF_ASM_HANDLER("int13_handler_ah08")
"                                    \n"
"   movb %cs:int13_reg_bx, %bl       \n"
"   mov  %cs:int13_reg_cx, %cx       \n"
"   mov  %cs:int13_reg_dx, %dx       \n"
"   movb %cs:int13_reg_ax+1, %ah     \n"
"                                    \n"
IRET_WITHOUT_CARRYFLAG);


__asm__ (
"int13_handler_asm_ah15:             \n"
HEAD_OF_ASM_HANDLER("int13_handler_ah15")
"                                    \n"
"   movb %cs:int13_reg_ax+1, %ah     \n"
"   mov  %cs:int13_reg_cx  , %cx     \n"
"   mov  %cs:int13_reg_dx  , %dx     \n"
"                                    \n"
IRET_WITHOUT_CARRYFLAG);


__asm__ (
"int13_handler_asm_ah41:             \n"
HEAD_OF_ASM_HANDLER("int13_handler_ah41")
"                                    \n"
"   mov  %cs:int13_reg_cx, %cx       \n"
"   mov  $0xAA55, %bx                \n"
"   movb $0x01,   %ah                \n"
"                                    \n"
IRET_WITHOUT_CARRYFLAG);

__asm__ (
"int13_handler_asm_ah42:             \n"
HEAD_OF_ASM_HANDLER("int13_handler_ah42")
"                                    \n"
"   movb %cs:int13_reg_ax+1, %ah     \n"
"   cmp  $0, %ah                     \n"
"   je 1f                            \n"
"                                    \n"
    IRET_WITH_CARRYFLAG
"1:                                  \n"
    IRET_WITHOUT_CARRYFLAG);


/*******************************************
 * Definitions of global variables in bios.c
 *******************************************/
extern unsigned long BufMem[1024/sizeof(unsigned long)];
extern unsigned int  hdd_ready;
extern unsigned int  hdd_cylinder;
extern unsigned int  hdd_head;
extern unsigned int  hdd_sector;
extern unsigned long hdd_total_sectors;
/*******************************************/

//#define INT13H_DEBUG


void int13_default_handler(void){
    char buf[128];
	unsigned int reg_ss;

    asm volatile("mov  %%ss, %w0" : "=a"(reg_ss) : );
    s_snprintf(buf, sizeof(buf), "INT 0x13\n");
    emuLogMessage(reg_ss, (uint16_t)buf);

    s_snprintf(buf, sizeof(buf), "AX: %04x BX: %04x CX: %04x DX: %04x ES: %04x \n", 
        int13_reg_ax, int13_reg_bx, int13_reg_cx, int13_reg_dx, int13_reg_es);
    emuLogMessage(reg_ss, (uint16_t)buf);
}

void int13_handler_ah00(void){
    // reset disk system
    return ;
}

void int13_handler_ah08(void){
    // Get current drive parameters

#ifdef INT13H_DEBUG
    s_printf("\nINT13H AH=8H   DX=0x%02x\n", int13_reg_dx);
#endif

    if( (int13_reg_dx&0x80) == 0x80 ){
        // HDD

        // AH: return code (0x00 : no error)
        int13_reg_ax = 0x0000;

        // BH: -, BL: Drive type 
        int13_reg_bx = 0x0000;

        if( hdd_ready ){
            // CH: Maximum value for cylinder, CL: higher 2-bit of cylinder , sectors per track
            int13_reg_cx = ( (hdd_cylinder&0xff) | ((hdd_cylinder>>2)&0xc0) | (hdd_sector&0x3f) );

            // DH: head (0based), DL: number of drives attached
            int13_reg_dx = ( ((hdd_head&0xff)<<8) | 0x01 );
        }else{
            int13_reg_cx = 0x0000;  // CH: Maximum value for cylinder, CL: sectors per track
            int13_reg_dx = 0x0000;  // DH: head (0based), DL: number of drives attached
        }
    }else{
        int13_reg_ax = 0x0000;
        int13_reg_bx = 0x0004;  // BH: -, BL: Drive type 
        int13_reg_cx = 0x4f12;  // CH: Maximum value for cylinder, CL: sectors per track
        int13_reg_dx = 0x0102;  // DH: head (0based), DL: number of drives attached
    }
}

void int13_handler_ah15(void){
    // Read disk type/DASD type

#ifdef INT13H_DEBUG
    s_printf("\nINT13H AH=15H   DX=0x%02x\n", int13_reg_dx);
#endif

    if( (int13_reg_dx&0x0080) ){
        // HDD

        if( hdd_ready && ((int13_reg_dx&0x80) == 0x80) ){
            int13_reg_ax = 0x0300;  // fixed disk present
            int13_reg_cx = ((hdd_total_sectors>>16) & 0xffff);
            int13_reg_dx = ( hdd_total_sectors      & 0xffff);
        }else{
            int13_reg_ax = 0x0000;
        }
    }else{
        // FDD
        if( (int13_reg_dx&0x7f) == 0 || (int13_reg_dx&0x7f) == 1 ){
            // diskette, no change detection present
            int13_reg_ax = 0x0100;
        }else{
            int13_reg_ax = 0x0000;
        }
    }
}

void int13_handler_ah41(void){
    // Check Extensions Present

    if( (int13_reg_dx&0x0080) ){
        // HDD
        int13_reg_cx = 0x0000;
    }else{
        // FDD
        int13_reg_cx = 0x0000;
    }

    return ;
}


void int13_handler_ah02(void){
    unsigned int  counter  = 0;
    unsigned int  nsector  = (int13_reg_ax&0xff);
    unsigned long cylinder = ((int13_reg_cx&0xff00)>>8) + ((int13_reg_cx&0xc0)<<2);
    unsigned long sector   = (int13_reg_cx&0x3f);
    unsigned long head     = ((int13_reg_dx&0xff00)>>8);
    unsigned int  drive    = (int13_reg_dx&0xff);

	unsigned int reg_ss;
    unsigned long pos;
	uint8_t result;

    asm volatile("mov  %%ss, %w0" : "=a"(reg_ss) : );

    if( (drive == 0) || (drive == 1) ){
        pos = ((cylinder*2 + head)*18UL + sector - 1);
    }else if( drive == 0x80 ){
        pos = ((cylinder*(hdd_head+1) + head)*((unsigned long)hdd_sector) + sector - 1);
        drive = 2;
    }else{
        goto drive_err;
    }

    for(counter=0; nsector > counter; counter++){
    	result = emuReadDriveSector(drive, pos + counter, int13_reg_es, (uint16_t)(int13_reg_bx+512*counter));
    	if(result != EMU_INTERFACE_RESULT_OK) goto drive_err; 
    }

    int13_reg_ax = ( 0x00 | (counter&0xff));

    return;

drive_err:
    int13_reg_ax = (0xAA<<8);	// Drive not ready 
}


void int13_handler_ah03(void){
    unsigned int  counter  = -1;
    unsigned int  nsector  = (int13_reg_ax&0xff);
    unsigned long cylinder = ((int13_reg_cx&0xff00)>>8) + ((int13_reg_cx&0xc0)<<2);
    unsigned long sector   = (int13_reg_cx&0x3f);
    unsigned long head     = ((int13_reg_dx&0xff00)>>8);
    unsigned int  drive    = (int13_reg_dx&0xff);

	unsigned int reg_ss;
    unsigned long pos;
	uint8_t result;

    asm volatile("mov  %%ss, %w0" : "=a"(reg_ss) : );

    if( (drive == 0) || (drive == 1) ){
        pos = ((cylinder*2 + head)*18UL + sector - 1);
    }else if( drive == 0x80 ){
        pos = ((cylinder*(hdd_head+1) + head)*((unsigned long)hdd_sector) + sector - 1);
        drive = 2;
    }else{
        goto drive_err;
    }

    for(counter=0; nsector > counter; counter++){
    	result = emuWriteDriveSector(drive, pos + counter, int13_reg_es, (uint16_t)(int13_reg_bx+512*counter));
    	if(result != EMU_INTERFACE_RESULT_OK) goto drive_err; 
    }

    int13_reg_ax = ( 0x00 | (counter&0xff));

    return;

drive_err:
    int13_reg_ax = (0xAA<<8);	// Drive not ready 
}



void int13_handler_ah42(void){
    unsigned char arg[16];

    fetch_data_word(int13_reg_ds, (unsigned char *)int13_reg_si, arg, sizeof(arg)/2);

    unsigned int  counter  = 0;
    unsigned int  nsector  = *((unsigned int  *)(arg + 2));
    unsigned int  drive    = (int13_reg_dx&0xff);
    unsigned long s_sector = *((unsigned long *)(arg + 8));
    unsigned int  dest_seg = *((unsigned int  *)(arg + 6));
    unsigned int  dest_off = *((unsigned int  *)(arg + 4));

    if(arg[0] != 0x10) goto drive_err;

	unsigned int reg_ss;
	uint8_t result;

    asm volatile("mov  %%ss, %w0" : "=a"(reg_ss) : );

    if( (drive == 0) || (drive == 1) ){
        ;
    }else if( drive == 0x80 ){
        drive = 2;
    }else{
        goto drive_err;
    }

    for(counter=0; nsector > counter; counter++){
    	result = emuReadDriveSector(drive, s_sector + counter, reg_ss, (uint16_t)BufMem);
    	if(result != EMU_INTERFACE_RESULT_OK) goto drive_err; 
        copy_data_word((unsigned char *)BufMem, dest_seg, (unsigned char *)(dest_off+512*counter), 512/2);
    }

    int13_reg_ax = ( 0x00 | (counter&0xff));

    return;

drive_err:
    int13_reg_ax = (0xAA<<8);	// Drive not ready 
}
