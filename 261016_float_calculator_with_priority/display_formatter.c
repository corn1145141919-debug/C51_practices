#include "display_formatter.h"

#define DISPLAY_MAX_TEXT 16
#define RESULT_SIG_FIGS 7
#define RESULT_MAX_DECIMALS 7

static void clear_frame(Data4DigitalTube *target)
{
    unsigned char i;

    target->is_error = 0;
    target->numbers = 0;
    target->digital_point_place = -1;
    target->decimal_point_mask = 0;
    for (i = 0; i < DISPLAY_TUBE_COUNT; ++i)
    {
        target->digits[i] = DISPLAY_BLANK;
    }
}

// void DisplayFormatter_Clear(Data4DigitalTube *target)
// {
//     clear_frame(target);
// }

static unsigned char count_unsigned_long_digits(unsigned long value)
{
    unsigned char count = 1;

    while (value >= 10UL)
    {
        value /= 10UL;
        ++count;
    }
    return count;
}

static unsigned char append_unsigned_long_digits(
    unsigned char *text,
    unsigned char length,
    unsigned long value)
{
    unsigned char reverse_digits[10];
    unsigned char count = 0;
    unsigned char i;

    do
    {
        reverse_digits[count] = (unsigned char)(value % 10UL);
        value /= 10UL;
        ++count;
    } while (value != 0UL && count < 10);

    for (i = 0; i < count; ++i)
    {
        text[length + i] = reverse_digits[count - i - 1];
    }
    return (unsigned char)(length + count);
}

/*
 * Copy a left-to-right textual representation into right-aligned tube
 * storage.  If it is too wide, the highest-order digits are discarded;
 * the sign is retained whenever possible.  The point index is the digit
 * carrying the point, or -1 when no point is present.
 */
static void copy_text_to_frame(
    Data4DigitalTube *target,
    const unsigned char *text,
    unsigned char length,
    signed char point_index)
{
    unsigned char start;
    unsigned char i;
    unsigned char destination;
    signed char adjusted_point;

    clear_frame(target);

    start = 0;
    if (length > DISPLAY_TUBE_COUNT)
    {
        if (text[0] == DISPLAY_MINUS)
        {
            /* Keep the sign and the least-significant seven digits. */
            target->digits[DISPLAY_TUBE_COUNT - 1] = DISPLAY_MINUS;
            if (point_index == 0)
            {
                target->decimal_point_mask |= (unsigned char)(1u << (DISPLAY_TUBE_COUNT - 1));
                target->digital_point_place = DISPLAY_TUBE_COUNT - 1;
            }
            start = (unsigned char)(length - (DISPLAY_TUBE_COUNT - 1));
        }
        else
        {
            start = (unsigned char)(length - DISPLAY_TUBE_COUNT);
        }
    }

    adjusted_point = point_index;
    if (adjusted_point >= 0)
    {
        adjusted_point = (signed char)(adjusted_point - start);
    }

    for (i = start; i < length; ++i)
    {
        if (text[0] == DISPLAY_MINUS && length > DISPLAY_TUBE_COUNT)
        {
            destination = (unsigned char)(DISPLAY_TUBE_COUNT - 2 - (i - start));
        }
        else
        {
            destination = (unsigned char)(length - 1 - i + (DISPLAY_TUBE_COUNT - (length - start)));
        }
        if (destination < DISPLAY_TUBE_COUNT)
        {
            target->digits[destination] = text[i];
            if (adjusted_point == (signed char)(i - start))
            {
                target->decimal_point_mask |= (unsigned char)(1u << destination);
                target->digital_point_place = (signed char)destination;
            }
        }
    }

    /* A point whose integer zero was cropped is attached to the sign. */
    if (point_index >= 0 && adjusted_point < 0 && text[0] == DISPLAY_MINUS
        && length <= DISPLAY_TUBE_COUNT)
    {
        target->decimal_point_mask |= (unsigned char)(1u << (DISPLAY_TUBE_COUNT - 1));
        target->digital_point_place = DISPLAY_TUBE_COUNT - 1;
    }
}

