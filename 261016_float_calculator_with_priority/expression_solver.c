#include "expression_solver.h"
#include "token.h"

void expression_solver_push_operand(Operand operand)
{}

void expression_solver_push_operator(Operator operator)
{}

// 实际上只进行跑到底的最终结算，可能只涉及“未被使用的operator token直接忽略”。
// 不过它的重要性在于它是输出结果的唯一通道。
Operand expression_solver_solve() 
{}
