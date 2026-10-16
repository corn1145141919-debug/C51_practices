#include <REGX52.H>

#define GPIO_Key P1
#include "delay.h" // 小项目不使用多个计时器

unsigned char map(unsigned char input)
{
    switch (input)
    {
    case 0x08:
        return 3;
    case 0x04:
        return 2;
    case 0x02:
        return 1;
    case 0x01:
        return 0;
    default:
        return 0;
    }
}

// 暂时无法处理任何并发情况——并发可能会出错
unsigned char MatrixButton_getKey()
{
    // unsigned char column, row;
    unsigned char key = 0x00;
    // 先固定在行扫描

    GPIO_Key = 0x0f;
    while (1)
    {
        if (GPIO_Key != 0x0f)
        {
            delay(20);
            if (GPIO_Key != 0x0f)
            {
                // column = map((~GPIO_Key) & 0x0f);
                key += map((~GPIO_Key) & 0x0f);

                GPIO_Key = 0xf0;
                // 认为仍然处于按下状态——消抖已经保证此处不会因跳变而读到未按键的信号；人也不至于在按下20ms外加几个时钟周期后立即松开
                // row = map(((~GPIO_Key) & 0xf0)) * 4;
                key += (map(((~GPIO_Key) & 0xf0) >> 4) << 2);

                while (1)
                {
                    if (GPIO_Key == 0xf0)
                    {
                        delay(20);
                        if (GPIO_Key == 0xf0)
                        {
                            // return column + row << 2;
                            return key;
                        }
                    }
                }
            }
        }
    }
}
