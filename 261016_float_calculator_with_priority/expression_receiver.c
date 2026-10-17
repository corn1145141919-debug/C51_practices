#include "expression_receiver.h"
#include "expression_solver.h"
#include "token.h"
#include <string.h> // 给数组进行memset

#define SIG_FIGS 7 // 录入的小数的有效数字只保留7位
#define MAX_ULONG_VALUE 0xFFFFFFFFUL

#include <INTRINS.H> // 检测浮点数是否为error

enum State
{
    showing_answer,
    waiting_for_operand,
    entering_number__integer_part,
    entering_number__decimal_part
} state;

typedef struct
{
    unsigned char is_entered_number_negative; // 最终被压入operand token的符号

    // 必须存储足够多的数据
    // 这种形式似乎更好实现
    unsigned long temp_integer_part; 
    unsigned char temp_decimal_part[SIG_FIGS]; // 录入的小数强制规定为8位
    unsigned char curr_decimal_part_bit;
    unsigned char is_temp_integer_part_overflow;
} EnteringNubmerStatus;

typedef struct
{
    unsigned char is_next_operand_negative; // 可能要传给下个entering_number状态的临时变量
    Operator curr_operator;
} WaitingForOperandOrShowingAnswerStatus;

static union 
{
    EnteringNubmerStatus entering_number_status;
    WaitingForOperandOrShowingAnswerStatus waiting_for_operand_or_showing_answer_status;
} status;
static Operand ans; // 似乎只适用于showing_answer一个状态？

// 规定-1情况下不显示小数点
signed char decimal_point_place;

// 各种构造函数
// 注意：所有跳转到EN状态的都需要实现该初始化函数
void expression_receiver_EN_init(unsigned char is_this_number_negative, unsigned char initial_integer_part)
{
    status.entering_number_status.is_entered_number_negative = is_this_number_negative;
    status.entering_number_status.temp_integer_part = initial_integer_part;
    memset(status.entering_number_status.temp_decimal_part, 0, sizeof(status.entering_number_status.temp_decimal_part)); // 数组清零
    status.entering_number_status.curr_decimal_part_bit = 0;
    status.entering_number_status.is_temp_integer_part_overflow = 0;
}

Operand expression_receiver_EN_get_operand()
{
    unsigned char i;
    float weight = +0.1;
    Operand output;
    output.value = (float)status.entering_number_status.temp_integer_part;

    for (i = 0; i < status.entering_number_status.curr_decimal_part_bit; i++, weight *= +0.1)
    {
        output.value += weight * status.entering_number_status.temp_decimal_part[i];
    }
    // 尽管不太可能在这里产生，但是仍旧在这里写防御代码。
    // 实际上，error可能因整数溢出而产生
    if (2 <= _chkfloat_(output.value) || status.entering_number_status.is_temp_integer_part_overflow) // 利用Keil的库——如果是+Infinity、-Infinity 或者 NaN
    {
        output.is_error = 1;
        return output;
    }
    else
    {
        if (status.entering_number_status.is_entered_number_negative)
        {
            output.value = - output.value;
        }
        output.is_error = 0;
        return output;
    }
}

// ans寄存器是否需要处理？
void expression_receiver_W4O_Or_SA_init(ButtonInput curr_operator)
{
    switch (curr_operator)
    {
        case add:
        {
            status.waiting_for_operand_or_showing_answer_status.curr_operator = ADD;
            status.waiting_for_operand_or_showing_answer_status.is_next_operand_negative = 0;
            break;
        }
        case sub:
        {
            status.waiting_for_operand_or_showing_answer_status.curr_operator = ADD;
            status.waiting_for_operand_or_showing_answer_status.is_next_operand_negative = 1;
            break;
        }
        case mul:
        {
            status.waiting_for_operand_or_showing_answer_status.curr_operator = MUL;
            status.waiting_for_operand_or_showing_answer_status.is_next_operand_negative = 0;
            break;
        }
        case div:
        {
            status.waiting_for_operand_or_showing_answer_status.curr_operator = DIV;
            status.waiting_for_operand_or_showing_answer_status.is_next_operand_negative = 0;
            break;
        }
        default:
        {
            break; // 暂定仅仅静默退出
        }
    }
}

// 这里就体现出C++强制你写构造函数并且自动调用构造函数的优势了
void expression_receiver_init()
{
    state = showing_answer;

    // 初始化ans
    ans.is_error = 0;
    ans.value = +0.0;

    expression_receiver_W4O_Or_SA_init(add); // 进入showing_answer状态的默认配置

    // 由于外界只与expression_receiver交流，理论上不该知道solver的存在，
    // 我们需要在这里完成连带的初始化
    expression_solver_init();
}

void make_integer_data(Data4DigitalTube* target)
{}

void make_real_data(Data4DigitalTube* target)
{}


