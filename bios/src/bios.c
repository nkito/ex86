#include <stdio.h>
#include "basicio.h"

#include "terminal.h"
#include "timer.h"
#include "system.h"

#include "asmfuncs.h"

#include "int10h.h"

#include "emu_interface.h"

int bootDrive = -1;

unsigned long BufMem[1024/sizeof(unsigned long)];


void printSection(unsigned int level, char *str){
	if( level == 0 ){
		disp_printf(0x2f, "\n\r %s", str);
		disp_printf(0x2f, "\n\r");
	}else if( level == 1 ){
		disp_printf(0x3f, "%s", str);
		disp_printf(0x3f, "\r\n");
	}else{
		disp_printf(0xa, "%s", str);
		disp_printf(0xf, "\r\n");
	}
}

unsigned int  hdd_ready    =  0;
unsigned int  hdd_cylinder =  0; // 0-based
unsigned int  hdd_head     =  0; // 0-based
unsigned int  hdd_sector   = 63; // 1-based
unsigned long hdd_total_sectors =  0;

#define DISP_ATTR_NORMAL 0x07

int fstest(void){
 	unsigned char *buf = (unsigned char *)BufMem;

	uint8_t result;
	uint32_t driveSize;
	unsigned int reg_ss;

 	disp_printf(DISP_ATTR_NORMAL, "Drive A: ");
	result = emuGetDriveSize(0, &driveSize);
 	if(result == EMU_INTERFACE_RESULT_OK){
 		disp_printf(DISP_ATTR_NORMAL, " OK  (%lu bytes)\n", driveSize);
 		if( bootDrive < 0 ) bootDrive = 0;
 	}else{
 		disp_printf(DISP_ATTR_NORMAL, " -- \n");
 	}

 	disp_printf(DISP_ATTR_NORMAL, "Drive B: ");
	result = emuGetDriveSize(1, &driveSize);
 	if(result == EMU_INTERFACE_RESULT_OK){
 		disp_printf(DISP_ATTR_NORMAL, " OK  (%lu bytes)\n", driveSize);
 		if( bootDrive < 0 ) bootDrive = 0;
 	}else{
 		disp_printf(DISP_ATTR_NORMAL, " -- \n");
 	}

	hdd_ready = 0;
	disp_printf(DISP_ATTR_NORMAL,"Drive C: ");
	result = emuGetDriveSize(2, &driveSize);
 	if(result == EMU_INTERFACE_RESULT_OK){
		disp_printf(DISP_ATTR_NORMAL, " OK  (%lu bytes)\n", driveSize);
		if( bootDrive < 0 ) bootDrive = 2;

		//--------------------------------
		asm volatile("mov  %%ss, %w0" : "=a"(reg_ss) : );
		result = emuReadDriveSector(2, 0, reg_ss, (uint16_t)buf);
		if(result != EMU_INTERFACE_RESULT_OK) goto drive_err;

		if( buf[510] != 0x55 || buf[511] != 0xAA ){
			disp_printf(DISP_ATTR_NORMAL, "  no MBR found\n");
		}else{
			disp_printf(DISP_ATTR_NORMAL, "  MBR found ");
			for(int p=0; p<4;p++){
				int start_cyl  = (((unsigned int)buf[0x1be + 2 +0x10*p] & 0xc0)<<2) + (unsigned int)buf[0x1be + 3 +0x10*p];
				int start_head =   buf[0x1be + 1 +0x10*p];
				int start_sec  =  (buf[0x1be + 2 +0x10*p] & 0x3f);
				int end_cyl  = (((unsigned int)(buf[0x1be + 6 +0x10*p] & 0xc0))<<2) + (unsigned int)buf[0x1be + 7 +0x10*p];
				int end_head =   buf[0x1be + 5 +0x10*p];
				int end_sec  =  (buf[0x1be + 6 +0x10*p] & 0x3f);

				if( end_cyl == 0 || end_head == 0 || end_sec == 0 ) continue;

				hdd_ready    = 1;
				hdd_total_sectors = (driveSize >> 9);
				hdd_head     = end_head;
				hdd_sector   = end_sec;
				hdd_cylinder = hdd_total_sectors / ( ((unsigned long)hdd_sector) * (hdd_head+1));

				disp_printf(DISP_ATTR_NORMAL, " (Partition %d: CHS = %u-%u-%u - %u-%u-%u (%lu sectors))\n", p,
							start_cyl, start_head, start_sec,
							end_cyl,   end_head,   end_sec,
							hdd_total_sectors);
				disp_printf(DISP_ATTR_NORMAL, "  CHS is set to %u-%u-%u.\n", hdd_cylinder, hdd_head, hdd_sector);
				break;
			}
		}
		//--------------------------------
	}else{
		disp_printf(DISP_ATTR_NORMAL, " -- \n");
	}

	if( bootDrive < 0 ){
 	    disp_printf(DISP_ATTR_NORMAL, "There are no images.\n\n");
 		return 1;
 	}

 	return 0;

drive_err:
    disp_printf(DISP_ATTR_NORMAL, "Drive error... \n\n");
 	return 1;

}

