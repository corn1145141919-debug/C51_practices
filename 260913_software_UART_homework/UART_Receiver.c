#include <REGX52.H>
#include "digital_tube.h"

// 能让 VS Code 的 IntelliSense 和 Keil 同时正确读取（需要安装Keil Assistant插件）
// 理论上来讲，在把这个软件发布给任何只有Keil软件的用户时，这些源代码都能被正确编译
#ifndef __VSCODE_C51__
#define INTERRUPT(x) interrupt x
#else
#define INTERRUPT(x)
#endif

sfr IPH = 0xb7;
volatile unsigned char UART_received_data_buffer;
volatile unsigned long tube_display_data_buffer;

void UART_Receiver_init(void)
{
    // 相关设定对于软件uart不适用
    // 串口相关寄存器
    // // 模式1
    // SM0 = 0;
    // SM1 = 1;

    // SM2 = 0;
    // // 允许接收
    // REN = 1;

    // TB8 = 0;
    // RB8 = 0;

    // TI = 0;
    // RI = 0;
    // 一个时钟周期内完成上述配置
    // SCON = 0x50;
    // PCON &= 0x7f; // 波特率不加倍？

    EA = 1; // 允许中断
    ES = 0; // 禁止来自原生硬件 UART 的中断

    // 下降沿中断设置范式
    EX0 = 1; // 允许INT0中断；
    IT0 = 1; // 使用下降沿中断

    // 定时器相关寄存器
    // 以下为负责接收的的 Timer2 初始化

    // 定时器允许与清零与模式设定（16位自动重装）
    T2CON = 0x04;
    // TF2 = 0; // 这条代码被上面包含了
    // 注意后续代码中必须手动将TF2清零！

    // 定时器2本身发出(3 * 9600)bps@11.0592MHz
    // 定时器初值（注意每次重设相位的时候都需要该操作）
    TH2 = 0xff;
    TL2 = 0xc0;
    // 定时器重装值
    RCAP2H = 0xff;
    RCAP2L = 0xc0;
    
    // 下降沿和定时器2中断同时设定为最高优先级 11
    IPH &= 0x21;
    PT2 = 1;
    PX0 = 1;

    UART_received_data_buffer = 0x00;
    tube_display_data_buffer = 0x00000000;
}

void UART_Receiver_ISR() INTERRUPT(5) // 不是计时器中断而是串口中断
{
    // 定时器初值（注意每次重设相位的时候都需要该操作）
    TH2 = 0xff;
    TL2 = 0xc0;

    if (RI == 1)
    {
        // 由于接收和输入共用中断，接收后的ISR必须放在这里
        UART_received_data_buffer = SBUF;
        // 为了精确显示累积的数据，必须在ISR中执行该操作。这可能会破坏封装性，但是这是小项目无所谓。
        tube_display_data_buffer = (tube_display_data_buffer << 8) + UART_received_data_buffer;
        
        // 手动复位
        RI = 0;
    }

    TF2 = 0; // 必须在软件层面上将该标志位清零
}
