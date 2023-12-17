#include "terminal.h"
#include "basicio.h"

void termSetCharColor(enum terminalCharColor eCharColor){
	if( IS_TERM_RGB_COLOR(eCharColor) ){
		s_printf("\033[38;2;%d;%d;%dm", TERM_COLOR_R(eCharColor), TERM_COLOR_G(eCharColor), TERM_COLOR_B(eCharColor));
	}else{
		s_printf("\033[%dm", eCharColor);
	}
}
void termSetBGColor(enum terminalBGColor eBGColor){
	if( IS_TERM_RGB_COLOR(eBGColor) ){
		s_printf("\033[48;2;%d;%d;%dm", TERM_COLOR_R(eBGColor), TERM_COLOR_G(eBGColor), TERM_COLOR_B(eBGColor));
	}else{
		s_printf("\033[%dm", eBGColor);
	}
}

void termResetColor(void){
	s_printf("\033[39m");
	s_printf("\033[49m");
}

void termSetBlinkOff(void){
	s_printf("\033[?25l"); // turn blaning off
}
void termResetBlink(void){
	s_printf("\033[?25h");
}

void termGoTo(int x, int y){
	if(x<=0) x=1;
	if(y<=0) y=1;
	s_printf("\033[%d;%dH", y, x);
}
