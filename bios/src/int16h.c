#include <stdio.h>
#include "basicio.h"

#include "terminal.h"
#include "system.h"
#include "asmfuncs.h"
#include "emu_interface.h"

unsigned int int16_reg_ax;
unsigned int int16_reg_bx;
unsigned int int16_reg_cx;
unsigned int int16_reg_dx;
unsigned int int16_reg_es;

unsigned int int16_reg_sp;
unsigned int int16_reg_ss;

unsigned int int16_zf;

unsigned int int16_reg_ss2;

#define PUSHA \
"mov %ss, %cs:int16_reg_ss2\n" \
"push %ax\n" \
"push %cx\n" \
"push %dx\n" \
"push %bx\n" \
"push %cs:int16_reg_ss2\n" \
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

unsigned int keybuf;
int  keybuf_avail = 0;

unsigned char convert_table[128] = {
//     0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f
	0x00, 0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18,  // 0x
	0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00,  // 1x
	0x00, 0x02, 0x00, 0x04, 0x05, 0x06, 0x08, 0x00, 0x0A, 0x0B, 0x09, 0x00, 0x00, 0x00, 0x53, 0x00,  // 2x
	0x0B, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 3x
	0x03, 0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18,  // 4x
	0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C, 0x00, 0x00, 0x00, 0x07, 0x00,  // 5x
	0x00, 0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18,  // 6x
	0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00   // 7x
};

__asm__ (
"   .global int16_handler_asm        \n"
"int16_handler_asm:                  \n"
"	push %ds                         \n"
"	push %es                         \n"
PUSHA /*"	pusha                            \n"*/
"                                    \n"
"   mov %ax, %cs:int16_reg_ax        \n"
"   mov %bx, %cs:int16_reg_bx        \n"
"   mov %cx, %cs:int16_reg_cx        \n"
"   mov %dx, %cs:int16_reg_dx        \n"
"   mov %es, %cs:int16_reg_es        \n"
"   mov %sp, %cs:int16_reg_sp        \n"
"   mov %ss, %cs:int16_reg_ss        \n"
"                                    \n"
"	mov	%cs, %ax                     \n"
"	mov	%ax, %ss                     \n"
"	mov	%ax, %ds                     \n"
"	mov	$0xffff, %sp                 \n"
"                                    \n"
"   mov int16_reg_ax, %ax            \n"
"	cmp	$0x00, %ah                   \n"
"	je	int16_handler_asm_ah00       \n"
"	cmp	$0x10, %ah                   \n"
"	je	int16_handler_asm_ah10       \n"
"	cmp	$0x01, %ah                   \n"
"	je	int16_handler_asm_ah01       \n"
"	cmp	$0x11, %ah                   \n"
"	je	int16_handler_asm_ah11       \n"
"	cmp	$0x02, %ah                   \n"
"	je	int16_handler_asm_ah02       \n"
"	cmp	$0x05, %ah                   \n"
"	je	int16_handler_asm_ah05       \n"
"	cmp	$0x12, %ah                   \n"
"	je	int16_handler_asm_ah12       \n"
"                                    \n"
"	call int16_default_handler       \n"
"                                    \n"
"   mov %cs:int16_reg_sp, %sp        \n"
"   mov %cs:int16_reg_ss, %ss        \n"
"                                    \n"
POPA /*"	popa                             \n"*/
"	pop %es                          \n"
"	pop %ds                          \n"
"	iret                             \n");

#define HEAD_OF_ASM_HANDLER(func)  \
"	call " func "                    \n" \
"                                    \n" \
"   mov %cs:int16_reg_sp, %sp        \n" \
"   mov %cs:int16_reg_ss, %ss        \n" \
"                                    \n" \
POPA /*"	popa                             \n"*/ \
"	pop %es                          \n" \
"	pop %ds                          \n" 

