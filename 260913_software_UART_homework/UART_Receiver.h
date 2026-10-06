#ifndef __UART_RECEIVER__H__
#define __UART_RECEIVER__H__

extern volatile unsigned char UART_received_data_buffer;
extern volatile unsigned long tube_display_data_buffer;

void UART_Receiver_init(void);

#endif