void DisplayFormatter_FormatInput(
    Data4DigitalTube *target,
    unsigned char is_negative,
    unsigned long integer_part,
    const unsigned char *decimal_digits,
    unsigned char decimal_count,
    unsigned char has_decimal_point,
    unsigned char integer_overflow)
{
    unsigned char text[DISPLAY_MAX_TEXT];
    unsigned char length;
    unsigned char integer_digits;
    unsigned char first_non_zero;
    unsigned char i;
    signed char point_index;
    unsigned char drop_integer_zero;

    clear_frame(target);
    if (integer_overflow)
    {
        target->is_error = 1;
        return;
    }

    length = 0;
    point_index = -1;
    if (is_negative)
    {
        text[length++] = DISPLAY_MINUS;
    }

    integer_digits = count_unsigned_long_digits(integer_part);
    length = append_unsigned_long_digits(text, length, integer_part);

    /* Omit redundant integer leading zeroes, but retain one zero. */
    first_non_zero = is_negative ? 1 : 0;
    while (integer_digits > 1 && text[first_non_zero] == 0)
    {
        ++first_non_zero;
        --integer_digits;
    }
    if (first_non_zero > (is_negative ? 1 : 0))
    {
        for (i = first_non_zero; i < length; ++i)
        {
            text[i - (first_non_zero - (is_negative ? 1 : 0))] = text[i];
        }
        length = (unsigned char)(length - first_non_zero + (is_negative ? 1 : 0));
    }

    if (has_decimal_point)
    {
        point_index = (signed char)(length - 1);
        for (i = 0; i < decimal_count && length < DISPLAY_MAX_TEXT; ++i)
        {
            text[length++] = decimal_digits[i];
        }

        /* If the zero before the point would make a negative value too wide,
         * use the accepted compact form -.xxxxxxx. */
        drop_integer_zero = 0;
        if (is_negative && integer_part == 0UL
            && length > DISPLAY_TUBE_COUNT
            && text[1] == 0)
        {
            drop_integer_zero = 1;
        }
        if (drop_integer_zero)
        {
            for (i = 2; i < length; ++i)
            {
                text[i - 1] = text[i];
            }
            --length;
            point_index = 0;
        }
    }

    copy_text_to_frame(target, text, length, point_index);
}

static unsigned char float_is_invalid(float value)
{
    unsigned char *bytes;
    bytes = (unsigned char *)&value;
    /* IEEE-754 binary32: exponent all ones means NaN or infinity. */
    if ((bytes[3] & 0x7f) == 0x7f
        && (bytes[2] & 0x80) == 0x80)
    {
        return 1;
    }
    return 0;
}

static float power10(unsigned char exponent)
{
    float value;
    unsigned char i;

    value = 1.0f;
    for (i = 0; i < exponent; ++i)
    {
        value *= 10.0f;
    }
    return value;
}

static unsigned char decimal_exponent(float value)
{
    unsigned char exponent;

    exponent = 0;
    while (value >= 10.0f && exponent < 39)
    {
        value *= 0.1f;
        ++exponent;
    }
    return exponent;
}

static unsigned char rounded_digits(
    float value,
    unsigned char decimals,
    unsigned long *rounded)
{
    float scaled;
    float fraction;
    unsigned long lower;

    scaled = value * power10(decimals);
    if (scaled > 4294967295.0f)
    {
        return 0;
    }

    lower = (unsigned long)scaled;
    fraction = scaled - (float)lower;
    /*
     * Binary32 often turns an exact decimal such as 1/3 into ...3333333.5
     * after scaling.  Treat an apparent half produced at a fractional
     * precision as the lower digit; exact integer results remain unchanged.
     */
    if (decimals > 0 && fraction >= 0.49f && fraction <= 0.51f)
    {
        *rounded = lower;
    }
    else if (fraction >= 0.5f)
    {
        *rounded = lower + 1UL;
    }
    else
    {
        *rounded = lower;
    }
    return 1;
}

