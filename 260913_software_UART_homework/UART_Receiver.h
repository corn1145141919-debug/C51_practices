#ifndef __UART_RECEIVER__H__
#define __UART_RECEIVER__H__

/**
 * 说明：
 * 本模块不会忽略上电初期内部的噪声。如果需要忽略，请模块使用者在外部逻辑中自己实现这一功能。
 */

// 该模块对外部暴露的接口，外部真正读取数据的地方。
extern volatile unsigned char UART_Receiver_data_output_buffer;
// “成功地”（下文将定义怎样才算成功）将字节推入上面那个output buffer之后，该模块还会将该变量置为1，
// 外部使用者读取该变量后还需要将下面这个布尔变量置为0，以可信地昭示自己读取了这个变量。
// 如果该布尔变量仍为非0值，那么该模块将字节推入这个output buffer的行为将会失败——该读到的字节将被直接丢弃。
// （这会确保外部处理的是一致的数据——外部不能读到两个字节的融合版本）
extern volatile unsigned char UART_Receiver_output_flag;
// 失败记录：假如真的发生了上述的丢弃事件，UART_Receiver模块需要将失败信息传递出去。
// 此时外部自己决定采取什么样的方式来弥补失败的后果，并且在处理之后将该变量清零。
// 这些弥补措施不由本模块实现，本模块只负责忠实地记录失败。
// （预计在本项目中，我会预留几个数码管显示这个数据。）
extern volatile unsigned char UART_Receiver_abandoned_byte_count;

void UART_Receiver_init(void);

#endif