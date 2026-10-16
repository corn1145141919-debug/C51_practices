#include "expression_solver.h"
#include "token.h"

void expression_solver_push_operand(Operand operand)
{}

void expression_solver_push_operator(Operator operator)
{}

Operand expression_solver_solve() // 实际上只进行跑到底的最终结算，可能只涉及“未被使用的operator token直接忽略”
{}
