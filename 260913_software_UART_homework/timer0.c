#include <REGX52.H>
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

    // 低优先级
    PX0 = 0;
}
