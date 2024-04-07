#ifndef __I8086_H__ 
#define __I8086_H__ 

#include <setjmp.h>
#include <stdint.h>

#define EMU_MEM_SIZE (8*1024*1024)

#define EMU_CPU_8086	1
#define EMU_CPU_80186	2
#define EMU_CPU_80286	3
#define EMU_CPU_80386	4


#ifndef DEBUG
	extern int DEBUG;
#endif

struct stEmuSetting{
	int emu_cpu;
	uint32_t breakPoint;
	uint32_t breakMask;
	uint32_t runAfterBreak;
	uint64_t breakCounter;
	sigjmp_buf env;
	uint64_t nExecInsts;
    int      stop;
	unsigned int log_enabled_cat;
	unsigned int log_level;
};

#define PREF_REP_UNSPECIFIED -1
#define PREF_SEG_UNSPECIFIED -1

struct stPrefix{
	int8_t seg;
	int8_t repz;   // 
	int8_t addr32; // 0 : 16-bit, 1 : 32-bit
	int8_t data32; // 0 : 16-bit, 1 : 32-bit
};

struct stRawSegmentDesc{
	uint32_t base;
	uint32_t limit;
	uint8_t access;
	uint8_t flags;
};

struct stGateDesc{
	uint32_t offset;
	uint16_t selector;
	uint8_t access;
	uint8_t len;

	uint8_t DPL;
};

struct stDataDesc{
	uint32_t base;
	uint32_t limit;
	uint8_t access;
	uint8_t flags;

	uint8_t gran;
	uint8_t big;
	uint8_t writable;
	uint8_t DPL;
	uint32_t limit_min, limit_max;
};

struct stCodeDesc{
	uint32_t base;
	uint32_t limit;
	uint8_t access;
	uint8_t flags;

	uint8_t gran;
	uint8_t def;
	uint8_t readable;
	uint8_t conforming;
	uint8_t DPL;
	uint32_t limit_min, limit_max;
};

#define TLB_ENTRY_BITS 10
#define TLB_NASOC       2

struct stTLB{
	uint32_t addr[TLB_NASOC];
	uint32_t pte [TLB_NASOC];
};

/*
FIRST_WORD_IDX_IN_DWORD should be defined so that
"ax[FIRST_WORD_IDX_IN_DWORD]" is the lower half word of "eax"
*/
#if defined(HOST_BYTE_ORDER_LITTLE_ENDIAN) && defined(HOST_BYTE_ORDER_BIG_ENDIAN)
#error "Please define HOST_BYTE_ORDER_LITTLE_ENDIAN *OR* HOST_BYTE_ORDER_BIG_ENDIAN"
#elif defined(HOST_BYTE_ORDER_LITTLE_ENDIAN)
#define FIRST_WORD_IDX_IN_DWORD 0
#elif defined(HOST_BYTE_ORDER_BIG_ENDIAN)
#define FIRST_WORD_IDX_IN_DWORD 1
#elif defined(__BYTE_ORDER__)&&(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define FIRST_WORD_IDX_IN_DWORD 0
#elif defined(__BYTE_ORDER__)&&(__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define FIRST_WORD_IDX_IN_DWORD 1
#else
#error "Please define HOST_BYTE_ORDER_LITTLE_ENDIAN or HOST_BYTE_ORDER_BIG_ENDIAN"
#endif

struct stReg{
	union{
		uint32_t eax;
		uint16_t ax[2];
	};
	union{
		uint32_t ebx;
		uint16_t bx[2];
	};
	union{
		uint32_t ecx;
		uint16_t cx[2];
	};
	union{
		uint32_t edx;
		uint16_t dx[2];
	};

	union{
		uint32_t esp;
		uint16_t sp[2];
	};
	union{
		uint32_t ebp;
		uint16_t bp[2];
	};
	union{
		uint32_t esi;
		uint16_t si[2];
	};
	union{
		uint32_t edi;
		uint16_t di[2];
	};

	uint16_t es;
	uint16_t cs;
	uint16_t ss;
	uint16_t ds;

	uint16_t fs; // added from 386
	uint16_t gs; // added from 386

	union{
		uint32_t eip;
		uint16_t ip[2];
	};
	union{
		uint32_t eflags;
		uint16_t flags[2];
	};

