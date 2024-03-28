#ifndef __TIMER_H__
#define __TIMER_H__


#define ConvertTimerToSecond(     timer )   ((timer) / 100)
#define ConvertTimerToMiliSecond( timer )   ((timer) *  10)

#define PIT_CLOCK_FREQ 1193180

#define TICK_PER_HOUR 65543

void initTimer(void);
void startTimer(void);
void stopTimer(void);
unsigned long getTimer(void);
unsigned long setTimer(unsigned long val);
unsigned long updateCounter(void);
unsigned long updateTickOffset(long newTick);

#endif
