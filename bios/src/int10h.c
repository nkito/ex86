/*
Implimentation of INT10h service routines.

This implementation is a bit sloppy. A part of the reason is that this code considers a terminal 
as a output device and it is hard to emulate video related functionalities.
But it works for many applications.
*/

#include <stdio.h>
#include "basicio.h"

#include "terminal.h"
#include "system.h"
#include "asmfuncs.h"
#include "emu_interface.h"
#include "int10h.h"
#include "int16h.h"


unsigned int int10_reg_ax;
unsigned int int10_reg_bx;
unsigned int int10_reg_cx;
unsigned int int10_reg_dx;
unsigned int int10_reg_di;
unsigned int int10_reg_es;

unsigned int int10_reg_sp;
unsigned int int10_reg_ss;

unsigned int int10_reg_ss2;

#define PUSHA \
"mov %ss, %cs:int10_reg_ss2\n" \
"push %ax\n" \
"push %cx\n" \
"push %dx\n" \
"push %bx\n" \
"push %cs:int10_reg_ss2\n" \
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
"   .global int10_handler_asm        \n"
"int10_handler_asm:                  \n"
"	push %ds                         \n"
"	push %es                         \n"
PUSHA //"	pusha                            \n"
"                                    \n"
"   mov %ax, %cs:int10_reg_ax        \n"
"   mov %bx, %cs:int10_reg_bx        \n"
"   mov %cx, %cs:int10_reg_cx        \n"
"   mov %dx, %cs:int10_reg_dx        \n"
"   mov %di, %cs:int10_reg_di        \n"
"   mov %es, %cs:int10_reg_es        \n"
"   mov %sp, %cs:int10_reg_sp        \n"
"   mov %ss, %cs:int10_reg_ss        \n"
"                                    \n"
"	mov	%cs, %ax                     \n"
"	mov	%ax, %ss                     \n"
"	mov	%ax, %ds                     \n"
"	mov	$0xffff, %ax                 \n"
"	mov	%ax, %sp                     \n"
"                                    \n"
"   mov int10_reg_ax, %ax            \n"
"	cmp	$0x00, %ah                   \n"
"	je	int10_handler_asm_ah00       \n"
"	cmp	$0x01, %ah                   \n"
"	je	int10_handler_asm_ah01       \n"
"	cmp	$0x02, %ah                   \n"
"	je	int10_handler_asm_ah02       \n"
"	cmp	$0x03, %ah                   \n"
"	je	int10_handler_asm_ah03       \n"
"	cmp	$0x05, %ah                   \n"
"	je	int10_handler_asm_ah05       \n"
"	cmp	$0x06, %ah                   \n"
"	je	int10_handler_asm_ah06       \n"
"	cmp	$0x07, %ah                   \n"
"	je	int10_handler_asm_ah07       \n"
"	cmp	$0x08, %ah                   \n"
"	je	int10_handler_asm_ah08       \n"
"	cmp	$0x09, %ah                   \n"
"	je	int10_handler_asm_ah09       \n"
"	cmp	$0x0a, %ah                   \n"
"	je	int10_handler_asm_ah0a       \n"
"	cmp	$0x0e, %ah                   \n"
"	je	int10_handler_asm_ah0e       \n"
"	cmp	$0x0f, %ah                   \n"
"	je	int10_handler_asm_ah0f       \n"
"	cmp	$0x10, %ah                   \n"
"	je	int10_handler_asm_ah10       \n"
"	cmp	$0x11, %ah                   \n"
"	je	int10_handler_asm_ah11       \n"
"	cmp	$0x12, %ah                   \n"
"	je	int10_handler_asm_ah12       \n"
"	cmp	$0x1a, %ah                   \n"
"	je	int10_handler_asm_ah1a       \n"
"	cmp	$0xfe, %ah                   \n"
"	je	int10_handler_asm_ahfe       \n"
"	cmp	$0xff, %ah                   \n"
"	je	int10_handler_asm_ahff       \n"
"                                    \n"
"	call int10_default_handler       \n"
"                                    \n"
"   mov %cs:int10_reg_sp, %ax        \n"
"   mov %ax, %sp                     \n"
"   mov %cs:int10_reg_ss, %ax        \n"
"   mov %ax, %ss                     \n"
"                                    \n"
POPA /*"	popa                             \n"*/
"	pop %es                          \n"
"	pop %ds                          \n"
"	iret                             \n");

