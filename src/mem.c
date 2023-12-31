#include <stdio.h>
#include <stdint.h>

#include "i8086.h"
#include "terminal.h"
#include "mem.h"
#include "logfile.h"

#include "ExInst_common.h"

inline uint32_t readPhysMemDoubleWord(struct stMachineState *pM, uint32_t addr){
    uint32_t addr1 = addr + 1;
    uint32_t addr2 = addr + 2;
    uint32_t addr3 = addr + 3;

/*
    if( addr  >= EMU_MEM_SIZE ) return 0; // addr  = (addr  % EMU_MEM_SIZE);
    if( addr1 >= EMU_MEM_SIZE ) return 0; // addr1 = (addr1 % EMU_MEM_SIZE);
    if( addr2 >= EMU_MEM_SIZE ) return 0; // addr2 = (addr2 % EMU_MEM_SIZE);
*/
    if( addr3 >= EMU_MEM_SIZE ) return 0; // addr3 = (addr3 % EMU_MEM_SIZE);

    uint32_t result =
           (  ((uint32_t)(pM->mem.mem[addr ]))       |
             (((uint32_t)(pM->mem.mem[addr1])) << 8) |
             (((uint32_t)(pM->mem.mem[addr2])) <<16) |
             (((uint32_t)(pM->mem.mem[addr3])) <<24) );

    return result;
}

inline void writePhysMemByte(struct stMachineState *pM, uint32_t addr, uint8_t data){
    if( addr >= EMU_MEM_SIZE ) return ;

    pM->mem.mem[addr] = data;
}

int checkLinearAccessible(struct stMachineState *pM, uint32_t linear){
    uint32_t dir, pde, ptable, pte;

    if( 0 == (REG_CR0 & (1<<CR0_BIT_PG)) ){
        return 1;
    }

    dir     = (REG_CR3 & 0xfffff000); // pM->reg.cr[3];
    dir    += ( ((linear >> 22) & ((1<<10)-1)) << 2);

    pde     = readPhysMemDoubleWord(pM, dir);
    ptable  = (pde & 0xfffff000);
    ptable += ( ((linear >> 12) & ((1<<10)-1)) << 2);

    if(0==(pde&1)){ // check P (Present) bit
        return 0;
    }

    pte     = readPhysMemDoubleWord(pM, ptable);

    if(0 == (pte&1)){
        return 0;
    }

    return 1;
}

#define PF_ERROR_CODE(US, WR, PRESENT) (((US)? 4 : 0) | ((WR)? 2:0) | ((PRESENT)? 1:0))

