#ifndef __ENV_MEM_H__
#define __ENV_MEM_H__

#include <stdint.h>

#define USE_SIGLONG_JMP

struct stMemRegion {
    uint8_t *mem;
};

#define MEM_READ( pM, addr)      ((pM)->pMemIo->stMem.mem[(addr)])
#define MEM_WRITE(pM, addr,val)  ((pM)->pMemIo->stMem.mem[(addr)] = (val))

#endif