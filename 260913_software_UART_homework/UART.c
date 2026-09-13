#include <REGX52.H>
#include "digital_tube.h"

volatile unsigned char UART_received_data_buffer;
volatile unsigned long tube_display_data_buffer;

void UART_init(void)
{
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
    // 一个时钟周期内完成
    SCON = 0x50;

    PCON &= 0x7f; // 波特率不加倍

    // 定时器相关寄存器
    // 以下为 Timer1 初始化
    TMOD = (TMOD & 0x0f) | 0x20;

    TF1 = 0; // 定时器清零
    TR1 = 1; // 允许定时器运行

    // 9600bps@11.0592MHz
    TL1 = 0xfd; // 定时器初值
    TH1 = 0xfd; // 定时器重装值

    // 中断相关寄存器
    EA = 1;
    ET1 = 0; // 禁止定时器1中断——中断只能来自串口通信，不能来自定时器1本身。定时器1仅为波特率发生器
    ES = 1; // 但是要允许来自UART的中断！
    // 中断优先级：串口优先级为3
    // PX0H = 1; // 不可位寻址 // 可能STC89C52RC没有 IPH 寄存器
    // 高优先级
    PS = 1;

    UART_received_data_buffer = 0x00;
    tube_display_data_buffer = 0x00000000;
}

void UART_SendByte(unsigned char byte)
{
    unsigned char ES_before = ES;
    ES = 0; // 暂时禁止串口中断，以免该函数与串口ISR发生冲突
    TI = 0; // 清除可能残留的发送完成标志 // “函数自己建立自己的初始条件”
    SBUF = byte;
    while(TI == 0)
    {} // 只有当TI == 1 即消息被发出之后才跳出来，否则一直占用时钟周期保证写入的消息不被覆盖
    TI = 0; // 手动重置 // 谁等待或者消费 TI == 1 的事件，谁就应该来清TI
    ES = (bit)ES_before; // 恢复该函数运行前的串口中断设置
}

void UART_ISR() interrupt 4 // 不是计时器中断而是串口中断
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
