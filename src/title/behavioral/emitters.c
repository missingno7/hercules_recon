/* The angle table is provided by calibration/title/title_sine.c. */
extern short g_243e0[1024];

void title_03100(unsigned char *stream, TitleObject *obj, unsigned char count)
{
    unsigned long tex;
    unsigned long flags;
    int x0;
    int y0;
    unsigned char hb;
    unsigned char flag;
    unsigned short cell;
    unsigned long attr;
    unsigned short clut;
    unsigned long *bucket;
    unsigned short flip;
    TitlePrim *prim;
    unsigned char u;
    unsigned char v;
    signed char g;
    int a;
    int w;
    int h;
    int dx;
    int y;
    int x1;
    int x2;
    int B;
    unsigned int off;

    tex = make_colour(obj);
    flags = obj->unknown_054;
    dx = (signed char)*stream++;
    if (flags & 0x10)
        dx = -dx;
    x0 = obj->unknown_00c + dx;
    hb = stream[1];
    cell = obj->unknown_046;
    y0 = obj->unknown_00e + (signed char)*stream++;
    attr = (flags & 3) << 5;
    clut = ((obj->unknown_01e + 0xe0) << 6) | 0x30;
    flag = hb & 0x80;
    stream++;
    bucket = &g_2cc04->ot[TITLE_OT_SIZE - 2 - obj->unknown_010];
    flip = (unsigned short)(flags & 0x10);
    prim = g_2df40;
    do {
        g = g_264f8[cell & 0xf];
        u = ((cell >> 4) * 2 + (g >> 1)) << 4;
        v = ((cell >> 8) * 2 + (g & 1)) << 4;
        if (flag) {
            v += (hb & 0xf) << 3;
            u += (hb >> 1) & 0x38;
        }
        a = (signed char)*stream++;
        w = (a & 0xf) << 3;
        dx = (signed char)*stream++ * 2;
        h = (a >> 1) & ~7;
        B = (signed char)*stream++ * 2;
        y = B + y0;
        if (flip) {
            x1 = (unsigned short)(x0 - dx);
            prim->xy_008 = (y << 16) + x1;
            x2 = (unsigned short)(x0 - w - dx);
            prim->xy_010 = (y << 16) + x2;
            y = (y + h) << 16;
            prim->xy_018 = x1 + y;
            prim->xy_020 = y + x2;
            if (v == 0) {
                v = 1;
                w--;
            }
            if (u + h == 0x100)
                h--;
            prim->w_014 = (u << 8) + (unsigned char)(v + w - 1);
            prim->w_00c = (u << 8) + (unsigned char)(v - 1);
            prim->w_01c = ((h + u) << 8) + (unsigned char)(v - 1);
            prim->w_024 = ((h + u) << 8) + (unsigned char)(v + w - 1);
        } else {
            x1 = (unsigned short)(dx + x0);
            x2 = (unsigned short)(dx + w + x0);
            prim->xy_008 = x1 + (y << 16);
            prim->xy_010 = (y << 16) + x2;
            y = (y + h) << 16;
            prim->xy_018 = x1 + y;
            prim->xy_020 = x2 + y;
            if (v + w == 0x100)
                w--;
            if (u + h == 0x100)
                h--;
            prim->w_014 = (u << 8) + (unsigned char)(v + w);
            prim->w_00c = (u << 8) + v;
            prim->w_01c = ((h + u) << 8) + v;
            prim->w_024 = ((h + u) << 8) + (unsigned char)(v + w);
        }
        off = (cell >> 4) * sizeof(TitleCell);
        prim->w_016 = ((TitleCell *)((char *)g_2d340 + off))->w_02 | attr;
        prim->tex_004 = tex;
        prim->w_00e = clut;
        prim->next_000 = (TitlePrim *)*bucket;
        *bucket = (unsigned long)prim;
        hb = *stream;
        prim++;
        flag = hb & 0x80;
        stream++;
        if (!flag)
            cell = ((TitleCell *)((char *)g_2d340 + off))->next_04[g_264f8[cell & 0xf]];
    } while (--count);
    g_2df40 = prim;
}

