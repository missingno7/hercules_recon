/* Behavioral context and typed external bindings needed by title_1c500.
   Field offsets/signedness are copied from candidates/w1c500b/best.body.c;
   this declares dependencies only and creates no storage. */
typedef struct TallyState {
    signed char level_000;
    unsigned char unknown_001[0x0b];
    signed char unknown_00c;
    signed char unknown_00d;
    signed char unknown_00e;
    unsigned char unknown_00f;
    unsigned char unknown_010;
    unsigned char unknown_011;
    unsigned char unknown_012[2];
    unsigned short unknown_014;
    unsigned short unknown_016;
    unsigned short unknown_018;
    unsigned short unknown_01a;
    unsigned short unknown_01c;
    unsigned char unknown_01e[0x12];
    unsigned long flags_030;
} TallyState;

#define TS ((TallyState *)g_engine_interface.context_004)

/* Tally-only state absent from the previously recovered u197f0 declarations;
   candidate provenance and the target unit's native private BSS support these. */
static int g_2afc0;
static int g_2b1d4;
static int g_2b088;
static int g_2b310;
static int g_2afd8;
static int g_2b314;

extern int g_2b36c;       /* external cell; provider/ownership is not inferred here */
extern int g_25a30[];     /* existing 42x6 table in u0cc90; no extra row is defined */
extern char g_29128[];    /* shared loader buffer, defined by r01000.c */
extern char *g_screen_files[]; /* title_files.c / title_files.h provider */

void title_102f0(TitleProc *self);
void title_164b0(TitleProc *self);



