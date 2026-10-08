extern int g_29134;
extern int g_29144;

void title_01a30(void)
{
    switch (g_29144) {
    case 0: if (g_29134 < 0x80) { g_29134 += 0x10; return; } break;
    case 1: if (g_29134 > 0) { g_29134 -= 0x10; return; } break;
    default: return;
    }
    g_29144 = 3;
}

void title_01dd0(void)
{
}
