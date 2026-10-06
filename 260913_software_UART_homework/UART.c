#include <REGX52.H>
#include "digital_tube.h"

// 能让 VS Code 的 IntelliSense 和 Keil 同时正确读取（需要安装Keil Assistant插件）
// 理论上来讲，在把这个软件发布给任何只有Keil软件的用户时，这些源代码都能被正确编译
#ifndef __VSCODE_C51__
#define INTERRUPT(x) interrupt x
#else
#define INTERRUPT(x)
#endif

volatile unsigned char UART_received_data_buffer;
volatile unsigned long tube_display_data_buffer;

void UART_init(void)
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

    // 定时器相关寄存器
    // 以下为负责接收的的 Timer2 初始化

    // 定时器清零
    // 允许定时器运行

    // 定时器2本身发出9600bps@11.0592MHz
    // 定时器初值
    // 定时器重装值
    
    // 默认没有；这在硬件上是中断寄存器设置
    
    // 中断优先级：串口优先级为3
    // PX0H = 1; // 不可位寻址 // 可能STC89C52RC没有 IPH 寄存器
    // 高优先级
    // PS = 1;

    UART_received_data_buffer = 0x00;
    tube_display_data_buffer = 0x00000000;
}

void UART_ISR() INTERRUPT(4) // 不是计时器中断而是串口中断
{
    // 手动复位
    // TI的处理全权由 SendByte() 负责
    // if (TI == 1)
    // {
    //     TI = 0;
    // }

    if (RI == 1)
    {
        // 由于接收和输入共用中断，接收后的ISR必须放在这里
        UART_received_data_buffer = SBUF;
        // 为了精确显示累积的数据，必须在ISR中执行该操作。这可能会破坏封装性，但是这是小项目无所谓。
        tube_display_data_buffer = (tube_display_data_buffer << 8) + UART_received_data_buffer;
        
        // 手动复位
        RI = 0;
    }
}
