#ifndef __ENV_UNIX_H__
#define __ENV_UNIX_H__

#define PRINTF(...)  printf(__VA_ARGS__)
#define FPRINTF(...) fprintf(__VA_ARGS__)
#define FLUSH_STDOUT() fflush(stdout)

int initEmulator(struct stMachineState *pM, int argc, char *argv[]);
void shutdownEmulator(struct stMachineState *pM);


void initTerminalSetting(struct stMachineState *pM);
void saveTerminalSetting(struct stMachineState *pM);
void restoreTerminalSetting(struct stMachineState *pM);

int getIntFlag(struct stMachineState *pM);
int getTimerFlag(struct stMachineState *pM);
void resetTimerFlag(struct stMachineState *pM);

void setTimerInUSec(struct stMachineState *pM, uint32_t interval_in_Usec);

uint64_t getTimeInMs(struct stMachineState *pM);

#endif