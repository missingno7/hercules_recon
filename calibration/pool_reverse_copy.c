typedef char dword_is_32_bits[(sizeof(unsigned long) == 4) ? 1 : -1];
typedef char int_is_32_bits[(sizeof(int) == 4) ? 1 : -1];

void reverse_copy_dwords(unsigned long *destination,
                         const unsigned long *source,
                         int byte_count)
{
    int dword_count = (byte_count + 3) >> 2;
    int group_count;
    unsigned long *destination_end = destination + dword_count;
    const unsigned long *source_end = source + dword_count;

    if (dword_count > 7) {
        group_count = dword_count >> 3;
        do {
            unsigned long value0;
            unsigned long value1;
            unsigned long value2;
            unsigned long value3;
            unsigned long value4;
            unsigned long value5;
            unsigned long value6;
            unsigned long value7;

            destination_end -= 8;
            source_end -= 8;
            value0 = source_end[0];
            value1 = source_end[1];
            value2 = source_end[2];
            value3 = source_end[3];
            value4 = source_end[4];
            value5 = source_end[5];
            value6 = source_end[6];
            value7 = source_end[7];
            destination_end[0] = value0;
            destination_end[1] = value1;
            destination_end[2] = value2;
            destination_end[3] = value3;
            destination_end[4] = value4;
            destination_end[5] = value5;
            destination_end[6] = value6;
            destination_end[7] = value7;
            dword_count -= 8;
        } while (--group_count != 0);
    }

    while (dword_count > 0) {
        --destination_end;
        --source_end;
        *destination_end = *source_end;
        --dword_count;
    }
}
