/* HERCULES.EXE relocation-free helpers. Provisional semantic names;
 * not a recovered original translation unit. */
typedef struct Slot { long a; unsigned short b; long c; unsigned short d; } Slot;

void reset_slots(Slot *slot, unsigned char count)
{
    while (count--) {
        slot->a = 0; slot->b = 0xffff; slot->c = 0; slot->d = 0;
        slot++;
    }
}

unsigned int reverse_low_bits(unsigned int value, unsigned char count)
{
    unsigned int result = 0;
    while (count--) {
        result <<= 1;
        if (value & 1) result |= 1;
        value >>= 1;
    }
    return result;
}

int name_hash(const char *name)
{
    int length, sum = 0, shift = 0;
    for (length = 0; name[length]; length++) {
        sum += name[length] << shift;
        shift += 8;
        if (shift > 24) shift = 0;
    }
    return sum + length;
}

int identity_value(int value)
{
    return value;
}

short return_true(int unused)
{
    return 1;
}

short return_false(int unused)
{
    return 0;
}
