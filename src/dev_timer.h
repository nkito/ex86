#ifndef __DEV_TIMER_H__ 
#define __DEV_TIMER_H__ 

#define I8254_CONTROL_SEL_BIT  6
#define I8254_CONTROL_SEL_MASK (3<<I8254_CONTROL_SEL_BIT)

#define I8254_CONTROL_RW_BIT  4
#define I8254_CONTROL_RW_MASK (3<<I8254_CONTROL_RW_BIT)

#define I8254_CONTROL_MODE_BIT  1
#define I8254_CONTROL_MODE_MASK (7<<I8254_CONTROL_MODE_BIT)

#define I8254_CONTROL_BCD_BIT  0
#define I8254_CONTROL_BCD_MASK (1<<I8254_CONTROL_BCD_BIT)


void    writeTimerReg(struct stMachineState *pM, uint8_t addr, uint8_t data);
uint8_t readTimerReg(struct stMachineState *pM, uint8_t addr);

#endif
