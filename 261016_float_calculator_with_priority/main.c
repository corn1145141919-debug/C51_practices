#pragma once
#include <REGX52.H> // 可能需要删掉
#include "digital_tube.h"
#include "matrixButton.h"
#include "timer0.h"
#include "expression_receiver.h"

const ButtonInput map_from_button_to_receiver[16] = {
    add,
    eq,
    0,
    dec_pt,
    sub,
    3,
    2,
    1,
    mul,
    6,
    5,
    4,
    div,
    9,
    8,
    7
};

void main()
{
    ButtonInput key_got;
    Timer0_Init();

    MatrixButton_getKey();

    while(1)
    {
        key_got = map_from_button_to_receiver[MatrixButton_getKey()];
        DigitalTube_Display_float(expression_receiver_process_input(key_got));
    }
}