uint32_t getPhysFromLinear(struct stMachineState *pM, uint32_t linear, uint8_t readAcc, uint8_t pl){
    uint32_t dir, pde, ptable, pte=0, paddr=0;

    dir     = (REG_CR3 & 0xfffff000); // pM->reg.cr[3];
    dir    += ( ((linear >> 22) & ((1<<10)-1)) << 2);

    pde     = readPhysMemDoubleWord(pM, dir);
    ptable  = (pde & 0xfffff000);
    ptable += ( ((linear >> 12) & ((1<<10)-1)) << 2);

    if(0==(pde&1)){ // check P (Present) bit
        pM->reg.cr[2]  = linear;
        pM->reg.error_code = PF_ERROR_CODE(pl!=0, !readAcc, 0);
        pM->reg.fault |= (1<<FAULTNUM_PAGEFAULT);
        goto fault_pf;
    }
    if( 0==(pde&4) && pl != 0 ){ // check U/S (user/supervisor) bit
        pM->reg.cr[2]  = linear;
        pM->reg.error_code = PF_ERROR_CODE(pl!=0, !readAcc, 1);
        pM->reg.fault |= (1<<FAULTNUM_PAGEFAULT);
        goto fault_pf;
    }
    if( 0==(pde&2) && !readAcc && pl != 0 ){ // check R/W bit
        pM->reg.cr[2]  = linear;
        pM->reg.error_code = PF_ERROR_CODE(pl!=0, !readAcc, 1);
        pM->reg.fault |= (1<<FAULTNUM_PAGEFAULT);
        goto fault_pf;
    }


    if(0==(pde&0x20)){ // set A (Access) bit and D (Dirty) bit
        writePhysMemByte(pM, dir, readAcc ? (pde|0x20) : (pde|0x20|0x40) );
    }

    pte     = readPhysMemDoubleWord(pM, ptable);
    paddr   = (pte & 0xfffff000);
    paddr  += (linear & 0xfff);

    if(0 == (pte&1)){
        pM->reg.cr[2]  = linear;
        pM->reg.error_code = PF_ERROR_CODE(pl!=0, !readAcc, 0);
        pM->reg.fault |= (1<<FAULTNUM_PAGEFAULT);
        goto fault_pf;
    }
    if( 0==(pte&4) && pl != 0 ){ // check U/S (user/supervisor) bit
        pM->reg.cr[2]  = linear;
        pM->reg.error_code = PF_ERROR_CODE(pl!=0, !readAcc, 1);
        pM->reg.fault |= (1<<FAULTNUM_PAGEFAULT);
        goto fault_pf;
    }
    if( 0==(pte&2) && !readAcc && pl != 0 ){ // check R/W bit
        pM->reg.cr[2]  = linear;
        pM->reg.error_code = PF_ERROR_CODE(pl!=0, !readAcc, 1);
        pM->reg.fault |= (1<<FAULTNUM_PAGEFAULT);
        goto fault_pf;
    }

    if(0==(pte&0x20)){ // set A (Access) bit and D (Dirty) bit
        writePhysMemByte(pM, ptable, readAcc ? (pte|0x20) : (pte|0x20|0x40) );
    }
    return paddr;

fault_pf:
    if( pM->reg.fault ){
        logfile_printf(LOGCAT_CPU_MEM | LOGLV_NOTICE, "L2P (CR3: %x, dir: %x, pde:%x, ptable: %x, pte:%x) %x -> %x\n", 
        pM->reg.cr[3], dir, pde, ptable, pte, linear, paddr);
        siglongjmp(pM->emu.env, -1);
        pM->reg.fault = 0;
    }
    return paddr;
}


uint8_t  fetchCodeDataByte(struct stMachineState *pM, uint32_t addr){
    if( pM->reg.cr[0] & (1<<CR0_BIT_PG)){
        addr = getPhysFromLinear(pM, addr, 1, pM->reg.cpl); // read access, privilege level = CPL
    }

    if( pM->mem.a20m )          addr  &= 0xfffff;
    if( addr  >= EMU_MEM_SIZE ) return 0; // addr   = (addr  % EMU_MEM_SIZE);

    return pM->mem.mem[addr];
}

uint8_t readDataMemByteAsSV(struct stMachineState *pM, uint32_t addr){
    if( pM->reg.cr[0] & (1<<CR0_BIT_PG)){
        addr = getPhysFromLinear(pM, addr, 1, 0); // read access, privilege level = 0
    }

    if( pM->mem.a20m )          addr  &= 0xfffff;
    if( addr  >= EMU_MEM_SIZE ) return 0; // addr   = (addr  % EMU_MEM_SIZE);

    return pM->mem.mem[addr];
}

uint8_t readDataMemByte(struct stMachineState *pM, uint32_t addr){
    if( pM->reg.cr[0] & (1<<CR0_BIT_PG)){
        addr = getPhysFromLinear(pM, addr, 1, pM->reg.cpl); // read access, privilege level = CPL
    }

    if( pM->mem.a20m )          addr  &= 0xfffff;
    if( addr  >= EMU_MEM_SIZE ) return 0; // addr   = (addr  % EMU_MEM_SIZE);

    return pM->mem.mem[addr];
}

