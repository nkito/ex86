#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include "basicio.h"
#include "system.h"
#include "int10h.h"

#define serial_putc(c)         systemSerialPutc(c)
#define serial_getc()          systemSerialGetc()
#define isUSARTDataAvailable() systemIsUSARTDataAvailable()


void display_putc(char c, unsigned char attr){
	putCharWithAttr(c, 0, attr);
}


int memcmp(const void *s1, const void *s2, size_t n){
    const unsigned char  *p1 = (const unsigned char *)s1;
    const unsigned char  *p2 = (const unsigned char *)s2;

    while (n-- > 0) {
        if (*p1 != *p2)
            return (*p1 - *p2);
        p1++;
        p2++;
    }
    return 0;
}

int s_memcmp(const void *s1, const void *s2, size_t n){
    const unsigned char  *p1 = (const unsigned char *)s1;
    const unsigned char  *p2 = (const unsigned char *)s2;

    while (n-- > 0) {
        if (*p1 != *p2)
            return (*p1 - *p2);
        p1++;
        p2++;
    }
    return 0;
}

char *s_strncpy(char *s1, const char *s2, size_t n){
    char  *p = s1;
    
    while(n > 0){ n--; if (!(*s1++ = *s2++)) break; }
    while(n > 0){ n--; *s1++ = '\0';                }

    return p;
}

char *s_strcpy(char *s1, const char *s2){
	unsigned int i;
	for(i=0; s2[i]!='\0'; i++) s1[i] = s2[i];
	s1[i] ='\0';

	return s1;
}

int s_strcmp(const char *s1, const char *s2){
	for(; *s1==*s2; s1++,s2++){
		if(*s1=='\0') return 0;
	}
	return *s1 - *s2;
}

int s_strncmp(const char *s1, const char *s2, size_t n){
	for(n--; *s1==*s2; s1++,s2++,n--){
		if(*s1=='\0') return 0;
		if(n==0) return 0;
	}
	return *s1 - *s2;
}

size_t s_strlen(const char *s){
	size_t i=0;
	while(*s++ != '\0') i++;
	return i;
}

char *s_strstr(const char *s1, const char *s2){
	int len;
	
	if(s2==NULL || (len=s_strlen(s2))==0) return ((char *)s1);
	
	for( ; *s1 !='\0'; s1++){
		if( ! s_strncmp(s1,s2,len) ) return ((char *)s1);
	}
	return NULL;
}

char *s_strrchr(const char *s, int c){
	char *str= (char *)s, *res=NULL;
	for( ;*str != '\0'; str++){
		if(*str == c) res=str;
	}
	
	return res;
}

char *s_strchr(const char *s, int c){
	char *str= (char *)s;
	for( ;*str != '\0'; str++){
		if(*str == c) return str;
	}
	
	return NULL;
}

void *memset(void *buf, int ch, size_t n){
	const unsigned char uc = ch;
    unsigned char       *p = (unsigned char *)buf;

    while (n-- > 0) *p++ = uc;

    return buf;
}


void *s_memset(void *buf, int ch, size_t n){
	const unsigned char uc = ch;
    unsigned char       *p = (unsigned char *)buf;

    while (n-- > 0) *p++ = uc;

    return buf;
}

void *memcpy(void *buf1, const void *buf2, size_t n){
	return s_memcpy(buf1, buf2, n);
}

void *s_memcpy(void *buf1, const void *buf2, size_t n){
    char        *p1 = (char *)buf1;
    const char  *p2 = (const char *)buf2;

    while (n-- > 0) *p1++ = *p2++;

    return buf1;
}


struct output_mem{
	char *buf;
	unsigned char attr;
	int  size;
	int  pos;
};

void output_putc(struct output_mem *out, char c){
	if(out->buf == 0){
		if( out->attr == 0 ){
			serial_putc(c);
		}else{
			display_putc(c, out->attr);
		}
		return ;
	}
	
	if( out->size < 0 || out->pos < (out->size-1)){
		out->buf[out->pos++] = c;
	}
	out->buf[out->pos] = '\0';
}

void output_string(struct output_mem *out, char *str){
	if(out->buf == 0){
		while(*str != '\0'){
			if( out->attr == 0 ){
				serial_putc(*str++);
			}else{
				display_putc(*str++, out->attr);
			}
		}

		return ;
	}
	
	out->buf[out->pos] = '\0';

	if(out->size >= 0){
		while(*str != '\0' && out->pos < (out->size-1)){
			out->buf[out->pos++] = *str++;
		}
		out->buf[out->pos] = '\0';
	}else{
		while(*str != '\0'){
			out->buf[out->pos++] = *str++;
		}
		out->buf[out->pos] = '\0';
	}
}

int scan_hex(char *str){
	int num = 0;
	
	for( ; *str == ' ' || *str == '\n' || *str == '\t' ; str++ ) ;
	
	for( ; (*str >= '0' && *str <= '9') || (*str >= 'a' && *str <= 'f') || (*str >= 'A' && *str <= 'F'); str++ ){
		num = num * 16;
		if( *str >= '0' && *str <= '9' ) num += (*str - '0');
		if( *str >= 'a' && *str <= 'f' ) num += (*str - 'a' + 0xa);
		if( *str >= 'A' && *str <= 'F' ) num += (*str - 'A' + 0xa);
	}
	
	return num;
}

