#ifndef __EMU_INTERFACE_H__
#define __EMU_INTERFACE_H__

/*
Programs in emulators can communicate with the emulator through OUT instruction.

Example:
 OUT 0x23(=IOADDR_EMU_INTERFACE), Offset

where Offset is the address containing the following data structure.

        0   1      2    3 ...
MAGIC_NUM CMD result args ...

MAGIC_NUM : fill the value of EMU_INTERFACE_MAGIC_NUM
CMD       : fill one of EMU_INTERFACE_CMD_... 
result    : don't care. The execution result of the command is placeed here by the emulater.
args      : varies according to commands.
*/
#define  IOADDR_EMU_INTERFACE 0x23


#define EMU_INTERFACE_MAGIC_NUM  0x5a

// args (1byte) : 1 byte 
#define EMU_INTERFACE_CMD_PRINT_MSG  0x00

// args (4byte) : [segment(L)] [segment(H)] [offset(L)] [offset(H)]
#define EMU_INTERFACE_CMD_LOG_MESSAGE  0x01

// args (6byte) : [result(year-1900)] [result(month(1-based))] [result(dayInMonth(1-based))] [result(hour)] [result(min)] [result(sec)]
#define EMU_INTERFACE_CMD_GET_TIME 0x02

// args (4byte) : [result(LSB)] [result] [result] [result(MSB)]
#define EMU_INTERFACE_CMD_GET_MEM_CAPACITY 0x03

// args (2byte) : [result(LSB)] [result(MSB)]
#define EMU_INTERFACE_CMD_GET_CPU_TYPE 0x04

// args (8byte) : [segment(L)] [segment(H)] [offset(filename)(L)] [offset(filename)(H)] [offset(buffer)(L)] [offset(buffer)(H)] ([512byte-block#(L)] [512byte-block#(H)]) / ([#read byte(L)] [#read byte(H)])
#define EMU_INTERFACE_CMD_READ_HOST_FILE   0x05

// args (8byte) : [segment(L)] [segment(H)] [offset(filename)(L)] [offset(filename)(H)] [offset(buffer)(L)] [offset(buffer)(H)] [#bytes / #write byte] [opt]
#define EMU_INTERFACE_CMD_WRITE_HOST_FILE  0x06
#define EMU_INTERFACE_CMD_WRITE_HOST_FILE_OPT_CREATE    0x16
#define EMU_INTERFACE_CMD_WRITE_HOST_FILE_OPT_APPEND    0x19
#define EMU_INTERFACE_CMD_WRITE_HOST_FILE_OPT_OVERWRITE 0x22


// args (5byte) : [drive number(0="A", 1="B",...)] [result(LSB)] [result] [result] [result(MSB)]
#define EMU_INTERFACE_CMD_GET_DRIVE_SIZE 0x10

// args (9byte) : [drive number(0="A", 1="B",...)] [sector(LSB)] [sector] [sector] [sector(MSB)] [segment(L)] [segment(H)] [offset(L)] [offset(H)]
#define EMU_INTERFACE_CMD_READ_DRIVE_SECTOR 0x11

// args (9byte) : [drive number(0="A", 1="B",...)] [sector(LSB)] [sector] [sector] [sector(MSB)] [segment(L)] [segment(H)] [offset(L)] [offset(H)]
#define EMU_INTERFACE_CMD_WRITE_DRIVE_SECTOR 0x12


#define EMU_INTERFACE_RESULT_OK      0x00
#define EMU_INTERFACE_RESULT_IOERROR 0x01



uint8_t emuPrintMessage(uint8_t num);
uint32_t emuGetMemoryCapacity(void);
uint16_t emuGetCPUType(void);
uint8_t emuLogMessage(uint16_t buf_seg, uint16_t buf_offset);
uint8_t emuGetTime(uint16_t *pyear, uint8_t *pmonth, uint8_t *pday, uint8_t *phour, uint8_t *pmin, uint8_t *psec);

uint8_t emuGetDriveSize(uint8_t drive, uint32_t *size);
uint8_t emuReadDriveSector(uint8_t drive, uint32_t sector, uint16_t buf_seg, uint16_t buf_offset);
uint8_t emuWriteDriveSector(uint8_t drive, uint32_t sector, uint16_t buf_seg, uint16_t buf_offset);

#endif
