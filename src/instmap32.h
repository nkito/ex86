#ifndef __INSTMAP32_H__
#define __INSTMAP32_H__

#include "ExInst_common.h"
#include "ExInst86.h"
#include "ExInst186.h"
#include "ExInst386.h"


const void * instCodeFunc386_0xfe_0xff[8] = {
exINCDEC, exINCDEC, exCALL, exCALL, exJMP, exJMP, exPUSH, 0
};
const void * instCodeFunc386_0xf6_0xf7[8] = {
exTEST, 0, exNEGNOT, exNEGNOT, exMUL, exMUL, exDIV, exDIV
};


#define EX386PUSH exPUSH
#define EX386POP  exPOP
#define EX386SIFT exShiftDouble
#define EX386IMUL exIMUL2Op
#define EX386MOV  0
#define exJccc    exCondJump
#define exXADD    0
#define exCMPXCHG 0
#define exBSWAP   0
#define exBSF     exBitScan
#define exBSR     exBitScan

#define exLAR     exLARLSL
#define exLSL     exLARLSL

#define EX386LSEG exLDSLES

const void * instCodeFunc386_0x0f[256] = {
//      0         1         2         3         4         5         6         7        8          9         A         B         C         D         E         F
 exLSDesc, exLSDesc,    exLAR,    exLSL,        0,        0,   exCLTS,        0,        0,        0,        0,     exUD,        0,        0,        0,        0, // 0x00-
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0, // 0x10-
exMOVCRDR,exMOVCRDR,exMOVCRDR,exMOVCRDR, EX386MOV,        0, EX386MOV,        0,        0,        0,        0,        0,        0,        0,        0,        0, // 0x20-
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0, // 0x30-
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0, // 0x40-
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0, // 0x50-
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0, // 0x60-
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0, // 0x70-
   exJccc,   exJccc,   exJccc,   exJccc,   exJccc,   exJccc,   exJccc,   exJccc,   exJccc,   exJccc,   exJccc,   exJccc,   exJccc,   exJccc,   exJccc,   exJccc, // 0x80-
  exSETcc,  exSETcc,  exSETcc,  exSETcc,  exSETcc,  exSETcc,  exSETcc,  exSETcc,  exSETcc,  exSETcc,  exSETcc,  exSETcc,  exSETcc,  exSETcc,  exSETcc,  exSETcc, // 0x90-
EX386PUSH, EX386POP,        0,     exBT,EX386SIFT,EX386SIFT,exCMPXCHG,exCMPXCHG,EX386PUSH, EX386POP,        0,     exBT,EX386SIFT,EX386SIFT,        0,EX386IMUL, // 0xa0-
        0,        0,EX386LSEG,     exBT,EX386LSEG,EX386LSEG, exMOVSZX, exMOVSZX,        0,        0,     exBT,     exBT,    exBSF,    exBSR, exMOVSZX, exMOVSZX, // 0xb0-
   exXADD,   exXADD,        0,        0,        0,        0,        0,        0,  exBSWAP,  exBSWAP,  exBSWAP,  exBSWAP,  exBSWAP,  exBSWAP,  exBSWAP,  exBSWAP, // 0xc0-
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0, // 0xd0-
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0, // 0xe0-
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0, // 0xf0-
};

#define EX_PREF  exPrefixDummy
#define EX_LOCK  exPrefixDummy

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


#define EX_WAIT   exWAIT

const void * instCodeFunc386[256] = {
//      0         1         2         3         4         5         6         7        8          9         A         B         C         D         E         F
 exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,   exPUSH,    exPOP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,   exPUSH,   /*x*/0, // 0x00-
 exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,   exPUSH,    exPOP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,   exPUSH,    exPOP, // 0x10-
 exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,  EX_PREF,    exDAA, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,  EX_PREF, exDASAAS, // 0x20-
 exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,  EX_PREF, /*AAA*/0, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP, exALU2OP,  EX_PREF, exDASAAS, // 0x30-
 exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, exINCDEC, // 0x40-
   exPUSH,   exPUSH,   exPUSH,   exPUSH,   exPUSH,   exPUSH,   exPUSH,   exPUSH,    exPOP,    exPOP,    exPOP,    exPOP,    exPOP,    exPOP,    exPOP,    exPOP, // 0x50-
EX186PSHA,EX186POPA,EX186BOND,/*ARPL*/0,  EX_PREF,  EX_PREF,  EX_PREF,  EX_PREF,EX186PUSH,EX186IMUL,EX186PUSH,EX186IMUL, EX186IOS, EX186IOS, EX186IOS, EX186IOS, // 0x60-
  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP,  EX_CJMP, // 0x70-
 exALU2OP, exALU2OP,        0, exALU2OP,   exTEST,   exTEST,   exXCHG,   exXCHG,    exMov,    exMov,    exMov,    exMov,    exMov,    exLEA,    exMov,    exPOP, // 0x80-
   exXCHG,   exXCHG,   exXCHG,   exXCHG,   exXCHG,   exXCHG,   exXCHG,   exXCHG,exConvSiz,exConvSiz,   exCALL,  EX_WAIT,  EX_STAF,  EX_STAF,    exAHF,    exAHF, // 0x90-
    exMov,    exMov,    exMov,    exMov,   EX_STR,   EX_STR,   EX_STR,   EX_STR,   exTEST,   exTEST,   EX_STR,   EX_STR,   EX_STR,   EX_STR,   EX_STR,   EX_STR, // 0xa0-
    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov,    exMov, // 0xb0-
EX186SIFT,EX186SIFT,    exRET,    exRET, exLDSLES, exLDSLES,    exMov,    exMov,EX186ENTR,EX186LEAV,    exRET,    exRET,    exINT,    exINT,   exINTO,   exIRET, // 0xc0-
  exShift,  exShift,  exShift,  exShift, exAADAAM, exAADAAM,        0,   exXLAT,    exESC,    exESC,    exESC,    exESC,    exESC,    exESC,    exESC,    exESC, // 0xd0-
   exLOOP,   exLOOP,   exLOOP,   exJCXZ,  exINOUT,  exINOUT,  exINOUT,  exINOUT,   exCALL,    exJMP,    exJMP,    exJMP,  exINOUT,  exINOUT,  exINOUT,  exINOUT, // 0xe0-
  EX_LOCK,        0,  EX_PREF,  EX_PREF,    exHLT,  EX_FLAG,   /*x*/0,   /*x*/0,  EX_FLAG,  EX_FLAG,  EX_FLAG,  EX_FLAG,  EX_FLAG,  EX_FLAG,   /*x*/0,   /*x*/0  // 0xf0-
};

#endif
