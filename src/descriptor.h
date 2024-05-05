#ifndef __DESCRIPTOR_H__ 
#define __DESCRIPTOR_H__ 


#define SYSDESC_TYPE_LDT				2
#define SYSDESC_TYPE_TASKGATE			5

#define SYSDESC_TYPE_TSS16_AVAIL		1
#define SYSDESC_TYPE_TSS16_BUSY		    3
#define SYSDESC_TYPE_CALLGATE16	    	4
#define SYSDESC_TYPE_INTGATE16			6
#define SYSDESC_TYPE_TRAPGATE16 		7

#define SYSDESC_TYPE_TSS32_AVAIL		9
#define SYSDESC_TYPE_TSS32_BUSY 		0xb
#define SYSDESC_TYPE_CALLGATE32	    	0xc
#define SYSDESC_TYPE_INTGATE32			0xe
#define SYSDESC_TYPE_TRAPGATE32		    0xf


#define SEGACCESS_CSEG_IS_READABLE(x)		(((x)&0x1a) == 0x1a)
/* Code segment may only be executed when CPL >= DPL and CPL remains unchanged. */
#define SEGACCESS_CSEG_IS_CONFORMING(x)		(((x)&0x1c) == 0x1c)

#define SEGACCESS_DSEG_IS_WRITABLE(x)		(((x)&0x1a) == 0x12)
#define SEGACCESS_DSEG_IS_EXPANDS_DOWN(x)	(((x)&0x1c) == 0x14)

#define SEGACCESS_IS_DSEG(x)				(((x)&0x18) == 0x10)
#define SEGACCESS_IS_CSEG(x)				(((x)&0x18) == 0x18)
#define SEGACCESS_IS_SYSTEMSEG(x)			(((x)&0x10) == 0x00)

#define SEGACCESS_IS_TSS32(x)               (((x)&0x1d) == SYSDESC_TYPE_TSS32_AVAIL)
#define SEGACCESS_IS_TSS32_AVAIL(x)         (((x)&0x1f) == SYSDESC_TYPE_TSS32_AVAIL)
#define SEGACCESS_IS_TSS32_BUSY(x)          (((x)&0x1f) == SYSDESC_TYPE_TSS32_BUSY)
#define SEGACCESS_IS_CALLGATE16(x)          (((x)&0x1f) == SYSDESC_TYPE_CALLGATE16)
#define SEGACCESS_IS_CALLGATE32(x)          (((x)&0x1f) == SYSDESC_TYPE_CALLGATE32)
#define SEGACCESS_IS_INTGATE16(x)           (((x)&0x1f) == SYSDESC_TYPE_INTGATE16)
#define SEGACCESS_IS_INTGATE32(x)           (((x)&0x1f) == SYSDESC_TYPE_INTGATE32)
#define SEGACCESS_IS_TRAPGATE16(x)          (((x)&0x1f) == SYSDESC_TYPE_TRAPGATE16)
#define SEGACCESS_IS_TRAPGATE32(x)          (((x)&0x1f) == SYSDESC_TYPE_TRAPGATE32)


#define SEGACCESS_CSEG_ACCESSED		0x01
#define SEGACCESS_CSEG_READABLE		0x02
#define SEGACCESS_CSEG_CONFORMING	0x04

#define SEGACCESS_DSEG_ACCESSED		0x01
#define SEGACCESS_DSEG_WRITABLE 	0x02
#define SEGACCESS_DSEG_EXDOWN		0x04

#define SEGACCESS_CSEG		  		0x08
#define SEGACCESS_CODE_DATA_SEG		0x10
#define SEGACCESS_PRESENT           0x80  /* present */

#define SEGACCESS_BIT_DPL    		5
#define SEGACCESS_DPL_MASK     		(3<<SEGACCESS_BIT_DPL)
#define SEGACCESS_DPL(x)            (((x)&SEGACCESS_DPL_MASK) >> SEGACCESS_BIT_DPL)


//#define SEGFLAGS_DSEG_AVAIL    0x01  /* available */
//#define SEGFLAGS_CSEG_AVAIL    0x01  /* available */
#define SEGFLAGS_DSEG_B_BIT    0x04  /* big (ESP/SP is used if this bit is 1/0) */
#define SEGFLAGS_CSEG_D_BIT    0x04  /* default bit (1: 32-bit) */
#define SEGFLAGS_DSEG_GRAN     0x08  /* granularity (1: page 0: bit) */
#define SEGFLAGS_CSEG_GRAN     0x08  /* granularity (1: page 0: bit) */


#define TSS_MINIMUM_LIMIT_VALUE_32BIT	0x67 /* size of a 32-bit TSS is 0x67(=103, i.e., 4x26-1) or greater */


// Functions for segment descriptors (CODE, DATA, LDT, TSS)

void loadRawSegmentDescFromGDT(struct stMachineState *pM, uint16_t selector, struct stRawSegmentDesc *pRS);
void loadRawSegmentDescFromLDT(struct stMachineState *pM, uint16_t selector, struct stRawSegmentDesc *pRS);
void loadRawSegmentDesc       (struct stMachineState *pM, uint16_t selector, struct stRawSegmentDesc *pRS);

void loadDataSegmentDesc (struct stMachineState *pM, uint16_t selector, struct stDataDesc *pDD);
void loadStackSegmentDesc(struct stMachineState *pM, uint16_t selector, struct stDataDesc *pDD);
void loadCodeSegmentDesc (struct stMachineState *pM, uint16_t selector, struct stCodeDesc *pCD);
void loadTaskRegister    (struct stMachineState *pM, uint16_t selector, struct stRawSegmentDesc *pRS, int setTaskBusy, uint16_t prevTaskLink);
void unloadTaskRegister  (struct stMachineState *pM, uint32_t nextEIP, int clearTaskBusy, int clearNTflag);
void loadTaskState       (struct stMachineState *pM);
int  readTSSIOMapBit     (struct stMachineState *pM, uint16_t ioaddr);


// Functions for gate descriptors (Call, Trap, Interrupt, Task gates)
void loadIntDesc   (struct stMachineState *pM, uint8_t   int_num, struct stGateDesc *pID);
void loadGateDesc(struct stMachineState *pM, uint16_t selector, struct stGateDesc *pGD);

int getDescType(struct stMachineState *pM, uint16_t selector, uint8_t *pAccess);

#endif