#define HEAD_OF_ASM_HANDLER(func)  \
"	call " func "                    \n" \
"                                    \n" \
"   mov %cs:int10_reg_sp, %sp        \n" \
"   mov %cs:int10_reg_ss, %ss        \n" \
"                                    \n" \
POPA/* "	popa                             \n"*/ \
"	pop %es                          \n" \
"	pop %ds                          \n" 


__asm__ (
"int10_handler_asm_ah00:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah00")
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah01:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah01")
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah02:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah02")
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah03:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah03")
"   mov %cs:int10_reg_cx, %cx        \n"
"   mov %cs:int10_reg_dx, %dx        \n"
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah05:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah05")
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah06:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah06")
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah07:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah07")
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah08:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah08")
"   mov %cs:int10_reg_ax, %ax        \n"
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah09:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah09")
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah0a:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah0a")
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah0e:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah0e")
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah0f:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah0f")
"   mov %cs:int10_reg_bx, %ax        \n"
"   mov %ah, %bh                     \n"
"   mov %cs:int10_reg_ax, %ax        \n"
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah10:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah10")
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah11:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah11")
"	iret                             \n");

__asm__ (
"int10_handler_asm_ah12:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah12")
"   mov %cs:int10_reg_ax, %ax        \n"
"   mov %cs:int10_reg_bx, %bx        \n"
"   mov %cs:int10_reg_cx, %cx        \n"
"	iret                             \n");


__asm__ (
"int10_handler_asm_ah1a:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ah1a")
"   mov %cs:int10_reg_bx, %bx        \n"
"	iret                             \n");


__asm__ (
"int10_handler_asm_ahfe:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ahfe")
"   mov %cs:int10_reg_es, %es        \n"
"   mov $0, %di                      \n"
"	iret                             \n");

__asm__ (
"int10_handler_asm_ahff:             \n"
HEAD_OF_ASM_HANDLER("int10_handler_ahff")
"	iret                             \n");


void int10_default_handler(void){
    char buf[128];
	unsigned int reg_ss;

    asm volatile("mov  %%ss, %w0" : "=a"(reg_ss) : );

    s_snprintf(buf, sizeof(buf), "\nINT 0x10\n");
    emuLogMessage(reg_ss, (uint16_t)buf);
    s_snprintf(buf, sizeof(buf), "AX: %04x BX: %04x CX: %04x DX: %04x ES: %04x \n", 
        int10_reg_ax, int10_reg_bx, int10_reg_cx, int10_reg_dx, int10_reg_es);
    emuLogMessage(reg_ss, (uint16_t)buf);
}

#define MAX_X 80 /* Columns */
#define MAX_Y 25 /* Rows   */
#define PAGES 8

unsigned char disp_page = 0;
unsigned char posx[PAGES]   = {0,0,0,0,0,0,0,0}; /* 0 ... MAX_X - 1  */
unsigned char posy[PAGES]   = {0,0,0,0,0,0,0,0}; /* 0 ... MAX_Y - 1  */

static unsigned int getDispSeg(unsigned int page){
    return 0xd000 + (page << 8);
}

static void updateDispBuf(unsigned char x, unsigned char y, unsigned char *buf){
    unsigned int seg = getDispSeg( disp_page );

    unsigned int pos = ((y*MAX_X + x)<<1);
    *((unsigned int __far *) _MK_FP(   seg, pos)) = ((unsigned int *)buf)[0];
    *((unsigned int __far *) _MK_FP(0xb800, pos)) = ((unsigned int *)buf)[0];
}


