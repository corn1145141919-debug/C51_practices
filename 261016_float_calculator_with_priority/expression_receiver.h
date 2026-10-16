#ifndef __EXPRESSION_RECEIVER__H__
#define __EXPRESSION_RECEIVER__H__
#include "token.h"

void expression_receiver_push_operand(Operand operand);

void expression_receiver_push_operator(Operator operator);

Operand expression_receiver_solve(Operator operator);

#endif