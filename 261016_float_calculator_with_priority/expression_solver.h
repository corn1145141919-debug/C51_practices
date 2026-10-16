#ifndef __EXPRESSION_SOLVER__H__
#define __EXPRESSION_SOLVER__H__

void expression_solver_push_operand(Operand operand);

void expression_solver_push_operator(Operator operator);

Operand expression_solver_solve(Operator operator);

#endif