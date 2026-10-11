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
sbit receiver_idle = 0xa8; // 给EX0起别名
sbit myRX = 0xb2; // 接收传到P3_2，这里起个别名并且方便修改

// 由于初始化由interrupt 0实现，我们必须把本来应该封装给interrupt 5的一些变量放在这里：
// 用计数来隐式实现状态机模式图中的状态转移。这里我们规定0、1、2对应StartChecking；3到26是正常接收；27到29（或者后续更多）是EndChecking
static unsigned char timer2_isr_count = 0;
static unsigned char ones_count_of_curr_bit = 0;

void UART_Receiver_init(void)
{
    EA = 1; // 允许中断
    ES = 0; // 禁止来自原生硬件 UART 的中断

    // 下降沿中断设置范式
    receiver_idle = 1; // 允许INT0中断； // 主要通过这个来设置
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

    // 先禁止定时器中断
    ET2 = 0;
    
    // 下降沿和定时器2中断同时设定为最高优先级 11
    IPH &= 0x21;
    PT2 = 1;
    PX0 = 1;

    UART_received_data_buffer = 0x00;
    tube_display_data_buffer = 0x00000000;
}

void UART_Receiver_FallingEdge_ISR() INTERRUPT(0) // 不是计时器中断而是串口中断
{
    receiver_idle = 0;

    // 重设相位并且在软件层面上将标志位清零
    TH2 = 0xff;
    TL2 = 0xc0;
    TF2 = 0; // 并且在软件层面上将该标志位清零
    timer2_isr_count = 0;
    ones_count_of_curr_bit = 0;
    ET2 = 1; // 这样应该会立即进入 bit 检测的ISR
}

void UART_Receiver_Back2Idle(void)
{
    ET2 = 0;
    receiver_idle = 1;
}

void UART_Receiver_Timer2_ISR() INTERRUPT(5)
{
    if (RI == 1)
    {
        // 由于接收和输入共用中断，接收后的ISR必须放在这里
        UART_received_data_buffer = SBUF;
        // 为了精确显示累积的数据，必须在ISR中执行该操作。这可能会破坏封装性，但是这是小项目无所谓。
        tube_display_data_buffer = (tube_display_data_buffer << 8) + UART_received_data_buffer;
        
        // 手动复位
        RI = 0;
    }

    // StartChecking的上半部分
    if (timer2_isr_count < 3 && !myRX) // myRX == 0，是标准位
    {
        ones_count_of_curr_bit++;
    }
    else if (3 <= timer2_isr_count && timer2_isr_count < 27) // 正在检测
    {}
    // StartChecking的下半部分
    if (timer2_isr_count == 2 && ones_count_of_curr_bit < 2) // 0或1次则认为下降沿误报，需要恢复状态
    {
        ET2 = 0;
        receiver_idle = 1;
        return;
    }

    timer2_isr_count++;
}