uint16_t readDataMemWord(struct stMachineState *pM, uint32_t addr){ 
    if( pM->reg.cr[0] & (1<<CR0_BIT_PG)){
        addr = getPhysFromLinear(pM, addr, 1, pM->reg.cpl);  // read access, privilege level = CPL
    }

    uint32_t addr1 = addr + 1;

    if( pM->mem.a20m ){
        addr  &= 0xfffff;
        addr1 &= 0xfffff;
    }
    if( addr  >= EMU_MEM_SIZE ) return 0; // addr  = (addr  % EMU_MEM_SIZE);
    if( addr1 >= EMU_MEM_SIZE ) return 0; // addr1 = (addr1 % EMU_MEM_SIZE);

    return ( pM->mem.mem[addr ] | (((uint16_t)pM->mem.mem[addr1])<<8) );
}

uint32_t readDataMemDoubleWord(struct stMachineState *pM, uint32_t addr){ 
    if( pM->reg.cr[0] & (1<<CR0_BIT_PG)){
        addr = getPhysFromLinear(pM, addr, 1, pM->reg.cpl);  // read access, privilege level = CPL
    }

    uint32_t addr1 = addr + 1;
    uint32_t addr2 = addr + 2;
    uint32_t addr3 = addr + 3;
    if( pM->mem.a20m ){
        addr  &= 0xfffff;
        addr1 &= 0xfffff;
        addr2 &= 0xfffff;
        addr3 &= 0xfffff;
    }
    if( addr  >= EMU_MEM_SIZE ) return 0; // addr  = (addr  % EMU_MEM_SIZE);
    if( addr1 >= EMU_MEM_SIZE ) return 0; // addr1 = (addr1 % EMU_MEM_SIZE);
    if( addr2 >= EMU_MEM_SIZE ) return 0; // addr2 = (addr2 % EMU_MEM_SIZE);
    if( addr3 >= EMU_MEM_SIZE ) return 0; // addr3 = (addr3 % EMU_MEM_SIZE);

    return (  ((uint32_t)pM->mem.mem[addr ])       |
             (((uint32_t)pM->mem.mem[addr1]) << 8) |
             (((uint32_t)pM->mem.mem[addr2]) <<16) |
             (((uint32_t)pM->mem.mem[addr3]) <<24) );
}

static unsigned char getCharacterForConsole(unsigned char c){
    if(c==0xfe){
        return 0xDB;
    }

    if( c >= 0x20 && c <=0xff && c != 0x7f ){
//    if( c >= 0x20 && c <=0xfe ){
        return c;
    }
    if( c == 0x1b){
        return 0xa3;
    }

    return ' ';
}


void updateCursorPosition(struct stMachineState *pM, uint32_t addr){
    int posx = pM->mem.mem[ addr & (~1) ];
    int posy = pM->mem.mem[ addr |   1  ];

    termGoTo(posx+1, posy+1);
    fflush(stdout);
}

#define DISP_CHAR_POS(x,y)  (0xb8000 + (2*80*(y))+(2*(x)) + 0)
#define DISP_COLOR_POS(x,y) (0xb8000 + (2*80*(y))+(2*(x)) + 1)
#define IS_SJIS_1STBYTE(c)  (((c)>=0x81)&&((c)<=0x9f)) || (((c)>=0xe0)&&((c)<=0xfc))

/*
static volatile int displayUpdateFlag = 0;
void *displayUpdateNotify(void *param){
    while (1) {
        usleep(10*1000);
        displayUpdateFlag = 1;
    }
    return NULL;
}
*/

