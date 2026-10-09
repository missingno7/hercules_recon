extern "C" {
typedef struct TitleSlot { unsigned char unknown_00[0x94]; } TitleSlot;
extern TitleSlot *g_2dfa0;
extern unsigned short g_2bf72;
void title_094c0(TitleSlot *slot);

void title_09480(void)
{
    int i;
    for (i = 0; i < g_2bf72; i++)
        title_094c0(&g_2dfa0[i]);
}
}