#define IRET_SAVE_ZEROFLAG  \
"   push %ax                         \n" \
"   push %bp                         \n" \
"   mov  %sp, %bp                    \n" \
"   movw 8(%bp), %ax                 \n" \
"                                    \n" \
"   jz 1f                            \n" \
"                                    \n" \
"   andw $0xffbf, %ax                \n" /* clear the zero flag */ \
"   movw %ax, 8(%bp)                 \n" \
"   pop  %bp                         \n" \
"   pop  %ax                         \n" \
"   iret                             \n" \
"                                    \n" \
"1:                                  \n" \
"   orw  $0x0040, %ax                \n" /* set the zero flag */ \
"   movw %ax, 8(%bp)                 \n" \
"   pop  %bp                         \n" \
"   pop  %ax                         \n" \
"   iret                             \n"


__asm__ (
"int16_handler_asm_ah00:             \n"
HEAD_OF_ASM_HANDLER("int16_handler_ah00")
"	mov	%cs:int16_reg_ax, %ax        \n"
"   iret                             \n");

__asm__ (
"int16_handler_asm_ah10:             \n"
HEAD_OF_ASM_HANDLER("int16_handler_ah10")
"	mov	%cs:int16_reg_ax, %ax        \n"
"   iret                             \n");

__asm__ (
"int16_handler_asm_ah01:             \n"
HEAD_OF_ASM_HANDLER("int16_handler_ah01")
"	mov	 %cs:int16_reg_ax, %ax        \n"
"	cmpw $1, %cs:int16_zf             \n"
IRET_SAVE_ZEROFLAG);

__asm__ (
"int16_handler_asm_ah11:             \n"
HEAD_OF_ASM_HANDLER("int16_handler_ah11")
"	mov	 %cs:int16_reg_ax, %ax        \n"
"	cmpw $1, %cs:int16_zf             \n"
IRET_SAVE_ZEROFLAG);

__asm__ (
"int16_handler_asm_ah02:             \n"
HEAD_OF_ASM_HANDLER("int16_handler_ah02")
"	movb %cs:int16_reg_ax, %al       \n"
"   iret                             \n");

__asm__ (
"int16_handler_asm_ah05:             \n"
HEAD_OF_ASM_HANDLER("int16_handler_ah05")
"	movb %cs:int16_reg_ax, %al       \n"
"   iret                             \n");

__asm__ (
"int16_handler_asm_ah12:             \n"
HEAD_OF_ASM_HANDLER("int16_handler_ah12")
"	mov	 %cs:int16_reg_ax, %ax        \n"
"   iret                             \n");

unsigned int getKeyCodeNonBlocking(void);

void int16_default_handler(void){
	char buf[128];
	unsigned int reg_ss;

    asm volatile("mov  %%ss, %w0" : "=a"(reg_ss) : );

    s_snprintf(buf, sizeof(buf),
		"INT 0x16\n");
    emuLogMessage(reg_ss, (uint16_t)buf);

    s_snprintf(buf, sizeof(buf),
		"AX: %04x BX: %04x CX: %04x DX: %04x ES: %04x \n", 
        int16_reg_ax, int16_reg_bx, int16_reg_cx, int16_reg_dx, int16_reg_es);
    emuLogMessage(reg_ss, (uint16_t)buf);
}


void int16_handler_ah00(void){
    if( keybuf_avail == 0 ){
        do{
            keybuf = getKeyCodeNonBlocking();
        }while(keybuf == 0);
    }
    int16_zf     = 0;
    int16_reg_ax = keybuf;
    keybuf_avail = 0;
}

void int16_handler_ah10(void){
    if( keybuf_avail == 0 ){
        do{
            keybuf = getKeyCodeNonBlocking();
        }while(keybuf == 0);
    }

    int16_zf     = 0;
    int16_reg_ax = keybuf;
    keybuf_avail = 0;
}

