/* New file-static state for title_0d910 (absent from the current owning TU). */
static int g_2a25c;
static int g_2a390;

/* One shared native table supplies this handler and TITLE entry initialization. */
extern int g_257e8[30];
extern int g_2bb34, g_2bb38;

/* Already recovered under its canonical source name in src/title/pilot.c. */
int sentinel_count(short *p);


void title_0d910(TitleObject *p)
{
    int s3, s2, s1, s0;
    int idx;

    if (g_29f98 != 0) {
        if (g_2a270 != 0) {
            title_0c4e0();
            g_2a270 = 0;
        }
        return;
    }
    switch (g_2a254) {
    case 0:
        p->unknown_000 += g_257e8[p->unknown_034++];
        if (p->unknown_034 == 0x16) {
            p->unknown_034 = 9;
            g_2a254 = 1;
        }
        break;
    case 1:
        p->unknown_000 += g_257e8[p->unknown_034++];
        if (p->unknown_034 == 0x1d) {
            p->unknown_034 = 0xa7;
            g_2a254 = 2;
        }
        break;
    case 2:
        p->unknown_034++;
        if (p->unknown_034 == 0xab) {
            g_2a270 = title_0c4d0(0);
            g_2a254 = 3;
        }
        break;
    case 3:
        p->unknown_034 = title_0cc90() + 0x2d;
        if (g_2a270 <= 0) {
            title_0c4e0();
            g_2a270 = 0;
            g_2a3ac = sentinel_count(g_25890);
            g_2a390 = 0;
            g_2a254 = 4;
        }
        g_2a270--;
        break;
    case 4:
        switch (g_25a28) {
        case 1: p->unknown_000 = 0xffb30000; break;
        case 2: p->unknown_000 = 0xfff40000; break;
        case 3: p->unknown_000 = 0x350000; break;
        }
        p->unknown_054 &= ~0x10;
        if (g_25890[g_2a3ac] != -1) {
            p->unknown_034 = g_25890[g_2a3ac] + 0xa6;
            g_2a3ac++;
            g_2a390 = 0;
        }
        g_2a390++;
        if (g_2a390 == 0x3c) {
            p->unknown_034 = 0x21;
            g_2a254 = 6;
            g_2a25c = 0;
            g_2a3ac = 0;
        }
        title_0d720();
        break;
    case 6:
        switch (g_2a25c) {
        case 0:
            p->unknown_034 = g_25968[g_2a3ac] + 0x20;
            g_2a3ac++;
            while (g_25968[g_2a3ac] == 999) {
                title_054f0(g_25968[g_2a3ac + 1], 0);
                g_2a3ac += 2;
            }
            if (g_25968[g_2a3ac] == -1) {
                g_2a25c = 1;
                g_2a3ac = 0;
                g_2a390 = 0;
            }
            break;
        case 1:
            p->unknown_034 = g_259b8[g_2a3ac] + 0x20;
            while (g_259b8[++g_2a3ac] == 999) {
                title_054f0(g_259b8[g_2a3ac + 1], 0);
                g_2a3ac++;
            }
            if (g_259b8[g_2a3ac] == -1) {
                g_2a25c = 1;
                g_2a3ac = 1;
            }
            g_2a390++;
            if (g_2a390 >= 0x3c) {
                g_2a25c = 4;
                g_2a3ac = 0;
            }
            break;
        case 4:
            p->unknown_034 = g_259d0[g_2a3ac] + 0x20;
            g_2a3ac++;
            while (g_259d0[g_2a3ac] == -1) {
                g_2a25c = 2;
                g_2a3ac = 0;
            }
            break;
        case 2:
            title_0d6c0();
            g_2a25c = 3;
            g_2a3ac = 0;
            break;
        case 3:
            p->unknown_034 = title_0cc90() + 0x2d;
            if (g_2a270 <= 0) {
                title_0c4e0();
                g_2a270 = 0;
                g_2a25c = 0;
                g_2a390 = 0;
                g_2a3ac = 0;
            }
            g_2a270--;
            break;
        }
        title_0d720();
        break;
    case 5:
        switch (g_25a28) {
        case 0: p->unknown_000 = 0xffe20000; break;
        case 1: p->unknown_000 = 0x230000; break;
        case 2: p->unknown_000 = 0x630000; break;
        }
        p->unknown_054 |= 0x10;
        if (g_25890[g_2a3ac] != -1) {
            p->unknown_034 = g_25890[g_2a3ac] + 0xa6;
            g_2a3ac++;
            g_2a390 = 0;
        }
        g_2a390++;
        if (g_2a390 == 0xa) {
            p->unknown_034 = 0x21;
            g_2a254 = 7;
            g_2a25c = 0;
            g_2a3ac = 0;
        }
        title_0d4b0();
        break;
    case 7:
        switch (g_2a25c) {
        case 0:
            p->unknown_034 = g_25968[g_2a3ac] + 0x20;
            g_2a3ac++;
            while (g_25968[g_2a3ac] == 999) {
                title_054f0(g_25968[g_2a3ac + 1], 0);
                g_2a3ac += 2;
            }
            if (g_25968[g_2a3ac] == -1) {
                g_2a25c = 1;
                g_2a3ac = 0;
                g_2a390 = 0;
            }
            break;
        case 1:
            p->unknown_034 = g_259b8[g_2a3ac] + 0x20;
            while (g_259b8[++g_2a3ac] == 999) {
                title_054f0(g_259b8[g_2a3ac + 1], 0);
                g_2a3ac++;
            }
            if (g_259b8[g_2a3ac] == -1) {
                g_2a25c = 1;
                g_2a3ac = 1;
            }
            g_2a390++;
            if (g_2a390 >= 0x3c) {
                title_0d6c0();
                g_2a25c = 4;
                g_2a3ac = 0;
            }
            break;
        case 4:
            p->unknown_034 = g_259d0[g_2a3ac] + 0x20;
            g_2a3ac++;
            while (g_259d0[g_2a3ac] == -1) {
                g_2a25c = 2;
                g_2a3ac = 0;
            }
            break;
        case 2:
            p->unknown_034 = title_0cc90() + 0x2d;
            if (g_2a270 <= 0) {
                title_0c4e0();
                g_2a270 = 0;
                g_2a25c = 0;
                g_2a390 = 0;
                g_2a3ac = 0;
            }
            g_2a270--;
            g_2a3ac++;
            break;
        }
        title_0d4b0();
        break;
    case 26:
        p->unknown_034 = g_259e8[g_2a3ac] + 0x20;
        if (g_259e8[g_2a3ac] != 9 || !(g_2a284 & 0x80))
            g_2a3ac++;
        if (g_259e8[g_2a3ac] == -1) {
            g_2a3ac = sentinel_count(g_25890);
            g_2a254 = g_2a3a0;
            g_2a390 = 0;
        }
        break;
    case 27:
        p->unknown_034 = g_259e8[g_2a3ac] + 0x20;
        if (g_259e8[g_2a3ac] != 9 || !(g_2a284 & 0x20))
            g_2a3ac++;
        if (g_259e8[g_2a3ac] == -1) {
            g_2a3ac = sentinel_count(g_25890);
            g_2a254 = g_2a3a0;
            g_2a390 = 0;
        }
        break;
    case 8:
        if (g_25900[g_2a3ac] != -1 && g_2a39c != 1) {
            p->unknown_034 = g_25900[g_2a3ac] + 0xa6;
            g_2a3ac++;
            if (!(g_2a3ac & 1)) {
                if (p->unknown_054 & 0x10) {
                    switch (g_25a28) {
                    case 0:
                        g_2a278->unknown_034++;
                        if (g_2a278->unknown_034 >= 0x23f)
                            g_2a278->unknown_034 = 0x1e7;
                        break;
                    case 1:
                        g_2a27c->unknown_034++;
                        if (g_2a27c->unknown_034 >= 0x17d)
                            g_2a27c->unknown_034 = 0x125;
                        break;
                    case 2:
                        g_2a280->unknown_034++;
                        if (g_2a280->unknown_034 >= 0x1de)
                            g_2a280->unknown_034 = 0x186;
                        break;
                    case 3:
                        g_2a274->unknown_034++;
                        if (g_2a274->unknown_034 >= 0x11c)
                            g_2a274->unknown_034 = 0xc4;
                        break;
                    }
                } else {
                    switch (g_25a28) {
                    case 0:
                        g_2a278->unknown_034--;
                        if (g_2a278->unknown_034 <= 0x1e7)
                            g_2a278->unknown_034 = 0x23f;
                        break;
                    case 1:
                        g_2a27c->unknown_034--;
                        if (g_2a27c->unknown_034 <= 0x125)
                            g_2a27c->unknown_034 = 0x17d;
                        break;
                    case 2:
                        g_2a280->unknown_034--;
                        if (g_2a280->unknown_034 <= 0x186)
                            g_2a280->unknown_034 = 0x1de;
                        break;
                    case 3:
                        g_2a274->unknown_034--;
                        if (g_2a274->unknown_034 <= 0xc4)
                            g_2a274->unknown_034 = 0x11c;
                        break;
                    }
                }
                switch (g_25a28) {
                case 0: g_2a39c = g_2a3b8[g_2a278->unknown_034 - 0x1e3]; break;
                case 1: g_2a39c = g_2a538[g_2a27c->unknown_034 - 0x121]; break;
                case 2: g_2a39c = g_2a6b8[g_2a280->unknown_034 - 0x182]; break;
                case 3: g_2a39c = g_2a838[g_2a274->unknown_034 - 0xc0]; break;
                }
            }
        } else {
            g_2a3ac = sentinel_count(g_25890);
            g_2a254 = g_2a3a0;
            g_2a390 = 0;
            g_2a39c = 0;
            if ((g_2a284 & 0x2000) || (g_2a284 & 0x8000)) {
                g_2a3ac = 0;
                if (!(p->unknown_054 & 0x10)) {
                    if (g_2a284 & 0x2000)
                        g_2a254 = 9;
                    else
                        g_2a254 = 8;
                } else {
                    if (g_2a284 & 0x2000)
                        g_2a254 = 8;
                    else
                        g_2a254 = 9;
                }
                switch (g_25a28) {
                case 0: title_054f0(0x4311, g_2a278); break;
                case 1: title_054f0(0x4311, g_2a27c); break;
                case 2: title_054f0(0x4311, g_2a280); break;
                case 3: title_054f0(0x4311, g_2a274); break;
                }
            }
        }
        break;
    case 9:
        if (g_258b0[g_2a3ac] != -1 && g_2a39c != 1) {
            p->unknown_034 = g_258b0[g_2a3ac] + 0xa6;
            g_2a3ac++;
            if (!(g_2a3ac & 1)) {
                if (!(p->unknown_054 & 0x10)) {
                    switch (g_25a28) {
                    case 0:
                        g_2a278->unknown_034++;
                        if (g_2a278->unknown_034 >= 0x23f)
                            g_2a278->unknown_034 = 0x1e7;
                        break;
                    case 1:
                        g_2a27c->unknown_034++;
                        if (g_2a27c->unknown_034 >= 0x17d)
                            g_2a27c->unknown_034 = 0x125;
                        break;
                    case 2:
                        g_2a280->unknown_034++;
                        if (g_2a280->unknown_034 >= 0x1de)
                            g_2a280->unknown_034 = 0x186;
                        break;
                    case 3:
                        g_2a274->unknown_034++;
                        if (g_2a274->unknown_034 >= 0x11c)
                            g_2a274->unknown_034 = 0xc4;
                        break;
                    }
                } else {
                    switch (g_25a28) {
                    case 0:
                        g_2a278->unknown_034--;
                        if (g_2a278->unknown_034 <= 0x1e7)
                            g_2a278->unknown_034 = 0x23f;
                        break;
                    case 1:
                        g_2a27c->unknown_034--;
                        if (g_2a27c->unknown_034 <= 0x125)
                            g_2a27c->unknown_034 = 0x17d;
                        break;
                    case 2:
                        g_2a280->unknown_034--;
                        if (g_2a280->unknown_034 <= 0x186)
                            g_2a280->unknown_034 = 0x1de;
                        break;
                    case 3:
                        g_2a274->unknown_034--;
                        if (g_2a274->unknown_034 <= 0xc4)
                            g_2a274->unknown_034 = 0x11c;
                        break;
                    }
                }
                switch (g_25a28) {
                case 0: g_2a39c = g_2a3b8[g_2a278->unknown_034 - 0x1e3]; break;
                case 1: g_2a39c = g_2a538[g_2a27c->unknown_034 - 0x121]; break;
                case 2: g_2a39c = g_2a6b8[g_2a280->unknown_034 - 0x182]; break;
                case 3: g_2a39c = g_2a838[g_2a274->unknown_034 - 0xc0]; break;
                }
            }
        } else {
            g_2a3ac = sentinel_count(g_25890);
            g_2a254 = g_2a3a0;
            g_2a390 = 0;
            if ((g_2a284 & 0x2000) || (g_2a284 & 0x8000)) {
                g_2a3ac = 0;
                g_2a39c = 0;
                if (!(p->unknown_054 & 0x10)) {
                    if (g_2a284 & 0x2000)
                        g_2a254 = 9;
                    else
                        g_2a254 = 8;
                } else {
                    if (g_2a284 & 0x2000)
                        g_2a254 = 8;
                    else
                        g_2a254 = 9;
                }
                switch (g_25a28) {
                case 0: title_054f0(0x4311, g_2a278); break;
                case 1: title_054f0(0x4311, g_2a27c); break;
                case 2: title_054f0(0x4311, g_2a280); break;
                case 3: title_054f0(0x4311, g_2a274); break;
                }
            }
        }
        break;
    case 18:
        if (g_25950[g_2a3ac] != -1) {
            p->unknown_034 = g_25950[g_2a3ac] + 0xa6;
            g_2a3ac++;
        } else {
            g_2a254 = 0x13;
            p->unknown_034 = 2;
        }
        break;
    case 19:
        p->unknown_034++;
        p->unknown_000 += g_257e8[p->unknown_034];
        if (p->unknown_034 == 0x1a)
            g_25a28++;
        if (p->unknown_034 == 0x1d) {
            g_2a3ac = 0;
            g_2a254 = 4;
            g_2a390 = 0;
        }
        break;
    case 10:
        if (g_25950[g_2a3ac] != -1) {
            p->unknown_034 = g_25950[g_2a3ac] + 0xa6;
            g_2a3ac++;
        } else {
            g_2a254 = 0xb;
            p->unknown_034 = 2;
        }
        break;
    case 11:
        p->unknown_034++;
        p->unknown_000 -= g_257e8[p->unknown_034];
        if (p->unknown_034 == 0x1a)
            g_25a28--;
        if (p->unknown_034 == 0x1d) {
            g_2a3ac = 0;
            g_2a254 = 5;
            g_2a390 = 0;
        }
        break;
    case 15:
        if (g_25950[g_2a3ac] != -1) {
            p->unknown_034 = g_25950[g_2a3ac] + 0xa6;
            g_2a3ac++;
        } else {
            g_2a254 = 0x10;
            p->unknown_034 = 0xc;
        }
        break;
    case 16:
        p->unknown_034++;
        p->unknown_000 += g_257e8[p->unknown_034];
        if (p->unknown_034 == 0x1d) {
            g_2a3ac = 0;
            g_2a254 = 0x11;
        }
        break;
    case 17:
        if (g_25860[g_2a3ac] != -1) {
            p->unknown_034 = g_25860[g_2a3ac] + 0x1c;
            g_2a3ac++;
        } else {
            g_2a254 = 5;
            g_2a3ac = 0;
            g_25a28--;
        }
        break;
    case 12:
        if (g_25950[g_2a3ac] != -1) {
            p->unknown_034 = g_25950[g_2a3ac] + 0xa6;
            g_2a3ac++;
        } else {
            g_2a254 = 0xd;
            p->unknown_034 = 0xc;
        }
        break;
    case 13:
        p->unknown_034++;
        p->unknown_000 -= g_257e8[p->unknown_034];
        if (p->unknown_034 == 0x1d) {
            g_2a3ac = 0;
            g_2a254 = 0xe;
        }
        break;
    case 14:
        p->unknown_054 &= ~0x10;
        if (g_25878[g_2a3ac] != -1) {
            p->unknown_034 = g_25878[g_2a3ac] + 0x1c;
            g_2a3ac++;
        } else {
            g_2a254 = 4;
            g_2a3ac = 0;
            g_25a28++;
        }
        break;
    case 20:
        ((TitleCtxView *)g_engine_interface.context_004)->byte_012 = 0xff;
        switch (title_0c4a0(5)) {
        case 0: g_2a270 = title_0c4d0(7); break;
        case 1: g_2a270 = title_0c4d0(0x14); break;
        case 2: g_2a270 = title_0c4d0(6); break;
        case 3: g_2a270 = title_0c4d0(0x15); break;
        case 4: g_2a270 = title_0c4d0(0x13); break;
        }
        g_2a254 = 0x16;
        break;
    case 22:
        g_2a254 = 0x17;
        break;
    case 23:
        p->unknown_034 = title_0cc90() + 0x2d;
        if (--g_2a270 <= 0 && g_2a394 != 5) {
            title_0c4f0(1);
            g_2a394 = 5;
        }
        break;
    case 24:
        g_2bb34 = 1;
        g_2bb38 = g_2bb3c;
        switch (g_2bb3c) {
        case 1: g_2a270 = title_0c4d0(9); break;
        case 2: g_2a270 = title_0c4d0(0xa); break;
        case 3: g_2a270 = title_0c4d0(0xc); break;
        case 5: g_2a270 = title_0c4d0(0x12); break;
        case 7: g_2a270 = title_0c4d0(8); break;
        case 6: g_2a270 = title_0c4d0(0xe); break;
        case 8: g_2a270 = title_0c4d0(0xf); break;
        case 9: g_2a270 = title_0c4d0(0x10); break;
        case 10: g_2a270 = title_0c4d0(9); break;
        case 11: g_2a270 = title_0c4d0(9); break;
        case 12: g_2a270 = title_0c4d0(0x11); break;
        }
        title_016e0(" Meg_Speech_Time %d", g_2a270);
        g_2a25c = 0;
        g_2a254 = 0x19;
        p->unknown_034 = 0x33;
    case 25:
        if (g_2a270 > 0) {
            g_2a270--;
            p->unknown_034 = title_0cc90() + 0x3c;
        } else {
            title_0c4e0();
            g_2a270 = 0;
        }
        title_0c4a0(0xdebd7);
        switch (g_2a25c) {
        case 6:
        case 5:
        case 4:
            g_2a274->unknown_04a += title_0d380(g_2a274->unknown_000 + ((title_0c4a0(0x18) - 0xc) << 16),
                                                g_2a274->unknown_004 - (title_0c4a0(0xc) << 16));
            g_2a274->unknown_034++;
            if (g_2a274->unknown_034 >= 0x11c)
                g_2a274->unknown_034 = 0xc4;
        case 3:
            g_2a280->unknown_04a += title_0d380(g_2a280->unknown_000 + ((title_0c4a0(0x18) - 0xc) << 16),
                                                g_2a280->unknown_004 - (title_0c4a0(0xc) << 16));
            g_2a280->unknown_034++;
            if (g_2a280->unknown_034 >= 0x1de)
                g_2a280->unknown_034 = 0x186;
        case 2:
            g_2a27c->unknown_04a += title_0d380(g_2a27c->unknown_000 + ((title_0c4a0(0x18) - 0xc) << 16),
                                                g_2a27c->unknown_004 - (title_0c4a0(0xc) << 16));
            g_2a27c->unknown_034++;
            if (g_2a27c->unknown_034 >= 0x17d)
                g_2a27c->unknown_034 = 0x125;
        case 1:
            g_2a278->unknown_04a += title_0d380(g_2a278->unknown_000 + ((title_0c4a0(0x18) - 0xc) << 16),
                                                g_2a278->unknown_004 - (title_0c4a0(0xc) << 16));
            g_2a278->unknown_034++;
            if (g_2a278->unknown_034 >= 0x23f)
                g_2a278->unknown_034 = 0x1e7;
            g_2a278->unknown_04a += (0x80 - g_2a278->unknown_04a) >> 1;
            g_2a27c->unknown_04a += (0x80 - g_2a27c->unknown_04a) >> 1;
            g_2a280->unknown_04a += (0x80 - g_2a280->unknown_04a) >> 1;
            g_2a274->unknown_04a += (0x80 - g_2a274->unknown_04a) >> 1;
        }
        s3 = ((short *)g_engine_interface.data_014)[((g_2a390 - 4) * 0x100) & 0xfff] << 3;
        s2 = ((short *)g_engine_interface.data_014)[((g_2a390 - 8) * 0x100) & 0xfff] << 3;
        s1 = ((short *)g_engine_interface.data_014)[((g_2a390 + 4) * 0x100) & 0xfff] << 3;
        s0 = ((short *)g_engine_interface.data_014)[(g_2a390 * 0x100) & 0xfff] << 3;
        switch (g_2a25c) {
        case 6:
        case 5:
            g_2a274->unknown_004 -= s3;
            g_2a280->unknown_004 -= s2;
            g_2a27c->unknown_004 -= s1;
            g_2a278->unknown_004 -= s0;
            break;
        case 4:
            g_2a280->unknown_004 -= s2;
            g_2a27c->unknown_004 -= s1;
            g_2a278->unknown_004 -= s0;
            if (g_2a390 < 0x10)
                g_2a274->unknown_004 -= g_2a390 << 12;
            else
                g_2a274->unknown_004 -= s3;
            break;
        case 3:
            g_2a27c->unknown_004 -= s1;
            g_2a278->unknown_004 -= s0;
            if (g_2a390 < 0x10)
                g_2a280->unknown_004 -= g_2a390 << 12;
            else
                g_2a280->unknown_004 -= s2;
            break;
        case 2:
            g_2a278->unknown_004 -= s0;
            if (g_2a390 < 0x10)
                g_2a27c->unknown_004 -= g_2a390 << 12;
            else
                g_2a27c->unknown_004 -= s1;
            break;
        case 1:
            if (g_2a390 < 0x10)
                g_2a278->unknown_004 -= g_2a390 << 12;
            else
                g_2a278->unknown_004 -= s0;
            break;
        }
        switch (g_2a25c) {
        case 0:
            idx = title_0cce0();
            if (idx != -1) {
                g_2a288[idx] = title_17ad0(0, 0, 0, 0x2008, 0);
                g_2a288[idx]->unknown_000 = g_2a278->unknown_000;
                g_2a288[idx]->unknown_004 = g_2a278->unknown_004 - 0x100000;
            }
            g_2a25c = 1;
            g_2a390 = 0;
            title_054f0(0x4316, g_2a278);
            break;
        case 1:
            g_2a390++;
            if (g_2a390 > 0x20) {
                idx = title_0cce0();
                if (idx != -1) {
                    g_2a288[idx] = title_17ad0(0, 0, 0, 0x2008, 0);
                    g_2a288[idx]->unknown_000 = g_2a27c->unknown_000;
                    g_2a288[idx]->unknown_004 = g_2a27c->unknown_004 - 0x100000;
                }
                g_2a25c = 2;
                g_2a390 = 0;
                title_054f0(0x4316, g_2a27c);
            }
            break;
        case 2:
            g_2a390++;
            if (g_2a390 > 0x20) {
                idx = title_0cce0();
                if (idx != -1) {
                    g_2a288[idx] = title_17ad0(0, 0, 0, 0x2008, 0);
                    g_2a288[idx]->unknown_000 = g_2a280->unknown_000;
                    g_2a288[idx]->unknown_004 = g_2a280->unknown_004 - 0x100000;
                }
                g_2a25c = 3;
                g_2a390 = 0;
                title_054f0(0x4316, g_2a280);
            }
            break;
        case 3:
            g_2a390++;
            if (g_2a390 > 0x20) {
                idx = title_0cce0();
                if (idx != -1) {
                    g_2a288[idx] = title_17ad0(0, 0, 0, 0x2008, 0);
                    g_2a288[idx]->unknown_000 = g_2a274->unknown_000;
                    g_2a288[idx]->unknown_004 = g_2a274->unknown_004 - 0x100000;
                }
                g_2a25c = 4;
                g_2a390 = 0;
                title_054f0(0x4316, g_2a274);
            }
            break;
        case 4:
            g_2a390++;
            if (g_2a270 <= 0 && g_2a390 >= 0x30) {
                title_0c4e0();
                g_2a270 = 0;
                g_2a25c = 5;
                g_2a390 = 0;
            }
            break;
        case 5:
            g_2a390++;
            g_2a3b0 = 0;
            title_0c450(&g_2a3b0, 0x15000, 0);
            switch (g_2bb3c) {
            case 1: title_0c2a0(g_29128, g_sequence_files[TITLE_SEQ_PSWD_LEV1], 0, g_2a3b0, 0x14312); break;
            case 2: title_0c2a0(g_29128, g_sequence_files[TITLE_SEQ_PSWD_LEV2], 0, g_2a3b0, 0x14312); break;
            case 3: title_0c2a0(g_29128, g_sequence_files[TITLE_SEQ_PSWD_LEV3], 0, g_2a3b0, 0x14312); break;
            case 5: title_0c2a0(g_29128, g_sequence_files[TITLE_SEQ_PSWD_LEV4], 0, g_2a3b0, 0x14312); break;
            case 6: title_0c2a0(g_29128, g_sequence_files[TITLE_SEQ_PSWD_LEV6], 0, g_2a3b0, 0x14312); break;
            case 7: title_0c2a0(g_29128, g_sequence_files[TITLE_SEQ_PSWD_LEV7], 0, g_2a3b0, 0x14312); break;
            case 8: title_0c2a0(g_29128, g_sequence_files[TITLE_SEQ_PSWD_LEV8], 0, g_2a3b0, 0x14312); break;
            case 9: title_0c2a0(g_29128, g_sequence_files[TITLE_SEQ_PSWD_LEV9], 0, g_2a3b0, 0x14312); break;
            case 10: title_0c2a0(g_29128, g_sequence_files[TITLE_SEQ_PSWD_LEV10], 0, g_2a3b0, 0x14312); break;
            case 11: title_0c2a0(g_29128, g_sequence_files[TITLE_SEQ_PSWD_LEV11], 0, g_2a3b0, 0x14312); break;
            case 12: title_0c2a0(g_29128, g_sequence_files[TITLE_SEQ_PSWD_LEV12], 0, g_2a3b0, 0x14312); break;
            }
            g_2a25c = 6;
            break;
        case 6:
            g_2a390++;
            if (g_engine_interface.context_004->mode_09c == 0) {
                switch (g_2bb3c) {
                case 1: title_0c990(0, g_sequence_files[TITLE_SEQ_PSWD_LEV1]); break;
                case 2: title_0c990(0, g_sequence_files[TITLE_SEQ_PSWD_LEV2]); break;
                case 3: title_0c990(0, g_sequence_files[TITLE_SEQ_PSWD_LEV3]); break;
                case 5: title_0c990(0, g_sequence_files[TITLE_SEQ_PSWD_LEV4]); break;
                case 6: title_0c990(0, g_sequence_files[TITLE_SEQ_PSWD_LEV6]); break;
                case 7: title_0c990(0, g_sequence_files[TITLE_SEQ_PSWD_LEV7]); break;
                case 8: title_0c990(0, g_sequence_files[TITLE_SEQ_PSWD_LEV8]); break;
                case 9: title_0c990(0, g_sequence_files[TITLE_SEQ_PSWD_LEV9]); break;
                case 10: title_0c990(0, g_sequence_files[TITLE_SEQ_PSWD_LEV10]); break;
                case 11: title_0c990(0, g_sequence_files[TITLE_SEQ_PSWD_LEV11]); break;
                case 12: title_0c990(0, g_sequence_files[TITLE_SEQ_PSWD_LEV12]); break;
                }
                title_01dd0(g_2a3b0);
                title_0c8b0(g_2a3b0 + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
                title_0c6d0(0);
                title_0c330(g_2a3b0);
                g_2a278->unknown_04a = 0xff;
                g_2a278->unknown_054 |= 5;
                g_2a27c->unknown_04a = 0xff;
                g_2a27c->unknown_054 |= 5;
                g_2a280->unknown_04a = 0xff;
                g_2a280->unknown_054 |= 5;
                g_2a274->unknown_04a = 0xff;
                g_2a274->unknown_054 |= 5;
                g_2a394 = 2;
                g_2a25c = 7;
                g_2a390 = 0;
            }
            break;
        case 7:
            g_2a390++;
            if (g_2a390 > 0x3c)
                g_2a394 = 4;
            break;
        }
        break;
    }
}
