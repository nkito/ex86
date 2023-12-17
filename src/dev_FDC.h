#ifndef __DEV_FDC_H__ 
#define __DEV_FDC_H__ 

#define IOADDR_FDC_BASE  0x3f0
#define IOADDR_FDC_SIZE  0x008
#define IOADDR_FDC_MASK  0x007

#define FDC_STATUS_A     0x00
#define FDC_STATUS_B     0x01
#define FDC_DOR          0x02 /* digital output register */
#define FDC_MSR          0x04 /* read, main status register */
#define FDC_DATARATE_SEL 0x04 /* write*/
#define FDC_DATA_FIFO    0x05
#define FDC_DIR          0x07 /* read, digital input register */
#define FDC_CONF_CTRL    0x07 /* write */

#define FDC_CMD_RECALIBRATE          0x07
#define FDC_CMD_SEEK                 0x0F
#define FDC_CMD_READ                 0xE6
#define FDC_CMD_WRITE                0xC5
#define FDC_CMD_SENSEI               0x08
#define FDC_CMD_SPECIFY              0x03
#define FDC_CMD_FORMAT               0x4D
#define FDC_CMD_VERSION              0x10
#define FDC_CMD_CONFIGURE            0x13
#define FDC_CMD_PERPENDICULAR        0x12
#define FDC_CMD_GETSTATUS            0x04
#define FDC_CMD_DUMPREGS             0x0E
#define FDC_CMD_READID               0xEA
#define FDC_CMD_UNLOCK               0x14
#define FDC_CMD_FD_LOCK              0x94
#define FDC_CMD_RSEEK_OUT            0x8f
#define FDC_CMD_RSEEK_IN             0xcf
#define FDC_CMD_PARTID               0x18

uint8_t readFDCReg(struct stMachineState *pM, struct periFDC *pFDC, uint32_t addr);
void writeFDCReg  (struct stMachineState *pM, struct periFDC *pFDC, uint32_t addr, uint8_t data);

#endif
