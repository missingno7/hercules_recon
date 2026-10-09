/* TITLE.DLL lane w03 region: functions that reached MASKED EQUAL, ascending RVA. */

static int g_29570;
static int g_29574;
static int g_29578;
static int g_2957c;
static int g_29580;
static int g_29584;

void title_0c330(int value);
void title_0c6d0(int a);
void title_0cc70(int a);
void title_0c670(void *p, int a, int b, int c);

void title_02350(void)
{
    if (g_29570) {
        title_0c330(g_29584);
        title_0c330(g_29574);
        title_0c330(g_29580);
        title_0c330(g_2957c);
        title_0c330(g_29578);
        title_0c330(g_29570);
        g_29570 = 0;
    }
}

void title_02a20(void)
{
    unsigned short rc[4];

    rc[0] = 0;
    rc[1] = 0;
    rc[2] = 0x140;
    rc[3] = 0x200;
    title_0c6d0(0);
    title_0cc70(0);
    title_0c670(rc, 0, 0, 0);
    title_0c6d0(0);
}
