#ifndef __DEV_DMAC_H__ 
#define __DEV_DMAC_H__ 

#define IOADDR_DMAC0_BASE  0x000

#define IOADDR_DMAC_SIZE  0x010
#define IOADDR_DMAC_MASK  0x00f

#define DMAC_REG_START0      0x0 /* write */
#define DMAC_REG_CNT0        0x1 /* write */
#define DMAC_REG_START1      0x2 /* write */
#define DMAC_REG_CNT1        0x3 /* write */
#define DMAC_REG_START2      0x4 /* write */
#define DMAC_REG_CNT2        0x5 /* write */
#define DMAC_REG_START3      0x6 /* write */
#define DMAC_REG_CNT3        0x7 /* write */

#define DMAC_REG_CMD         0x8 /* write */
#define DMAC_REG_MODE        0xb /* write */
#define DMAC_REG_REQ         0x9 /* write */
#define DMAC_REG_MASK_SR     0xa /* set/reset */
#define DMAC_REG_MASK        0xf /* write */
#define DMAC_REG_TEMP        0xd /* read */
#define DMAC_REG_STATUS      0x8 /* read */

#define IOADDR_DMAC_PAGEADDR_BASE  0x080
#define IOADDR_DMAC_PAGEADDR_SIZE  0x010
#define IOADDR_DMAC_PAGEADDR_MASK  0x00f

#define DMA_PAGEADDR_CH0    0x87
#define DMA_PAGEADDR_CH1    0x83
#define DMA_PAGEADDR_CH2    0x81
#define DMA_PAGEADDR_CH3    0x82
#define DMA_PAGEADDR_CH4    0x8f
#define DMA_PAGEADDR_CH5    0x8b
#define DMA_PAGEADDR_CH6    0x89
#define DMA_PAGEADDR_CH7    0x8a


void setDMAPageAddr(struct stMachineState *pM, struct periDMAPageAddr *pPage, uint32_t addr, uint8_t data);

uint8_t readDMACReg(struct stMachineState *pM, struct periDMAC *pDMAC, uint32_t addr);
void writeDMACReg  (struct stMachineState *pM, struct periDMAC *pDMAC, uint32_t addr, uint8_t data);

#endif
