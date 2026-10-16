#pragma once
#include <REGX52.H> // 可能需要删掉
#include "digital_tube.h"
#include "matrixButton.h"
#include "timer0.h"

void main()
{
    signed char temp = -128;
    Timer0_Init();
    DigitalTube_Display_Error(); // test

    MatrixButton_getKey();
    DigitalTube_Display_Overflow(); // test

    while(1)
    {
        temp +=MatrixButton_getKey();
        DigitalTube_Display_decimal_int(temp);
    }
}
