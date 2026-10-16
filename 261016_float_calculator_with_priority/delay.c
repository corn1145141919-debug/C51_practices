void Delay1ms(void) //@12.000MHz
{
    unsigned char i, j;

    i = 2;
    j = 239;
    do
    {
        while (--j)
            ;
    } while (--i);
}

void delay(unsigned char ms)
{
    while (ms--)
    {
        Delay1ms();
    }
}