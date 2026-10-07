typedef unsigned long U32;
typedef signed long S32;
typedef char dword_is_32_bits[(sizeof(U32) == 4) ? 1 : -1];
typedef char long_is_32_bits[(sizeof(S32) == 4) ? 1 : -1];

void __cdecl reverse_dword_copy(void *destination,
                                const void *source,
                                U32 byte_count)
{
    S32 dword_count = (S32)(byte_count + 3UL) >> 2;
    S32 group_count;
    U32 *destination_end = (U32 *)destination + dword_count;
    const U32 *source_end = (const U32 *)source + dword_count;

    if (dword_count > 7) {
        group_count = (unsigned int)dword_count >> 3;
        do {
            U32 value0;
            U32 value1;
            U32 value2;
            U32 value3;
            U32 value4;
            U32 value5;
            U32 value6;
            U32 value7;

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
