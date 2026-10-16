#include "expression_solver.h"
#include "token.h"
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
    unsigned char temp_integer_part;
    unsigned char temp_decimal_part;
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

// 各种构造函数
// 注意：所有跳转到EN状态的都需要实现该初始化函数
void expression_solver_EN_init(unsigned char is_this_number_negative)
{
    status.entering_number_status.is_entered_number_negative = is_this_number_negative;
    status.entering_number_status.temp_integer_part = 0;
    status.entering_number_status.temp_decimal_part = 0;
}

Operand expression_solver_EN_get_operand()
{
    Operand output;
    output.value = status.entering_number_status.temp_integer_part / status.entering_number_status.temp_decimal_part;
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

void expression_solver_W4O_Or_SA_init()
{}

void expression_solver_init()
{
    // state = ?;
}

// TODO：构建给数码管显示的状态机，或者考虑解耦
unsigned long expression_solver_process_input(ButtonInput input)
{
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
                if (state == entering_number__decimal_part)
                {
                    status.entering_number_status.temp_decimal_part = input + 10 * status.entering_number_status.temp_decimal_part;
                }
                else // if (state == entering_number__integral_part)
                {
                    status.entering_number_status.temp_integer_part = input + 10 * status.entering_number_status.temp_integer_part;
                }
            }
            else if (add <= input && input <= mul) // 是加减乘除中的一个
            {}
            else if (input == eq)
            {}
            
            break;
        }
        default:
        {
            break;
        }
    }
}