void title_1c500(TitleProc *self)
{
    void *local;
    int x = ((ScreenContext *)g_engine_interface.context_004)->cursor_5c;
    unsigned long buttons = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5e;
    int i;
    int k;

    switch (self->state_04) {
    case 1:
        switch (g_2b1d0) {
        case 0:
            title_01de0(0x140, 0, g_2afe4, 0, 0);
            if (g_2afe4 < 0x80) {
                g_2afe4 += 8;
            } else {
                g_2b1d0 = 1;
            }
            break;
        case 1:
            title_01de0(0x140, 0, g_2afe4, 0, 0);
            break;
        case 2:
            title_01de0(0x140, 0, g_2afe4, 0, 0);
            if (!(buttons & 0x4000)) {
                g_2b1d0 = 3;
            }
            break;
        case 3:
            title_01de0(0x140, 0, g_2afe4, 0, 0);
            if (buttons & 0x4000) {
                title_054f0(0x302, 0);
                g_2afe8 = 0x80;
                title_0c4f0(1);
                g_2b1d0 = 4;
            }
            break;
        case 4:
            title_01de0(0x140, 0, g_2afe4, 0, 0);
            if (g_2afe4 > 0) {
                g_2afe4 -= 8;
            } else {
                g_2b1d0 = 0;
                g_2b178 = 0;
                g_2afe4 = 0;
                title_1c240();
                g_2b100 = 0;
                if (TS->unknown_011 != 0xf) {
                    self->state_04 = 0xfffe;
                    self->delay_08 = 1;
                    return;
                }
                self->state_04 = 2;
                self->delay_08 = 2;
                return;
            }
            break;
        case 5:
            title_01de0(0x140, 0, g_2afe4, 0, 0);
            if (g_2afe4 > 0) {
                g_2afe4 -= 8;
            } else {
                g_2b1d0 = 0;
                g_2b178 = 0;
                g_2afe4 = 0;
                g_2b100 = 0;
                self->state_04 = 2;
                self->delay_08 = 2;
                return;
            }
            break;
        }
        if (g_2afe8 > 0) {
            title_028c0(0, 0, 0x140, 0x100, 0xff, 0xff, 0xff, g_2afe8, 1, 0, 0x4fe);
            g_2afe8 -= 8;
        } else {
            g_2afe8 = 0;
        }
        title_1bc60();
        title_1b710();
        title_19870(0x23f);
        title_1a630();
        title_1a3a0();
        title_19b40();
        title_1bdc0();
        title_1beb0();
        title_1a180();
        title_1a7c0();
        title_19c90();
        title_19fa0();
        title_1b150();
        title_1b900();
        if (g_2b1d0 == 1 && (buttons & 0x4000)) {
            g_2b1d0 = 4;
            title_054f0(0x302, 0);
            g_2afe8 = 0x80;
        }
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 2:
        title_0c9c0(0x4000, 8);
        switch (TS->level_000) {
        case 1:
            title_0c970(2, TS->unknown_00e);
            break;
        case 2:
            title_0c970(3, TS->unknown_00e);
            break;
        case 3:
            title_0c970(5, TS->unknown_00e);
            break;
        case 5:
            title_0c970(6, TS->unknown_00e);
            break;
        case 6:
            title_0c970(7, TS->unknown_00e);
            break;
        case 7:
            title_0c970(8, TS->unknown_00e);
            break;
        case 8:
            title_0c970(9, TS->unknown_00e);
            break;
        case 9:
            title_0c970(10, TS->unknown_00e);
            break;
        case 10:
            title_0c970(11, TS->unknown_00e);
            break;
        case 11:
            title_0c970(11, TS->unknown_00e);
            break;
        }
        switch (g_2b1d0) {
        case 0:
            title_01de0(0x140, 0, g_2afe4, 0, 0);
            if (g_2afe4 < 0x80) {
                g_2afe4 += 8;
            } else {
                g_2b1d0 = 1;
            }
            break;
        case 1:
            title_01de0(0x140, 0, g_2afe4, 0, 0);
            break;
        case 2:
            title_01de0(0x140, 0, g_2afe4, 0, 0);
            if (!(buttons & 0x4000)) {
                g_2b1d0 = 3;
            }
            break;
        case 3:
            title_01de0(0x140, 0, g_2afe4, 0, 0);
            if (buttons & 0x4000) {
                title_054f0(0x302, 0);
                g_2afe8 = 0x80;
                title_0c4f0(1);
                g_2b1d0 = 4;
            }
            break;
        case 4:
            title_01de0(0x140, 0, g_2afe4, 0, 0);
            if (g_2afe4 > 0) {
                g_2afe4 -= 8;
            } else {
                g_2b1d0 = 0;
                g_2afe4 = 0;
                title_1c430();
                self->state_04 = 0xfffe;
                self->delay_08 = 2;
                return;
            }
            break;
        case 5:
            title_01de0(0x140, 0, g_2afe4, 0, 0);
            if (g_2afe4 > 0) {
                g_2afe4 -= 8;
            } else {
                g_2b1d0 = 0;
                g_2afe4 = 0;
                title_1c430();
                self->state_04 = 3;
                self->delay_08 = 2;
                return;
            }
            break;
        }
        if (g_2afe8 > 0) {
            title_028c0(0, 0, 0x140, 0x100, 0xff, 0xff, 0xff, g_2afe8, 1, 0, 0x4fe);
            g_2afe8 -= 8;
        } else {
            g_2afe8 = 0;
        }
        title_1bcf0();
        title_19870(0x23f);
        title_1ac90();
        title_1bab0();
        g_2b108->unknown_034 = 0x59;
        g_2b05c->unknown_034 = 0x59;
        if ((g_2b1d0 == 1 || g_2b1d0 == 2 || g_2b1d0 == 3) && (buttons & 8)) {
            g_2b0ec = 0;
            g_2aff8[0] = 6;
            g_2aff8[1] = 6;
            g_2aff8[2] = 6;
            g_2aff8[3] = 6;
            switch (TS->level_000) {
            case 1:
                title_0c9b0(((2 << 8 | TS->unknown_00e) << 8) | TS->unknown_00c);
                break;
            case 2:
                title_0c9b0(((3 << 8 | TS->unknown_00e) << 8) | TS->unknown_00c);
                break;
            case 3:
                title_0c9b0(((5 << 8 | TS->unknown_00e) << 8) | TS->unknown_00c);
                break;
            case 5:
                title_0c9b0(((6 << 8 | TS->unknown_00e) << 8) | TS->unknown_00c);
                break;
            case 6:
                title_0c9b0(((7 << 8 | TS->unknown_00e) << 8) | TS->unknown_00c);
                break;
            case 7:
                title_0c9b0(((8 << 8 | TS->unknown_00e) << 8) | TS->unknown_00c);
                break;
            case 8:
                title_0c9b0(((9 << 8 | TS->unknown_00e) << 8) | TS->unknown_00c);
                break;
            case 9:
                title_0c9b0(((10 << 8 | TS->unknown_00e) << 8) | TS->unknown_00c);
                break;
            case 10:
                title_0c9b0(((11 << 8 | TS->unknown_00e) << 8) | TS->unknown_00c);
                break;
            case 11:
                title_0c9b0(((11 << 8 | TS->unknown_00e) << 8) | TS->unknown_00c);
                break;
            }
            g_2b1d0 = 4;
            title_054f0(0x302, 0);
        }
        if (g_2b1d0 == 1 && (buttons & 0x4000)) {
            g_2aff8[0] = 6;
            g_2aff8[1] = 6;
            g_2b1d0 = 4;
            g_2aff8[2] = 6;
            g_2aff8[3] = 6;
            title_054f0(0x302, 0);
            g_2afe8 = 0x80;
        }
        self->state_04 = 2;
        self->delay_08 = 1;
        return;
    case 3:
        title_05db0(title_102f0, 0, 1, 0);
        self->state_04 = 4;
        self->delay_08 = 0;
        return;
    case 4:
        g_2b36c = 1;
        title_0c4e0();
        title_0c5a0();
        self->state_04 = 0xffff;
        self->delay_08 = 1;
        return;
    case 0xfffe:
        title_0c930(0x435);
        title_0c930(0x431);
        title_0c930(0x432);
        title_0c930(0x433);
        title_0c930(0x434);
        title_0c930(0x447);
        title_0c930(0x442);
        title_0c930(0x43d);
        title_0c930(0x3fe);
        title_0c930(0x402);
        title_0c930(0x404);
        title_0c930(0x40a);
        title_0c930(0x409);
        title_0c930(0x403);
        title_0c930(0x407);
        title_0c9c0(0, 0);
        title_0c4e0();
        title_0c5a0();
        title_04ea0(0xc);
        title_05e90(title_164b0, 0);
        title_05f10(self);
        return;
    case 0xffff:
        title_0c920(0x435);
        title_0c920(0x431);
        title_0c920(0x432);
        title_0c920(0x433);
        title_0c920(0x434);
        title_0c920(0x447);
        title_0c920(0x442);
        title_0c920(0x43d);
        title_0c920(0x3fe);
        title_0c920(0x402);
        title_0c920(0x404);
        title_0c920(0x40a);
        title_0c920(0x409);
        title_0c920(0x403);
        title_0c920(0x407);
        title_0c990(-6, 0);
        title_0c9c0(0x4000, 0x4000);
        title_1d770();
        local = 0;
        if (title_0c450(&local, 0x15000, 0) == 0) {
            title_016e0("\n Error Unable to malloc memory for tally screen ");
            title_05e90(title_164b0, 0);
            title_05f10(self);
        }
        title_0c3f0(g_29128, g_screen_files[18], local, 0x14312);
        title_01dd0(local);
        title_0c8b0((char *)local + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(local);
        title_04b70(0xc, 0);
        g_2af68 = 0;
        g_2b178 = 0;
        if (TS->flags_030 & 0x800000) {
            TS->unknown_011 = 0xf;
        }
        if (TS->unknown_011 == 0xf) {
            k = TS->unknown_00e * 12;
            g_2b0f0[0] = g_25a30[(k + TS->level_000) * 6 + 0];
            g_2b0f0[1] = g_25a30[(k + TS->level_000) * 6 + 1];
            g_2b0f0[2] = g_25a30[(k + TS->level_000) * 6 + 2];
            g_2b0f0[3] = g_25a30[(k + TS->level_000) * 6 + 3];
        } else {
            g_2b0f0[0] = 4;
            g_2b0f0[1] = 4;
            g_2b0f0[2] = 4;
            g_2b0f0[3] = 4;
        }
        g_2afc0 = 0;
        g_2b1d4 = 0;
        g_2b088 = 0xf;
        title_0c560(0, 4);
        title_197f0();
        for (i = 0; i < 8; i++) {
            g_2b330[i] = 0;
            if (TS->level_000 == 6 || TS->level_000 == 7) {
                g_2ae88[i] = 0xc;
            } else if (TS->level_000 == 11) {
                g_2ae88[i] = 0xc;
            } else {
                g_2ae88[i] = 0;
            }
        }
        g_2afcc = 0;
        g_2acf8 = 0;
        g_2b324 = 0;
        g_2acfc = 0;
        g_2b310 = 0;
        g_2afdc = 0;
        g_2b318 = 0;
        g_2afd8 = 0;
        g_2b314 = 0;
        g_2afe0 = 0;
        g_2adc4 = 0;
        g_2cc64 = 0;
        g_2afc4 = 0;
        g_2afec = 0;
        g_2aff8[0] = 0;
        g_2aff8[1] = 0;
        g_2aff8[2] = 0;
        g_2aff8[3] = 0;
        g_2b090 = 0;
        g_2b010 = 0;
        g_2afd4 = 0x3e80000;
        g_2b320 = 0x3e80000;
        g_2afe4 = 0;
        g_2afe8 = 0;
        g_2b1d0 = 0;
        g_2b0ec = 0;
        switch (TS->unknown_00e) {
        case 0:
            g_2b328 = 0x28;
            break;
        case 1:
            g_2b328 = 0x3c;
            break;
        case 2:
            g_2b328 = 0x50;
            break;
        }
        if (TS->unknown_01c == 0xff) {
            TS->unknown_010++;
        }
        switch (TS->level_000) {
        case 2:
            g_2aff0 = ((TS->unknown_01a + TS->unknown_016) == 0 ? 0 : (TS->unknown_018 + TS->unknown_014) * 100 / (TS->unknown_01a + TS->unknown_016)) / 2 + 50;
            break;
        case 3:
            g_2aff0 = ((TS->unknown_01a + TS->unknown_016) == 0 ? 0 : (TS->unknown_018 + TS->unknown_014) * 100 / (TS->unknown_01a + TS->unknown_016)) / 2 + 50;
            break;
        case 5:
            g_2aff0 = ((TS->unknown_01a + TS->unknown_016) == 0 ? 0 : (TS->unknown_018 + TS->unknown_014) * 100 / (TS->unknown_01a + TS->unknown_016)) / 2 + 50;
            break;
        case 4:
            g_2aff0 = ((TS->unknown_01a + TS->unknown_016) == 0 ? 0 : (TS->unknown_018 + TS->unknown_014) * 100 / (TS->unknown_01a + TS->unknown_016)) / 2 + 50;
            break;
        case 6:
            g_2aff0 = ((TS->unknown_01a + TS->unknown_016) == 0 ? 0 : (TS->unknown_018 + TS->unknown_014) * 100 / (TS->unknown_01a + TS->unknown_016)) / 2 + 50;
            break;
        case 7:
            g_2aff0 = ((TS->unknown_01a + TS->unknown_016) == 0 ? 0 : (TS->unknown_018 + TS->unknown_014) * 100 / (TS->unknown_01a + TS->unknown_016)) / 2 + 50;
            break;
        case 8:
            g_2aff0 = ((TS->unknown_01a + TS->unknown_016) == 0 ? 0 : (TS->unknown_018 + TS->unknown_014) * 100 / (TS->unknown_01a + TS->unknown_016)) / 2 + 50;
            break;
        case 9:
            g_2aff0 = ((TS->unknown_01a + TS->unknown_016) == 0 ? 0 : (TS->unknown_018 + TS->unknown_014) * 100 / (TS->unknown_01a + TS->unknown_016)) / 2 + 50;
            break;
        case 10:
            g_2aff0 = ((TS->unknown_01a + TS->unknown_016) == 0 ? 0 : (TS->unknown_018 + TS->unknown_014) * 100 / (TS->unknown_01a + TS->unknown_016)) / 2 + 50;
            break;
        case 11:
            g_2aff0 = ((TS->unknown_01a + TS->unknown_016) == 0 ? 0 : (TS->unknown_018 + TS->unknown_014) * 100 / (TS->unknown_01a + TS->unknown_016)) / 2 + 50;
            break;
        default:
            g_2aff0 = ((TS->unknown_01a + TS->unknown_016) == 0 ? 0 : (TS->unknown_018 + TS->unknown_014) * 100 / (TS->unknown_01a + TS->unknown_016)) / 2 + 50;
            break;
        }
        g_2aff0 -= TS->unknown_00d * 25;
        if (g_2aff0 < 0) {
            g_2aff0 = 0;
        }
        if (g_2aff0 > 100) {
            g_2aff0 = 100;
        }
        g_2b318 = (TS->unknown_01a + TS->unknown_016) == 0 ? 0 : (TS->unknown_018 + TS->unknown_014) * 100 / (TS->unknown_01a + TS->unknown_016);
        if (g_2b318 < 0) {
            g_2b318 = 0;
        }
        if (g_2b318 > 100) {
            g_2b318 = 100;
        }
        g_2b08c = 0;
        title_0c5d0(4);
        title_0c3c0();
        if (g_2b36c == 0) {
            if (g_2aff0 > g_2b328 && TS->unknown_00c < 9) {
                TS->unknown_00c++;
                g_2b08c = 1;
            }
        } else {
            g_2b1d0 = 5;
        }
        title_1d790(1);
        break;
    default:
        return;
    }
    self->state_04 = 1;
    self->delay_08 = 1;
}
