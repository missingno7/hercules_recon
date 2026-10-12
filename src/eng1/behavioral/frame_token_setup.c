/*
 * Private ordinary-C reconstruction of the ENG1 frame token setup chain.
 *
 * The target call/data contract is recorded in
 * evidence/frame_configuration_contract.json and
 * evidence/resource_writer_layout.json.  Address labels are evidence labels;
 * this source does not claim original names, TU ownership, or module layout.
 */
typedef short Eng1TokenReference[2];

extern Eng1TokenReference g_actor_token_references[32]; /* 0x727e0, 32 x 4 */
extern void *g_frame_owner_7348c;                       /* owner slot 0x7348c */
extern unsigned char g_frame_global_73498;              /* byte cell 0x73498 */

/* 0x734ec is written as one byte by the 0x5710 initializer.  Its wider
 * aggregate or source ownership is not inferred here. */
extern unsigned char g_frame_setup_result_734ec;
extern unsigned short g_frame_seed_words_4d818[32];

void __cdecl dispatch_local_05780(const unsigned short *source,
                                  unsigned long slot,
                                  unsigned long word_count);

unsigned char __cdecl dispatch_local_05660(unsigned long key,
                                           unsigned long option)
{
    unsigned long index;
    unsigned short *row;
    unsigned short token_flags;

    token_flags = (unsigned short)(((option & 0x80UL) | 2UL) >> 1);
    row = (unsigned short *)&g_actor_token_references[0];
    for (index = 0; index < 32UL; ++index) {
        if (row[index * 2UL + 1UL] == (unsigned short)key) {
            ++row[index * 2UL];
            return (unsigned char)index;
        }
    }

    for (index = 0; index < 32UL; ++index) {
        if (row[index * 2UL] == 0) {
            row[index * 2UL + 1UL] = (unsigned short)key;
            row[index * 2UL] = token_flags;
            return (unsigned char)(index | 0x80UL);
        }
    }

    return 0xffU;
}

void __cdecl dispatch_local_05780(const unsigned short *source,
                                  unsigned long slot,
                                  unsigned long word_count)
{
    unsigned short *destination;
    unsigned long index;

    destination = (unsigned short *)((unsigned char *)g_frame_owner_7348c +
                                     slot * 0x200UL);
    if (word_count == 0x100UL) {
        for (index = 0; index < 0x100UL; ++index)
            destination[index] = source[index];
    } else {
        destination[0] = 0;
        if (word_count > 1UL) {
            for (index = 1; index < word_count; ++index)
                destination[index] = (unsigned short)(source[index] | 0x8000U);
        }

        /* The native tail is an overlapping forward copy: for count N>0,
         * destination[N..255] is extended by repeating the already-written
         * N-word prefix. Each source index is index-N, including values that
         * the preceding tail iteration has just produced. */
        if (word_count == 0UL) {
            /* Native code self-copies words 1..256.  Word 256 is the next
             * row's first word; all bytes retain their values.  Preserve the
             * exact net memory effect without reading uninitialized storage. */
        } else if (word_count < 0x100UL) {
            for (index = word_count; index < 0x100UL; ++index)
                destination[index] = destination[index - word_count];
        }
    }

    g_frame_global_73498 = 1;
}

unsigned char __cdecl dispatch_local_056c0(const unsigned short *source,
                                           unsigned long key,
                                           unsigned long word_count,
                                           unsigned long option)
{
    unsigned char token;

    token = dispatch_local_05660(key, option);
    if ((token & 0x80U) != 0) {
        unsigned long slot;

        slot = (unsigned long)(token & 0x7fU);
        dispatch_local_05780(source, slot, word_count);
        token = (unsigned char)slot;
    }
    return token;
}

void __cdecl dispatch_local_05710(void)
{
    unsigned long index;
    unsigned char result;

    for (index = 0; index < 32UL; ++index) {
        g_actor_token_references[index][0] = 0;
        g_actor_token_references[index][1] = -1;
    }

    result = dispatch_local_056c0(g_frame_seed_words_4d818,
                                  0x7641UL, 0x20UL, 5UL);
    g_frame_setup_result_734ec = result;
}
