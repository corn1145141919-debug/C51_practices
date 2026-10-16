#include "expression_solver.h"
#include "token.h"
#include <string.h>
#include <INTRINS.H> // 检测浮点数是否为error

// 理论分析求得
#define OPERAND_STACK_SIZE 3
#define OPERATOR_STACK_SIZE 2

// 防御operator和operand没有交替到来的情况，我们暂时静默忽略多余的operator token 或者 operand token
static unsigned char is_last_token_operand;
static Operand operand_stack[OPERAND_STACK_SIZE];
static Operator operator_stack[OPERATOR_STACK_SIZE];
// 指向当前栈顶元素所在位置。栈空的时候指向-1。
static signed char operand_stack_top;
static signed char operator_stack_top;

// 不做太多多余的防御了，我们不妨相信receiver总会给出正确的行为

// private:
unsigned char priority(Operator operator)
{
    switch (operator)
    {
        case ADD:
        {
            return 0; // 低优先级
        }
        case MUL:
        case DIV:
        {
            return 1; // 高优先级
        }
        // 不写default，不做太多防御
    }
}

void operand_stack_push_back(Operand input)
{
    operand_stack_top++; // 先指向栈顶位置
    operand_stack[operand_stack_top] = input;
}

Operand operand_stack_pop()
{
    // 先引用后自减。这种情况下能保证被运行。
    // lazy delete，该栈位再次被使用的时候值自然会被覆盖；栈顶指针不指向这里就不会有问题
    return operand_stack[operand_stack_top--];
}

void operator_stack_push_back(Operator input)
{
    operator_stack_top++;
    operator_stack[operator_stack_top] = input;
}

Operator operator_stack_pop()
{
    return operator_stack[operator_stack_top--];
}

// 计算核心
// 弹出operator栈顶作为规则；让operand栈顶先后弹出本次运算的右操作数和左操作数，运算结束之后压回栈顶
// 内含error处理
void calculate()
{
    Operator curr_operator = operator_stack_pop();
    Operand right = operand_stack_pop();
    Operand left = operand_stack_pop();
    Operand result;

    if (right.is_error || left.is_error)
    {
        // error的传递
        result.is_error = 1;
    }
    else
    {
        result.is_error = 0;

        switch (curr_operator)
        {
            case ADD:
            {
                result.value = left.value + right.value;
                break;
            }
            case MUL:
            {
                result.value = left.value * right.value;
                break;
            }
            case DIV:
            {
                result.value = left.value / right.value;
                break;
            }
        }

        if (2 <= _chkfloat_(result.value)) // 利用Keil的库——如果是+Infinity、-Infinity 或者 NaN
        {
            result.is_error = 1;
        }
    }

    operand_stack_push_back(result);
}

// public:

void expression_solver_init()
{
    // 每个表达式第一个推进来的token必然是operand，因此强行规定上一个token是operator
    is_last_token_operand = 0;

    memset(operand_stack, 0, sizeof(operand_stack));
    memset(operator_stack, 0, sizeof(operator_stack));

    operand_stack_top = -1;
    operator_stack_top = -1;
}

void expression_solver_push_operand(Operand input)
{
    if (!is_last_token_operand)
    {
        // 非常简单的行为
        operand_stack_push_back(input);
        
        is_last_token_operand = 1; // 之前忘记进行状态标定和转移了；我们只考虑合法的输入，因此在这里转移
    }
}

void expression_solver_push_operator(Operator input)
{
    if (is_last_token_operand)
    {
        while (1)
        {
            // 必须加上运算符栈还空的情况；||在C语言中具有短路求值规则，因此当栈空时||后面的代码不会被执行
            if (operator_stack_top == -1 || priority(input) > priority(operator_stack[operator_stack_top]))
            {
                operator_stack_push_back(input);
                break;
            }
            else // 需要一直循环比较
            {
                calculate();
            }
        }

        is_last_token_operand = 0;
    }
}

// 实际上只进行跑到底的最终结算，可能只涉及“未被使用的operator token直接忽略”。
// 不过它的重要性在于它是输出结果的唯一通道。
Operand expression_solver_solve() 
{
    Operand output;
    // while (1)
    // {
    //     if (operator_stack_top == -1) // 运算符栈已空
    //     {
    //         break;
    //     }
    //     calculate();
    // }

    // 更好的写法
    // 无论何时，只要栈空就能离开；
    // 并且运算符栈空的情况下就归约（即只来一个operand token）情况下，calculate()不会被执行，直接输出最终答案
    while (operator_stack_top >= 0)
    {
        calculate();
    }

    output = operand_stack[operand_stack_top];
    expression_solver_init(); // 让solve()函数自己负责扫尾和清除跨表达式污染
    return output;
}
