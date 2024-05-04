#ifndef __EXINST_COMMON_H__ 
#define __EXINST_COMMON_H__ 

#define EX_RESULT_SUCCESS  0
#define EX_RESULT_UNKNOWN  1
#define EX_RESULT_HALT     2


#define EXI_LOGLEVEL             (LOGCAT_CPU_EXE | LOGLV_VERBOSE)
#define EXI_LOG_PRINTF(...)      (logfile_printf_without_header(EXI_LOGLEVEL, __VA_ARGS__))
#define EXI_LOG_ERR_PRINTF(...)  (logfile_printf(EXI_LOGLEVEL, __VA_ARGS__))

#define REG_AX    (pM->reg.ax[FIRST_WORD_IDX_IN_DWORD])
#define REG_CX    (pM->reg.cx[FIRST_WORD_IDX_IN_DWORD])
#define REG_DX    (pM->reg.dx[FIRST_WORD_IDX_IN_DWORD])
#define REG_BX    (pM->reg.bx[FIRST_WORD_IDX_IN_DWORD])

#define REG_EAX    (pM->reg.eax)
#define REG_ECX    (pM->reg.ecx)
#define REG_EDX    (pM->reg.edx)
#define REG_EBX    (pM->reg.ebx)

#define REG_SP    (pM->reg.sp[FIRST_WORD_IDX_IN_DWORD])
#define REG_BP    (pM->reg.bp[FIRST_WORD_IDX_IN_DWORD])
#define REG_SI    (pM->reg.si[FIRST_WORD_IDX_IN_DWORD])
#define REG_DI    (pM->reg.di[FIRST_WORD_IDX_IN_DWORD])

#define REG_ESP    (pM->reg.esp)
#define REG_EBP    (pM->reg.ebp)
#define REG_ESI    (pM->reg.esi)
#define REG_EDI    (pM->reg.edi)

#define REG_ES    (pM->reg.es)
#define REG_CS    (pM->reg.cs)
#define REG_SS    (pM->reg.ss)
#define REG_DS    (pM->reg.ds)
#define REG_FS    (pM->reg.fs)
#define REG_GS    (pM->reg.gs)

#define REG_FLAGS  (pM->reg.flags[FIRST_WORD_IDX_IN_DWORD])
#define REG_EFLAGS (pM->reg.eflags)

#define REG_CR0   (pM->reg.cr[0])
#define REG_CR2   (pM->reg.cr[2])
#define REG_CR3   (pM->reg.cr[3])

#define REG_DR0   (pM->reg.dr[0])
#define REG_DR1   (pM->reg.dr[1])
#define REG_DR2   (pM->reg.dr[2])
#define REG_DR3   (pM->reg.dr[3])
#define REG_DR6   (pM->reg.dr[6])
#define REG_DR7   (pM->reg.dr[7])

#define REG_IP     (pM->reg.ip[FIRST_WORD_IDX_IN_DWORD])
#define REG_EIP    (pM->reg.eip)

#define REG_ES_BASE    (pM->reg.descc_es.base)
#define REG_CS_BASE    (pM->reg.descc_cs.base)
#define REG_SS_BASE    (pM->reg.descc_ss.base)
#define REG_DS_BASE    (pM->reg.descc_ds.base)
#define REG_FS_BASE    (pM->reg.descc_fs.base)
#define REG_GS_BASE    (pM->reg.descc_gs.base)

#define MODE_PROTECTED     (REG_CR0 & (1<<CR0_BIT_PE))
#define MODE_PROTECTED32   (MODE_PROTECTED && !(REG_EFLAGS & (1<<EFLAGS_BIT_VM)))
#define MODE_PROTECTEDVM   (MODE_PROTECTED &&  (REG_EFLAGS & (1<<EFLAGS_BIT_VM)))

#define IOPL(x)   ( ((x) & FLAGS_BIT_IOPL_MASK) >> FLAGS_BIT_IOPL )

#define PREFIX_SEG  (pM->prefix.seg)
#define PREFIX_REPZ (pM->prefix.repz)
#define PREFIX_AD32 (pM->prefix.addr32)
#define PREFIX_OP32 (pM->prefix.data32)


#include "descriptor.h"
#define CODESEG_D_BIT (pM->reg.descc_cs.flags & SEGFLAGS_CSEG_D_BIT)


//------------------------------------------------------------
// 
//------------------------------------------------------------

#define ECODE_SEGMENT_GDT_LDT_EXT(seg)   (((seg) & (~0x3)) | 1)
#define ECODE_SEGMENT_GDT_LDT(seg)       ((seg) & (~0x3))
#define ECODE_SEGMENT_IDT(int_num, ext)  (((int_num) << 3) | 0x2 | (ext ? 0x1 : 0))

#define ENTER_UD  do{                           \
    pM->reg.error_code  = 0;                    \
    pM->reg.fault      |= (1<<INTNUM_UDOPCODE); \
    siglongjmp(pM->emu.env, -1);                \
}while(0)

#define ENTER_GP(ecode) do{                    \
    pM->reg.error_code  = ecode;               \
    pM->reg.fault      |= (1<<FAULTNUM_GP);    \
    siglongjmp(pM->emu.env, -1);               \
}while(0)

#define ENTER_SS(ecode) do{                         \
    pM->reg.error_code  = ecode;                    \
    pM->reg.fault      |= (1<<FAULTNUM_STACKFAULT); \
    siglongjmp(pM->emu.env, -1);                    \
}while(0)

