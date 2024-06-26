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

    if( addr3 >= EMU_MEM_SIZE ) return 0; // addr3 = (addr3 % EMU_MEM_SIZE);

    uint32_t result =
           (   (uint32_t)(MEM_READ(pM, addr ))        |
             (((uint32_t)(MEM_READ(pM, addr1))) << 8) |
             (((uint32_t)(MEM_READ(pM, addr2))) <<16) |
             (((uint32_t)(MEM_READ(pM, addr3))) <<24) );

    return result;
}

inline void writePhysMemByte(struct stMachineState *pM, uint32_t addr, uint8_t data){
    if( addr >= EMU_MEM_SIZE ) return ;

    MEM_WRITE(pM, addr, data);
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

void flushTLB(struct stMachineState *pM){
    int i, j;

    for(i=0; i<(1<<TLB_ENTRY_BITS); i++){
        for(j=0; j<TLB_NASOC; j++){
            pM->reg.tlb[i].addr[j] = 0;
            pM->reg.tlb[i].pte[j]  = 0;
        }
    }
}

#define PF_ERROR_CODE(US, WR, PRESENT) (((US)? 4 : 0) | ((WR)? 2:0) | ((PRESENT)? 1:0))


#define PTE_BIT_PRESENT  0
#define PTE_BIT_RW       1
#define PTE_BIT_UserSvr  2
#define PTE_BIT_ACCESSED 5
#define PTE_BIT_DIRTY    6

#define PTE_ADDR_MASK    0xfffff000
#define PAGE_OFFSET_MASK 0x00000fff

#define PAGE_PRESENT(pte)        ((pte) & (1<<PTE_BIT_PRESENT))
#define PAGE_IS_WRITABLE(pte)    ((pte) & (1<<PTE_BIT_RW))
#define PAGE_IS_USER_LEVEL(pte)  ((pte) & (1<<PTE_BIT_UserSvr))
#define PAGE_IS_ACCESSED(pte)    ((pte) & (1<<PTE_BIT_ACCESSED))
#define PAGE_IS_DIRTY(pte)       ((pte) & (1<<PTE_BIT_DIRTY))

#define IS_USER_LEVEL_PRIVLEVEL(pl)  ((pl)==3)


uint32_t getPhysFromLinear(struct stMachineState *pM, uint32_t linear, uint8_t readAcc, uint8_t pl){
    uint32_t dir=0, pde=0, ptable=0, paddr=0;
    uint32_t i, tlb_entry, entry_idx;
    uint32_t pte=0, new_pte=0, effective_pte;

    tlb_entry = ((linear >> (32-TLB_ENTRY_BITS)) & ((1<<TLB_ENTRY_BITS)-1));
    for(i=0; i<TLB_NASOC; i++){
        if( pM->reg.tlb[tlb_entry].pte[i]  != 0 &&
            pM->reg.tlb[tlb_entry].addr[i] == (linear & PTE_ADDR_MASK) &&
            (readAcc || PAGE_IS_DIRTY(pM->reg.tlb[tlb_entry].pte[i])) ){
            effective_pte = pM->reg.tlb[tlb_entry].pte[i];
            goto TLB_hit;
        }
    }

    dir     = (REG_CR3 & PTE_ADDR_MASK);
    dir    += ( ((linear >> 22) & ((1<<10)-1)) << 2);

    pde     = readPhysMemDoubleWord(pM, dir);
    ptable  = (pde & PTE_ADDR_MASK);
    ptable += ( ((linear >> 12) & ((1<<10)-1)) << 2);

    if( ! PAGE_PRESENT(pde) ){ // check P (Present) bit
        pM->reg.cr[2]  = linear;
        pM->reg.error_code = PF_ERROR_CODE(IS_USER_LEVEL_PRIVLEVEL(pl), !readAcc, 0);
        pM->reg.fault |= (1<<FAULTNUM_PAGEFAULT);
        goto fault_pf;
    }

    if( ! PAGE_IS_ACCESSED(pde) ){ // set A (Access) bit and D (Dirty) bit
        // The dirty bit in directory entries is undefined.
        // So this code does not reflesh the bit once the accessed bit is set.
        writePhysMemByte(pM, dir, readAcc ? (pde|(1<<PTE_BIT_ACCESSED)) : (pde|(1<<PTE_BIT_ACCESSED)|(1<<PTE_BIT_DIRTY)) );
    }

    pte           = readPhysMemDoubleWord(pM, ptable);
    new_pte       = readAcc ? (pte|(1<<PTE_BIT_ACCESSED)) : (pte|(1<<PTE_BIT_ACCESSED)|(1<<PTE_BIT_DIRTY));
    effective_pte = new_pte;

    if( ! PAGE_IS_WRITABLE(pde)   && PAGE_IS_WRITABLE(pte)  ){
        effective_pte &= (~(1<<PTE_BIT_RW));
    }
    if( ! PAGE_IS_USER_LEVEL(pde) && PAGE_IS_USER_LEVEL(pte)){
        effective_pte &= (~(1<<PTE_BIT_UserSvr));
    }

TLB_hit:
    paddr   = (effective_pte & PTE_ADDR_MASK);
    paddr  += (linear & PAGE_OFFSET_MASK);

    if( ! PAGE_PRESENT(effective_pte) ){
        pM->reg.cr[2]  = linear;
        pM->reg.error_code = PF_ERROR_CODE(IS_USER_LEVEL_PRIVLEVEL(pl), !readAcc, 0);
        pM->reg.fault |= (1<<FAULTNUM_PAGEFAULT);
        goto fault_pf;
    }
    if( (! PAGE_IS_USER_LEVEL(effective_pte)) && IS_USER_LEVEL_PRIVLEVEL(pl) ){ // check U/S (user/supervisor) bit
        pM->reg.cr[2]  = linear;
        pM->reg.error_code = PF_ERROR_CODE(IS_USER_LEVEL_PRIVLEVEL(pl), !readAcc, 1);
        pM->reg.fault |= (1<<FAULTNUM_PAGEFAULT);
        goto fault_pf;
    }
    if( (! PAGE_IS_WRITABLE(effective_pte)) && !readAcc && IS_USER_LEVEL_PRIVLEVEL(pl) ){ // check R/W bit
        pM->reg.cr[2]  = linear;
        pM->reg.error_code = PF_ERROR_CODE(IS_USER_LEVEL_PRIVLEVEL(pl), !readAcc, 1);
        pM->reg.fault |= (1<<FAULTNUM_PAGEFAULT);
        goto fault_pf;
    }

    if( pte != new_pte ){ // update A (Access) bit and D (Dirty) bit
        writePhysMemByte(pM, ptable, new_pte);
    }

    // Update the TLB
    for(i=0; i<TLB_NASOC; i++){
        if( pM->reg.tlb[tlb_entry].addr[i] == (linear & PTE_ADDR_MASK) ){
            goto tlb_reflesh_exit;
        }
        if( pM->reg.tlb[tlb_entry].pte[i] == 0 ){
            entry_idx = i;
            goto tlb_reflesh;
        }
    }

    // If there is no unused slot, old items are moved to higher slots and the first slot is used for the current PTE.
    for(i=TLB_NASOC-1; i>0; i--){
        pM->reg.tlb[tlb_entry].addr[i] = pM->reg.tlb[tlb_entry].addr[i-1];
        pM->reg.tlb[tlb_entry].pte [i] = pM->reg.tlb[tlb_entry].pte [i-1];
    }
    entry_idx = 0;
 
tlb_reflesh:
    pM->reg.tlb[tlb_entry].addr[entry_idx] = (linear & PTE_ADDR_MASK);
    pM->reg.tlb[tlb_entry].pte [entry_idx] = effective_pte;

tlb_reflesh_exit:

    return paddr;

fault_pf:
    if( pM->reg.fault ){
        logfile_printf(LOGCAT_CPU_MEM | LOGLV_NOTICE, "L2P (CR3: %x, dir: %x, pde:%x, ptable: %x, pte:%x) %x -> %x\n", 
        pM->reg.cr[3], dir, pde, ptable, pte, linear, paddr);
        LONGJMP(pM->reg.env, -1);
        pM->reg.fault = 0;
    }
    return paddr;
}

uint32_t fetchCodeDataDoubleWord(struct stMachineState *pM, uint32_t addr){
    if( MODE_PROTECTED32 && (addr < (REG_CS_BASE+pM->reg.descc_cs.limit_min) || addr+3 > (REG_CS_BASE+pM->reg.descc_cs.limit_max)) ){
        logfile_printf(LOGCAT_CPU_MEM | LOGLV_ERROR, "access violation in code fetch at 0x%x. Segment offset min %x max %x (CS:EIP=%x:%x pointer %x)\n", 
        addr, pM->reg.descc_cs.limit_min, pM->reg.descc_cs.limit_max, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
        ENTER_GP(0);
    }

    if( pM->reg.cr[0] & (1<<CR0_BIT_PG) ){
        if( (addr & 0xfff) + 3 >= 0x1000 ){
            return (  ((uint32_t)fetchCodeDataWord(pM, addr+0))       |
                     (((uint32_t)fetchCodeDataWord(pM, addr+2)) <<16) );
        }

        addr = getPhysFromLinear(pM, addr, 1, pM->reg.cpl); // read access, privilege level = CPL
    }

    uint32_t addr1 = addr + 1;
    uint32_t addr2 = addr + 2;
    uint32_t addr3 = addr + 3;

    if( pM->pMemIo->a20m ){
        addr  &= 0xfffff;
        addr1 &= 0xfffff;
        addr2 &= 0xfffff;
        addr3 &= 0xfffff;

        if( EMU_MEM_SIZE < (1<<20) ){
            if( addr3 >= EMU_MEM_SIZE ) return 0;
        }
    }else{
        if( addr3 >= EMU_MEM_SIZE ) return 0;
    }

    return (  ((uint32_t)MEM_READ(pM, addr ))       |
             (((uint32_t)MEM_READ(pM, addr1)) << 8) |
             (((uint32_t)MEM_READ(pM, addr2)) <<16) |
             (((uint32_t)MEM_READ(pM, addr3)) <<24) );
}

uint16_t fetchCodeDataWord(struct stMachineState *pM, uint32_t addr){
    if( MODE_PROTECTED32 && (addr < (REG_CS_BASE+pM->reg.descc_cs.limit_min) || addr+1 > (REG_CS_BASE+pM->reg.descc_cs.limit_max)) ){
        logfile_printf(LOGCAT_CPU_MEM | LOGLV_ERROR, "access violation in code fetch at 0x%x. Segment offset min %x max %x (CS:EIP=%x:%x pointer %x)\n", 
        addr, pM->reg.descc_cs.limit_min, pM->reg.descc_cs.limit_max, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
        ENTER_GP(0);
    }

    if( pM->reg.cr[0] & (1<<CR0_BIT_PG) ){
        if( (addr & 0xfff) + 1 >= 0x1000 ){
            return (  ((uint16_t)fetchCodeDataByte(pM, addr+0)) |
                     (((uint16_t)fetchCodeDataByte(pM, addr+1)) << 8) );
        }

        addr = getPhysFromLinear(pM, addr, 1, pM->reg.cpl); // read access, privilege level = CPL
    }

    uint32_t addr1 = addr + 1;

    if( pM->pMemIo->a20m ){
        addr  &= 0xfffff;
        addr1 &= 0xfffff;
    }
    if( addr  >= EMU_MEM_SIZE ) return 0;
    if( addr1 >= EMU_MEM_SIZE ) return 0;

    return (  ((uint16_t)MEM_READ(pM, addr)) |
             (((uint16_t)MEM_READ(pM, addr1)) << 8) );
}

uint8_t fetchCodeDataByte(struct stMachineState *pM, uint32_t addr){
    if( (pM->reg.fetchCacheBase <= addr) && (addr < pM->reg.fetchCacheBase + FETCH_CACHE_SIZE) && (addr != FETCH_CACHE_BASE_INVALID) ){
        return pM->reg.fetchCache[ addr - pM->reg.fetchCacheBase ];
    }
    if( MODE_PROTECTED32 && (addr < (REG_CS_BASE+pM->reg.descc_cs.limit_min) || addr > (REG_CS_BASE+pM->reg.descc_cs.limit_max)) ){
        logfile_printf(LOGCAT_CPU_MEM | LOGLV_ERROR, "access violation in code fetch at 0x%x. Segment offset min %x max %x (CS:EIP=%x:%x pointer %x)\n", 
        addr, pM->reg.descc_cs.limit_min, pM->reg.descc_cs.limit_max, REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);
        ENTER_GP(0);
    }

    if( pM->reg.cr[0] & (1<<CR0_BIT_PG) ){
        addr = getPhysFromLinear(pM, addr, 1, pM->reg.cpl); // read access, privilege level = CPL
    }

    if( pM->pMemIo->a20m ){
        addr &= 0xfffff;
    }
    if( addr >= EMU_MEM_SIZE ) return 0;

    return MEM_READ(pM, addr);
}

uint8_t readDataMemByteAsSV(struct stMachineState *pM, uint32_t addr){
    if( pM->reg.cr[0] & (1<<CR0_BIT_PG) ){
        addr = getPhysFromLinear(pM, addr, 1, 0); // read access, privilege level = 0
    }

    if( pM->pMemIo->a20m ){
        addr &= 0xfffff;
    }
    if( addr >= EMU_MEM_SIZE ) return 0;

    return MEM_READ(pM, addr);
}

uint8_t readDataMemByte(struct stMachineState *pM, uint32_t addr){
    if( pM->reg.cr[0] & (1<<CR0_BIT_PG) ){
        addr = getPhysFromLinear(pM, addr, 1, pM->reg.cpl); // read access, privilege level = CPL
    }

    if( pM->pMemIo->a20m ){
        addr &= 0xfffff;
    }
    if( addr >= EMU_MEM_SIZE ) return 0;

    return MEM_READ(pM, addr);
}

uint16_t readDataMemWord(struct stMachineState *pM, uint32_t addr){ 
    if( pM->reg.cr[0] & (1<<CR0_BIT_PG) ){

        if( (addr & 0xfff) + 1 >= 0x1000 ){
            return (  ((uint16_t)readDataMemByte(pM, addr+0)) |
                     (((uint16_t)readDataMemByte(pM, addr+1)) << 8) );
        }

        addr = getPhysFromLinear(pM, addr, 1, pM->reg.cpl);  // read access, privilege level = CPL
    }

    uint32_t addr1 = addr + 1;

    if( pM->pMemIo->a20m ){
        addr  &= 0xfffff;
        addr1 &= 0xfffff;
    }
    if( addr  >= EMU_MEM_SIZE ) return 0;
    if( addr1 >= EMU_MEM_SIZE ) return 0;

    return ( MEM_READ(pM, addr) | (((uint16_t)MEM_READ(pM, addr1))<<8) );
}

uint32_t readDataMemDoubleWord(struct stMachineState *pM, uint32_t addr){ 
    if( pM->reg.cr[0] & (1<<CR0_BIT_PG)){

        if( (addr & 0xfff) + 3 >= 0x1000 ){
            return (  ((uint32_t)readDataMemWord(pM, addr+0))       |
                     (((uint32_t)readDataMemWord(pM, addr+2)) <<16) );
        }

        addr = getPhysFromLinear(pM, addr, 1, pM->reg.cpl);  // read access, privilege level = CPL
    }

    uint32_t addr1 = addr + 1;
    uint32_t addr2 = addr + 2;
    uint32_t addr3 = addr + 3;

    if( pM->pMemIo->a20m ){
        addr  &= 0xfffff;
        addr1 &= 0xfffff;
        addr2 &= 0xfffff;
        addr3 &= 0xfffff;

        if( EMU_MEM_SIZE < (1<<20) ){
            if( addr3 >= EMU_MEM_SIZE ) return 0;
        }
    }else{
        if( addr3 >= EMU_MEM_SIZE ) return 0;
    }

    return (  ((uint32_t)MEM_READ(pM, addr ))       |
             (((uint32_t)MEM_READ(pM, addr1)) << 8) |
             (((uint32_t)MEM_READ(pM, addr2)) <<16) |
             (((uint32_t)MEM_READ(pM, addr3)) <<24) );
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
    int posx = MEM_READ(pM, addr & (~1) );
    int posy = MEM_READ(pM, addr |   1  );

    termGoTo(posx+1, posy+1);
    FLUSH_STDOUT();
}

#define DISP_CHAR_POS(x,y)  (0xb8000 + (2*80*(y))+(2*(x)) + 0)
#define DISP_COLOR_POS(x,y) (0xb8000 + (2*80*(y))+(2*(x)) + 1)
#define IS_SJIS_1STBYTE(c)  (((c)>=0x81)&&((c)<=0x9f)) || (((c)>=0xe0)&&((c)<=0xfc))

void updateConsole(struct stMachineState *pM, uint32_t addr){
    unsigned char buf[2];
    unsigned char c;
    unsigned int pos,x,y,xs, kanji1b = 0, kanji1b_prev = 0;

    if( addr >= 0xb8000 && addr < 0xb8000+4000 ){
        pos = (addr - 0xb8000) % (2*80*25);
        pos /= 2;
        x = (pos%80);
        y =  pos/80;
        xs= x;

        buf[0] = MEM_READ(pM, 0xb8000 + (2*80*y)+(2*xs) + 0 );
        buf[1] = MEM_READ(pM, 0xb8000 + (2*80*y)+(2*xs) + 1 );

        int posx = MEM_READ(pM, BIOS_DATA_AREA_CURSOR_X_0 );
        int posy = MEM_READ(pM, BIOS_DATA_AREA_CURSOR_Y_0 );

        for(int k=0; k<=x; k++){
            c = MEM_READ(pM, DISP_CHAR_POS(k,y) );
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
            buf[0] = MEM_READ(pM, DISP_CHAR_POS(xs,y) );
            PRINTF( "%c", getCharacterForConsole(buf[0]) );
        }
        termGoTo( posx+1, posy+1 );
        termResetBlink();
        FLUSH_STDOUT();
    }
}

void writeDataMemByteAsSV(struct stMachineState *pM, uint32_t addr, uint8_t  data){
    if( addr == pM->pEmu->watchAddr ){
        PRINTF("WRITE 0x%x (at [cs:ip] = 0x%04x:0x%04x)", addr, REG_CS, REG_EIP);
        PRINTF(" 0x%02x -> 0x%02x>\n", MEM_READ(pM, addr&0xfffff), data);
    }
    
    if( pM->reg.cr[0] & (1<<CR0_BIT_PG) ){
        addr = getPhysFromLinear(pM, addr, 0, 0); // write access, privilege level = 0
    }

    if( pM->pMemIo->a20m ){
        addr &= 0xfffff;
    }
    if( addr >= EMU_MEM_SIZE ) return ;

    MEM_WRITE(pM, addr, data);

    if( addr == BIOS_DATA_AREA_CURSOR_X_0 || addr == BIOS_DATA_AREA_CURSOR_Y_0 ) updateCursorPosition(pM, addr);
    if( (addr&0xffff8000) == 0xb8000 ) updateConsole(pM, addr);
}

void writeDataMemByte(struct stMachineState *pM, uint32_t addr, uint8_t  data){
    if( addr == pM->pEmu->watchAddr ){
        logfile_printf(LOGCAT_CPU_MEM|LOGLV_ERROR, "<WRITE addr:0x%x, data:0x%x (at [cs:eip] = 0x%04x:0x%04x)\n", addr, data, REG_CS, REG_EIP);
    }
    
    if( pM->reg.cr[0] & (1<<CR0_BIT_PG) ){
        addr = getPhysFromLinear(pM, addr, 0, pM->reg.cpl); // write access, privilege level = CPL
    }

    if( pM->pMemIo->a20m ){
        addr &= 0xfffff;
    }
    if( addr >= EMU_MEM_SIZE ) return ;

    MEM_WRITE(pM, addr, data);

    if( addr == BIOS_DATA_AREA_CURSOR_X_0 || addr == BIOS_DATA_AREA_CURSOR_Y_0 ) updateCursorPosition(pM, addr);
    if( (addr&0xffff8000) == 0xb8000 ) updateConsole(pM, addr);
}

void writeDataMemWord(struct stMachineState *pM, uint32_t addr, uint16_t data){
    if( addr == pM->pEmu->watchAddr || addr+1 == pM->pEmu->watchAddr ){
        logfile_printf(LOGCAT_CPU_MEM|LOGLV_ERROR, "<WRITE addr:0x%x, data:0x%x (at [cs:eip] = 0x%04x:0x%04x)\n", addr, data, REG_CS, REG_EIP);
    }

    if( pM->reg.cr[0] & (1<<CR0_BIT_PG) ){
        if( (addr & 0xfff) + 1 >= 0x1000 ){
            writeDataMemByte(pM, addr+1, data>>8);
            writeDataMemByte(pM, addr+0, data&0xff);
            return;
        }
        addr = getPhysFromLinear(pM, addr, 0, pM->reg.cpl); // write access, privilege level = CPL
    }

    uint32_t addr1 = addr + 1;
    if( pM->pMemIo->a20m ){
        addr  &= 0xfffff;
        addr1 &= 0xfffff;
    }
    if( addr  >= EMU_MEM_SIZE ) return ;
    if( addr1 >= EMU_MEM_SIZE ) return ;

    MEM_WRITE(pM, addr,  data&0xff);
    MEM_WRITE(pM, addr1, (data>>8));

    if( BIOS_DATA_AREA_CURSOR_X_0 -1 <= addr && addr <= BIOS_DATA_AREA_CURSOR_Y_0 ) updateCursorPosition(pM, addr);
//    if( addr1 == BIOS_DATA_AREA_CURSOR_X_0 || addr == BIOS_DATA_AREA_CURSOR_X_0 || addr == BIOS_DATA_AREA_CURSOR_Y_0 ) updateCursorPosition(pM, addr);
    if( (addr&0xffff8000) == 0xb8000 || (addr1&0xffff8000) == 0xb8000 ) updateConsole(pM, addr);
}

void writeDataMemDoubleWord(struct stMachineState *pM, uint32_t addr, uint32_t data){
    if( addr == pM->pEmu->watchAddr || addr+1 == pM->pEmu->watchAddr || addr+2 == pM->pEmu->watchAddr || addr+3 == pM->pEmu->watchAddr ){
        logfile_printf(LOGCAT_CPU_MEM|LOGLV_ERROR, "<WRITE addr:0x%x, data:0x%x (at [cs:eip] = 0x%04x:0x%04x)\n", addr, data, REG_CS, REG_EIP);
    }

    if( pM->reg.cr[0] & (1<<CR0_BIT_PG) ){
        if( (addr & 0xfff) + 3 >= 0x1000 ){
            writeDataMemWord(pM, addr+2, (data>>16)&0xffff);
            writeDataMemWord(pM, addr+0,  data     &0xffff);
            return;
        }

        addr = getPhysFromLinear(pM, addr, 0, pM->reg.cpl); // write access, privilege level = CPL
    }

    uint32_t addr1 = addr + 1;
    uint32_t addr2 = addr + 2;
    uint32_t addr3 = addr + 3;

    if( pM->pMemIo->a20m ){
        addr  &= 0xfffff;
        addr1 &= 0xfffff;
        addr2 &= 0xfffff;
        addr3 &= 0xfffff;

        if( EMU_MEM_SIZE < (1<<20) ){
            if( addr3 >= EMU_MEM_SIZE ) return;
        }
    }else{
        if( addr3 >= EMU_MEM_SIZE ) return;
    }

    MEM_WRITE(pM, addr ,  data     &0xff);
    MEM_WRITE(pM, addr1, (data>> 8)&0xff);
    MEM_WRITE(pM, addr2, (data>>16)&0xff);
    MEM_WRITE(pM, addr3, (data>>24)&0xff);

    if( BIOS_DATA_AREA_CURSOR_X_0 -3 <= addr && addr <= BIOS_DATA_AREA_CURSOR_Y_0 ) updateCursorPosition(pM, addr);
    if( (addr&0xffff8000) == 0xb8000 || (addr3&0xffff8000) == 0xb8000 ) updateConsole(pM, addr);
}

