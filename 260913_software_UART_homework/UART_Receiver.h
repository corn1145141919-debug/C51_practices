#ifndef __UART_RECEIVER__H__
#define __UART_RECEIVER__H__

// 对外部暴露的接口，外部真正读取数据的地方。
extern volatile unsigned char UART_Receiver_data_output_buffer;
// 如果该output buffer正在被外部读取，那么“用Receiver接到的数据覆盖”行为将不被允许——该读到的字节将被直接丢弃。
// （确保外部处理的是一致的数据——外部不能读到两个字节的融合版本）
extern volatile unsigned char is_output_buffer_occupied;
// 失败记录：假如真的发生了上述的丢弃事件，UART_Receiver模块需要将失败信息传递出去。
// 此时外部自己决定采取什么样的方式来弥补失败的后果，并且在处理之后将该变量清零。
// 这些弥补措施不由本模块实现，本模块只负责忠实地记录失败。
// （预计在本项目中，我们会预留几个数码管显示这个数据，以供调试。）
extern volatile unsigned char abandoned_byte_count;

void UART_Receiver_init(void);

#endif