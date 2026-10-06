#include <REGX52.H>

// 能让 VS Code 的 IntelliSense 和 Keil 同时正确读取（需要安装Keil Assistant插件）
// 理论上来讲，在把这个软件发布给任何只有Keil软件的用户时，这些源代码都能被正确编译
#ifndef __VSCODE_C51__
#define INTERRUPT(x) interrupt x
#else
#define INTERRUPT(x)
#endif

volatile unsigned char UART_data_to_be_transmitted;
// 起个别名，该位不仅控制timer1能否被触发，而且语义上控制“发送端是否正忙”（对应流程图中的状态机模型）
sbit tx_busy = 0xAB; 

void UART_Transmitter_init()
{   
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
{
    if (!tx_busy) // 如果正在发送那么就拒绝发送传来的byte，而是直接丢弃
    {
        UART_data_to_be_transmitted = byte;
        tx_busy = 1;
    }
}

void UART_Transmitter_SendByte_Real() INTERRUPT(3)
{
    // 8位模式下从首校验位到尾校验位是0到9，1到8是数据位
    static unsigned char curr_bit = 0; // 先初始化为首位
    static unsigned char mask = 0x80; // 初始状态

    if (curr_bit == 0)
    {
        P2_0 = 0; // 拉低
    }
    else if (1 <= curr_bit && curr_bit <= 8)
    {
        P2_0 = mask & UART_data_to_be_transmitted; // 非0则赋值为1；这种用法在官方示例中也出现了，是可以依赖的
        mask >>= 1; // 这一条在curr_bit == 8的时候也会执行；这样做只是为了减少条件判断
    }
    else if (curr_bit == 9)
    {
        P2_0 = 1;
        tx_busy = 0; // 恢复等待状态
        mask = 0x80; // 恢复初始掩码
    }
    curr_bit = (curr_bit + 1) % 10; // 自然地将圈子兜回来
}