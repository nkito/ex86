#ifndef __INSTMAP16_H__
#define __INSTMAP16_H__

#include "ExInst_common.h"
#include "ExInst86.h"
#include "ExInst186.h"


const void * instCodeFunc_0xfe_0xff[8] = {
exINCDEC, exINCDEC, exCALL, exCALL, exJMP, exJMP, exPUSH, 0
};
const void * instCodeFunc_0xf6_0xf7[8] = {
exTEST, 0, exNEGNOT, exNEGNOT, exMUL, exMUL, exDIV, exDIV
};

#define EX_STR   exStringInst
#define EX_CJMP  exCondJump
#define EX_FLAG  exSetClearFlag
#define EX_STAF  exPUSHFPOPF

#define EX186SIFT exShift
#define EX186ENTR exENTER
#define EX186LEAV exLEAVE
#define EX186PSHA exPUSHA
#define EX186POPA exPOPA
#define EX186BOND exBOUND
#define EX186PUSH exPUSHimm
#define EX186IOS  exINSOUTS
#define EX186IMUL exIMULimm


const void * instCodeFunc8086[256] = {
//      0         1         2         3         4         5         6         7        8          9         A         B         C         D         E         F
 exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,   exPUSH,    exPOP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,   exPUSH,    exPOP, // 0x00-
 exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,   exPUSH,    exPOP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,   exPUSH,    exPOP, // 0x10-
 exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,/*PREF*/0,    exDAA, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,/*PREF*/0, exDASAAS, // 0x20-
 exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,/*PREF*/0, /*AAA*/0, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,/*PREF*/0, exDASAAS, // 0x30-
 exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, // 0x40-
   exPUSH,   exPUSH,   exPUSH,   exPUSH,   exPUSH,   exPUSH,   exPUSH,   exPUSH,    exPOP,    exPOP,    exPOP,    exPOP,    exPOP,    exPOP,    exPOP,    exPOP, // 0x50-
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0, // 0x60-
  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP, // 0x70-
 exALU2OP, exALU2OP,        0, exALU2OP,   exTEST,   exTEST,   exXCHG,   exXCHG,    exMov,    exMov,    exMov,    exMov,    exMov,    exLEA,    exMov,    exPOP, // 0x80-
   exXCHG,   exXCHG,   exXCHG,   exXCHG,   exXCHG,   exXCHG,   exXCHG,   exXCHG,exConvSiz,exConvSiz,   exCALL,/*WAIT*/0,  EX_STAF,  EX_STAF,    exAHF,    exAHF, // 0x90-
    exMov,    exMov,    exMov,    exMov,   EX_STR,   EX_STR,   EX_STR,   EX_STR,   exTEST,   exTEST,   EX_STR,   EX_STR,   EX_STR,   EX_STR,   EX_STR,   EX_STR, // 0xa0-
    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov, // 0xb0-
        0,        0,    exRET,    exRET, exLDSLES, exLDSLES,    exMov,    exMov,        0,        0,    exRET,    exRET,    exINT,    exINT,   exINTO,   exIRET, // 0xc0-
  exShift,  exShift,  exShift,  exShift, exAADAAM, exAADAAM,        0,   exXLAT, /*ESC*/0,    exESC, /*ESC*/0,    exESC, /*ESC*/0,    exESC, /*ESC*/0, /*ESC*/0, // 0xd0-
   exLOOP,   exLOOP,   exLOOP,   exJCXZ,  exINOUT,  exINOUT,  exINOUT,  exINOUT,   exCALL,    exJMP,    exJMP,    exJMP,  exINOUT,  exINOUT,  exINOUT,  exINOUT, // 0xe0-
/*LOCK*/0,        0, /*REP*/0, /*REP*/0, /*HLT*/0,  EX_FLAG,   /*x*/0,   /*x*/0,  EX_FLAG,  EX_FLAG,  EX_FLAG,  EX_FLAG,  EX_FLAG,  EX_FLAG,   /*x*/0,   /*x*/0  // 0xf0-
};

const void * instCodeFunc80186[256] = {
//      0         1         2         3         4         5         6         7        8          9         A         B         C         D         E         F
 exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,   exPUSH,    exPOP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,   exPUSH,    exPOP, // 0x00-
 exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,   exPUSH,    exPOP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,   exPUSH,    exPOP, // 0x10-
 exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,/*PREF*/0,    exDAA, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,/*PREF*/0, exDASAAS, // 0x20-
 exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,/*PREF*/0,/*PREF*/0, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,/*PREF*/0, exDASAAS, // 0x30-
 exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, // 0x40-
   exPUSH,   exPUSH,   exPUSH,   exPUSH,   exPUSH,   exPUSH,   exPUSH,   exPUSH,    exPOP,    exPOP,    exPOP,    exPOP,    exPOP,    exPOP,    exPOP,    exPOP, // 0x50-
EX186PSHA,EX186POPA,EX186BOND,        0,        0,        0,        0,        0,EX186PUSH,EX186IMUL,EX186PUSH,EX186IMUL, EX186IOS, EX186IOS, EX186IOS, EX186IOS, // 0x60-
  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP, // 0x70-
 exALU2OP, exALU2OP,        0, exALU2OP,   exTEST,   exTEST,   exXCHG,   exXCHG,    exMov,    exMov,    exMov,    exMov,    exMov,    exLEA,    exMov,    exPOP, // 0x80-
   exXCHG,   exXCHG,   exXCHG,   exXCHG,   exXCHG,   exXCHG,   exXCHG,   exXCHG,exConvSiz,exConvSiz,   exCALL,/*WAIT*/0,  EX_STAF,  EX_STAF,    exAHF,    exAHF, // 0x90-
    exMov,    exMov,    exMov,    exMov,   EX_STR,   EX_STR,   EX_STR,   EX_STR,   exTEST,   exTEST,   EX_STR,   EX_STR,   EX_STR,   EX_STR,   EX_STR,   EX_STR, // 0xa0-
    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov, // 0xb0-
EX186SIFT,EX186SIFT,    exRET,    exRET, exLDSLES, exLDSLES,    exMov,    exMov,EX186ENTR,EX186LEAV,    exRET,    exRET,    exINT,    exINT,   exINTO,   exIRET, // 0xc0-
  exShift,  exShift,  exShift,  exShift, exAADAAM, exAADAAM,        0,   exXLAT, /*ESC*/0,    exESC, /*ESC*/0,    exESC, /*ESC*/0,    exESC, /*ESC*/0, /*ESC*/0, // 0xd0-
   exLOOP,   exLOOP,   exLOOP,   exJCXZ,  exINOUT,  exINOUT,  exINOUT,  exINOUT,   exCALL,    exJMP,    exJMP,    exJMP,  exINOUT,  exINOUT,  exINOUT,  exINOUT, // 0xe0-
/*LOCK*/0,        0, /*REP*/0, /*REP*/0, /*HLT*/0,  EX_FLAG,   /*x*/0,   /*x*/0,  EX_FLAG,  EX_FLAG,  EX_FLAG,  EX_FLAG,  EX_FLAG,  EX_FLAG,   /*x*/0,   /*x*/0  // 0xf0-
};

#endif