static void set_cursorPos_memarea(unsigned char page){
    // Set the Cursor position on 40:50 - 40:57 in BIOS Data Area
    // See: https://stanislavs.org/helppc/bios_data_area.html

    // lower: col(x), higher: row(y), 0-based (checked it with VMWARE)

    unsigned int pos = 0x50 + (page<<1);

    *((unsigned char __far *) _MK_FP(0x40, pos+0)) = posx[page];
    *((unsigned char __far *) _MK_FP(0x40, pos+1)) = posy[page];
}

void set_cursorPosition(unsigned char page, unsigned char x, unsigned char y){
    /*
    page : 0-7
    x,y 0-based position
    */
    if(page >= PAGES) return ;
    posx[page] = x;
    posy[page] = y;
    set_cursorPos_memarea(page);
}

void int10_handler_ah00(void){
    // Set Video Mode

    if( (int10_reg_ax & 0x0080) == 0 ){
        // if bit7 of AL is 1, clearing the display is not necessary.
        unsigned char buf[2*MAX_X];
        unsigned int seg = getDispSeg( disp_page );

        for(int i=0; i<sizeof(buf) ; i+=2){
            buf[i+1] = 0x07;
            buf[i+0] = ' ';
        }

        for(int y = 0; y < MAX_Y; y++){
            copy_data_word (buf,    seg, (unsigned char *)((y*MAX_X)<<1), MAX_X);
            copy_data_word (buf, 0xb800, (unsigned char *)((y*MAX_X)<<1), MAX_X);
        }

        posx[disp_page] = posy[disp_page] = 0;
        set_cursorPos_memarea( disp_page );
        termGoTo( 0, 0 );
    }

    return ;
}

void int10_handler_ah01(void){
    // Set Cursor Type
    return ;
}

void int10_handler_ah02(void){
    // Set Cursor Position
    int page = ((int10_reg_bx>>8) & 0xff);
    if(page >= PAGES) page = PAGES -1;

    // DH: row(y), DL: column(x)
    posy[page] = (int10_reg_dx>>8);
    posx[page] = (int10_reg_dx&0xff);

    if(posx[page] >= MAX_X) posx[page] = MAX_X - 1;
    if(posy[page] >= MAX_Y) posy[page] = MAX_Y - 1;

    if( page == disp_page ){
        termGoTo( posx[page]+1, posy[page]+1 );
    }

    set_cursorPos_memarea( page );
}

void int10_handler_ah03(void){
    // Read Cursor Position and Size
    int page = ((int10_reg_bx>>8) & 0xff);
    page = (page >= PAGES) ? PAGES -1 : page;

    int10_reg_cx = 0x0607;  // Cursor Start and End line values (Default value for CGA)
    int10_reg_dx = posx[page] + (posy[page]<<8); // 0x1950;
}

void int10_handler_ah05(void){
    // Select Active Display Page
    int page = (int10_reg_ax & 0xff);
    disp_page = (page >= PAGES) ? PAGES -1 : page;

    termGoTo( posx[disp_page]+1, posy[disp_page]+1 );
}

#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))