void title_03420(unsigned char *stream, TitleObject *obj, unsigned char count)
{
    unsigned long tex;
    unsigned long flags;
    int dx;
    int x0;
    int y0;
    unsigned char hb;
    unsigned char flag;
    unsigned short cell;
    unsigned long attr;
    unsigned short clut;
    unsigned long *bucket;
    unsigned short flip;
    TitlePrim *prim;
    unsigned int u;
    unsigned int v;
    signed char *q;
    int g;
    int a;
    int w;
    int h;
    int A;
    int Vn;
    int B;
    int W;
    int sin2;
    int cos2;
    unsigned int off;

    tex = make_colour(obj);
    flags = obj->unknown_054;
    dx = (signed char)*stream++;
    if (flags & 0x10)
        dx = -dx;
    x0 = obj->unknown_00c + dx;
    y0 = obj->unknown_00e + (signed char)*stream++;
    cell = obj->unknown_046;
    clut = ((obj->unknown_01e + 0xe0) << 6) | 0x30;
    hb = *stream++;
    flag = hb & 0x80;
    sin2 = g_243e0[(short)obj->unknown_074 & 0x3ff] * 2;
    cos2 = g_243e0[((short)obj->unknown_074 + 0x100) & 0x3ff] * 2;
    attr = (flags & 3) << 5;
    bucket = &g_2cc04->ot[TITLE_OT_SIZE - 2 - obj->unknown_010];
    prim = g_2df40;
    flip = (unsigned short)(flags & 0x10);
    do {
        q = &g_264f8[cell & 0xf];
        g = *q;
        u = ((cell >> 4) * 2 + (g >> 1)) << 4;
        v = ((cell >> 8) * 2 + (g & 1)) << 4;
        if (flag) {
            v += (hb & 0xf) * 8;
            u += (hb >> 1) & 0x38;
        }
        a = (signed char)*stream++;
        w = (a & 0xf) << 3;
        h = (a >> 1) & ~7;
        prim->w_00c = (unsigned char)v + (u << 8);
        prim->w_014 = (unsigned char)(v + w - 1) + (u << 8);
        prim->w_01c = (unsigned char)v + ((u + h + 0xff) << 8);
        prim->w_024 = (unsigned char)(v + w - 1) + ((u + h + 0xff) << 8);
        if (flip) {
            A = -(signed char)*stream++ * 2;
            Vn = A - w;
        } else {
            A = (signed char)*stream++ * 2;
            Vn = A + w;
        }
        B = (signed char)*stream++ * 2;
        W = B + h;
        prim->xy_008 = (unsigned short)(x0 + ((cos2 * A - sin2 * B) >> 16)) + ((y0 + ((sin2 * A + cos2 * B) >> 16)) << 16);
        prim->xy_010 = (unsigned short)(x0 + ((cos2 * Vn - sin2 * B) >> 16)) + ((y0 + ((sin2 * Vn + cos2 * B) >> 16)) << 16);
        prim->xy_018 = (unsigned short)(x0 + ((cos2 * A - sin2 * W) >> 16)) + ((y0 + ((sin2 * A + cos2 * W) >> 16)) << 16);
        prim->xy_020 = (unsigned short)(x0 + ((cos2 * Vn - sin2 * W) >> 16)) + ((y0 + ((sin2 * Vn + cos2 * W) >> 16)) << 16);
        off = (cell >> 4) * sizeof(TitleCell);
        prim->w_016 = ((TitleCell *)((char *)g_2d340 + off))->w_02 | attr;
        prim->tex_004 = tex;
        prim->next_000 = (TitlePrim *)*bucket;
        *bucket = (unsigned long)prim;
        prim->w_00e = clut;
        hb = *stream;
        prim++;
        flag = hb & 0x80;
        stream++;
        if (!flag)
            cell = ((TitleCell *)((char *)g_2d340 + off))->next_04[*q];
    } while (--count);
    g_2df40 = prim;
}

