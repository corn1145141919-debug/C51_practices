#include "expression_receiver.h"
#include "expression_solver.h"
#include "token.h"
#include <string.h> // 给数组进行memset

#define SIG_FIGS 8 // 录入的小数的有效数字只保留8位

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
    unsigned char temp_decimal_part[SIG_FIGS]; // 总有效数字强制规定为8位
    unsigned char curr_decimal_part_bit;
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

// 各种构造函数
// 注意：所有跳转到EN状态的都需要实现该初始化函数
void expression_receiver_EN_init(unsigned char is_this_number_negative)
{
    status.entering_number_status.is_entered_number_negative = is_this_number_negative;
    status.entering_number_status.temp_integer_part = 0;
    memset(status.entering_number_status.temp_decimal_part, 0, sizeof(status.entering_number_status.temp_decimal_part)); // 数组清零
    status.entering_number_status.curr_decimal_part_bit = 0;
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
    if (2 <= _chkfloat_(output.value)) // 利用Keil的库——如果是+Infinity、-Infinity 或者 NaN
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
}

// TODO：构建给数码管显示的状态机，或者考虑解耦
unsigned long expression_receiver_process_input(ButtonInput input)
{
    unsigned long output = 0; // TODO 关于该变量的逻辑代码
    switch (state)
    {
        case showing_answer:
        case waiting_for_operand:
        // 都执行下列语句
        {
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
            else if (0 <= input && input <= 9)
            {
                if (state == entering_number__decimal_part && status.entering_number_status.temp_decimal_part < SIG_FIGS) // 如果超过小数有效数字，就直接忽略输入
                {
                    status.entering_number_status.temp_decimal_part[status.entering_number_status.curr_decimal_part_bit] = input;
                    status.entering_number_status.curr_decimal_part_bit++;
                }
                else if (state == entering_number__integer_part)
                {
                    status.entering_number_status.temp_integer_part = input + 10 * status.entering_number_status.temp_integer_part;
                    status.entering_number_status.curr_decimal_part_bit++;
                }
            }
            else if (add <= input && input <= mul) // 是加减乘除中的一个
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
            
            break;
        }
        default:
        {
            break;
        }
    }

    return output;
}