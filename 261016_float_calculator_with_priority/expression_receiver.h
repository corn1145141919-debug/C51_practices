#ifndef __EXPRESSION_RECEIVER__H__
#define __EXPRESSION_RECEIVER__H__
#include "token.h"

// 0 到 9 正常输入，毕竟C的enum比C++的enum class松很多
typedef enum
{
    add = 10,
    sub = 11,
    mul = 12,
    div = 13,
    dec_pt = 14,
    eq = 15
} ButtonInput;

void expression_receiver_init();

/* 对于super_loop的接口：对一次按键行为相应一步操作。
 * 上电 or Reset 之后，先执行上述初始化函数。
 * 然后外部执行以下函数作为完整的初始化（需要接收该函数给的给数码管的输出），输入按键为“键入0”？
 * 输入：super_loop接收到的按键
 * 输出：数码管显示的数据
 */
unsigned long expression_receiver_process_input(ButtonInput input);

#endif