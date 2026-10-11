/*
 * TITLE rendering behavior additions for calibration/title/u17cb0.cpp.
 * Extracted from structural drafts named in evidence/title_stage1.json;
 * adapted to the existing same-TU object names and the shared FT4 memory view.
 */
extern "C" {

typedef struct TitleCell {
    unsigned short mask_00;
    unsigned short w_02;
    unsigned short next_04[4];
} TitleCell;

typedef struct TitleVtx {
    short x;
    short y;
    short z;
    short pad;
} TitleVtx;

typedef struct TitleFrameBox {
    unsigned short ext_00;
    unsigned short ext_02;
    signed char dx_04;
    signed char dy_05;
} TitleFrameBox;

typedef struct TitlePrim {
    struct TitlePrim *next_000;
    unsigned long tex_004;
    unsigned long xy_008;
    union {
        unsigned long packed;
        struct { unsigned short uv; unsigned short clut; } words;
    } uv_00c;
    unsigned long xy_010;
    union {
        unsigned long packed;
        struct { unsigned short uv; unsigned short tpage; } words;
    } uv_014;
    unsigned long xy_018;
    union {
        unsigned long packed;
        struct { unsigned short uv; unsigned short pad; } words;
    } uv_01c;
    unsigned long xy_020;
    union {
        unsigned long packed;
        struct { unsigned short uv; unsigned short pad; } words;
    } uv_024;
} TitlePrim;

typedef struct TitleVertex {
    unsigned long xy;
    long z;
} TitleVertex;
typedef void (*TitleHandler)(TitleObj *o, unsigned char *p, int n);

extern TitleCell g_2d340[];
extern signed char g_264f8[];
extern int g_2d328;
extern unsigned int *g_2cbec;
extern unsigned char *g_2cbf4;
extern int g_2acf4;

void title_0c770(void *a, void *b, void *c);
void title_0c7c0(void);
void title_0c820(TitlePrim *p);
void title_0c740(unsigned long a, unsigned long b, unsigned long c);
void title_0c7a0(void);
void title_0c800(int *p);
void title_0c830(TitlePrim *p);
void title_01dd0(TitleObj *o, unsigned long *ot);

void title_17f50(unsigned char *stream, TitleObj *obj, unsigned char count)
{
    unsigned char code;
    unsigned char more;
    unsigned short w;
    unsigned char u0;
    unsigned short flip;
    unsigned long flags;
    int b;
    unsigned int row;
    unsigned long *ot;
    unsigned short clut;
    TitleVtx v0;
    TitleVtx v1;
    TitleVtx v2;
    int a;
    int h;
    int t;
    int c;
    unsigned int dw;
    int u;
    int vv;
    int x;
    int x2;
    int y;
    unsigned char u1;
    unsigned int off;
    TitlePrim *p;

    title_0c7d0(g_260c0);
    title_0c7f0(g_2dfa0->trans_078);
    g_2dfa0->trans_078[1] += obj->w_028 * 2;
    title_0c7e0(g_2dfa0->trans_078);
    v0.z = 0;
    v1.z = 0;
    v2.z = 0;
    a = (signed char)*stream++;
    b = (signed char)*stream++;
    clut = ((obj->b_01e + 0xe0) << 6) | 0x30;
    flags = obj->flags_054;
    code = *stream++;
    w = obj->w_046;
    ot = &g_2cc04->ot[TITLE_OT_SIZE - 2 - obj->w_10];
    p = (TitlePrim *)g_2df40;
    more = code & 0x80;
    flip = flags & 0x10;
    do {
        row = w >> 4;
        t = g_264f8[w & 15];
        vv = ((t >> 1) + row * 2) << 4;
        u = ((t & 1) + (w >> 8) * 2) << 4;
        if (more) {
            u += (code & 15) * 8;
            vv += (code >> 1) & 0x38;
        }
        c = (signed char)*stream;
        dw = (c & 15) * 8;
        h = (c >> 1) & ~7;
        stream++;
        u0 = (unsigned char)u;
        p->uv_00c.words.uv = u0 + (vv << 8);
        u1 = (unsigned char)(u + dw - 1);
        p->uv_014.words.uv = u1 + (vv << 8);
        p->uv_01c.words.uv = u0 + ((vv + h + 0xff) << 8);
        p->uv_024.words.uv = ((vv + h + 0xff) << 8) + u1;
        if (flip) {
            x = -a - (signed char)*stream++ * 2;
            x2 = x - dw;
        } else {
            x = a + (signed char)*stream++ * 2;
            x2 = x + dw;
        }
        y = b + (signed char)*stream++ * 2;
        v0.x = x;
        v2.x = x;
        v1.y = y;
        v0.y = y + h;
        v2.y = y;
        v1.x = x2;
        title_0c770(&v2, &v1, &v0);
        title_0c7c0();
        off = row * sizeof(TitleCell);
        p->uv_014.words.tpage = ((TitleCell *)((char *)g_2d340 + off))->w_02;
        p->tex_004 = 0x2e000000;
        title_0c820(p);
        v0.x = v1.x;
        title_0c760(&v0);
        title_0c7b0();
        p->next_000 = (TitlePrim *)*ot;
        *ot = (unsigned long)p;
        p->uv_00c.words.clut = clut;
        code = *stream;
        more = code & 0x80;
        stream++;
        if (!more) {
            w = ((TitleCell *)((char *)g_2d340 + off))->next_04[g_264f8[w & 15]];
        }
        title_0c810(&p->xy_020);
        p++;
    } while (--count);
    g_2df40 = (TitleTile *)p;
}

void title_181e0(unsigned char *stream, TitleObj *obj, unsigned char count)
{
    unsigned char code;
    unsigned char more;
    unsigned short w;
    unsigned char u0;
    int flip;
    unsigned long flags;
    unsigned long sel;
    unsigned long colour;
    int zoff;
    unsigned long blend;
    int b;
    unsigned int row;
    unsigned long *ot;
    unsigned short clut;
    TitleVtx v0;
    TitleVtx v1;
    TitleVtx v2;
    int a;
    int h;
    int t;
    int c;
    unsigned int dw;
    int u;
    int vv;
    int x;
    int x2;
    int y;
    unsigned char u1;
    unsigned int off;
    TitlePrim *p;

    flags = obj->flags_054;
    if (flags & 0x2000) {
        obj->w_10 = obj->w_03e;
    }
    ot = &g_2cc04->ot[TITLE_OT_SIZE - 2 - obj->w_10];
    sel = flags & 0x10000;
    if (sel) {
        title_01dd0(obj, ot);
    }
    if (g_2d328 != 0) {
        title_196f0(obj, 0);
        title_0c790();
    }
    colour = make_colour(obj);
    zoff = 0;
    if (flags & 0x40000) {
        zoff = obj->w_026;
    }
    v0.z = 0;
    v1.z = 0;
    v2.z = 0;
    a = (signed char)*stream++;
    b = (signed char)*stream++;
    blend = (flags & 3) << 5;
    code = *stream++;
    clut = ((obj->b_01e + 0xe0) << 6) | 0x30;
    w = obj->w_046;
    p = (TitlePrim *)g_2df40;
    more = code & 0x80;
    flip = flags & 0x10;
    do {
        row = w >> 4;
        t = g_264f8[w & 15];
        vv = ((t >> 1) + row * 2) << 4;
        u = ((t & 1) + (w >> 8) * 2) << 4;
        if (more) {
            u += (code & 15) * 8;
            vv += (code >> 1) & 0x38;
        }
        c = (signed char)*stream;
        dw = (c & 15) * 8;
        h = (c >> 1) & ~7;
        stream++;
        u0 = (unsigned char)u;
        p->uv_00c.words.uv = u0 + (vv << 8);
        u1 = (unsigned char)(u + dw - 1);
        p->uv_014.words.uv = u1 + (vv << 8);
        p->uv_01c.words.uv = u0 + ((vv + h + 0xff) << 8);
        p->uv_024.words.uv = ((vv + h + 0xff) << 8) + u1;
        if (flip) {
            x = -a - (signed char)*stream++ * 2;
            x2 = x - dw;
        } else {
            x = a + (signed char)*stream++ * 2;
            x2 = x + dw;
        }
        y = b + (signed char)*stream++ * 2 + zoff;
        v0.y = y + h;
        v0.x = x;
        v2.x = x;
        v1.y = y;
        v2.y = y;
        v1.x = x2;
        title_0c770(&v2, &v1, &v0);
        title_0c7c0();
        off = row * sizeof(TitleCell);
        p->uv_014.words.tpage = ((TitleCell *)((char *)g_2d340 + off))->w_02 | blend;
        p->tex_004 = colour;
        title_0c820(p);
        v0.x = v1.x;
        title_0c760(&v0);
        title_0c7b0();
        p->next_000 = (TitlePrim *)*ot;
        *ot = (unsigned long)p;
        p->uv_00c.words.clut = clut;
        more = *stream;
        code = more;
        more &= 0x80;
        stream++;
        if (!more) {
            w = ((TitleCell *)((char *)g_2d340 + off))->next_04[g_264f8[w & 15]];
        }
        title_0c810(&p->xy_020);
        p++;
    } while (--count);
    g_2df40 = (TitleTile *)p;
    if (sel) {
        title_01dd0(obj, ot);
    }
}

int title_184c0(TitleShape *o, TitleFrameBox *f, int mode)
{
    int x, y;
    int flip;
    int e;
    short l, r, t, b;
    int scale, sx;
    short t0;

    x = o->unknown_00c;
    y = o->unknown_00e;
    flip = o->unknown_054 & 0x10;
    if (flip) {
        x -= f->dx_04;
    } else {
        x += f->dx_04;
    }
    y += f->dy_05;
    e = f->ext_00;
    r = (signed char)e;
    l = (signed char)(e >> 8);
    e = f->ext_02;
    b = (signed char)e;
    t = (signed char)(e >> 8);
    scale = (o->unknown_03a * (g_2cbe0[2] >> 1)) / o->unknown_010;
    if (mode == 1) {
        y += (o->unknown_028 * scale) >> 8;
    }
    sx = (mode == 1) ? scale * 2 : scale;
    r = (sx * r) >> 7;
    l = (sx * l) >> 7;
    b = (scale * b) >> 7;
    t = (scale * t) >> 7;
    if (flip) {
        t0 = -r;
        r = -l;
        l = t0;
    }
    if (x + l < 0) {
        return 1;
    }
    if (y + t < 0) {
        return 1;
    }
    if (x + r > 0x140) {
        return 1;
    }
    return y + b > 0xf0;
}

void title_186e0(unsigned char *stream, TitleObj *obj, unsigned char count)
{
    unsigned long colour;
    unsigned long flags;
    int c;
    int c2;
    int x0;
    int y0;
    unsigned char code;
    unsigned char more;
    unsigned short w;
    unsigned long blend;
    unsigned short clut;
    unsigned long *ot;
    unsigned short flip;
    TitlePrim *p;
    unsigned int row;
    int t;
    int vv;
    int u;
    int dw;
    int h;
    int sx;
    int sy;
    unsigned int off;

    colour = make_colour(obj);

    flags = obj->flags_054;
    c = (signed char)*stream++;
    if (flags & 0x10) { c = -c; }
    w = obj->w_046;
    x0 = (short)obj->w_0c + c;
    c2 = (signed char)*stream++;
    y0 = (short)obj->w_0e + c2;
    more = *stream;
    code = more;
    blend = (flags & 3) << 5;
    clut = ((obj->b_01e + 0xe0) << 6) | 0x30;
    more &= 0x80;
    stream++;
    ot = &g_2cc04->ot[TITLE_OT_SIZE - 2 - obj->w_10];
    p = (TitlePrim *)g_2df40;
    flip = flags & 0x10;

    do {
        row = w >> 4;
        t = g_264f8[w & 15];
        vv = ((t >> 1) + row * 2) << 4;
        u = ((t & 1) + (w >> 8) * 2) << 4;
        if (more) {
            u += (code & 15) * 8;
            vv += (code >> 1) & 0x38;
        }
        c = (signed char)*stream;
        dw = (c & 15) * 8;
        h = (c >> 1) & ~7;
        stream++;
        sx = (signed char)*stream++ * 2;
        sy = (signed char)*stream++ * 2;
        sy += y0;
        if (flip) {
            p->uv_00c.words.uv = (unsigned char)(u - 1) + (vv << 8);
            p->uv_014.words.uv = (unsigned char)(u + dw - 1) + (vv << 8);
            p->uv_01c.words.uv = (unsigned char)(u - 1) + ((vv + h) << 8);
            p->uv_024.words.uv = ((vv + h) << 8) + (unsigned char)(u + dw - 1);
            p->xy_008 = (sy << 16) + (unsigned short)(x0 - sx);
            p->xy_010 = (unsigned short)(x0 - dw - sx) + (sy << 16);
            p->xy_018 = ((sy + h) << 16) + (unsigned short)(x0 - sx);
            p->xy_020 = ((sy + h) << 16) + (unsigned short)(x0 - dw - sx);
        } else {
            p->uv_00c.words.uv = (unsigned char)u + (vv << 8);
            p->uv_014.words.uv = (unsigned char)(u + dw) + (vv << 8);
            p->uv_01c.words.uv = (unsigned char)u + ((vv + h) << 8);
            p->uv_024.words.uv = (unsigned char)(u + dw) + ((vv + h) << 8);
            p->xy_008 = (unsigned short)(sx + x0) + (sy << 16);
            p->xy_010 = (sy << 16) + (unsigned short)(dw + sx + x0);
            p->xy_018 = (unsigned short)(sx + x0) + ((sy + h) << 16);
            p->xy_020 = (unsigned short)(dw + sx + x0) + ((sy + h) << 16);
        }
        off = row * sizeof(TitleCell);
        p->uv_014.words.tpage = ((TitleCell *)((char *)g_2d340 + off))->w_02 | blend;
        p->tex_004 = colour;
        p->uv_00c.words.clut = clut;
        p->next_000 = (TitlePrim *)*ot;
        *ot = (unsigned long)p;
        p++;
        more = *stream;
        code = more;
        more &= 0x80;
        stream++;
        if (!more) {
            w = ((TitleCell *)((char *)g_2d340 + off))->next_04[g_264f8[w & 15]];
        }
    } while (--count);
    g_2df40 = (TitleTile *)p;
}

void title_18b20(TitleObj *o)
{
    unsigned char *q;
    int idx;
    unsigned long c;
    unsigned int *list;
    unsigned int count;
    unsigned int k;
    TitleObj *e;

    title_196f0(o, o->rot_070.pad);
    title_0c7e0(o->trans_078);
    if (o->w_034 != 0) {
        idx = o->w_034;
        q = (unsigned char *)(g_2cbf0 + *(int *)(g_2cbf0 + idx * 4));
        q += idx * 4 + 2;
        q += *q * 8 + 8;
        ((TitleHandler)g_260f0[o->byte_023])(o, q + 2, *(short *)q);
        list = o->list_120;
        count = *list;
        list++;
        c = make_colour(o);
        if (count-- != 0) {
            k = count + 1;
            do {
                e = (TitleObj *)*list++;
                idx = e->w_034 * 4;
                q = (unsigned char *)(g_2cbf0 + *(int *)(g_2cbf0 + idx)) + idx;
                title_196f0(e, e->rot_070.pad);
                title_18bf0(e, q, c);
            } while (--k);
        }
    }
}

int title_18bf0(TitleObj *o, unsigned char *q, unsigned long c)
{
    TitleVertex *v1;
    short more;
    TitleVertex *v0;
    int lvl;
    int dbl;
    unsigned int u2;
    unsigned long xy1;
    unsigned short m;
    unsigned short h;
    unsigned short n;
    TitleVertex *v3;
    int k;
    unsigned int w;
    TitlePrim **ot;
    TitleVertex *v2;
    unsigned long col;
    int x;
    int clip;
    TitlePrim *prim;
    unsigned long xy3;
    int z;
    TitlePair8 *rec;
    unsigned char *uvt;
    unsigned int uv;
    char *vtx;
    unsigned char *b;
    unsigned int *t;
    TitleVertex *d;
    unsigned short i;
    int cnt;

    g_2bb28 = 0;
    ot = (TitlePrim **)&g_2cc04->ot[TITLE_OT_SIZE - 2];
    lvl = o->w_10 >> 5;
    if (lvl < 1) {
        lvl = 1;
    } else if (lvl > 0x1d) {
        lvl = 0x1d;
    }
    rec = (TitlePair8 *)(g_2cbec + *(short *)q);
    q += 2;
    k = *q;
    q += 2;
    col = (lvl << 22) + 0x78300000;
    x = *(int *)q;
    q += k * 8 + 6;
    if (x != 0) {
        title_193e0(o, x);
    } else {
        title_0c7e0(o->trans_078);
    }
    uvt = g_2cbf4;
    o->flags_01f |= 2;
    h = o->byte_023;
    dbl = o->flags_054 & 0x40;
    vtx = (char *)g_engine_interface.data_010 + 0x18;
    prim = (TitlePrim *)g_2df40;
    if (g_engine_interface.context_004->unknown_040 != 0) {
        return 1;
    }
    do {
        cnt = *(short *)q;
        q += 2;
        ((TitleHandler)g_260f0[h])(o, q, cnt);
        n = ((short *)rec)[0];
        more = ((short *)rec)[1];
        rec = (TitlePair8 *)((char *)rec - 4);
        do {
            rec++;
            w = rec->a_00;
            v0 = (TitleVertex *)(vtx + (w & 0x3f) * 8);
            w >>= 3;
            v1 = (TitleVertex *)(vtx + (w & 0x1f8));
            w >>= 6;
            v2 = (TitleVertex *)(vtx + (w & 0x1f8));
            w >>= 6;
            v3 = (TitleVertex *)(vtx + (w & 0x1f8));
            w >>= 9;
            if (w & 8) {
                if (w & 1) {
                    prim->xy_008 = v0->xy;
                    prim->xy_010 = v1->xy;
                    prim->xy_018 = v3->xy;
                    goto emit;
                }
                if ((w & 4) && g_2acf4 == 0) {
                    goto next;
                }
            }
            title_0c740(v0->xy, v1->xy, v3->xy);
            title_0c7a0();
            title_0c800(&clip);
            if (clip < 0) {
                if (!(w & 0x40)) {
                    goto next;
                }
                xy1 = v1->xy;
                xy3 = v3->xy;
                title_0c740(xy1, v2->xy, xy3);
                title_0c7a0();
                prim->xy_008 = v0->xy;
                prim->xy_010 = xy1;
                prim->xy_018 = xy3;
                title_0c800(&clip);
                if (clip < 0) {
                    goto next;
                }
            } else {
                title_0c830(prim);
            }
        emit:
            prim->xy_020 = v2->xy;
            z = v2->z;
            if (dbl) {
                z += v3->z;
                z += v0->z;
                z += v1->z;
                z >>= 4;
            } else {
                z += v0->z;
                z >>= 3;
            }
            if ((unsigned int)z > 0x4fe) {
                z = 0x4fe;
            }
            prim->next_000 = ot[-z];
            ot[-z] = prim;
            prim->tex_004 = c;
            uv = rec->b_04;
            if (uv & 0xff000000) {
                uv = (unsigned short)uv;
                uv |= 0xff00;
                prim->uv_00c.packed = uv | col;
                prim->uv_014.packed = uv | 0x9c0000;
                prim->uv_01c.words.uv = (unsigned short)uv;
                prim->uv_024.words.uv = (unsigned short)uv;
            } else {
                t = (unsigned int *)(uvt + uv);
                u2 = t[2];
                prim->uv_01c.words.uv = (unsigned short)u2;
                prim->uv_014.packed = t[1];
                prim->uv_00c.packed = col | t[0];
                prim->uv_024.words.uv = (unsigned short)(u2 >> 16);
            }
            prim++;
        next:
            ;
        } while (--n);
        if (more == 0) {
            break;
        }
        b = (unsigned char *)(rec + 1);
        q += cnt * 4 + 2;
        m = (unsigned short)more >> 8;
        for (i = 0; i != m; i++) {
                d = (TitleVertex *)(vtx + (*b & 0x3f) * 8);
                g_2bb20->a_00 = d->xy;
                g_2bb20->b_04 = d->z;
                g_2bb20++;
                b++;
        }
        rec = (TitlePair8 *)(((int)b + 3) & ~3);
    } while ((char)more != 0);
    g_2df40 = (TitleTile *)prim;
    return 1;
}

} /* extern "C" */
