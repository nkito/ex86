#ifndef __ASMFUNCS_H__
#define __ASMFUNCS_H__

/*
Those functions are defined in crt_emu.s
*/ 
extern void copy_data      (unsigned char *src_addr, unsigned int dest_seg, unsigned char *dest_addr, unsigned int len);
extern void copy_data_word (unsigned char *src_addr, unsigned int dest_seg, unsigned char *dest_addr, unsigned int nwords);
extern void fetch_data     (unsigned int  src_seg, unsigned char *src_addr, unsigned char *dest_addr, unsigned int len);
extern void fetch_data_word(unsigned int  src_seg, unsigned char *src_addr, unsigned char *dest_addr, unsigned int nwords);

#endif
