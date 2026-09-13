#include <REGX52.H>
#include <math.h>

// private:
#include "timer0.h"
#define BIT_SELECTION_REG P2
#define SEG_SELECTION_REG P0

const char table[] = {
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


// 利用 IEEE 754 的 32 位浮点数的特性，检测该浮点数是否为无效的
unsigned char is_FloatInvalid(float input)
{
    unsigned char *p = (unsigned char*)&input;

    if (((p[0] & 0x7f) == 0x7f)
        && ((p[1] & 0x80) == 0x80))
    {
        return 1;
    }
    return 0;
}

// buffer 应该直接储存显示段选信息！这样才支持自定义的灵活修改（比如显示o和r，或者添加小数点）
static unsigned char buffer[8] = {blank, blank, blank, blank, blank, blank, blank, blank, };

void bit_selection(unsigned char number)
{
    BIT_SELECTION_REG = (BIT_SELECTION_REG & 0xe3) | ((number & 0x07) << 2);
}

// 由于默认每个数码管位的buffer都是在display“重载”函数中整个修改的，所以不用添加响应的erase_decimal_point().
// 按位或的特性决定了重复添加后仍会有小数点
void add_decimal_point(unsigned char bit_target)
{
    buffer[bit_target] |= 0x80;
}

// 核心机制。不能暴露对外接口
void DigitalTube_Refresh_ISR(void) interrupt 1 // 写完记得恢复
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
void DigitalTube_Display_decimal_int(int input)
{
    unsigned char i;
    unsigned char bits_used = 8;
    if (input < 0)
    {
        bits_used = 7;
        buffer[7] = 0x40; // "-"
        // 取余运算，结果符号被规定为与被除数相同，因此下面的算法在 input 为负数时无法正常运行。
        // 这里只用一步确定性的代码来解决问题（而不是在每一处取余处做处理——那样还要叠加状态记录等等）
        input = -input;
    }

    // 从低到高
    for (i = 0; i < bits_used; ++i)
    {
        buffer[i] = table[input % 10]; // 下面的结构保证了传入数据是0的时候仍能显示

        input /= 10;
        if (input == 0)
        {
            i++; // 必须保证本来该有数字的一位不被侵占！
            break;
        }
        // 这样更新应该能使0正常显示的同时其他数字只有高位正常显示
    }
    // 自然地把剩余位置为blank 我感觉应该不会涉及状态的污染之类
    for (; i < bits_used; ++i)
    {
        buffer[i] = table[blank];
    }
}

// 为了这个项目加的
void DigitalTube_Dispay_hex_unsigned_char(unsigned char input)
{
    buffer[1] = table[(input >> 4)];
    buffer[0] = table[(input & 0x0f)];
    buffer[2] = table[blank];
    buffer[3] = table[blank];
    buffer[4] = table[blank];
    buffer[5] = table[blank];
    buffer[6] = table[blank];
    buffer[7] = table[blank];
}

void DigitalTube_Display_hex_unsigned_long(unsigned long input)
{
    unsigned char i = 0;
    // 破坏数据
    for (; i < 8; i++, input >>= 4)
    {
        buffer[i] = table[input & 0x0000000f];
    }
}

// TODO

void DigitalTube_Display_float(float input)
{
    // available_tube_bit = highest - lowest + 1
    unsigned char highest_tube_bit = 7;
    unsigned char lowest_tube_bit = 0;

    if (is_FloatInvalid(input))
    {
        DigitalTube_Display_Error();
        return;
    }

    if (input < 0 || input == -0.0)
    {
        highest_tube_bit = 6;
        buffer[7] = 0x40; // "-"
        input = -input;
    }
}

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
void DigitalTube_Display_Overflow(void)
{
    buffer[7] = table[0];
    buffer[6] = 0x3e; // U
    buffer[5] = table[E];
    buffer[4] = 0x50; // r
    buffer[3] = table[F];
    buffer[2] = 0x38; // L
    buffer[1] = table[0];
    buffer[0] = table[3];
}
