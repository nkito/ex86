#include <stdio.h>

#include "basicio.h"
#include "system.h"
#include "emu_interface.h"

unsigned short intr_num;

void intr_print_default_msg(unsigned short sp, unsigned short ss, unsigned short ax, unsigned short bx, unsigned short cx, unsigned short dx){
    char buf[128];
	unsigned int reg_ss;

    asm volatile("mov  %%ss, %w0" : "=a"(reg_ss) : );

    s_snprintf(buf, sizeof(buf), "INT 0x%02x\n", intr_num);
    emuLogMessage(reg_ss, (uint16_t)buf);

    s_snprintf(buf, sizeof(buf), "AX: %04x BX: %04x CX: %04x DX: %04x SS: %04x SP: %04x\n", ax, bx, cx, dx, ss, sp);
    emuLogMessage(reg_ss, (uint16_t)buf);

}

