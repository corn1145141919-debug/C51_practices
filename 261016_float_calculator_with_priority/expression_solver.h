#ifndef __EXPRESSION_SOLVER__H__
#define __EXPRESSION_SOLVER__H__

// main中必须在最开始调用一次；之后每次利用expression_solver_solve输出结果之后该函数又会被调用一次
void expression_solver_init();

void expression_solver_push_operand(Operand operand);

void expression_solver_push_operator(Operator operator);

Operand expression_solver_solve();

#endif
