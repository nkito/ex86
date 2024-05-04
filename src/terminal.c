#include <stdio.h>
#include "terminal.h"

const int fcolor[] = 
    {charColorBlack, charColorBlue, charColorGreen, charColorCyan, charColorRed, charColorMagenta, charColorBrown, charColorWhite,
    charColorGray, charColorBrightBlue, charColorBrightGreen, charColorBrightCyan, charColorBrightRed, charColorBrightMagenta, charColorBrightYellow, charColorBrightWhite };
const int bgcolor[] = 
    {BGColorBlack, BGColorBlue, BGColorGreen, BGColorCyan, BGColorRed, BGColorMagenta, BGColorBrown, BGColorWhite,
    BGColorGray, BGColorBrightBlue, BGColorBrightGreen, BGColorBrightCyan, BGColorBrightRed, BGColorBrightMagenta, BGColorBrightYellow, BGColorBrightWhite };

static int currentCharColor = -1;
static int currentBGColor   = -1;

void termSetCharColor(enum terminalCharColor eCharColor){

    if( currentCharColor == eCharColor ) return;
    currentCharColor = eCharColor;

    if( IS_TERM_RGB_COLOR(eCharColor) ){
        printf("\033[38;2;%d;%d;%dm", TERM_COLOR_R(eCharColor), TERM_COLOR_G(eCharColor), TERM_COLOR_B(eCharColor));
    }else{
        printf("\033[%dm", eCharColor);
    }
}
void termSetBGColor(enum terminalBGColor eBGColor){

    if( currentBGColor == eBGColor ) return;
    currentBGColor = eBGColor;

    if( IS_TERM_RGB_COLOR(eBGColor) ){
        printf("\033[48;2;%d;%d;%dm", TERM_COLOR_R(eBGColor), TERM_COLOR_G(eBGColor), TERM_COLOR_B(eBGColor));
    }else{
        printf("\033[%dm", eBGColor);
    }
}

void termResetColor(void){
    currentCharColor   = -1;
    currentBGColor = -1;

    printf("\033[39m");
    printf("\033[49m");
}

void termSetBlinkOff(void){
    printf("\033[?25l"); // turn blaning off
}
void termResetBlink(void){
    printf("\033[?25h");
}

void termGoTo(int x, int y){
    if(x<=0) x=1;
    if(y<=0) y=1;
    printf("\033[%d;%dH", y, x);
}

void termClear(void){
    printf("\033[2J");
}

void termResetSettingForExit(void){
    // set cursor position
    printf("\033[%d;%dH", 25, 1);

    // reset terminal color
    printf("\033[39m");
    printf("\033[49m");

    // reset blinking
    printf("\033[?25h");
}