	uint32_t cr[4];
	uint32_t dr[8];

	uint32_t tr6;
	uint32_t tr7;

	uint32_t gdtr_base;
	uint32_t idtr_base;

	uint16_t gdtr_limit;
	uint16_t idtr_limit;

	uint16_t ldtr;
	uint16_t tr; // task register

	struct stDataDesc descc_es;
	struct stCodeDesc descc_cs;
	struct stDataDesc descc_ss;
	struct stDataDesc descc_ds;
	struct stDataDesc descc_fs;
	struct stDataDesc descc_gs;

	struct stRawSegmentDesc descc_ldt;	// cache of local descriptor
	struct stRawSegmentDesc descc_tr;	// cache of task register

	struct stTLB tlb[1<<TLB_ENTRY_BITS];

	uint32_t fault;
	uint32_t error_code;
	uint8_t  cpl;

	uint32_t current_eip;
	uint16_t current_cs;
	uint8_t fetchCache[2];
};

enum eOpType {
	OpTypeReg,
	OpTypeSegReg,
	OpTypeMemWithSeg,
	OpTypeMemWithSeg_Reg,
	OpTypeMemDirect
};

struct stOpl{
	enum eOpType type;
	uint8_t  width; // 0: byte, 1:word
	uint8_t  reg;
	uint32_t addr;
};

struct periTimer{
	uint8_t  control[3];
	uint8_t  rwCount[3];
	uint16_t counter[3];
	uint16_t counter_shadow[3];
	uint16_t dummy_counter[3];
};

struct periPIC{
	uint8_t  acc_idx;
	uint8_t  icw1;
	uint8_t  icw2;
	uint8_t  icw3;
	uint8_t  icw4;
	uint8_t  ocw1;
	uint8_t  ocw2;
	uint8_t  ocw3;
};

struct periFDC{
	uint8_t fifo[16];
	uint8_t fifo_pointer;
	uint8_t resp_len;
	uint8_t resp_cnt;
	uint8_t dor;
	uint8_t busy;
	uint8_t irq;
	uint8_t cylinder;	 /* C */
	uint8_t head;		 /* H */
	uint8_t sector;		 /* R */
	uint8_t sector_size; /* N */
};

struct periDMAC{
	uint32_t addr[4];
	uint8_t cnt[4];
};

struct periDMAPageAddr{
	uint8_t addr[8];
};

#include <time.h>

struct stIO_CMOS{
	uint8_t reg_addr;
	struct tm prevTM;
};

struct stIO_UART{
    int buffered;
	uint8_t int_enable;
    uint8_t buf[1];
	uint8_t scratch;
	uint8_t chkCntForInt;
};

struct stMemIORegion{
	// watch address (disabled if watchAddr >= EMU_MEM_SIZE)
	uint32_t watchAddr;

	struct periTimer ioTimer;
	struct periPIC ioPICmain;
	struct periPIC ioPICsub;

	struct periFDC  ioFDC;
	struct periDMAC ioDMAC;
	struct periDMAPageAddr ioDMAPage;
	struct stIO_CMOS ioCMOS;
	struct stIO_UART ioUART0;
	struct stIO_UART ioUART1;

	uint8_t ioSysCtrlA;
	uint8_t ioSysCtrlB;

	uint8_t a20m;
	uint8_t enableOldIO;

    uint8_t *mem;
};

struct stMachineState{
	struct stEmuSetting  emu;
	struct stReg reg;
	struct stPrefix prefix;
	struct stMemIORegion mem;
};



#define MEMADDR(seg, oft) (((((uint32_t)(seg)) << 4) + ((uint32_t)(oft))) & 0xfffff)

void printOpl(struct stOpl *pOp);

void printReg(struct stReg *preg);
uint32_t parseHex(char *str);

#define INST_V_BIT     bit1
#define INST_S_BIT     bit1
#define INST_D_BIT     bit1
#define INST_W_BIT     bit0
#define INST_W_BIT_MOV_IMMREG bit3

#define INST_V_CNT1     0
#define INST_V_CNTCL    1

#define INST_S_NOSIGNEX 0
#define INST_S_SIGNEX   1

#define INST_W_BYTEACC   0
#define INST_W_WORDACC   1

#define INST_D_FIRSTOP  0
#define INST_D_SECONDOP 1

