#include <REGX52.H>

// 能让 VS Code 的 IntelliSense 和 Keil 同时正确读取（需要安装Keil Assistant插件）
// 理论上来讲，在把这个软件发布给任何只有Keil软件的用户时，这些源代码都能被正确编译
#ifndef __VSCODE_C51__
#define INTERRUPT(x) interrupt x
#else
#define INTERRUPT(x)
#endif

void UART_Transmitter_init()
{
    // 起个别名，该位不仅控制timer1能否被触发，而且语义上控制“发送端是否正忙”（对应流程图中的状态机模型）
    sbit tx_busy = 0xAB; 
    
    EA = 1; // 允许中断
    ES = 0; // 禁止来自原生硬件 UART 的中断

    // 定时器相关寄存器
    // 以下为负责发送的 Timer1 初始化
    TMOD = (TMOD & 0x0f) | 0x20;

    TF1 = 0; // 定时器清零
    TR1 = 1; // 允许定时器运行

    // 定时器1本身发出9600bps@11.0592MHz
    TL1 = 0x40; // 定时器初值
    TH1 = 0x40; // 定时器重装值
    
    tx_busy = 0; // 默认没有发送；这在硬件上是中断寄存器设置

    // 优先级还没有设置
    // 倍速模式如何设置？
}
void UART_Transmitter_SendByte(unsigned char byte)
{}

void UART_Transmitter_SendByte_Real() INTERRUPT(3)
{}