void int16_handler_ah01(void){
    unsigned int c;

    if( (keybuf_avail == 0) && (0 != (c = getKeyCodeNonBlocking())) ){
        keybuf = c;
        keybuf_avail = 1;
    }

    int16_zf     = (keybuf_avail ? 0 : 1);
    int16_reg_ax = (keybuf_avail ? keybuf : 0);
}

void int16_handler_ah11(void){
    unsigned int c;

    if( (keybuf_avail == 0) && (0 != (c = getKeyCodeNonBlocking())) ){
        keybuf = c;
        keybuf_avail = 1;
    }

    int16_zf     = (keybuf_avail ? 0 : 1);
    int16_reg_ax = (keybuf_avail ? keybuf : 0);
}

void int16_handler_ah02(void){
    // Read Keyboard Flags
    int16_reg_ax = 0;
}

void int16_handler_ah12(void){
    // Extended Get Keyboard Status
    int16_reg_ax = 0;
}

int  isKeyBufFull(void);
void insertKeyBuf(unsigned int code);

void int16_handler_ah05(void){
    // Keyboard Buffer Write

	if( !isKeyBufFull() ){
		insertKeyBuf( int16_reg_cx );
	    int16_reg_ax = 0;
	}else{
	    int16_reg_ax = 1;
	}
}


//--------------------------------------------------------
// 
//--------------------------------------------------------
static void getkeyDBGprint(char *str){  /* s_printf(str); */  }

#define KEYBUF_SIZE		16
#define KEYBUF_START	0x1e

unsigned int __far * pKeyBufHead = _MK_FP(0x40, 0x1a);
unsigned int __far * pKeyBufTail = _MK_FP(0x40, 0x1c);
unsigned int __far * pKeyBuf     = _MK_FP(0x40, KEYBUF_START);

void insertKeyBuf(unsigned int code){
	unsigned int KeyBufHead = *pKeyBufHead - KEYBUF_START;
	unsigned int KeyBufTail = *pKeyBufTail - KEYBUF_START;

	if(   KeyBufTail >= KEYBUF_SIZE) KeyBufTail = KeyBufHead = 0;
	pKeyBuf[KeyBufTail] = code;
	if( ++KeyBufTail >= KEYBUF_SIZE) KeyBufTail = 0;

	*pKeyBufHead = KeyBufHead + KEYBUF_START;
	*pKeyBufTail = KeyBufTail + KEYBUF_START;
}

int isKeyBufFull(void){
	unsigned int KeyBufHead = *pKeyBufHead - KEYBUF_START;
	unsigned int KeyBufTail = *pKeyBufTail - KEYBUF_START;

	if( (KeyBufHead -1 == KeyBufTail) || (KeyBufHead + KEYBUF_SIZE - 1 == KeyBufTail) ){
		getkeyDBGprint("Key Buffer Full\n");
		return 1;
	}
	return 0;
}

//------------------------------------------------------------------------
// Important:
// Double inputs of ESC key are treated as a input of ESC key to avoid 
// misinput of ESC key.
// 
// Special keys such as arrows and function keys are input as ESC sequences.
// When this BIOS was used in single board computer environments, 
// it tends to fail identifying inputs of those keys and they may be 
// recognized as inputs of ESC.
//------------------------------------------------------------------------

int isKeyBufEmpty(void){
	unsigned int KeyBufHead = *pKeyBufHead - KEYBUF_START;
	unsigned int KeyBufTail = *pKeyBufTail - KEYBUF_START;
	unsigned int code;

	if( KeyBufHead == KeyBufTail ) return 1;

	code = pKeyBuf[KeyBufHead];
	if( (code&0xff) == 0x1b ){	// ESC
		unsigned int nextHead = KeyBufHead + 1;
		if( nextHead >= KEYBUF_SIZE) nextHead= 0;
		if( nextHead == KeyBufTail ){
			return 1;
		}
	}

	return 0;
}