void updateConsole(struct stMachineState *pM, uint32_t addr){
    unsigned char buf[2];
    unsigned char c;
    unsigned int pos,x,y,xs, kanji1b = 0, kanji1b_prev = 0;

/*
    static int thread_init = 0;
    pthread_t tid1;
    if( ! thread_init ){
        if (pthread_create(&tid1, NULL, displayUpdateNotify, NULL) != 0) {
            fprintf(stderr, "pthread_create"); exit(1);
        }
        thread_init = 1;
    }
*/

    if( addr >= 0xb8000 && addr < 0xb8000+4000 ){
        pos = (addr - 0xb8000) % (2*80*25);
        pos /= 2;
        x = (pos%80);
        y =  pos/80;
        xs= x;

        buf[0] = pM->mem.mem[ 0xb8000 + (2*80*y)+(2*xs) + 0 ];
        buf[1] = pM->mem.mem[ 0xb8000 + (2*80*y)+(2*xs) + 1 ];

        int posx = pM->mem.mem[ BIOS_DATA_AREA_CURSOR_X_0 ];
        int posy = pM->mem.mem[ BIOS_DATA_AREA_CURSOR_Y_0 ];

        for(int k=0; k<=x; k++){
            c = pM->mem.mem[ DISP_CHAR_POS(k,y) ];
            kanji1b_prev = kanji1b;
            if(kanji1b){
                kanji1b = 0;
            }else if( IS_SJIS_1STBYTE(c) ){
                kanji1b = 1;
            }else{
                kanji1b = 0;
            }
        }
        if( kanji1b ){
            return ;
        }
        if( kanji1b_prev ){
            xs = xs-1;
        }

        termSetBlinkOff();
        termSetBGColor  ( bgcolor[buf[1]>>4] );
        termSetCharColor(  fcolor[buf[1]&0xf] );
        termGoTo( xs+1, y+1 );
        for( ; xs<=x; xs++){
            buf[0] = pM->mem.mem[ DISP_CHAR_POS(xs,y) ];
            printf( "%c", getCharacterForConsole(buf[0]) );
        }
        termGoTo( posx+1, posy+1 );
        termResetBlink();
        fflush(stdout);
    }
}

void writeDataMemByteAsSV(struct stMachineState *pM, uint32_t addr, uint8_t  data){
    if( addr == pM->mem.watchAddr ){
        printf("WRITE 0x%x (at [cs:ip] = 0x%04x:0x%04x)", addr, REG_CS, REG_EIP);
        printf(" 0x%02x -> 0x%02x>\n", pM->mem.mem[addr&0xfffff], data);
    }
    
    if( pM->reg.cr[0] & (1<<CR0_BIT_PG)){
        addr = getPhysFromLinear(pM, addr, 0, 0); // write access, privilege level = 0
    }

    if( pM->mem.a20m ) addr &= 0xfffff;
    if( addr >= EMU_MEM_SIZE ) return ; //addr = (addr % EMU_MEM_SIZE);

    pM->mem.mem[addr] = data;

    if( addr == BIOS_DATA_AREA_CURSOR_X_0 || addr == BIOS_DATA_AREA_CURSOR_Y_0 ) updateCursorPosition(pM, addr);
    if( (addr&0xffff8000) == 0xb8000 ) updateConsole(pM, addr);
}

void writeDataMemByte(struct stMachineState *pM, uint32_t addr, uint8_t  data){
    if( addr == pM->mem.watchAddr ){
        printf("<WRITE addr:0x%x, data:0x%x (at [cs:eip] = 0x%04x:0x%04x)", addr, data, REG_CS, REG_EIP);
        logfile_printf(LOGCAT_CPU_MEM|LOGLV_ERROR, "<WRITE addr:0x%x, data:0x%x (at [cs:eip] = 0x%04x:0x%04x)\n", addr, data, REG_CS, REG_EIP);
    }
    
    if( pM->reg.cr[0] & (1<<CR0_BIT_PG)){
        addr = getPhysFromLinear(pM, addr, 0, pM->reg.cpl); // write access, privilege level = CPL
    }

    if( pM->mem.a20m ) addr &= 0xfffff;
    if( addr >= EMU_MEM_SIZE ) return ; //addr = (addr % EMU_MEM_SIZE);

    pM->mem.mem[addr] = data;

    if( addr == BIOS_DATA_AREA_CURSOR_X_0 || addr == BIOS_DATA_AREA_CURSOR_Y_0 ) updateCursorPosition(pM, addr);
    if( (addr&0xffff8000) == 0xb8000 ) updateConsole(pM, addr);
}

