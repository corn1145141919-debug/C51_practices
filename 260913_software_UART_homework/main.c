#include <REGX52.H>
#include "UART.h"
#include "delay.h"
#include "timer0.h"
#include "digital_tube.h"

void main()
{
    UART_init();
    Timer0_Init();
    while(1)
    {
        // 反复执行可能会有点耗费性能
        // 可能违背了保护现场的原则 但是这确实是一个人为设计的中断后就会被改变的变量
        DigitalTube_Display_hex_unsigned_long(tube_display_data_buffer);
        UART_SendByte(UART_received_data_buffer);
        delay(500);
    }
}