void title_03740(unsigned char *stream, TitleObject *obj, unsigned char count, long sx, long sy)
{
    unsigned long tex;
    unsigned long flags;
    int dx;
    int x0;
    int y0;
    unsigned char hb;
    unsigned char flag;
    unsigned short cell;
    unsigned long attr;
    unsigned short clut;
    unsigned long *bucket;
    unsigned short flip;
    TitlePrim *prim;
    unsigned int u;
    unsigned int v;
    signed char *q;
    int g;
    int a;
    int w;
    int h;
    int A;
    int Vn;
    int B;
    int W;
    unsigned short x1;
    unsigned short x2;
    int y1;
    int y2;
    unsigned int off;

    tex = make_colour(obj);
    flags = obj->unknown_054;
    dx = (sx * (signed char)*stream++) >> 8;
    if (flags & 0x10)
        dx = -dx;
    x0 = obj->unknown_00c + dx;
    hb = stream[1];
    y0 = ((sy * (signed char)*stream++) >> 8) + obj->unknown_00e;
    prim = g_2df40;
    clut = ((obj->unknown_01e + 0xe0) << 6) | 0x30;
    stream++;
    cell = obj->unknown_046;
    flag = hb & 0x80;
    attr = (flags & 3) << 5;
    bucket = &g_2cc04->ot[TITLE_OT_SIZE - 2 - obj->unknown_010];
    flip = (unsigned short)(flags & 0x10);
    do {
        q = &g_264f8[cell & 0xf];
        g = *q;
        u = ((cell >> 4) * 2 + (g >> 1)) << 4;
        v = ((cell >> 8) * 2 + (g & 1)) << 4;
        if (flag) {
            v += (hb & 0xf) * 8;
            u += (hb >> 1) & 0x38;
        }
        a = (signed char)*stream++;
        w = (a & 0xf) << 3;
        h = (a >> 1) & ~7;
        prim->w_00c = (unsigned char)v + (u << 8);
        prim->w_014 = (unsigned char)(v + w - 1) + (u << 8);
        prim->w_01c = (unsigned char)v + ((u + h + 0xff) << 8);
        prim->w_024 = (unsigned char)(v + w - 1) + ((u + h + 0xff) << 8);
        if (flip) {
            A = -(signed char)*stream++ * 2;
            Vn = A - w;
        } else {
            A = (signed char)*stream++ * 2;
            Vn = A + w;
        }
        B = (signed char)*stream++ * 2;
        A *= sx;
        Vn *= sx;
        W = (B + h) * sy;
        B *= sy;
        x1 = (unsigned short)((A >> 8) + x0);
        y1 = (B >> 8) + y0;
        prim->xy_008 = (y1 << 16) + x1;
        x2 = (unsigned short)((Vn >> 8) + x0);
        prim->xy_010 = (y1 << 16) + x2;
        y2 = (W >> 8) + y0;
        prim->xy_018 = (y2 << 16) + x1;
        prim->xy_020 = (y2 << 16) + x2;
        off = (cell >> 4) * sizeof(TitleCell);
        prim->w_016 = ((TitleCell *)((char *)g_2d340 + off))->w_02 | attr;
        prim->tex_004 = tex;
        prim->w_00e = clut;
        prim->next_000 = (TitlePrim *)*bucket;
        *bucket = (unsigned long)prim;
        hb = *stream;
        prim++;
        flag = hb & 0x80;
        stream++;
        if (!flag)
            cell = ((TitleCell *)((char *)g_2d340 + off))->next_04[*q];
    } while (--count);
    g_2df40 = prim;
}