#define FLAGS_BIT_CF 0  /* carry flag */
#define FLAGS_BIT_PF 2  /* parity flag */
#define FLAGS_BIT_AF 4  /* auxiliary carry flag */
#define FLAGS_BIT_ZF 6  /* zero flag */
#define FLAGS_BIT_SF 7  /* sign flag */
#define FLAGS_BIT_TF 8  /* trap flag */
#define FLAGS_BIT_IF 9  /* interrupt enable flag */
#define FLAGS_BIT_DF 10 /* direction flag */
#define FLAGS_BIT_OF 11 /* overflow flag */

#define EFLAGS_BIT_IOPL 12 /* I/O privilege level */
#define EFLAGS_BIT_IOPL_MASK (3<<EFLAGS_BIT_IOPL) /* mask for I/O privilege level */
#define EFLAGS_BIT_NT   14 /* nested task flag */
#define EFLAGS_BIT_RF   16 /* resume flag */
#define EFLAGS_BIT_VM   17 /* virtual mode */
#define EFLAGS_BIT_AC   18 /* alignment check */

#define CR0_BIT_PE 0   /* Protection Enable */
#define CR0_BIT_MP 1   /* Monitor Coprocessor */
#define CR0_BIT_EM 2   /* Emulate Coprocessor */
#define CR0_BIT_TS 3   /* Task Switched */
#define CR0_BIT_NE 5   /* Numerics Exception */
#define CR0_BIT_WP 16  /* Write Protect */
#define CR0_BIT_AM 18  /* Alignment Mask */
#define CR0_BIT_WT 29  /* Writes Transparent (cache) */
#define CR0_BIT_CE 30  /* Cache Enable */
#define CR0_BIT_PG 31  /* Paging Enable */


#define REG_NUM_AX 0
#define REG_NUM_CX 1
#define REG_NUM_DX 2
#define REG_NUM_BX 3
#define REG_NUM_SP 4
#define REG_NUM_BP 5
#define REG_NUM_SI 6
#define REG_NUM_DI 7

#define SEGREG_NUM_ES 0
#define SEGREG_NUM_CS 1
#define SEGREG_NUM_SS 2
#define SEGREG_NUM_DS 3
#define SEGREG_NUM_FS 4  /* added in 386 */
#define SEGREG_NUM_GS 5  /* added in 386 */

#define SEGREG_NUMS16 4  /* # segment registers */
#define SEGREG_NUMS32 6  /* # segment registers */


// Ref: iAPX86 88 186 188 Programmer Reference Table 5-2
//    : iAPX286 Programmers Reference Manual Table 2-4
#define INTNUM_DIVIDE_ERROR 0  /* #DE, Divide Error Exception */
#define INTNUM_SINGLE_STEP  1  /* #DB, Single Step Interrupt */
#define INTNUM_NMI          2  /* NMI */
#define INTNUM_BREAKPOINT   3  /* #BP, Breakpoint Interrupt */
#define INTNUM_OVERFLOW     4  /* #OF, Overflow Exception */
#define INTNUM_BOUNDS       5  /* #BR, Array Bounds Exception */
#define INTNUM_UDOPCODE     6  /* #UD, Unused-Opcode (Invalid opcode) Exception */
#define INTNUM_ESCOPCODE    7  /* #NM, ESC Opcode Exception (Processor extension not available exception) */

#define FAULTNUM_DOUBLE     8  /* #DF, double fault */
#define FAULTNUM_INVALIDTSS 10 /* #TS, Invalid TSS */
#define FAULTNUM_SEGNOTP    11 /* #NP, Segment not present */
#define FAULTNUM_STACKFAULT 12 /* #SS, stack  */
#define FAULTNUM_GP         13 /* #GP, general protection fault */
#define FAULTNUM_PAGEFAULT  14 /* #PF, Page Fault */

#define FAULTNUM_UNKNOWN    31

/* Bits in flags of segment descriptor bits[52:55] */
#define DESC_FLAGS_BIT_G	3	/* 55-bit Granularity bit */
#define DESC_FLAGS_BIT_D	2	/* 54-bit Default bit (0:16-bit, 1:32-bit) */


#define TSS_MINIMUM_LIMIT_VALUE_32BIT	0x67 /* size of a 32-bit TSS is 0x67(=103, i.e., 4x26-1) or greater */

#endif
