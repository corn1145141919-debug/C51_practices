#include <REGX52.H>
sfr IPH = 0xb7;
void Timer0_Init()
{
    // 不干扰计时器1
    TMOD = (TMOD & 0xf0) | 0x01;

    // TCON config
    TF0 = 0;
    TR0 = 1;

    TH0 = 255;
    TL0 = 0;

    // 中断寄存器
    ET0 = 1;
    EA = 1;

    // 最低优先级 00
    IPH |= 0x00;
    PX0 = 0;
}
