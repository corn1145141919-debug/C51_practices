#ifndef __TOKEN__H__
#define __TOKEN__H__

typedef enum
{
    ADD,
    MUL,
    DIV
} Operator;

typedef struct
{
    unsigned char is_error;
    float value;
} Operand;

// 看看需不需要
// typedef struct
// {
//     unsigned char is_operand;
//     union 
//     {
//         Operand operand;
//         Operator operator;
//     } token_data; 
// } Token;

#endif