#define ENTER_NP(ecode) do{                         \
    pM->reg.error_code  = ecode;                    \
    pM->reg.fault      |= (1<<FAULTNUM_SEGNOTP);    \
    siglongjmp(pM->emu.env, -1);                    \
}while(0)

#define ENTER_TS(ecode) do{                         \
    pM->reg.error_code  = ecode;                    \
    pM->reg.fault      |= (1<<FAULTNUM_INVALIDTSS); \
    siglongjmp(pM->emu.env, -1);                    \
}while(0)
//------------------------------------------------------------



#define UPDATE_IP( inc ) do{      \
    if( /*PREFIX_OP32*/ pM->reg.descc_cs.def ){            \
        REG_EIP += (inc);         \
    }else{                        \
        REG_IP += (inc);          \
    }                             \
}while(0)

#define __SEG_ERR_MSG(seg, oft, min, max) \
logfile_printf(LOGCAT_CPU_MEM | LOGLV_ERROR, "%s: access violation in writing segment %s offset %x : min %x max %x (CS:EIP=%x:%x pointer %x)\n", __func__, (seg), (oft), (min), (max), REG_CS, REG_EIP, REG_CS_BASE+REG_EIP);

#define PUSH_TO_STACK(x) do{                                                                      \
    if( MODE_PROTECTED32 ){                                                                       \
        uint32_t decl = (pM->prefix.data32) ? 4 : 2;                                              \
        uint32_t __sp_min = (pM->reg.descc_ss.big) ? REG_ESP-decl : (REG_SP-decl)&0xffff;         \
        uint32_t __sp_max = (pM->reg.descc_ss.big) ? REG_ESP-   1 : (REG_SP-   1)&0xffff;         \
        if( pM->reg.cpl > (REG_SS & 3) || __sp_min < pM->reg.descc_ss.limit_min || __sp_max > pM->reg.descc_ss.limit_max ){ \
            __SEG_ERR_MSG("SS", __sp_min, pM->reg.descc_ss.limit_min, pM->reg.descc_ss.limit_max);\
            ENTER_SS(0);                                                                 \
        }                                                                                         \
    }                                                                                             \
    if(! pM->prefix.data32){                                                                      \
        if( pM->emu.emu_cpu == EMU_CPU_8086 || pM->emu.emu_cpu == EMU_CPU_80186 ){                \
            REG_SP-=2;                                                                            \
            writeDataMemWord(pM, REG_SS_BASE+ REG_SP, (x));                                       \
        }else{                                                                                    \
            uint32_t __tmp_sp2 = (pM->reg.descc_ss.big) ? REG_ESP-2 : ((REG_SP-2)&0xffff);        \
            writeDataMemWord(pM, REG_SS_BASE+ __tmp_sp2, (x));                                    \
            if(pM->reg.descc_ss.big){ REG_ESP-=2; }else{ REG_SP-=2; }                             \
        }                                                                                         \
    }else{                                                                                        \
        uint32_t __tmp_sp4 = (pM->reg.descc_ss.big) ? REG_ESP-4 : ((REG_SP-4)&0xffff);            \
        writeDataMemDoubleWord(pM, REG_SS_BASE + __tmp_sp4, (x));                                 \
        if(pM->reg.descc_ss.big){ REG_ESP-=4; }else{ REG_SP-=4; }                                 \
    }                                                                                             \
}while(0)

#define POP_FROM_STACK(x) do{                                                                     \
    if( MODE_PROTECTED32 ){                                                                       \
        uint32_t incl = (pM->prefix.data32) ? 3 : 1;                                              \
        uint32_t __sp_min = (pM->reg.descc_ss.big) ? REG_ESP      : ((REG_SP     )&0xffff);       \
        uint32_t __sp_max = (pM->reg.descc_ss.big) ? REG_ESP+incl : ((REG_SP+incl)&0xffff);       \
        if( pM->reg.cpl > (REG_SS & 3) || __sp_min < pM->reg.descc_ss.limit_min || __sp_max > pM->reg.descc_ss.limit_max ){ \
            __SEG_ERR_MSG("SS", __sp_max, pM->reg.descc_ss.limit_min, pM->reg.descc_ss.limit_max);\
            ENTER_SS(0);                                                                          \
        }                                                                                         \
    }                                                                                             \
    if(! pM->prefix.data32){                                                                      \
        if( pM->emu.emu_cpu == EMU_CPU_8086 || pM->emu.emu_cpu == EMU_CPU_80186 ){                \
            (x) = readDataMemWord(pM, REG_SS_BASE + REG_SP);                                      \
            REG_SP+=2;                                                                            \
        }else{                                                                                    \
            (x) = readDataMemWord(pM, REG_SS_BASE + ((pM->reg.descc_ss.big) ? REG_ESP : REG_SP)); \
            if(pM->reg.descc_ss.big){ REG_ESP+=2; }else{ REG_SP+=2; }                             \
        }                                                                                         \
    }else{                                                                                        \
        (x)= readDataMemDoubleWord(pM, REG_SS_BASE + ((pM->reg.descc_ss.big) ? REG_ESP : REG_SP));\
        if(pM->reg.descc_ss.big){ REG_ESP+=4; }else{ REG_SP+=4; }                                 \
    }                                                                                             \
}while(0)



#endif