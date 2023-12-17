#ifndef _BASICIO_H_
#define _BASICIO_H_

#include <stddef.h>

int    s_memcmp(const void *s1, const void *s2, size_t n);
char  *s_strncpy(char *s1, const char *s2, size_t n);
char  *s_strcpy(char *s1, const char *s2);
int    s_strcmp(const char *s1, const char *s2);
int    s_strncmp(const char *s1, const char *s2, size_t n);
size_t s_strlen(const char *s);
char  *s_strstr(const char *s1, const char *s2);
char  *s_strrchr(const char *s, int c);
char  *s_strchr(const char *s, int c);
void  *s_memset(void *buf, int ch, size_t n);
void  *s_memcpy(void *buf1, const void *buf2, size_t n);

void disp_printf(unsigned char attr, char *format, ...);

void  s_printf(char *format, ... );
void  s_sprintf(char *str, char *format, ...);
void  s_snprintf(char *str, int size, char *format, ...);

#endif    /* of #ifndef _BASICIO_H_ */
