#ifndef __UART__H__
#define __UART__H__

extern volatile unsigned char UART_received_data_buffer;
extern volatile unsigned long tube_display_data_buffer;

void UART_init(void);
void UART_SendByte(unsigned char byte);

#endif