void int10_handler_ah06(void){
    // Scroll Window Up
    // (bottom lines are blanked)

    int lines = (int10_reg_ax & 0xff);      // AL: #lines
    int color = ((int10_reg_bx>>8) & 0xff); // BH: attribute

    int x1, x2, y1, y2;
    // from  row(y): CH col(x): CL
    // to    row(y): DH col(x): DL
    y1 = ((int10_reg_cx>>8) & 0xff);
    x1 = ( int10_reg_cx     & 0xff);
    y2 = ((int10_reg_dx>>8) & 0xff);
    x2 = ( int10_reg_dx     & 0xff);

    if(y2 >= MAX_Y) y2 = MAX_Y-1;
    if(x2 >= MAX_X) x2 = MAX_X-1;

    if( x2 < x1 || y2 < y1 ) return;

    if( lines == 0 ){
        lines = MAX_Y;
    }

    if( x2 >= x1 && y2 >= y1 ){
        unsigned char buf[2*MAX_X];
        unsigned int seg = getDispSeg( disp_page );
        for(int y= y1; y <= y2 - lines ; y++){
            fetch_data_word(     seg, (unsigned char *)(((y+lines)*MAX_X + x1)<<1), buf, x2 - x1 +1);
            copy_data_word (buf, seg, (unsigned char *)(( y       *MAX_X + x1)<<1),      x2 - x1 +1);

            fetch_data_word(     0xb800, (unsigned char *)(((y+lines)*MAX_X + x1)<<1), buf, x2 - x1 +1);
            copy_data_word (buf, 0xb800, (unsigned char *)(( y       *MAX_X + x1)<<1),      x2 - x1 +1);
        }
        for(int i=0; i<sizeof(buf) ; i+=2){
            buf[i+1] = color;
            buf[i+0] = ' ';
        }
        for(int y = MAX(y1, y2 - lines+1); y <= y2; y++){
            copy_data_word (buf,    seg, (unsigned char *)((y*MAX_X + x1)<<1), x2 - x1 +1);
            copy_data_word (buf, 0xb800, (unsigned char *)((y*MAX_X + x1)<<1), x2 - x1 +1);
        }
    }
}

void int10_handler_ah07(void){
    // Scroll Window Down
    // (upper lines are blanked)

    int lines = (int10_reg_ax & 0xff);      // AL: #lines
    int color = ((int10_reg_bx>>8) & 0xff); // BH: attribute

    int x1, x2, y1, y2;
    // from  row(y): CH col(x): CL
    // to    row(y): DH col(x): DL
    y1 = ((int10_reg_cx>>8) & 0xff);
    x1 = ( int10_reg_cx     & 0xff);
    y2 = ((int10_reg_dx>>8) & 0xff);
    x2 = ( int10_reg_dx     & 0xff);

    if(y2 >= MAX_Y) y2 = MAX_Y-1;
    if(x2 >= MAX_X) x2 = MAX_X-1;

    if( x2 < x1 || y2 < y1 ) return;

    if( lines == 0 ){
        lines = MAX_Y;
    }

    if( x2 >= x1 && y2 >= y1 ){
        unsigned char buf[2*MAX_X];
        unsigned int seg = getDispSeg( disp_page );
        for(int y= y2; y >= y1+lines ; y--){
            fetch_data_word(     seg, (unsigned char *)(((y-lines)*MAX_X + x1)<<1), buf, x2 - x1+1);
            copy_data_word (buf, seg, (unsigned char *)(( y       *MAX_X + x1)<<1),      x2 - x1+1);

            fetch_data_word(     0xb800, (unsigned char *)(((y-lines)*MAX_X + x1)<<1), buf, x2 - x1+1);
            copy_data_word (buf, 0xb800, (unsigned char *)(( y       *MAX_X + x1)<<1),      x2 - x1+1);
        }
        for(int i=0; i<sizeof(buf) ; i+=2){
            buf[i+1] = color;
            buf[i+0] = ' ';
        }
        for(int y = y1; y <= MIN(y2, y1 + lines-1); y++){
            copy_data_word (buf,    seg, (unsigned char *)((y*MAX_X + x1)<<1), x2 - x1+1);
            copy_data_word (buf, 0xb800, (unsigned char *)((y*MAX_X + x1)<<1), x2 - x1+1);
        }
    }

}

void int10_handler_ah08(void){
    // Read Character and Attribute at Cursor Position

    unsigned char p = ( (int10_reg_bx>>8) & 0xf);
    if( p >= PAGES ) p=0;
    fetch_data_word(0xb800, (unsigned char *)((posy[p]*MAX_X + posx[p])<<1), (unsigned char *)&int10_reg_ax, 1);
}

