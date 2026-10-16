#ifndef __DIGITAL_TUBE__
#define __DIGITAL_TUBE__

void DigitalTube_Display_signed_32_bit_decimal_int(signed long input);
void DigitalTube_Display_Error(void);
void DigitalTube_Display_Overflow(void);
void DigitalTube_Display_float(float input);
void DigitalTube_add_decimal_point(unsigned char bit_target);

#endif
