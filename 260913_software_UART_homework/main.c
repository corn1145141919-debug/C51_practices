#include <REGX52.H>
#include "UART_Receiver.h"
#include "UART_Transmitter.h"
#include "delay.h"
#include "timer0.h"
#include "digital_tube.h"

void main()
{
    unsigned char ring_buffer[4];
    unsigned char ring_buffer_head = 0;
    unsigned char ring_buffer_tail = 0;
    unsigned long temp_output = 0x00000000;

    UART_Receiver_init();
    UART_Transmitter_init();
    Timer0_Init();

    DigitalTube_Display_hex_unsigned_long(temp_output);
    
    while(1)
    {
        if (UART_Receiver_output_flag)
        {
            // 推掉旧有失败计数，将最旧字节推到即将被覆盖的位置；最低位字节自然为0x00，能确保buffer内容被可信加入并且不污染既有数据
            // 注意：加法优先级比位运算高！以后涉及位运算都需要加括号！
            temp_output = UART_Receiver_data_output_buffer + (temp_output << 8);
            UART_Receiver_output_flag = 0; // 及时标志output已被利用

            // 16进制下数码管最高2位（1个字节）显示失败次数
            // 先抹除最高位字节，然后再加入数据
            temp_output = temp_output & 0x00ffffff + (UART_Receiver_abandoned_byte_count << 24);
            DigitalTube_Display_hex_unsigned_long(temp_output);

            UART_Transmitter_SendByte(temp_output); // 利用低位截断直接发送刚刚接收到的数据——发送失败也无所谓，单片机自己会显示
        }
    }
}