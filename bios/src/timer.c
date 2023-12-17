#include <stdio.h>

#include "system.h"
#include "timer.h"
#include "basicio.h"
#include "asmfuncs.h"
#include "int10h.h"
#include "int16h.h"

unsigned int int_timer_reg_sp;
unsigned int int_timer_reg_ss;
unsigned int int_timer_reg_ss2;

#define PUSHA \
"mov %ss, %cs:int_timer_reg_ss2\n" \
"push %ax\n" \
"push %cx\n" \
"push %dx\n" \
"push %bx\n" \
"push %cs:int_timer_reg_ss2\n" \
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
"   .global int_timer_handler_asm    \n"
"int_timer_handler_asm:              \n"
"	push %ds                         \n"
"	push %es                         \n"
PUSHA /*"	pusha                            \n"*/
"                                    \n"
"   mov %sp, %cs:int_timer_reg_sp    \n"
"   mov %ss, %cs:int_timer_reg_ss    \n"
"                                    \n"
"	mov	%cs, %ax                     \n"
"	mov	%ax, %ss                     \n"
"	mov	%ax, %ds                     \n"
"	mov	$0xffe0, %sp                 \n"
"                                    \n"
"	call int_timer_default_handler   \n"
"                                    \n"
"   mov %cs:int_timer_reg_sp, %sp    \n"
"   mov %cs:int_timer_reg_ss, %ss    \n"
"                                    \n"
POPA /*"	popa                             \n"*/
"	pop %es                          \n"
"	pop %ds                          \n"
"	iret                             \n");



static unsigned long pc_timer_raw100 = 0;
static long TickOffset = 0;

void initTimer(void){
	// PSCLOCK -> 1MHz
	//systemOutByte(CLOCK_FREQUENCY_MHZ-2, IOADDR_CLKPRS);

	// Timer Setting (ch0)
	systemOutByte(0x34      , IOADDR_TCU_BASE+3); // write TMD (timer 0, l->h, mode 2, bin)
	unsigned int count = 1000*10;
	systemOutByte(count&0xff, IOADDR_TCU_BASE+0); // counter value (low)
	systemOutByte(count>>8  , IOADDR_TCU_BASE+0); // counter value (high)

	// Timer Setting (ch2)
	systemOutByte(0xb4      , IOADDR_TCU_BASE+3); // write TMD (timer 2, l->h, mode 2, bin)
	systemOutByte(count&0xff, IOADDR_TCU_BASE+2); // counter value (low)
	systemOutByte(count>>8  , IOADDR_TCU_BASE+2); // counter value (high)
}

void startTimer(void){
	// interval : 10ms
	pc_timer_raw100 = 0;

    asm volatile("cli" :  : );

	// interval : 10ms
	// interrupt vector : 0x08

	// Initialize the Interrupt Control Unit
	// Set up slave controller
	systemOutByte(0x11 + (0<<3)          , IOADDR_ICUSLV_BASE+0); // ICW1: Edge trigger(0)
	systemOutByte(0xf0                   , IOADDR_ICUSLV_BASE+1); // ICW2: Higher 5 bits of interrupt vector number
	systemOutByte(0x02                   , IOADDR_ICUSLV_BASE+1); // ICW3: no slave is attached and the internal slave is not used.
//	systemOutByte(0x00                   , IOADDR_ICUSLV_BASE+1); // ICW3: no slave is attached and the internal slave is not used.
	systemOutByte(0x01                   , IOADDR_ICUSLV_BASE+1); // ICW4: Normal Nesting(0), Self finish mode(1)
	systemOutByte(0xff                   , IOADDR_ICUSLV_BASE+1); // OCW1: interrupt request mask (INT0 is not masked)

	// Set up master controller
	systemOutByte(0x11 + (0<<3)          , IOADDR_ICUMST_BASE+0); // ICW1: Edge trigger(0)
	systemOutByte(0x08                   , IOADDR_ICUMST_BASE+1); // ICW2: Higher 5 bits of interrupt vector number
	systemOutByte(0x00                   , IOADDR_ICUMST_BASE+1); // ICW3: no slave is attached and the internal slave is not used.
	systemOutByte(0x01 + (0<<4) + (1<<1) , IOADDR_ICUMST_BASE+1); // ICW4: Normal Nesting(0), Self finish mode(1)
	systemOutByte(0xfe                   , IOADDR_ICUMST_BASE+1); // OCW1: interrupt request mask (INT0 is not masked)

	//asm volatile("sti" :  : );
}

unsigned long updateTickOffset(long newTick){
	TickOffset = 0;
	long tick = updateCounter();
	TickOffset = newTick - tick;
	tick = updateCounter();
	return tick;
}


unsigned long updateCounter(void){
	long tick;

	if( pc_timer_raw100 >= 24*3600L*100L ){
		pc_timer_raw100 -= 24*3600L*100L;
	}

	// 18.2 per 1s 
	// time100/100 * (18.2) = time100 * 182 / 1000 = time100 * 91 / 500
	tick  = pc_timer_raw100 * 9 /  50;
	tick += pc_timer_raw100     / 500;

	tick = tick + TickOffset;
	if(tick <                 0) tick += TICK_PER_HOUR*24;
	if(tick >= TICK_PER_HOUR*24) tick -= TICK_PER_HOUR*24;

	return tick;
}	

void int_timer_default_handler(void){
	pc_timer_raw100++;
	unsigned long tick = updateCounter();

	unsigned long __far * pTick     = _MK_FP(0x40, 0x6c);
	unsigned char __far * pRollOver = _MK_FP(0x40, 0x70);

	if( tick < *pTick ){
		*pRollOver = 1;
	}
	*pTick = tick;

	updateKeyInput();
}


unsigned long getTimer(void){
	return pc_timer_raw100;
}
unsigned long setTimer(unsigned long val){
	pc_timer_raw100 = val;
	return pc_timer_raw100;
}
void stopTimer(void){
	systemOutByte(0xff, IOADDR_ICUMST_BASE+1); // IIKW: interrupt request mask (All INT are masked)

    asm volatile("cli" :  : );
}

void restartTimer(void){
	systemOutByte(0xfe, IOADDR_ICUMST_BASE+1); // IIKW: interrupt request mask (INT0 is not masked)

    asm volatile("sti" :  : );
}