void title_039b0(unsigned char *stream, TitleObject *obj, unsigned char count, long sx, long sy)
{
    unsigned long tex;
    unsigned long flags;
    int dx;
    int x0;
    int y0;
    unsigned char hb;
    unsigned char flag;
    unsigned short cell;
    unsigned long attr;
    unsigned short clut;
    unsigned long *bucket;
    unsigned short flip;
    TitlePrim *prim;
    unsigned int u;
    unsigned int v;
    int g;
    int a;
    int w;
    int h;
    int A;
    int Vn;
    int B;
    int W;
    int sin2;
    int cos2;
    int sa;
    int ca;
    int sb;
    int cb;
    int sv;
    int cv;
    int sw;
    int cw;
    unsigned int off;

    tex = make_colour(obj);
    flags = obj->unknown_054;
    dx = (sx * (signed char)*stream++) >> 8;
    if (flags & 0x10)
        dx = -dx;
    x0 = obj->unknown_00c + dx;
    y0 = ((sy * (signed char)*stream) >> 8) + obj->unknown_00e;
    hb = stream[1];
    cos2 = g_243e0[((short)obj->unknown_074 + 0x100) & 0x3ff] * 2;
    clut = ((obj->unknown_01e + 0xe0) << 6) | 0x30;
    attr = (flags & 3) << 5;
    bucket = &g_2cc04->ot[TITLE_OT_SIZE - 2 - obj->unknown_010];
    stream += 2;
    sin2 = g_243e0[(short)obj->unknown_074 & 0x3ff] * 2;
    flag = hb & 0x80;
    flip = (unsigned short)(flags & 0x10);
    prim = g_2df40;
    cell = obj->unknown_046;
    do {
        g = g_264f8[cell & 0xf];
        v = ((cell >> 8) * 2 + (g & 1)) << 4;
        u = ((cell >> 4) * 2 + (g >> 1)) << 4;
        if (flag) {
            v += (hb & 0xf) * 8;
            u += (hb >> 1) & 0x38;
        }
        a = (signed char)*stream++;
        w = (a & 0xf) << 3;
        h = (a >> 1) & ~7;
        prim->w_00c = (unsigned char)v + (u << 8);
        prim->w_014 = (unsigned char)(v + w - 1) + (u << 8);
        prim->w_01c = (unsigned char)v + ((u + h + 0xff) << 8);
        prim->w_024 = (unsigned char)(v + w - 1) + ((u + h + 0xff) << 8);
        if (flip) {
            A = -(signed char)*stream++ * 2;
            Vn = A - w;
        } else {
            A = (signed char)*stream++ * 2;
            Vn = A + w;
        }
        A *= sx;
        Vn *= sx;
        A >>= 8;
        sa = sin2 * A;
        ca = cos2 * A;
        B = (signed char)*stream++ * 2;
        W = h + B;
        B *= sy;
        W *= sy;
        B >>= 8;
        sb = -(sin2 * B);
        cb = cos2 * B;
        Vn >>= 8;
        sv = sin2 * Vn;
        cv = cos2 * Vn;
        W >>= 8;
        sw = -(sin2 * W);
        cw = cos2 * W;
        prim->xy_008 = (unsigned short)(((sb + ca) >> 16) + x0) + ((((cb + sa) >> 16) + y0) << 16);
        prim->xy_010 = (unsigned short)(((cv + sb) >> 16) + x0) + ((((sv + cb) >> 16) + y0) << 16);
        prim->xy_018 = (unsigned short)(((sw + ca) >> 16) + x0) + ((((sa + cw) >> 16) + y0) << 16);
        prim->xy_020 = (unsigned short)(((sw + cv) >> 16) + x0) + ((((sv + cw) >> 16) + y0) << 16);
        off = (cell >> 4) * sizeof(TitleCell);
        prim->w_016 = ((TitleCell *)((char *)g_2d340 + off))->w_02 | attr;
        prim->tex_004 = tex;
        prim->w_00e = clut;
        prim->next_000 = (TitlePrim *)*bucket;
        *bucket = (unsigned long)prim;
        hb = *stream;
        prim++;
        flag = hb & 0x80;
        stream++;
        if (!flag)
            cell = ((TitleCell *)((char *)g_2d340 + off))->next_04[g_264f8[cell & 0xf]];
    } while (--count);
    g_2df40 = prim;
}
