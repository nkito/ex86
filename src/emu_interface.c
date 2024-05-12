#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>

#include "i8086.h"
#include "mem.h"
#include "logfile.h"
#include "terminal.h"

#include "emu_interface.h"


char * imageFileName[3] = {
    "driveA.img",
    "driveB.img",
    "driveC.img"
};



void callEmuInterfacePort(struct stMachineState *pM, uint16_t data){
    uint32_t sector, addr;
    uint32_t ea;
    uint16_t seg, oft;
    uint8_t  emu_cmd;
    int i;
    uint8_t buf[512];


    ea = pM->reg.ss;
    ea = (ea<<4) + ((uint32_t)data);

    emu_cmd = readDataMemByte(pM, ea+1);

    if( readDataMemByte(pM, ea) != EMU_INTERFACE_MAGIC_NUM ){
        return;
    }

    if( emu_cmd == EMU_INTERFACE_CMD_PRINT_MSG ){
        PRINTF(" <message from the EMULATOR: \"%d\" is received > \n", readDataMemByte(pM, ea+3));
        writeDataMemByte(pM, ea+2, EMU_INTERFACE_RESULT_OK);
        return;
    }

    if( emu_cmd == EMU_INTERFACE_CMD_LOG_MESSAGE ){
        seg = readDataMemByte(pM, ea+4); seg <<= 8;
        seg|= readDataMemByte(pM, ea+3);

        oft = readDataMemByte(pM, ea+6); oft <<= 8;
        oft|= readDataMemByte(pM, ea+5);

        addr = (((uint32_t)seg)<<4) + ((uint32_t)oft);

        for(i=0; i<sizeof(buf)-1; i++){
            buf[i+1] = '\0';
            buf[i] = readDataMemByte(pM, addr + i);
            if(buf[i] == '\0') break;
        }
        logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "%s", buf);
        writeDataMemByte(pM, ea+2, EMU_INTERFACE_RESULT_OK);
        return;
    }

    if( emu_cmd == EMU_INTERFACE_CMD_GET_TIME ){
        time_t  t;
        struct tm *tmp;

        time(&t);
        tmp = localtime(&t);

        writeDataMemByte(pM, ea+3, tmp->tm_year );
        writeDataMemByte(pM, ea+4, tmp->tm_mon + 1 );
        writeDataMemByte(pM, ea+5, tmp->tm_mday );
        writeDataMemByte(pM, ea+6, tmp->tm_hour );
        writeDataMemByte(pM, ea+7, tmp->tm_min );
        writeDataMemByte(pM, ea+8, tmp->tm_sec );

        writeDataMemByte(pM, ea+2, EMU_INTERFACE_RESULT_OK);

        return;
    }

    if( emu_cmd == EMU_INTERFACE_CMD_GET_MEM_CAPACITY ){

        writeDataMemByte(pM, ea+2, EMU_INTERFACE_RESULT_OK);

        uint32_t memsize = EMU_MEM_SIZE;
        if( pM->pEmu->emu_cpu < EMU_CPU_80286 && memsize >= 1024*1024 ){
            memsize = 1024*1024;
        }

        writeDataMemByte(pM, ea+3, ((memsize>> 0)&0xff));
        writeDataMemByte(pM, ea+4, ((memsize>> 8)&0xff));
        writeDataMemByte(pM, ea+5, ((memsize>>16)&0xff));
        writeDataMemByte(pM, ea+6, ((memsize>>24)&0xff));

        return;
    }

    if( emu_cmd == EMU_INTERFACE_CMD_GET_CPU_TYPE ){
        writeDataMemByte(pM, ea+2, EMU_INTERFACE_RESULT_OK);

        uint16_t cpu = 86;
        if( pM->pEmu->emu_cpu == EMU_CPU_80186 ) cpu = 186;
        if( pM->pEmu->emu_cpu == EMU_CPU_80286 ) cpu = 286;
        if( pM->pEmu->emu_cpu == EMU_CPU_80386 ) cpu = 386;

        writeDataMemByte(pM, ea+3, ((cpu>> 0)&0xff));
        writeDataMemByte(pM, ea+4, ((cpu>> 8)&0xff));

        return;
    }

    if( emu_cmd == EMU_INTERFACE_CMD_GET_DRIVE_SIZE ){
        struct stat statBuf;
        uint8_t drive = (readDataMemByte(pM, ea+3) & 3);

        if (stat(imageFileName[drive], &statBuf) == 0){
            writeDataMemByte(pM, ea+4, ((statBuf.st_size>> 0)&0xff));
            writeDataMemByte(pM, ea+5, ((statBuf.st_size>> 8)&0xff));
            writeDataMemByte(pM, ea+6, ((statBuf.st_size>>16)&0xff));
            writeDataMemByte(pM, ea+7, ((statBuf.st_size>>24)&0xff));

            writeDataMemByte(pM, ea+2, EMU_INTERFACE_RESULT_OK);
        }else{
            writeDataMemByte(pM, ea+2, EMU_INTERFACE_RESULT_IOERROR);
        }

        return;
    }

    if( emu_cmd == EMU_INTERFACE_CMD_READ_HOST_FILE ){
        FILE *fp;
        uint16_t boft, num;
        size_t count;

        seg = readDataMemByte(pM, ea+4); seg <<= 8;
        seg|= readDataMemByte(pM, ea+3);

        oft = readDataMemByte(pM, ea+6); oft <<= 8;
        oft|= readDataMemByte(pM, ea+5);

        boft = readDataMemByte(pM, ea+8); boft <<= 8;
        boft|= readDataMemByte(pM, ea+7);

        num = readDataMemByte(pM, ea+10); num <<= 8;
        num|= readDataMemByte(pM, ea+9);

        addr = (((uint32_t)seg)<<4) + ((uint32_t)oft);

        for(i=0; (buf[i]=readDataMemByte(pM, addr+i)) != '\0' && i+1<sizeof(buf); i++) ;
        buf[i] = '\0';

        logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "EMULATOR: opening host file \"%s\" for reading, block#: %d, buffer: 0x%x:0x%x\n", buf, num, seg, boft);

        if( NULL == (fp = fopen((char *)buf, "rb")) ) goto readFileError;

        if( 0 != fseek(fp, num*512, SEEK_SET) ){
            fclose(fp); goto readFileError;
        }
        count = fread(buf, 1, 512, fp);
        fclose(fp);

        addr = (((uint32_t)seg)<<4) + ((uint32_t)boft);
        for(i = 0; i < count; i++){
            writeDataMemByte(pM, addr+i, buf[i]);
        }
        writeDataMemByte(pM, ea+ 9, (count   )&0xff);
        writeDataMemByte(pM, ea+10, (count>>8)&0xff);

        writeDataMemByte(pM, ea+2, EMU_INTERFACE_RESULT_OK);
        return;

        readFileError:
        logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "EMULATOR: opening host file \"%s\" was failed\n", buf);
        writeDataMemByte(pM, ea+2, EMU_INTERFACE_RESULT_IOERROR);
        return;
    }

    if( emu_cmd == EMU_INTERFACE_CMD_WRITE_HOST_FILE ){
        FILE *fp = NULL;
        uint8_t nbyte, option;
        uint16_t boft;
        size_t count;

        seg = readDataMemByte(pM, ea+4); seg <<= 8;
        seg|= readDataMemByte(pM, ea+3);

        oft = readDataMemByte(pM, ea+6); oft <<= 8;
        oft|= readDataMemByte(pM, ea+5);

        boft = readDataMemByte(pM, ea+8); boft <<= 8;
        boft|= readDataMemByte(pM, ea+7);

        nbyte  = readDataMemByte(pM, ea+9);
        option = readDataMemByte(pM, ea+10);

        addr = (((uint32_t)seg)<<4) + ((uint32_t)oft);

        for(i=0; (buf[i]=readDataMemByte(pM, addr+i)) != '\0' && i+1<sizeof(buf); i++) ;
        buf[i] = '\0';

        logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "EMULATOR: opening host file \"%s\" for writing, nbyte: %d, option: %x, buffer: 0x%x:0x%x\n", buf, nbyte, option, seg, boft);

        if( option == EMU_INTERFACE_CMD_WRITE_HOST_FILE_OPT_OVERWRITE ){
            if( NULL == (fp = fopen((char *)buf, "rb+")) ) goto writeFileError;
        }else if( option == EMU_INTERFACE_CMD_WRITE_HOST_FILE_OPT_APPEND ) {
            if( NULL == (fp = fopen((char *)buf, "ab")) ) goto writeFileError;
        }else if( option == EMU_INTERFACE_CMD_WRITE_HOST_FILE_OPT_CREATE ){
            if( NULL == (fp = fopen((char *)buf, "wb")) ) goto writeFileError;
        }


        addr = (((uint32_t)seg)<<4) + ((uint32_t)boft);
        for(i = 0; i < nbyte; i++){
            buf[i] = readDataMemByte(pM, addr+i);
        }
        count = fwrite(buf, 1, nbyte, fp);
        fclose(fp);

        if( (count&0xff) != nbyte ) goto writeFileError;

        writeDataMemByte(pM, ea+9, count&0xff );
        writeDataMemByte(pM, ea+2, EMU_INTERFACE_RESULT_OK);
        return;

        writeFileError:
        logfile_printf(LOGCAT_EMU | LOGLV_NOTICE, "EMULATOR: opening host file \"%s\" was failed\n", buf);
        writeDataMemByte(pM, ea+2, EMU_INTERFACE_RESULT_IOERROR);
        return;
    }

    if( emu_cmd == EMU_INTERFACE_CMD_READ_DRIVE_SECTOR || emu_cmd == EMU_INTERFACE_CMD_WRITE_DRIVE_SECTOR ){
        FILE *fp;
        uint8_t drive = (readDataMemByte(pM, ea+3) & 3);
        size_t count;


        sector = readDataMemByte(pM, ea+7); sector <<= 8;
        sector|= readDataMemByte(pM, ea+6); sector <<= 8;
        sector|= readDataMemByte(pM, ea+5); sector <<= 8;
        sector|= readDataMemByte(pM, ea+4);

        seg = readDataMemByte(pM, ea+9); seg <<= 8;
        seg|= readDataMemByte(pM, ea+8);

        oft = readDataMemByte(pM, ea+11); oft <<= 8;
        oft|= readDataMemByte(pM, ea+10);

        addr = (((uint32_t)seg)<<4) + ((uint32_t)oft);

 //           printf("<EMULATOR:READ_DRIVE_SECTOR: drive %d sector %d addr 0x%x [0x%x:0x%x]>\n", drive, sector, addr, seg,oft);

        termResetColor(); termSetBlinkOff();
        termGoTo(1, 27); PRINTF("[%c %c %c]", drive==0 ? '*' : ' ', drive==1 ? '*' : ' ', drive==2 ? '*' : ' '); FLUSH_STDOUT();

        fp = fopen(imageFileName[drive], 
            emu_cmd == EMU_INTERFACE_CMD_WRITE_DRIVE_SECTOR ? "rb+" : "rb");

        if( fp == NULL ) goto readSectorIOerror;
        if( 0 != fseek(fp, ((unsigned long)sector)*512, SEEK_SET) ){
            fclose(fp); goto readSectorIOerror;
        }
        if( emu_cmd == EMU_INTERFACE_CMD_WRITE_DRIVE_SECTOR ){
            for(count = 0; count < 512; count++){
                buf[count] = readDataMemByte(pM, addr+count);
            }
            count = fwrite(buf, 1, 512, fp);
        }else{
            count = fread(buf, 1, 512, fp);
        }
        fclose(fp);

        termGoTo(1, 27); PRINTF("[     ]"); termResetBlink(); FLUSH_STDOUT();

        if( count == 512 ){
            if( emu_cmd == EMU_INTERFACE_CMD_READ_DRIVE_SECTOR ){
                for(count = 0; count < 512; count++){
                    writeDataMemByte(pM, addr+count, buf[count]);
                }
            }
            writeDataMemByte(pM, ea+2, EMU_INTERFACE_RESULT_OK);
            return;
        }

        readSectorIOerror:
        logfile_printf(LOGCAT_EMU | LOGLV_ERROR, "EMULATOR:READ_DRIVE_SECTOR:I/O ERROR: drive %d sector %d addr 0x%x [0x%x:0x%x]\n", drive, sector, addr, seg,oft);
        writeDataMemByte(pM, ea+2, EMU_INTERFACE_RESULT_IOERROR);
        return;
    }
}