int scan_num(char *str){
	int num = 0;
	
	for( ; *str == ' ' || *str == '\n' || *str == '\t' ; str++ ) ;
	
	for( ; *str >= '0' && *str <= '9'; str++ ){
		num = num * 10 + (*str - '0');
	}
	
	return num;
}

#define ABS(x)	(((x) > 0) ? (x) : -(x))
void print_unum_fill(struct output_mem *out, unsigned long num, unsigned int nw, unsigned int zerofill);

/*
 * num : a number to be printed
 * nw  : the minimum width of the number (to implement a syntax like %10d of printf, it is 10 if %10d is specified)
 * zerofill: zero-fill for the minimum width? 0 means no zero-fill, otherwise zero-fill (it is an arbitrary number other than 0 if %010d is specified)
 */
void print_num_fill(struct output_mem *out, long num, unsigned int nw, unsigned int zerofill){
	long i,n;	// they should NOT be unsigned !!!!
	long t;
	
	if(num != 0 ){
		for(i=1,n=1,t=num/10; t != 0; i*=10,n++,t/=10) ;
	}else{
		i=1; n=1;
	}

	if(num<0) output_putc(out, '-');
	for( ; n < nw; n++) output_putc(out, zerofill ? '0' : ' ');
	for( ; i != 0; i/=10){
		output_putc(out, '0'+ABS(num/i));
		num -= (num/i)*i;
	}
}

void print_unum_fill(struct output_mem *out, unsigned long num, unsigned int nw, unsigned int zerofill){
	unsigned long i,n;
	unsigned long t;
	
	if(num != 0 ){
		for(i=1,n=1,t=num/10; t != 0; i*=10,n++,t/=10) ;
	}else{
		i=1; n=1;
	}

	for( ; n < nw; n++) output_putc(out, zerofill ? '0' : ' ');
	for( ; i != 0; i/=10){
		output_putc(out, '0'+(num/i));
		num -= (num/i)*i;
	}
}

void print_hex_fill(struct output_mem *out, unsigned long num, unsigned int nw, unsigned int zerofill){
	unsigned long i,n;
	unsigned long t;

	if(num != 0 ){
		for(i=1,n=1,t=num>>4; t != 0; i=i<<4,n++,t=t>>4) ;
	}else{
		i=1; n=1;
	}

	for( ; n < nw; n++) output_putc(out, zerofill ? '0' : ' ');
	for( ; i != 0; i=i>>4){
		t = num/i;
		output_putc(out, "0123456789abcdef"[t&0xf] );
		num -= t*i;
	}
}

static void out_vprint(struct output_mem *out, char *format, va_list ap){
	long            i;
	unsigned long  ui;
	char c;
	char *s;
	unsigned int zerofill, len;

	for( ; *format!='\0'; format++){
		zerofill = len = 0;
				
		if(*format == '%'){
			++format;
			if('0'==*format){ zerofill = 1; format++; }
			for( ; *format >= '0' && *format <= '9'; format++){
				len = len*10 + (*format-'0');
			}
			if(*format == 'l'){
				switch(*++format){
					case 'd': i = va_arg(ap,          long); print_num_fill (out,  i, len, zerofill); break;
					case 'u': ui= va_arg(ap, unsigned long); print_unum_fill(out, ui, len, zerofill); break;
					case 'x': ui= va_arg(ap,          long); print_hex_fill (out, ui, len, zerofill); break;
					default: break;
				}
				continue;
			}
			
			switch(*format){
				case 'd': i = va_arg(ap,          int); print_num_fill (out,  i, len, zerofill); break;
				case 'u': ui= va_arg(ap, unsigned int); print_unum_fill(out, ui, len, zerofill); break;
				case 'x': ui= va_arg(ap, unsigned int); print_hex_fill (out, ui, len, zerofill); break;
				case 'c': c = va_arg(ap,          int); output_putc(out, c); break;
				case '%': output_putc(out, '%'); break;
				case 's': s = va_arg(ap, char *);
						ui=s_strlen(s);
						for(i=ui;i<len;i++) output_putc(out, ' ');
						output_string(out, s);
						break;
				default: break;
				
			}
		}else if(*format == '\n'){
			output_string(out, "\r\n");
		}else{
			output_putc(out, *format);
			if(*format >= 0x80){
				if(*++format != '\0'){
					output_putc(out, *format);
				}else{
					break;
				}
			}
		}
	}
}

void disp_printf(unsigned char attr, char *format, ...){
	struct output_mem out;
	va_list ap;
	
	out.buf = 0;
	out.attr= attr;
	out.pos = 0;
	out.size= -1;
	
	va_start(ap, format); 
	out_vprint(&out, format, ap);
	va_end(ap);
}

void s_printf(char *format, ...){
	struct output_mem out;
	va_list ap;
	
	out.buf = 0;
	out.attr= 0;
	out.pos = 0;
	out.size= -1;
	
	va_start(ap, format); 
	out_vprint(&out, format, ap);
	va_end(ap);
}
void s_sprintf(char *str, char *format, ...){
	struct output_mem out;
	va_list ap;
	
	out.buf = str;
	out.attr= 0;
	out.pos = 0;
	out.size= -1;
	
	va_start(ap, format); 
	out_vprint(&out, format, ap);
	va_end(ap);
}
void s_snprintf(char *str, int size, char *format, ...){
	struct output_mem out;
	va_list ap;
	
	out.buf = str;
	out.attr= 0;
	out.pos = 0;
	out.size= size;
	
	va_start(ap, format); 
	out_vprint(&out, format, ap);
	va_end(ap);
}


