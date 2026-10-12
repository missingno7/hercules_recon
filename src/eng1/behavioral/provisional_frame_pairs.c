/* Initialized row1 frame slots and opcode prefixes, from actual callback consumers. */
#include "provisional_frame_pairs.h"

/* RVA 0x63228: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_063228[5] = {
    { 54, 2 },
    { 55, 2 },
    { 56, 2 },
    { 55, 2 },
    { -1, 0 },
};

/* RVA 0x63248: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_063248[1] = {
    { 2, 999 },
};

/* RVA 0x6324c: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_06324c[1] = {
    { 3, 999 },
};

/* RVA 0x63250: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_063250[1] = {
    { 4, 999 },
};

/* RVA 0x63254: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_063254[1] = {
    { 5, 999 },
};

/* RVA 0x63258: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_063258[1] = {
    { 6, 999 },
};

/* RVA 0x6325c: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_06325c[1] = {
    { 9, 999 },
};

/* RVA 0x63260: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_063260[1] = {
    { 8, 999 },
};

/* RVA 0x63264: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_063264[1] = {
    { 7, 999 },
};

/* RVA 0x63a60: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_063a60[1] = {
    { 10, 999 },
};

/* RVA 0x63a64: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_063a64[1] = {
    { 12, 999 },
};

/* RVA 0x63ce0: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_063ce0[1] = {
    { 17, 999 },
};

/* RVA 0x63ce4: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_063ce4[1] = {
    { 16, 999 },
};

/* RVA 0x63ce8: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_063ce8[1] = {
    { 10, 999 },
};

/* RVA 0x63cec: 4-byte command records through native terminal or one explicitly readable record. */
Eng1CommandPair g_eng1_provisional_pair_063cec[1] = {
    { 12, 999 },
};

/* Observed slots;6323c has one explicitly selected slot, others end in native null/index. */
void *g_eng1_provisional_frames_06323c[1] = {
    (void *)(g_eng1_provisional_pair_063228 + 0), /* RVA 0x6323c */
};

/* Observed slots;6323c has one explicitly selected slot, others end in native null/index. */
void *g_eng1_provisional_frames_063268[10] = {
    (void *)(g_eng1_provisional_pair_063248 + 0), /* RVA 0x63268 */
    (void *)(g_eng1_provisional_pair_06324c + 0), /* RVA 0x6326c */
    (void *)(g_eng1_provisional_pair_063250 + 0), /* RVA 0x63270 */
    (void *)(g_eng1_provisional_pair_063254 + 0), /* RVA 0x63274 */
    (void *)(g_eng1_provisional_pair_063258 + 0), /* RVA 0x63278 */
    (void *)(g_eng1_provisional_pair_06325c + 0), /* RVA 0x6327c */
    (void *)(g_eng1_provisional_pair_063260 + 0), /* RVA 0x63280 */
    (void *)(g_eng1_provisional_pair_063264 + 0), /* RVA 0x63284 */
    0, /* RVA 0x63288 */
    0, /* RVA 0x6328c */
};

/* Observed slots;6323c has one explicitly selected slot, others end in native null/index. */
void *g_eng1_provisional_frames_063290[10] = {
    (void *)(g_eng1_provisional_pair_063248 + 0), /* RVA 0x63290 */
    (void *)(g_eng1_provisional_pair_06324c + 0), /* RVA 0x63294 */
    (void *)(g_eng1_provisional_pair_063250 + 0), /* RVA 0x63298 */
    (void *)(g_eng1_provisional_pair_063254 + 0), /* RVA 0x6329c */
    (void *)(g_eng1_provisional_pair_063258 + 0), /* RVA 0x632a0 */
    (void *)(g_eng1_provisional_pair_06325c + 0), /* RVA 0x632a4 */
    (void *)(g_eng1_provisional_pair_063260 + 0), /* RVA 0x632a8 */
    (void *)(g_eng1_provisional_pair_063264 + 0), /* RVA 0x632ac */
    0, /* RVA 0x632b0 */
    0, /* RVA 0x632b4 */
};

/* Observed slots;6323c has one explicitly selected slot, others end in native null/index. */
void *g_eng1_provisional_frames_063a68[4] = {
    (void *)(g_eng1_provisional_pair_063a60 + 0), /* RVA 0x63a68 */
    (void *)(g_eng1_provisional_pair_063a64 + 0), /* RVA 0x63a6c */
    0, /* RVA 0x63a70 */
    0, /* RVA 0x63a74 */
};

/* Observed slots;6323c has one explicitly selected slot, others end in native null/index. */
void *g_eng1_provisional_frames_063cf0[6] = {
    (void *)(g_eng1_provisional_pair_063ce0 + 0), /* RVA 0x63cf0 */
    (void *)(g_eng1_provisional_pair_063ce4 + 0), /* RVA 0x63cf4 */
    (void *)(g_eng1_provisional_pair_063ce8 + 0), /* RVA 0x63cf8 */
    (void *)(g_eng1_provisional_pair_063cec + 0), /* RVA 0x63cfc */
    0, /* RVA 0x63d00 */
    0, /* RVA 0x63d04 */
};