unsigned int getKeyBufItem(void){
	unsigned int KeyBufHead = *pKeyBufHead - KEYBUF_START;
	unsigned int KeyBufTail = *pKeyBufTail - KEYBUF_START;
	unsigned int code;

	unsigned int oldHead = KeyBufHead;
	if(++KeyBufHead >= KEYBUF_SIZE) KeyBufHead = 0;

	code = pKeyBuf[oldHead];
	if( (code&0xff) == 0x1B ){	// ESC
		oldHead = KeyBufHead;
		if(++KeyBufHead >= KEYBUF_SIZE) KeyBufHead = 0;
		code = pKeyBuf[oldHead];
	}

	*pKeyBufHead = KeyBufHead + KEYBUF_START;
	*pKeyBufTail = KeyBufTail + KEYBUF_START;

	return code;
}


void updateKeyInput(void){
	const int ESC_TIMEOUT = 100;

	unsigned char buf[10];
    char cbuf[32];
	int nrecv = 0;
	volatile int li;
	unsigned char c;

	if( ! systemIsUSARTDataAvailable() ) return;
	if( isKeyBufFull() ) return ;

	c = systemSerialGetc();

	// ESC
	if( c == 0x1b ){
		nrecv = 0;

		for(li=0; li < ESC_TIMEOUT; li++) if( systemIsUSARTDataAvailable() ) break;
		if(li == ESC_TIMEOUT) goto standard_process;

		systemSerialPutc(0x1b);
		c = systemSerialGetc();
		if(c == '['){
			systemSerialPutc(c);
			while(1){
				for(li=0; li < ESC_TIMEOUT; li++) if( systemIsUSARTDataAvailable() ) break;
				if(li == ESC_TIMEOUT) break;
				buf[nrecv++] = systemSerialGetc();
			}
			systemSerialPutc( '1' );
			systemSerialPutc( 0x7e );

			if( nrecv == 1 ){
				if( buf[0] == 'A' ){ getkeyDBGprint("UP   \n"); insertKeyBuf(0x4800); return; } // UP
				if( buf[0] == 'B' ){ getkeyDBGprint("DOWN \n"); insertKeyBuf(0x5000); return; } // Down
				if( buf[0] == 'C' ){ getkeyDBGprint("RIGHT\n"); insertKeyBuf(0x4D00); return; } // Right
				if( buf[0] == 'D' ){ getkeyDBGprint("LEFT \n"); insertKeyBuf(0x4B00); return; } // Left

			}else if( nrecv == 2 && buf[1] == 0x7e ){
				if( buf[0] == 0x31 ){ getkeyDBGprint("HOME\n"); insertKeyBuf(0x4700); return; } // HOME
				if( buf[0] == 0x32 ){ getkeyDBGprint("INS \n"); insertKeyBuf(0x5200); return; } // INS
				if( buf[0] == 0x34 ){ getkeyDBGprint("END \n"); insertKeyBuf(0x4F00); return; } // END
				if( buf[0] == 0x35 ){ getkeyDBGprint("PGUP\n"); insertKeyBuf(0x4900); return; } // PGUP
				if( buf[0] == 0x36 ){ getkeyDBGprint("PGDN\n"); insertKeyBuf(0x5100); return; } // PGDN

			}else if( nrecv == 3 && buf[2] == 0x7e ){
				if( buf[0] == 0x31 && buf[1] == 0x31 ){ getkeyDBGprint("F1 \n"); insertKeyBuf(0x3B00); return; } // F1
				if( buf[0] == 0x31 && buf[1] == 0x32 ){ getkeyDBGprint("F2 \n"); insertKeyBuf(0x3C00); return; } // F2
				if( buf[0] == 0x31 && buf[1] == 0x33 ){ getkeyDBGprint("F3 \n"); insertKeyBuf(0x3D00); return; } // F3
				if( buf[0] == 0x31 && buf[1] == 0x34 ){ getkeyDBGprint("F4 \n"); insertKeyBuf(0x3E00); return; } // F4
				if( buf[0] == 0x31 && buf[1] == 0x35 ){ getkeyDBGprint("F5 \n"); insertKeyBuf(0x3F00); return; } // F5
				if( buf[0] == 0x31 && buf[1] == 0x37 ){ getkeyDBGprint("F6 \n"); insertKeyBuf(0x4000); return; } // F6
				if( buf[0] == 0x31 && buf[1] == 0x38 ){ getkeyDBGprint("F7 \n"); insertKeyBuf(0x4100); return; } // F7
				if( buf[0] == 0x31 && buf[1] == 0x39 ){ getkeyDBGprint("F8 \n"); insertKeyBuf(0x4200); return; } // F8
				if( buf[0] == 0x32 && buf[1] == 0x30 ){ getkeyDBGprint("F9 \n"); insertKeyBuf(0x4300); return; } // F9
				if( buf[0] == 0x32 && buf[1] == 0x31 ){ getkeyDBGprint("F10\n"); insertKeyBuf(0x4400); return; } // F10
				if( buf[0] == 0x32 && buf[1] == 0x33 ){ getkeyDBGprint("F11\n"); insertKeyBuf(0x8500); return; } // F11
				if( buf[0] == 0x32 && buf[1] == 0x34 ){ getkeyDBGprint("F12\n"); insertKeyBuf(0x8600); return; } // F12
			}

			getkeyDBGprint(" Unknown input (Received: ESC [ ");
			for(int i=0; i<nrecv; i++){
                s_sprintf(cbuf, " 0x%02x", (unsigned int)buf[i]);
            	getkeyDBGprint( cbuf );
            }
			getkeyDBGprint(")\r\n");

			return;
		}else{
			//systemSerialPutc(c);
			buf[nrecv++] = c;
			while(1){
				for(li=0; li < ESC_TIMEOUT; li++) if( systemIsUSARTDataAvailable() ) break;
				if(li == ESC_TIMEOUT) break;
				c = systemSerialGetc();
				//systemSerialPutc(c);
				buf[nrecv++] = c;
			}
			systemSerialPutc( '[' );
			systemSerialPutc( '1' );
			systemSerialPutc( 0x7e );

			if( nrecv == 2 && buf[0] == 0x4f ){
				if( buf[1] == 0x50 ){ getkeyDBGprint("NumLK\n");                       return; } // NumLock
				if( buf[1] == 0x51 ){ getkeyDBGprint("</>  \n"); insertKeyBuf(0x352F); return; } // </>
				if( buf[1] == 0x52 ){ getkeyDBGprint("<*>  \n"); insertKeyBuf(0x372A); return; } // <*>
				if( buf[1] == 0x53 ){ getkeyDBGprint("<->  \n"); insertKeyBuf(0x4A2D); return; } // <->
			}

			getkeyDBGprint(" Unknown input (Received: ESC ");
			for(int i=0; i<nrecv; i++){
                s_sprintf(cbuf, " 0x%02x", (unsigned int)buf[i]);
            	getkeyDBGprint( cbuf );
            }
			getkeyDBGprint(")\r\n");

			return;
		}
	}

	standard_process:
	getkeyDBGprint("Received: ");
	s_sprintf(cbuf, "0x%02x", (unsigned int)c);
	getkeyDBGprint( cbuf );
	getkeyDBGprint("\r\n");

	// The key-code for [delete] key is "0x7f"(DEL).
	// "0x7f"(DEL) does work fine. 
	if( c == 0x7f ){
		getkeyDBGprint("[delete] key\n");
		insertKeyBuf(0x5300); // delete key
		return;
	}

	insertKeyBuf((unsigned int)c + (((unsigned int)convert_table[c&0x7f])<<8));
}


unsigned int getKeyCodeNonBlocking(void){
	updateKeyInput();
	if( isKeyBufEmpty() ) return 0;
	return getKeyBufItem();
}
