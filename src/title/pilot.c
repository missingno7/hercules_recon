/* Provisional offset evidence, not recovered historical declarations/names. */
typedef struct LinkNode {
    unsigned char unknown_000[0x50];
    unsigned long flags_050;
    unsigned char unknown_054[0x90];
    struct LinkNode *prev_0e4;
    struct LinkNode *next_0e8;
    unsigned char unknown_0ec[0x40];
    struct LinkNode *prev_12c;
    struct LinkNode *next_130;
} LinkNode;

void unlink_12c(LinkNode *p)
{
    if (p->prev_12c) p->prev_12c->next_130 = p->next_130;
    if (p->next_130) p->next_130->prev_12c = p->prev_12c;
}

void unlink_0e4(LinkNode *p)
{
    if (!(p->flags_050 & 0x80000)) {
        if (p->prev_0e4) p->prev_0e4->next_0e8 = p->next_0e8;
        if (p->next_0e8) p->next_0e8->prev_0e4 = p->prev_0e4;
        p->prev_0e4 = 0;
        p->next_0e8 = 0;
    }
}

typedef struct ColourInput {
    unsigned char unknown_000[0x4a];
    unsigned short intensity_04a;
    unsigned char unknown_04c[8];
    unsigned long flags_054;
} ColourInput;

unsigned long make_colour(ColourInput *p)
{
    int intensity = p->intensity_04a;
    unsigned long result;
    if (intensity < 0) intensity = 0;
    else if (intensity > 255) intensity = 255;
    result = (((intensity | 0x2c00) << 8) | intensity) << 8 | intensity;
    if (p->flags_054 & 4) result |= 0x02000000;
    return result;
}

int sentinel_count(short *p)
{
    int count = 0;
    while (*p != -1) { ++p; ++count; }
    return count - 1;
}
