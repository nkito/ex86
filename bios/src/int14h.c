#include <stdio.h>
#include "basicio.h"

#include "terminal.h"
#include "system.h"

#include "asmfuncs.h"
#include "emu_interface.h"

/*
#define INT14_DEBUG_PRINT
*/

unsigned int int14_reg_ax;
unsigned int int14_reg_bx;
unsigned int int14_reg_cx;
unsigned int int14_reg_dx;

unsigned int int14_reg_sp;
unsigned int int14_reg_ss;

unsigned int int14_reg_ss2;

#define PUSHA \
"mov %ss, %cs:int14_reg_ss2\n" \
"push %ax\n" \
"push %cx\n" \
"push %dx\n" \
"push %bx\n" \
"push %cs:int14_reg_ss2\n" \
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
"   .global int14_handler_asm        \n"
"int14_handler_asm:                  \n"
"	push %ds                         \n"
"	push %es                         \n"
PUSHA /*"	pusha                            \n"*/
"                                    \n"
"   mov %ax, %cs:int14_reg_ax        \n"
"   mov %bx, %cs:int14_reg_bx        \n"
"   mov %cx, %cs:int14_reg_cx        \n"
"   mov %dx, %cs:int14_reg_dx        \n"
"   mov %sp, %cs:int14_reg_sp        \n"
"   mov %ss, %cs:int14_reg_ss        \n"
"                                    \n"
"	mov	%cs, %ax                     \n"
"	mov	%ax, %ss                     \n"
"	mov	%ax, %ds                     \n"
"	mov	$0xffff, %sp                 \n"
"                                    \n"
"   mov int14_reg_ax, %ax            \n"
"	cmp	$0x00, %ah                   \n"
"	je	int14_handler_asm_ah00       \n"
"                                    \n"
"	call int14_default_handler       \n"
"                                    \n"
"   mov %cs:int14_reg_sp, %sp        \n"
"   mov %cs:int14_reg_ss, %ss        \n"
"                                    \n"
POPA /*"	popa                             \n"*/
"	pop %es                          \n"
"	pop %ds                          \n"
"	iret                             \n");

#define HEAD_OF_ASM_HANDLER(func)  \
"	call " func "                    \n" \
"                                    \n" \
"   mov %cs:int14_reg_sp, %sp        \n" \
"   mov %cs:int14_reg_ss, %ss        \n" \
"                                    \n" \
POPA/*"	popa                             \n"*/ \
"	pop %es                          \n" \
"	pop %ds                          \n" 

__asm__ (
"int14_handler_asm_ah00:             \n"
HEAD_OF_ASM_HANDLER("int14_handler_ah00")
"	movw	%cs:int14_reg_ax, %ax    \n"
"	iret                             \n");


void int14_default_handler(void){
    char buf[128];
	unsigned int reg_ss;

    asm volatile("mov  %%ss, %w0" : "=a"(reg_ss) : );
    
    s_snprintf(buf, sizeof(buf), "INT 0x14\n");
    emuLogMessage(reg_ss, (uint16_t)buf);

    s_snprintf(buf, sizeof(buf), "AX: %04x BX: %04x CX: %04x DX: %04x \n", 
        int14_reg_ax, int14_reg_bx, int14_reg_cx, int14_reg_dx);
    emuLogMessage(reg_ss, (uint16_t)buf);
}

void int14_handler_ah00(void){
    // Initializing Serial Port Parameters

#ifdef INT14_DEBUG_PRINT
    s_printf("\nINT 0x14\n");
    s_printf("AX: %04x BX: %04x CX: %04x DX: %04x \n", 
        int14_reg_ax, int14_reg_bx, int14_reg_cx, int14_reg_dx);
#endif

    return ;
}