void DisplayFormatter_FormatResult(
    Data4DigitalTube *target,
    unsigned char is_error,
    float value)
{
    unsigned char negative;
    float magnitude;
    unsigned char exponent;
    unsigned char decimals;
    unsigned char available_digits;
    unsigned char integer_digits;
    unsigned char total_digits;
    unsigned long rounded;
    unsigned long integer_limit;
    unsigned char text[DISPLAY_MAX_TEXT];
    unsigned char length;
    unsigned char integer_text_length;
    unsigned char i;
    unsigned long divisor;
    signed char point_index;

    clear_frame(target);
    if (is_error || float_is_invalid(value))
    {
        target->is_error = 1;
        return;
    }

    negative = value < 0.0f;
    magnitude = negative ? -value : value;
    if (magnitude == 0.0f)
    {
        text[0] = 0;
        copy_text_to_frame(target, text, 1, -1);
        return;
    }

    exponent = decimal_exponent(magnitude);
    available_digits = (unsigned char)(DISPLAY_TUBE_COUNT - (negative ? 1 : 0));

    if (exponent > 0)
    {
        integer_digits = (unsigned char)(exponent + 1);
        if (integer_digits > available_digits)
        {
            target->is_error = 1;
            return;
        }
        decimals = 0;
        if (integer_digits < RESULT_SIG_FIGS)
        {
            decimals = (unsigned char)(RESULT_SIG_FIGS - integer_digits);
        }
        if (decimals > RESULT_MAX_DECIMALS)
        {
            decimals = RESULT_MAX_DECIMALS;
        }
    }
    else if (magnitude >= 1.0f)
    {
        /* One integer digit leaves six fractional digits for seven
         * significant digits total. */
        integer_digits = 1;
        decimals = RESULT_SIG_FIGS - 1;
    }
    else
    {
        /* Values below one use the available fractional precision. */
        decimals = negative ? 6 : 7;
        integer_digits = 1;
    }

    while (1)
    {
        if (!rounded_digits(magnitude, decimals, &rounded))
        {
            if (decimals == 0)
            {
                target->is_error = 1;
                return;
            }
            --decimals;
            continue;
        }

        integer_limit = power10(decimals);
        if (rounded >= integer_limit)
        {
            total_digits = count_unsigned_long_digits(rounded / integer_limit);
            if (total_digits > available_digits)
            {
                target->is_error = 1;
                return;
            }
        }
        break;
    }

    if (rounded == 0UL)
    {
        text[0] = 0;
        copy_text_to_frame(target, text, 1, -1);
        return;
    }

    length = 0;
    if (negative)
    {
        text[length++] = DISPLAY_MINUS;
    }
    divisor = power10(decimals);
    if (decimals == 0)
    {
        length = append_unsigned_long_digits(text, length, rounded);
        point_index = -1;
    }
    else
    {
        length = append_unsigned_long_digits(text, length, rounded / divisor);
        integer_text_length = length;
        point_index = (signed char)(length - 1);
        divisor /= 10UL;
        for (i = 0; i < decimals; ++i)
        {
            text[length++] = (unsigned char)((rounded / divisor) % 10UL);
            divisor /= 10UL;
        }
        /* Remove only fractional trailing zeroes.  The zero at the end of
         * an integer such as 20 must remain part of the displayed value. */
        while (length > integer_text_length && text[length - 1] == 0)
        {
            --length;
        }
        if (length == integer_text_length)
        {
            point_index = -1;
        }
        if (length <= (negative ? 2 : 1))
        {
            if ((!negative && length == 1 && text[0] != 0)
                || (negative && length == 2 && text[1] != 0))
            {
                point_index = -1;
            }
            else if (negative)
            {
                text[1] = 0;
                length = 2;
            }
            else
            {
                text[0] = 0;
                length = 1;
            }
            point_index = -1;
        }
    }

    copy_text_to_frame(target, text, length, point_index);
}