int loadSector(void){
	unsigned char *buf = (unsigned char *)BufMem;
	unsigned int reg_ss;
	uint8_t result;

 	disp_printf(DISP_ATTR_NORMAL, "Loading boot sector ... ");

	asm volatile("mov  %%ss, %w0" : "=a"(reg_ss) : );
	result = emuReadDriveSector(bootDrive, 0, reg_ss, (uint16_t)buf);
	if(result != EMU_INTERFACE_RESULT_OK) goto drive_err; 

 	disp_printf(DISP_ATTR_NORMAL, "done\n");
 	if( buf[510] != 0x55 || buf[511] != 0xAA ){
 		disp_printf(DISP_ATTR_NORMAL, "Warning: tail of the sector is not 0x55AA (0x%02x%02x)\n", buf[510],buf[511]);
 	}

 	copy_data(buf, 0x0000, (unsigned char *)0x7C00, 512);
 	disp_printf(DISP_ATTR_NORMAL, "Placed the sector data at 0x0000:0x7C00.\n");

 	return 0;

drive_err:
    disp_printf(DISP_ATTR_NORMAL, "\n\nFailed to read the boot sector\n\n");
 	return 1;
}

extern short  __stext, __etext, __ltext;
extern short  __sdata, __edata, __ldata;
extern short  __sbss , __ebss , __lbss0;

extern unsigned short int_default_vector_table[]; 
extern unsigned short int10_handler_asm;
extern unsigned short int11_handler_asm;
extern unsigned short int12_handler_asm;
extern unsigned short int13_handler_asm;
extern unsigned short int14_handler_asm;
extern unsigned short int15_handler_asm;
extern unsigned short int16_handler_asm;
extern unsigned short int1a_handler_asm;
extern unsigned short int_timer_handler_asm;

/* 486-class CPUs and newer ones have cache memory. Cache flush is necessary for some memory-related operations such as controling the A20 line.  This value is set in a start-up asm code */
unsigned short processor_is_486_or_newer = 0;
/* the initial value of DX register was used to represent the processor model in some processors as CPUID instruction does in recent processors. This value is set in a start-up asm code */
unsigned short initial_dx_value = 0;

