/* 0x4fe0 initializes the 256 observed 12-byte sprite-cell records. */
typedef struct TitleCellInit {
    unsigned short mask_00;
    unsigned short w_02;
    unsigned short next_04[4];
} TitleCellInit;
extern TitleCellInit g_2d340[256];

#define TITLE_GET_TPAGE(tp, abr, x, y) \
    ((((tp) & 0x3) << 7) | (((abr) & 0x3) << 5) | \
     (((y) & 0x100) >> 4) | (((x) & 0x3ff) >> 6) | (((y) & 0x200) << 2))

void title_04fe0(void)
{
    int i;
    int c;
    TitleCellInit *p;

    for (p = g_2d340, i = 0; i < 256; i++, p++) {
        p->mask_00 = 0;
        c = ((i / 64) & 2) + ((i / 8) & 1);
        p->w_02 = TITLE_GET_TPAGE(1, 0, (c / 2) * 128 + 768, (c & 1) * 256);
    }
    g_2d340[0].mask_00 = 15;
    g_2d340[16].mask_00 = 15;
    g_2d340[4].mask_00 = 15;
    g_2d340[1].mask_00 = 15;
}