// 该函数完成状态机的主要职责，同时耦合宏观的数码管所需数据制造逻辑
Data4DigitalTube expression_receiver_process_input(ButtonInput input)
{
    // 每一轮更新。关于初始化：初始化是直接显示被初始化的ans = 0，没有问题。
    static Data4DigitalTube output;
    switch (state)
    {
        case showing_answer:
        {
            // 首先发送数码管信息
            if (ans.is_error)
            {
                output.is_error = 1;
            }
            else
            {
                // TODO
            }

            // 必然只是第一次按下，并且是状态转移，因此可以推出ans token之后直接调用之前写好的构造函数重设
            if (add <= input && input <= div) // 是加减乘除中的一个；
            {
                expression_solver_push_operand(ans);
                state = waiting_for_operand;
                expression_receiver_W4O_Or_SA_init(input);
            }
            else if (input == eq)
            {
                // 此时curr_operator没有转化为token，可以直接结算

                // 无需执行
                // state = showing_answer;
                // ans = ans;

                expression_receiver_W4O_Or_SA_init(add); // 统一框架就是好；不过这里确实重写了很多遍
            }
            else // 默认是小数点或者数字。这样就能少写点条件判断了，或者说能大量复用代码了
            // 这种条件下ans会被彻底覆盖
            {
                // 只进行状态转移，不推送任何token
                if (input == dec_pt)
                {
                    state = entering_number__decimal_part;
                    // “点几”是“零点几”的语法糖
                    expression_receiver_EN_init(status.waiting_for_operand_or_showing_answer_status.is_next_operand_negative, 0);
                }
                else
                {
                    state = entering_number__integer_part;
                    expression_receiver_EN_init(status.waiting_for_operand_or_showing_answer_status.is_next_operand_negative, input);
                }
            }

            break;
        }
        case waiting_for_operand:
        {
            // 显然这个状态只需要继承上一个状态（showing_answer或者entering_numbers）的显示信息，因此不必加代码

            // 是加减乘除中的一个
            if (input == add) 
            {
                status.waiting_for_operand_or_showing_answer_status.is_next_operand_negative = 0; // 清除既有的附加的负号状态，还原初始状态
                status.waiting_for_operand_or_showing_answer_status.curr_operator = ADD;
            }
            else if (input == mul)
            {
                status.waiting_for_operand_or_showing_answer_status.is_next_operand_negative = 0; // 清除既有的附加的负号状态，还原初始状态
                status.waiting_for_operand_or_showing_answer_status.curr_operator = MUL;
            }
            else if (input == div)
            {
                status.waiting_for_operand_or_showing_answer_status.is_next_operand_negative = 0; // 清除既有的附加的负号状态，还原初始状态
                status.waiting_for_operand_or_showing_answer_status.curr_operator = DIV;
            }
            else if (input == sub)
            {
                status.waiting_for_operand_or_showing_answer_status.is_next_operand_negative = !status.waiting_for_operand_or_showing_answer_status.is_next_operand_negative;
            }
            else if (input == eq)
            {
                // 此时curr_operator没有转化为token，可以直接结算
                state = showing_answer;
                ans = expression_solver_solve(); // 能在waiting_for_operand状态，solver一定之前收到过一些token
                expression_receiver_W4O_Or_SA_init(add);
            }
            else // 默认是小数点或者数字。这样就能少写点条件判断了，或者说能大量复用代码了
            {
                expression_solver_push_operator(status.waiting_for_operand_or_showing_answer_status.curr_operator);

                if (input == dec_pt)
                {
                    state = entering_number__decimal_part;
                    // “点几”是“零点几”的语法糖
                    expression_receiver_EN_init(status.waiting_for_operand_or_showing_answer_status.is_next_operand_negative, 0);

                    // 在这里更新数码管显示
                    output.is_error = 0;
                    output.digital_point_place = 1;
                    output.numbers = 0;
                }
                else
                {
                    state = entering_number__integer_part;
                    expression_receiver_EN_init(status.waiting_for_operand_or_showing_answer_status.is_next_operand_negative, input);
                }
            }

            break;
        }
        case entering_number__decimal_part:
        case entering_number__integer_part:
        // 都执行下列语句
        {
            if (input == dec_pt)
            {
                // 无论当前是在键入整数还是键入小数，都跳转为键入小数
                state = entering_number__decimal_part;
            }
            else if (add <= input && input <= div) // 是加减乘除中的一个
            {
                // 拿到一个 operand token，先执行操作
                expression_solver_push_operand(expression_receiver_EN_get_operand());

                // 状态转移
                state = waiting_for_operand;
                expression_receiver_W4O_Or_SA_init(input);
            }
            else if (input == eq)
            {
                // 拿到一个 operand token，先执行操作
                expression_solver_push_operand(expression_receiver_EN_get_operand());

                // 状态转移
                state = showing_answer;
                ans = expression_solver_solve();
                expression_receiver_W4O_Or_SA_init(add); // 默认
            }
            else // if (0 <= input && input <= 9) // 默认总是满足条件
            {
                if (state == entering_number__decimal_part && status.entering_number_status.curr_decimal_part_bit < SIG_FIGS) // 如果超过小数有效数字，就直接忽略输入
                {
                    status.entering_number_status.temp_decimal_part[status.entering_number_status.curr_decimal_part_bit] = input;
                    status.entering_number_status.curr_decimal_part_bit++;
                }
                else if (state == entering_number__integer_part)
                {
                    // 可严格证明这一判断是准确的
                    if (status.entering_number_status.temp_integer_part > (MAX_ULONG_VALUE - input) / 10UL)
                    {
                        // error
                        status.entering_number_status.is_temp_integer_part_overflow = 1;
                    }
                    else
                    {
                        status.entering_number_status.temp_integer_part = input + 10 * status.entering_number_status.temp_integer_part;
                    }
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }

    /*
     * Display generation is deliberately performed after the state-machine
     * transition.  This makes the frame returned for the key press describe
     * the new state (including the first digit or a freshly entered point),
     * while waiting_for_operand naturally keeps the previous frame.
     */
    if (state == showing_answer)
    {
        DisplayFormatter_FormatResult(&output, ans.is_error, ans.value);
    }
    else if (state == entering_number__integer_part
        || state == entering_number__decimal_part)
    {
        /* host-only diagnostic omitted in Keil build */
        DisplayFormatter_FormatInput(
            &output,
            status.entering_number_status.is_entered_number_negative,
            status.entering_number_status.temp_integer_part,
            status.entering_number_status.temp_decimal_part,
            status.entering_number_status.curr_decimal_part_bit,
            state == entering_number__decimal_part,
            status.entering_number_status.is_temp_integer_part_overflow);
    }

    return output;
}
