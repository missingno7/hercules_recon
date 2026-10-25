/* Provisional layout, supported by TITLE RVA 0x1a2c0. */
typedef struct DigitSprite {
    char unknown_00[0x34];
    short frame;
    char unknown_36[0x54-0x36];
    unsigned long flags;
} DigitSprite;

void set_decimal_digits(DigitSprite *thousands, DigitSprite *hundreds,
                        DigitSprite *tens, DigitSprite *units, int value)
{
    int remainder, digit;
    digit = value / 1000;
    thousands->frame = digit;
    remainder = value - digit * 1000;
    digit = remainder / 100;
    hundreds->frame = digit;
    remainder -= digit * 100;
    digit = remainder / 10;
    tens->frame = digit;
    units->frame = remainder - digit * 10;
    thousands->frame += 90;
    hundreds->frame += 90;
    tens->frame += 90;
    units->frame += 90;
    if (value < 1000) thousands->flags &= 0x7fffffff;
    else thousands->flags |= 0x80000000;
    if (value < 100) hundreds->flags &= 0x7fffffff;
    else hundreds->flags |= 0x80000000;
    if (value < 10) tens->flags &= 0x7fffffff;
    else tens->flags |= 0x80000000;
}