void writeDataMemWord(struct stMachineState *pM, uint32_t addr, uint16_t data){
    if( addr == pM->mem.watchAddr || addr+1 == pM->mem.watchAddr ){
        printf("<WRITE addr:0x%x, data:0x%x (at [cs:eip] = 0x%04x:0x%04x)", addr, data, REG_CS, REG_EIP);
        logfile_printf(LOGCAT_CPU_MEM|LOGLV_ERROR, "<WRITE addr:0x%x, data:0x%x (at [cs:eip] = 0x%04x:0x%04x)\n", addr, data, REG_CS, REG_EIP);
    }

    if( pM->reg.cr[0] & (1<<CR0_BIT_PG)){
        addr = getPhysFromLinear(pM, addr, 0, pM->reg.cpl); // write access, privilege level = CPL
    }

    uint32_t addr1 = addr + 1;
    if( pM->mem.a20m ){
        addr  &= 0xfffff;
        addr1 &= 0xfffff;
    }
    if( addr  >= EMU_MEM_SIZE ) return ; // addr  = (addr  % EMU_MEM_SIZE);
    if( addr1 >= EMU_MEM_SIZE ) return ; // addr1 = (addr1 % EMU_MEM_SIZE);

    pM->mem.mem[addr] = data&0xff; pM->mem.mem[addr1] = (data>>8);

    if( BIOS_DATA_AREA_CURSOR_X_0 -1 <= addr && addr <= BIOS_DATA_AREA_CURSOR_Y_0 ) updateCursorPosition(pM, addr);
//    if( addr1 == BIOS_DATA_AREA_CURSOR_X_0 || addr == BIOS_DATA_AREA_CURSOR_X_0 || addr == BIOS_DATA_AREA_CURSOR_Y_0 ) updateCursorPosition(pM, addr);
    if( (addr&0xffff8000) == 0xb8000 || (addr1&0xffff8000) == 0xb8000 ) updateConsole(pM, addr);
}

void writeDataMemDoubleWord(struct stMachineState *pM, uint32_t addr, uint32_t data){
    if( addr == pM->mem.watchAddr || addr+1 == pM->mem.watchAddr || addr+2 == pM->mem.watchAddr || addr+3 == pM->mem.watchAddr ){
        printf("<WRITE addr:0x%x, data:0x%x (at [cs:eip] = 0x%04x:0x%04x)", addr, data, REG_CS, REG_EIP);
        logfile_printf(LOGCAT_CPU_MEM|LOGLV_ERROR, "<WRITE addr:0x%x, data:0x%x (at [cs:eip] = 0x%04x:0x%04x)\n", addr, data, REG_CS, REG_EIP);
    }

    if( pM->reg.cr[0] & (1<<CR0_BIT_PG)){
        addr = getPhysFromLinear(pM, addr, 0, pM->reg.cpl); // write access, privilege level = CPL
    }

    uint32_t addr1 = addr + 1;
    uint32_t addr2 = addr + 2;
    uint32_t addr3 = addr + 3;
    if( pM->mem.a20m ){
        addr  &= 0xfffff;
        addr1 &= 0xfffff;
        addr2 &= 0xfffff;
        addr3 &= 0xfffff;
    }
    /*
    if( addr  >= EMU_MEM_SIZE ) return; // addr  = (addr  % EMU_MEM_SIZE);
    if( addr1 >= EMU_MEM_SIZE ) return; // addr1 = (addr1 % EMU_MEM_SIZE);
    if( addr2 >= EMU_MEM_SIZE ) return; // addr2 = (addr2 % EMU_MEM_SIZE);
    */
    if( addr3 >= EMU_MEM_SIZE ) return; // addr3 = (addr3 % EMU_MEM_SIZE);

    pM->mem.mem[addr ] = ( data     &0xff);
    pM->mem.mem[addr1] = ((data>> 8)&0xff);
    pM->mem.mem[addr2] = ((data>>16)&0xff);
    pM->mem.mem[addr3] = ((data>>24)&0xff);

    if( BIOS_DATA_AREA_CURSOR_X_0 -3 <= addr && addr <= BIOS_DATA_AREA_CURSOR_Y_0 ) updateCursorPosition(pM, addr);
    if( (addr&0xffff8000) == 0xb8000 || (addr3&0xffff8000) == 0xb8000 ) updateConsole(pM, addr);
}

