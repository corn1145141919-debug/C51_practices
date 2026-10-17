#include <REGX52.H>
#include <math.h>

#include "display_formatter.h"

// private:
#include "timer0.h"
#define BIT_SELECTION_REG P2
#define SEG_SELECTION_REG P0

// 能让 VS Code 的 IntelliSense 和 Keil 同时正确读取（需要安装Keil Assistant插件）
// 理论上来讲，在把这个软件发布给任何只有Keil软件的用户时，这些源代码都能被正确编译
#ifndef __VSCODE_C51__
#define INTERRUPT(x) interrupt x 
#else
#define INTERRUPT(x)
#endif

const unsigned char code table[] = {
    0x3f,
    0x06,
    0x5b,
    0x4f,
    0x66,
    0x6d,
    0x7d,
    0x07,
    0x7f,
    0x6f,
    0x77,
    0x7c,
    0x39,
    0x5e,
    0x79, // 14 uppercase E
    0x71,
    0x00, // 16 blank

    // 不应该额外自定义码表
    // for "Error"
    // 0x50, // 17 lowercase r
    // 0x5c, // 18 lowercase o
};

enum Letter
{
    A = 10,
    b,
    C,
    d,
    E,
    F,
    blank//, // 早期标准不允许enum尾部添加逗号
    // r,
    // o//, // 不允许尾部添加逗号
};


// buffer 应该直接储存显示段选信息！这样才支持自定义的灵活修改（比如显示o和r，或者添加小数点）
static unsigned char buffer[8] = {0}; // 直接储存段选信息：改对了（UART项目的那边暂时不改以免出错）

void bit_selection(unsigned char number)
{
    BIT_SELECTION_REG = (BIT_SELECTION_REG & 0xe3) | ((number & 0x07) << 2);
}

// 由于默认每个数码管位的buffer都是在display“重载”函数中整个修改的，所以不用添加响应的erase_decimal_point().
// 按位或的特性决定了重复添加后仍会有小数点

// 核心机制。不能暴露对外接口
void DigitalTube_Refresh_ISR(void) INTERRUPT(1) 
{
    // 旧的C标准只支持在任何可执行语句开头声明变量
    // 但是注意：初始化必须写在下面，必须与普通赋值语句区分开！
    static unsigned char curr_bit = 0; // 只执行一次

    // 在ISR开头手动重装
    TH0 = 255;
    TL0 = 0;

    SEG_SELECTION_REG = 0x00;
    bit_selection(curr_bit);
    SEG_SELECTION_REG = buffer[curr_bit];
    
    if (curr_bit == 7)
    {
        curr_bit = 0;
    }
    else
    {
        curr_bit++;
    }
}

// public:

void DigitalTube_Display_Error(void); // 先进行声明

// 暂时不检测显示溢出，会显示后八位（负数为7位）——反正以后会重构为 convert 任务外包，本类专心处理显示
// temp TODO: 改为只处理传入的八位数字（负数为7位，用ASCII中的'-'当负号）的数组，打包成的任务外包为单独一类 converter
// 实际应该使用32位有符号整数；这对于数码管显示足够了，因此传过来的input我们暂时认为不是溢出结果。于是我们不做特殊的溢出处理。

void DigitalTube_Display_Error(void)
{
    buffer[7] = table[blank];
    buffer[6] = table[blank];
    buffer[5] = table[blank];
    buffer[4] = table[E];
    buffer[3] = 0x50; // r
    buffer[2] = 0x50; // r
    buffer[1] = 0x5c; // o
    buffer[0] = 0x50; // r
}

void DigitalTube_Display_Data(const struct Data4DigitalTube *input)
{
    unsigned char i;
    unsigned char Code; // 不能用涉及宏定义的code

    if (input->is_error)
    {
        DigitalTube_Display_Error();
        return;
    }

    for (i = 0; i < 8; ++i)
    {
        Code = input->digits[i];
        if (Code <= 9)
        {
            buffer[i] = table[Code];
        }
        else if (Code == DISPLAY_MINUS)
        {
            buffer[i] = 0x40;
        }
        else
        {
            buffer[i] = table[blank];
        }

        if (input->decimal_point_mask & (unsigned char)(1u << i))
        {
            buffer[i] |= 0x80;
        }
    }
}
