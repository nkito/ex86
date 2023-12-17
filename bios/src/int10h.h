#ifndef __INT10H_H__
#define __INT10H_H__

void putCharWithAttr(char c, unsigned char page, unsigned char attr);

int  int10_updateAll(unsigned int seg, unsigned int offset);
void set_cursorPosition(unsigned char page, unsigned char x, unsigned char y);

#endif