void putCharWithAttr(char c, unsigned char page, unsigned char attr){
    unsigned char buf[2];

    if( page != disp_page ) return ;

    if( c == '\n' ){
        posy[disp_page] ++;
    }else if( c == '\r'  ){
        posx[disp_page] = 0;
    }else if( c == '\b' ){
        posx[disp_page] = (posx[disp_page] == 0) ? 0 : posx[disp_page] - 1;
    }else if( c == '\a' ){
        ;
    }else{
        buf[0] = c;
        buf[1] = attr;
        updateDispBuf(posx[disp_page], posy[disp_page], buf );
        posx[disp_page]++;
    }

    if( posx[disp_page] >= MAX_X ){
        posx[disp_page] = (posx[disp_page] % MAX_X);
        posy[disp_page] += 1;
    }
    if( posy[disp_page] >= MAX_Y ){
        posy[disp_page] = MAX_Y - 1;

        unsigned char buf[2*MAX_X];
        unsigned int seg = getDispSeg( disp_page );
        for(int y= 0; y+1 < MAX_Y; y++){
            fetch_data_word(     seg, (unsigned char *)(((y+1)*MAX_X)<<1), buf, MAX_X);
            copy_data_word (buf, seg, (unsigned char *)(( y   *MAX_X)<<1),      MAX_X);

            fetch_data_word(     0xb800, (unsigned char *)(((y+1)*MAX_X)<<1), buf, MAX_X);
            copy_data_word (buf, 0xb800, (unsigned char *)(( y   *MAX_X)<<1),      MAX_X);
        }
        for(int i=0; i<sizeof(buf) ; i+=2){
            buf[i+1] = 0x07;
            buf[i+0] = ' ';
        }

        copy_data_word (buf,    seg, (unsigned char *)(((MAX_Y-1)*MAX_X)<<1), MAX_X);
        copy_data_word (buf, 0xb800, (unsigned char *)(((MAX_Y-1)*MAX_X)<<1), MAX_X);
    }

    set_cursorPos_memarea( disp_page );
}

void int10_handler_ah09(void){
    // Write Character and Attribute at Cursor Position
    // The cursor does not move (but the characters print out from left to right)

    unsigned char page = ((int10_reg_bx>>8)&0xff);
    char c             = (int10_reg_ax&0xff);
    unsigned char attr = (int10_reg_bx&0xff);
    unsigned char buf[2];

    if( page != disp_page ) return ;

    unsigned char px = posx[disp_page];
    unsigned char py = posy[disp_page];

    buf[0] = c;
    buf[1] = attr;
    for(unsigned int i=0; i < int10_reg_cx; i++ ){
        updateDispBuf( px, py, buf );

        if( ++px >= MAX_X ){ px = 0; py += 1; }
        if(   py >= MAX_Y ){ py = 0; }
    }
}


void int10_handler_ah0a(void){
    // Write character at current cursor
    int10_handler_ah09();
}

void int10_handler_ah0e(void){
    // Write Text in Teletype Mode
    /*
    This service responds to the ASCII meanings of
    characters 07h (bell), 08h (backspace), 0Ah (line
    feed), and 0Dh (carriage return). All other ASCII
    values result in the displaying of a character, with
    the cursor moving one position.

    See: http://vitaly_filatov.tripod.com/ng/asm/asm_023.15.html
    */

    unsigned char page = ((int10_reg_bx>>8)&0xff);
    char c             = (int10_reg_ax&0xff);
    unsigned char attr = (int10_reg_bx&0xff);

    if( page != disp_page ) return ;

    if( c == '\n' ){
        posy[disp_page] ++;
    }else if( c == '\r'  ){
        posx[disp_page] = 0;
    }else if( c == '\b' ){
        posx[disp_page] = (posx[disp_page] == 0) ? 0 : posx[disp_page] - 1;
    }else if( c == '\a' ){
        ;
    }else{
        unsigned char buf[2];
        buf[0] = c;
        buf[1] = attr;
        updateDispBuf(posx[disp_page], posy[disp_page], buf );
        posx[disp_page]++;
    }

    if( posx[disp_page] >= MAX_X ){
        posx[disp_page] = (posx[disp_page] % MAX_X);
        posy[disp_page] += 1;
    }
    if( posy[disp_page] >= MAX_Y ){
        posy[disp_page] = MAX_Y - 1;

        unsigned char buf[2*MAX_X];
        unsigned int seg = getDispSeg( disp_page );
        for(int y= 0; y+1 < MAX_Y; y++){
            fetch_data_word(     seg, (unsigned char *)(((y+1)*MAX_X)<<1), buf, MAX_X);
            copy_data_word (buf, seg, (unsigned char *)(( y   *MAX_X)<<1),      MAX_X);

            fetch_data_word(     0xb800, (unsigned char *)(((y+1)*MAX_X)<<1), buf, MAX_X);
            copy_data_word (buf, 0xb800, (unsigned char *)(( y   *MAX_X)<<1),      MAX_X);
        }
        for(int i=0; i<sizeof(buf) ; i+=2){
            buf[i+1] = 0x07;
            buf[i+0] = ' ';
        }

        copy_data_word (buf,    seg, (unsigned char *)(((MAX_Y-1)*MAX_X)<<1), MAX_X);
        copy_data_word (buf, 0xb800, (unsigned char *)(((MAX_Y-1)*MAX_X)<<1), MAX_X);
    }

    set_cursorPos_memarea( disp_page );  
}

