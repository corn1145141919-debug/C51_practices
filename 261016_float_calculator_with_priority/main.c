#pragma once
#include <REGX52.H> // 可能需要删掉
#include "digital_tube.h"
#include "matrixButton.h"
#include "timer0.h"
#include "expression_receiver.h"

const ButtonInput code map_from_button_to_receiver[16] = {
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
    Data4DigitalTube data_4_digital_tube;
    Timer0_Init();
    expression_receiver_init();

    while(1)
    {
        key_got = map_from_button_to_receiver[MatrixButton_getKey()];
        data_4_digital_tube = expression_receiver_process_input(key_got);
        DigitalTube_Display_Data(&data_4_digital_tube);
    }
}
