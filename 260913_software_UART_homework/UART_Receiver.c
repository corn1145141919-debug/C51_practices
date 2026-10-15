#include <REGX52.H>
#include "digital_tube.h"

// 能让 VS Code 的 IntelliSense 和 Keil 同时正确读取（需要安装Keil Assistant插件）
// 理论上来讲，在把这个软件发布给任何只有Keil软件的用户时，这些源代码都能被正确编译
#ifndef __VSCODE_C51__
#define INTERRUPT(x) interrupt x
#else
#define INTERRUPT(x)
#endif

// public: // bro以为自己在写C++

volatile unsigned char UART_Receiver_data_output_buffer = 0x00;
volatile unsigned char UART_Receiver_output_flag = 0;
volatile unsigned char UART_Receiver_abandoned_byte_count = 0;

// private:

sfr IPH = 0xb7;
// ——作为UART Receiver在内部操纵时直接操作的buffer（相当于原生硬件UART中的读SBUF）。不对外部暴露。
static unsigned char UART_Receiver_private_received_data_buffer;
sbit receiver_idle = 0xa8; // 给EX0起别名
sbit myRX = 0xb2; // 接收传到P3_2，这里起个别名并且方便修改

/* 由于一些按理来说封装在interrupt 5中的变量的初始化有的要由interrupt 0执行（或者首次执行），
我们必须把本来应该封装给interrupt 5的一些变量放在外部。
用下述两个计数和一个布尔变量实现状态机模式图中相当一部分的状态转移。
这里我们规定，is_end_checking = 0时，curr_bit_mask = 0x00是StartChecking；0x01到0x80从低到高分别对应2^0到2^7位；
is_end_checking = 1则在end_checking，curr_bit_mask无意义。
此外还有sample_phase = 0, 1, 2
*/
static unsigned char curr_bit_mask = 0; // is_end_checking = 1时没有意义，不使用
static unsigned char is_end_checking = 0;
static unsigned char sample_phase = 0; // 无论是否is_end_checking，它一定都会被使用

// 当前bit的one的次数
static unsigned char ones_count_of_curr_bit = 0;

// public:

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
    IPH |= 0x21; // 注意，设置1需要用或，设置0需要用与!
    PT2 = 1;
    PX0 = 1;

    UART_Receiver_private_received_data_buffer = 0x00;
}

// private:

void UART_Receiver_FallingEdge_ISR() INTERRUPT(0) // 不是计时器中断而是串口中断
{
    receiver_idle = 0;

    // 重设相位并且在软件层面上将标志位清零
    TH2 = 0xff;
    TL2 = 0xc0;
    TF2 = 0; // 并且在软件层面上将该标志位清零（防御性地）
    curr_bit_mask = 0x00;
    is_end_checking = 0;
    sample_phase = 0;
    ones_count_of_curr_bit = 0;
    ET2 = 1; // 这样应该会立即进入 bit 检测的ISR
}

// 把所有退出所需操作封装成函数。之后假如遇到性能瓶颈，会复制回去。
void UART_Receiver_Back2Idle(void)
{
    ET2 = 0;
    // 由于要退出Timer2的ISR，我们必须处理中断位
    TF2 = 0;

    ones_count_of_curr_bit = 0; // 防御性地重设
    curr_bit_mask = 0x00;
    is_end_checking = 0;
    sample_phase = 0;

    receiver_idle = 1;
}

void UART_Receiver_Timer2_ISR() INTERRUPT(5)
{
    static unsigned char is_one;
    static unsigned char end_checking_container = 0x00;

    TF2 = 0; // 软件自己对TF2清零 // 尝试最早清零

    // 每个bit，无论是在StartChecking阶段还是Receiving阶段，都需要检测该位中0和1的情况（我们这里计数1）。
    // 它可能是最频繁被调用的语句之一，所以我们放在外边。
    // 不知道这样的处理是否最优。
    if (myRX)
    {
        ones_count_of_curr_bit++;
    }

    // 需要在每一位被接收完毕之后结算（EndChecking每一位都需要结算）。不同的状态对应下列if-else语句不同的入口。
    // 末尾需要将ones_count_of_curr_bit清零。EndChecking例外。

    // EndChecking放在最前面以减少逻辑判断次数
    // EndChecking：无论是否是末位都进来。忽略ones_count_of_curr_bit，利用位运算特性来实现“最近3次是否为1”的记忆。利用最低3位。
    if (is_end_checking)
    {
        end_checking_container >>= 1;
        if (myRX)
        {
            end_checking_container |= 0x04;
        }
        else
        {
            end_checking_container &= 0xfb;
        }
        if ((end_checking_container & 0x07) == 0x07) // 最近3次均为1
        {
            // 运行到“将字节推送到output或遗弃”环节
            if (UART_Receiver_output_flag) // 不能覆盖未被可靠读取的数据
            {
                UART_Receiver_abandoned_byte_count++; // 记录错误
            }
            else
            {
                UART_Receiver_data_output_buffer = UART_Receiver_private_received_data_buffer;
                UART_Receiver_output_flag = 1;
            }
            UART_Receiver_Back2Idle();
            return;
        }
    }
    // StartChecking末位
    else if (!curr_bit_mask && sample_phase == 2) // !curr_bit_mask means curr_bit_mask == 0x00，即正在处于StartChecking
    {
        // 对应流程图中的No：如果没有收到可靠的0则认为此处触发为误触。
        if (ones_count_of_curr_bit >= 2) // 2到3个1，不是期待的0，不可靠。如果改为“3个1触发”请将该行数字改为3。
        {
            UART_Receiver_Back2Idle();
            return;
        }
        ones_count_of_curr_bit = 0;
        // 对应流程图中的Yes：需要显式赋值0x01给curr_bit_mask
        // 仍旧感谢之前用注释写出状态转移位置的我自己
        curr_bit_mask = 0x01;
    }
    // Receiving末位
    else if (sample_phase == 2 && curr_bit_mask) // 条件判断需要保证是末位 // 与上边类似，非零证明处于BitDececting状态
    {
        // 先接收到的是最低位，即2^0位。
        if (ones_count_of_curr_bit >= 2) // 1
        {
            UART_Receiver_private_received_data_buffer |= curr_bit_mask;
        }
        else // 0
        {
            // 左移右移操作带1的数据比操作带0的数据更得心应手。因此我们将mask按位取反。
            UART_Receiver_private_received_data_buffer &= ~curr_bit_mask;
        }

        ones_count_of_curr_bit = 0;

        // 实现状态转移（感谢之前在没有实际代码情况下仍然用注释标示状态转移的我自己）
        curr_bit_mask <<= 1;
        // 判断是0x00；由我们的状态机模型，这就是EndChecking状态，而不是上述的StartChecking状态
        if (!curr_bit_mask) 
        {
            is_end_checking = 1;
        }
    }
    
    // 最后的状态转移
    // curr_bit_mask的状态转移已由上述StartChecking、Receiving两个状态自己实现
    // is_end_checking的转移应该也比较完备了
    // 实际上只需要实现sample_phase的状态转移
    sample_phase++;
    if (sample_phase == 3)
    {
        sample_phase = 0;
    }
}