void int10_handler_ah0f(void){
    // Get Video State

    int10_reg_bx = (disp_page<<8);    // BH : current display page
    // AH: #char columns per line
    // AL: Video mode
    int10_reg_ax = 0x5003;    // 03h     Text          80 x 25      All but MDA   16 fore/8 back    B8000
}

void int10_handler_ah10(void){
    // Set Palette Registers

    if( (int10_reg_ax&0xff) == 3 ){
        // Toggle intensify/blinking bit
        return ;
    }else{
    	unsigned int reg_ss;
        char buf[64];

        asm volatile("mov  %%ss, %w0" : "=a"(reg_ss) : );

        s_sprintf(buf, "INT 0x10\n");
        emuLogMessage(reg_ss, (uint16_t)buf);
        s_snprintf(buf, sizeof(buf), "AX: %04x BX: %04x CX: %04x DX: %04x ES: %04x \n", 
            int10_reg_ax, int10_reg_bx, int10_reg_cx, int10_reg_dx, int10_reg_es);
        emuLogMessage(reg_ss, (uint16_t)buf);
    }

    return ;
}

void int10_handler_ah11(void){
    //  Character Generator Routine 

    return ;
}

void int10_handler_ah12(void){
    // Video subsystem configuration

    if( (int10_reg_bx&0xff) == 0x10 ){
        int10_reg_bx = 0x0000;  // color mode, 64k EGA memory
        int10_reg_cx = 0x0000;
    }else if( (int10_reg_bx&0xff) == 0x32 ){
        // CPU access to video RAM
        int10_reg_ax = 0x1212;
    }else{
        int10_default_handler();
    }
}


void int10_handler_ah1a(void){
    int10_reg_bx = 0x02; // CGA with color display

    return ;
}

void int10_handler_ahfe(void){
    int10_reg_es = getDispSeg( disp_page );
    unsigned char buf[2];

    buf[0] = ' ';
    buf[1] = 0;
    for(int i=0; i<80*25; i++){
        copy_data( buf, int10_reg_es, (unsigned char *)(2*i), 2);
        //copy_data( buf,       0xb800, (unsigned char *)(2*i), 2); // ADDED
    }

    return ;
}

void int10_handler_ahff(void){
    unsigned char buf[2];
    int10_reg_di = (int10_reg_di % 4000);

    for(int i=0; i<int10_reg_cx; i++){
        buf[0] = *((unsigned char __far *) _MK_FP(int10_reg_es, int10_reg_di + 2*i    ));
        buf[1] = *((unsigned char __far *) _MK_FP(int10_reg_es, int10_reg_di + 2*i + 1));

        *((unsigned char __far *) _MK_FP(0xb800, int10_reg_di + 2*i    )) = buf[0];
        *((unsigned char __far *) _MK_FP(0xb800, int10_reg_di + 2*i + 1)) = buf[1];
    }

    return ;
}

