#ifndef __DISPLAY_FORMATTER__H__
#define __DISPLAY_FORMATTER__H__

/*
 * A frame is stored in physical tube order: digit 0 is the rightmost
 * tube and digit 7 is the leftmost tube.  A decimal point belongs to the
 * digit selected by decimal_point_mask and does not consume a tube.
 */
#define DISPLAY_TUBE_COUNT 8
#define DISPLAY_BLANK 10
#define DISPLAY_MINUS 11

typedef struct Data4DigitalTube
{
    unsigned char is_error;
    signed long numbers;              /* legacy numeric view */
    signed char digital_point_place;  /* legacy point view, -1 means none */
    unsigned char digits[DISPLAY_TUBE_COUNT];
    unsigned char decimal_point_mask;
} Data4DigitalTube;

void DisplayFormatter_Clear(Data4DigitalTube *target);

void DisplayFormatter_FormatInput(
    Data4DigitalTube *target,
    unsigned char is_negative,
    unsigned long integer_part,
    const unsigned char *decimal_digits,
    unsigned char decimal_count,
    unsigned char has_decimal_point,
    unsigned char integer_overflow);

void DisplayFormatter_FormatResult(
    Data4DigitalTube *target,
    unsigned char is_error,
    float value);

#endif