int main(void){
	int i;
	unsigned int reg_cs;

	//--------------------------------------------------
	asm volatile("mov  %%cs, %w0" : "=a"(reg_cs) : );
	unsigned short *p = (unsigned short *)BufMem;
	for(i=0; i<256; i++){
		*p++ = int_default_vector_table[i];
		*p++ = reg_cs;
	}

	p = (unsigned short *)BufMem;
	p[0x10<<1] = (unsigned short)&int10_handler_asm;
	p[0x11<<1] = (unsigned short)&int11_handler_asm;
	p[0x12<<1] = (unsigned short)&int12_handler_asm;
	p[0x13<<1] = (unsigned short)&int13_handler_asm;
	p[0x14<<1] = (unsigned short)&int14_handler_asm;
	p[0x15<<1] = (unsigned short)&int15_handler_asm;
	p[0x16<<1] = (unsigned short)&int16_handler_asm;
	p[0x1a<<1] = (unsigned short)&int1a_handler_asm;
	p[0x08<<1] = (unsigned short)&int_timer_handler_asm;

	copy_data((unsigned char *)BufMem, 0x00, 0x0000, 1024);
	//--------------------------------------------------

	s_memset(BufMem, 0x00, 256);

	unsigned char *pbyte = (unsigned char *)BufMem;
	*((unsigned int  *)(&pbyte[0x10])) = ((1<<8)+(0x01<<6)+(0x02<<4)+2+1);	// See: https://stanislavs.org/helppc/int_11.html
	*((unsigned int  *)(&pbyte[0x13])) = 640;	// Memory Size in KBytes (INT 12h)
	*((unsigned int  *)(&pbyte[0x1A])) = 0x1e;	// Keyboard buffer (40:1e-40:3d 32bytes) head from 40:0
	*((unsigned int  *)(&pbyte[0x1C])) = 0x1e;	// Keyboard buffer (40:1e-40:3d 32bytes) tail from 40:0
	*((unsigned char *)(&pbyte[0x49])) = 0x03;	// Video mode (Text  80 x 25 All but MDA 16 fore/8 back B8000)
	*((unsigned int  *)(&pbyte[0x4a])) = 80;	// Number of screen columns 
	*((unsigned int  *)(&pbyte[0x63])) = 0x3d4;	// Base port address for Video controller (TODO: check)
	*((unsigned char *)(&pbyte[0x84])) = 24;	// Rows on the screen (0-based)
	*((unsigned int  *)(&pbyte[0x4c])) = 0x1000; // Size of video region buffer (4KB)
	*((unsigned char *)(&pbyte[0x60])) = 07;	// Cuesor ending scan line
	*((unsigned char *)(&pbyte[0x61])) = 06;	// Cuesor starting scan line

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 40:AC : Second entry point of INT10 handler
	//
	// Note that 40:AC - 40:B3 (8bytes) are reserved space of BIOS Data area
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	*((unsigned char *)(&pbyte[0xAC])) = 0xEA;		// ljmp (JMP ptr16:16)
	*((unsigned int  *)(&pbyte[0xAD])) = (unsigned short)&int10_handler_asm; // offset
	*((unsigned int  *)(&pbyte[0xAF])) = reg_cs;	// segment
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	copy_data((unsigned char *)BufMem, 0x00, (unsigned char *)0x400, 256);
	//--------------------------------------------------

	/* Initialize the display of the emulated computer */
    unsigned char __far * dbuf = _MK_FP(0xb800, 0);
	for(i=0; i< 80*25; i++){
		*dbuf++ = ' ';
		*dbuf++ = 0x08;
	}

	// cursor position when OS boot
//	set_cursorPosition(0, 0, 24);
//	set_cursorPosition(0, 0, 15);

	s_printf("\033[2J");	// Clear the host terminal by the escape sequence code
	printSection(0, "Sweet pea x86 BIOS   (Build: " __DATE__ " " __TIME__") ");

	disp_printf(DISP_ATTR_NORMAL, "Text   : 0x%04x:0x%04x - 0x%04x:0x%04x (0x%04x byte)\r\n"
	         "Data   : 0x%04x:0x%04x - 0x%04x:0x%04x (0x%04x byte)\r\n"
			 "BSS    : 0x%04x:0x%04x - 0x%04x:0x%04x (0x%04x byte)\r\n",
		reg_cs, &__stext, reg_cs, &__etext, &__ltext, 
		reg_cs, &__sdata, reg_cs, &__edata, &__ldata, 
		reg_cs, &__sbss , reg_cs, &__ebss , &__lbss0);
	disp_printf(DISP_ATTR_NORMAL, "CPU : i80%d  Memory : %d KiB\r\n",
		emuGetCPUType(), emuGetMemoryCapacity()/1024);

	//--------------------------------------------------
	printSection(1, "Initializing devices ...");
	initTimer();

	printSection(1, "Disks");
	if( fstest() != 0 ){
		disp_printf(DISP_ATTR_NORMAL, "Boot process failed....");
		return 0;
	}

	printSection(1, "Boot");

//	bootDrive = 2;
	disp_printf(DISP_ATTR_NORMAL, "Booting from drive %c: ...\n", 'A'+bootDrive);
	loadSector();

	disp_printf(DISP_ATTR_NORMAL, "Switched to Operating System ...\n", 'A'+bootDrive);

	int BIOSdriveNum = bootDrive;
	if(BIOSdriveNum >= 2){
		BIOSdriveNum = (BIOSdriveNum-2) + 0x80;
	}

	//--------------------------------------------------
	// starting the timer
	startTimer();
	//--------------------------------------------------

	// DX : boot drive number
	asm volatile(
		"mov $0xbfff, %%sp\n"
		"sti\n"
		"mov  %w0, %%dx\n"
		"ljmp $0x00, $0x7c00\n"
		 :  : "a"(BIOSdriveNum) );

	return 0;
}

