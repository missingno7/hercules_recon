/* lane x6: ENG1 0x2e3b0..0x30010 mesh preparation and primitive emitters. */

typedef struct SVec {           /* PSX SVECTOR */
    short vx, vy, vz, pad;
} SVec;
typedef struct LVec2 {
    long x, y;
} LVec2;
typedef struct Mat {            /* PSX MATRIX */
    short m[3][3];
    long t[3];
} Mat;
typedef struct MeshRef {        /* 16-byte reference record */
    long x, z, unknown_008, unknown_00c;
} MeshRef;
typedef struct MeshChild {
    long x, z;
    unsigned long angle_008;
    unsigned short ref_00c;
    unsigned short row_00e;
} MeshChild;
typedef struct MeshVisit {
    unsigned char *header_000;
    MeshRef *header_004;
    MeshChild *child_008;
    short projected_00c[2];
    short depth_010;
    unsigned char unknown_012[3];
    unsigned char column_counter_015;
    unsigned char column_stop_016;
    unsigned char row_stop_017;
    long row_018;
    long column_01c;
    unsigned long packed_020;
    unsigned char winding_024, variant_025, shift_026, limit_027;
    unsigned char mode_028, side_029;
    unsigned short cell_02a, flags_02c;
    unsigned char unknown_02e[6];
    long tick_034, layer_y_038, layer_z_03c, cached_040, cached_044;
    long row_local_048, column_local_04c, anchor_row_050;
    long transform_054[8];
} MeshVisit;

typedef struct Scratch {        /* scratchpad view at *g_scratch_70f30 */
    Mat m0;
    Mat m1;
    SVec v[1];
} Scratch;

extern Scratch *g_scratch_70f30;
extern short *g_sine_table_70f34;
extern long g_camera_angle_x_70cd0;
extern long g_mesh_scale_5ceb0[3];
extern long g_mesh_cached_column_6ffb8;
extern unsigned long *g_shade_corners_70d50;
extern unsigned long g_shade_table_6f4e8[8][64];

void dispatch_engine_2c690(void *v);
void dispatch_engine_2c6c0(void);
void dispatch_engine_2c6e0(void);
void dispatch_engine_2c700(Mat *m);
void dispatch_engine_2c710(Mat *m);
void dispatch_engine_2c750(void *p);
void dispatch_engine_2c760(void *p);
void dispatch_engine_2c7b0(void *p);
void dispatch_engine_2c920(SVec *r, Mat *m);
void dispatch_engine_2c940(Mat *m, long *s);

int dispatch_local_2e3b0(MeshVisit *visit)
{
    Mat *m;
    SVec *sv;
    MeshChild *child;
    SVec ang;
    long theta, x, z, c, s, turn, column, row;

    m = &g_scratch_70f30->m0;
    sv = (SVec *)(m + 2);
    child = visit->child_008;
    if (visit->cell_02a != visit->cached_040) {
        visit->cached_040 = visit->cell_02a;
        theta = visit->transform_054[4];
        x = (child->x - visit->transform_054[0]) >> 6;
        z = (child->z - visit->transform_054[2]) >> 6;
        c = g_sine_table_70f34[(theta - 0x400) & 0xfff];
        s = g_sine_table_70f34[theta & 0xfff];
        m->t[0] = ((x * s - z * c) >> 18) << 1;
        m->t[2] = (visit->layer_z_03c + ((z * s + x * c) >> 18)) * 2;
        turn = -((long)(child->angle_008 & 0xfff) + visit->transform_054[4]);
        ang.vx = (short)visit->transform_054[3];
        ang.vy = (short)turn;
        ang.vz = 0;
        if (turn | g_camera_angle_x_70cd0)
            ang.vy = (short)(turn - 1);
        dispatch_engine_2c920(&ang, m);
        dispatch_engine_2c940(m, g_mesh_scale_5ceb0);
        dispatch_engine_2c700(m);
        visit->cached_044 = turn;
    } else {
        turn = visit->cached_044;
    }
    dispatch_engine_2c710(m);
    column = (visit->column_01c << 6) - visit->transform_054[1];
    row = (visit->row_018 - (child->row_00e & 0x3fff)) << 6;
    if ((turn | g_camera_angle_x_70cd0) == 0) {
        sv->vx = (short)row;
        sv->vy = (short)column;
        sv->vz = (short)visit->layer_y_038;
        dispatch_engine_2c690(sv);
        dispatch_engine_2c6e0();
        dispatch_engine_2c6c0();
        dispatch_engine_2c760(sv);
        if ((unsigned short)sv->vy > 240)
            visit->column_stop_016 = 1;
        if ((unsigned short)sv->vx > 320)
            visit->row_stop_017 = 1;
        dispatch_engine_2c750(visit->projected_00c);
        if (visit->depth_010 < 0xc0)
            return -1;
        visit->depth_010 >>= 2;
        return 0;
    }
    sv->vy = (short)column;
    sv->vx = (short)(row + 0x20);
    sv->vz = (short)visit->layer_y_038;
    dispatch_engine_2c690(sv);
    dispatch_engine_2c6e0();
    dispatch_engine_2c760(sv);
    if (visit->side_029 == 0) {
        if (sv->vx > 320)
            visit->row_stop_017 = 1;
    } else if (sv->vx < 0) {
        visit->row_stop_017 = 1;
    }
    dispatch_engine_2c750(visit->projected_00c);
    if (visit->depth_010 < 0x100)
        return -1;
    visit->depth_010 >>= 2;
    visit->row_local_048 = row;
    visit->column_local_04c = column;
    return 1;
}

void dispatch_local_2e600(unsigned long *stream, MeshVisit *visit, int count)
{
    SVec *sv;
    long w;

    sv = g_scratch_70f30->v;
    w = *stream++;
    if (visit->winding_024) {
        do {
            sv->vx = (char)w;
            sv->vy = (short)(w >> 8);
            sv->vz = (short)(w >> 24);
            dispatch_engine_2c690(sv);
            dispatch_engine_2c6e0();
            w = *stream++;
            *(long *)&sv->vz = 0;
            dispatch_engine_2c760(sv);
            sv++;
        } while (--count);
    } else {
        do {
            sv->vx = 0x40 - (char)w;
            sv->vy = (short)(w >> 8);
            sv->vz = (short)(w >> 24);
            dispatch_engine_2c690(sv);
            dispatch_engine_2c6e0();
            w = *stream++;
            *(long *)&sv->vz = 0;
            dispatch_engine_2c760(sv);
            sv++;
        } while (--count);
    }
}

void dispatch_local_2e6c0(unsigned long *stream, MeshVisit *visit, int count)
{
    SVec *sv;
    long w, row, column, layer;

    sv = g_scratch_70f30->v;
    w = *stream++;
    row = visit->row_local_048;
    column = visit->column_local_04c;
    layer = visit->layer_y_038;
    if (visit->winding_024) {
        do {
            sv->vx = (char)w + row;
            sv->vy = (short)((w >> 8) + column);
            sv->vz = (short)((w >> 24) + layer);
            dispatch_engine_2c690(sv);
            dispatch_engine_2c6e0();
            w = *stream++;
            dispatch_engine_2c760(sv);
            sv++;
        } while (--count);
    } else {
        row += 0x40;
        do {
            sv->vx = row - (char)w;
            sv->vy = (short)((w >> 8) + column);
            sv->vz = (short)((w >> 24) + layer);
            dispatch_engine_2c690(sv);
            dispatch_engine_2c6e0();
            w = *stream++;
            dispatch_engine_2c760(sv);
            sv++;
        } while (--count);
    }
}

int dispatch_local_2e7a0(MeshVisit *visit)
{
    Mat *m;
    LVec2 *lv;
    MeshChild *child;
    MeshRef *ref;
    SVec ang;
    long theta, orbit, key, x, z, c, s, tx, tz, curve, angle, radius, row;
    short depth;

    m = &g_scratch_70f30->m1;
    child = visit->child_008;
    theta = visit->transform_054[4];
    orbit = ((child->angle_008 >> 12) & 0xfff) + theta;
    key = orbit | 0x4000;
    if (key != visit->cached_044) {
        ref = &visit->header_004[child->ref_00c];
        x = (ref->x - visit->transform_054[0]) >> 6;
        z = (ref->z - visit->transform_054[2]) >> 6;
        c = g_sine_table_70f34[(theta - 0x400) & 0xfff];
        s = g_sine_table_70f34[theta & 0xfff];
        tx = (x * s - z * c) >> 6;
        tz = (z * s + x * c) >> 6;
        if (visit->variant_025 == 1) {
            tx += g_sine_table_70f34[(orbit + 0x400) & 0xfff] * 652;
            curve = orbit;
        } else {
            tx += g_sine_table_70f34[(orbit - 0x400) & 0xfff] * 652;
            curve = orbit - 0x800;
        }
        tz += g_sine_table_70f34[curve & 0xfff] * 652;
        m->t[0] = (tx >> 12) << 1;
        m->t[2] = (visit->layer_z_03c + (tz >> 12)) * 2;
        m->t[1] = ((visit->column_01c << 6) - visit->transform_054[1]) * 2;
        ang.vx = (short)visit->transform_054[3];
        ang.vy = ang.vz = 0;
        dispatch_engine_2c920(&ang, m);
        dispatch_engine_2c940(m, g_mesh_scale_5ceb0);
        dispatch_engine_2c710(m);
        dispatch_engine_2c700(m);
        visit->cached_044 = key;
        visit->cached_040 = visit->cell_02a;
        visit->anchor_row_050 = child->row_00e & 0x3fff;
        g_mesh_cached_column_6ffb8 = visit->column_01c;
    } else {
        m->t[1] = ((visit->column_01c << 6) - visit->transform_054[1]) * 2;
        dispatch_engine_2c710(m);
        orbit = visit->cached_044;
    }
    lv = (LVec2 *)g_scratch_70f30->v;
    row = child->row_00e & 0x3fff;
    if (visit->variant_025 == 1) {
        angle = ((visit->row_018 - row + 0x10) << 6) + orbit;
        radius = visit->layer_y_038 - 652;
    } else {
        angle = ((row - visit->row_018 - 0x11) << 6) + orbit;
        radius = -652 - visit->layer_y_038;
    }
    lv->x = (g_sine_table_70f34[(angle + 0x40) & 0xfff] * radius) >> 12;
    lv->y = (g_sine_table_70f34[(angle - 0x3c0) & 0xfff] * radius) >> 12;
    dispatch_engine_2c690(lv);
    dispatch_engine_2c6e0();
    dispatch_engine_2c760(lv);
    if (((unsigned short *)lv)[1] > 240)
        visit->column_stop_016 = 1;
    if (((unsigned short *)lv)[0] > 320)
        visit->row_stop_017 = 1;
    dispatch_engine_2c7b0(&depth);
    if (depth < 0x40)
        return -1;
    visit->depth_010 = depth;
    return 0;
}

void dispatch_local_2ea50(unsigned long *stream, MeshVisit *visit, int count)
{
    SVec *sv;
    long w, orbit, a0, r0, x, z, xl;
    short ang, d;

    sv = g_scratch_70f30->v;
    w = *stream++;
    orbit = visit->cached_044;
    a0 = visit->anchor_row_050;
    if (visit->variant_025 == 1) {
        r0 = 0x28c - visit->layer_y_038;
        a0 = ((a0 - visit->row_018 - 0x10) << 6) - orbit;
        if (visit->winding_024) {
            do {
                ang = (char)w - a0;
                d = (w >> 24) - r0;
                x = (g_sine_table_70f34[ang & 0xfff] * d) >> 12;
                z = (g_sine_table_70f34[(ang - 0x400) & 0xfff] * d) >> 12;
                sv->vx = (short)x;
                sv->vy = (short)(w >> 8);
                sv->vz = (short)z;
                dispatch_engine_2c690(sv);
                dispatch_engine_2c6e0();
                w = *stream++;
                dispatch_engine_2c760(sv);
                sv++;
            } while (--count);
        } else {
            do {
                xl = 0x40 - (char)w;
                ang = xl - a0;
                d = (w >> 24) - r0;
                x = (g_sine_table_70f34[ang & 0xfff] * d) >> 12;
                z = (g_sine_table_70f34[(ang - 0x400) & 0xfff] * d) >> 12;
                sv->vx = (short)x;
                sv->vy = (short)(w >> 8);
                sv->vz = (short)z;
                dispatch_engine_2c690(sv);
                dispatch_engine_2c6e0();
                w = *stream++;
                dispatch_engine_2c760(sv);
                sv++;
            } while (--count);
        }
    } else {
        a0 = ((visit->row_018 - a0 + 0x11) << 6) - orbit;
        r0 = -0x28c - visit->layer_y_038;
        if (visit->winding_024 == 0) {
            do {
                ang = (char)w - a0;
                d = r0 - (w >> 24);
                sv->vx = (short)((g_sine_table_70f34[ang & 0xfff] * d) >> 12);
                sv->vy = (short)(w >> 8);
                sv->vz = (short)((g_sine_table_70f34[(ang - 0x400) & 0xfff] * d) >> 12);
                dispatch_engine_2c690(sv);
                dispatch_engine_2c6e0();
                w = *stream++;
                dispatch_engine_2c760(sv);
                sv++;
            } while (--count);
        } else {
            do {
                xl = 0x40 - (char)w;
                ang = xl - a0;
                d = r0 - (w >> 24);
                sv->vx = (short)((g_sine_table_70f34[ang & 0xfff] * d) >> 12);
                sv->vy = (short)(w >> 8);
                sv->vz = (short)((g_sine_table_70f34[(ang - 0x400) & 0xfff] * d) >> 12);
                dispatch_engine_2c690(sv);
                dispatch_engine_2c6e0();
                w = *stream++;
                dispatch_engine_2c760(sv);
                sv++;
            } while (--count);
        }
    }
}

void dispatch_local_2ed10(unsigned long *stream, MeshVisit *visit, int count, int prepared)
{
    SVec *sv;
    unsigned long *shade;
    unsigned long col;
    long w, row, column, layer, top;
    short c0, c1, c2, c3, x, y;

    if (prepared) {
        row = visit->row_local_048;
        column = visit->column_local_04c;
        layer = visit->layer_y_038;
    } else {
        layer = 0;
        column = 0;
        row = 0;
    }
    col = g_shade_corners_70d50[visit->flags_02c >> 4];
    shade = g_shade_table_6f4e8[visit->flags_02c & 7];
    c0 = col & 0xff;
    c1 = (col >> 8) & 0xff;
    c2 = (col >> 16) & 0xff;
    c3 = (col >> 24) & 0xff;
    sv = g_scratch_70f30->v;
    w = *stream++;
    if (visit->winding_024) {
        do {
            x = (char)w;
            y = (short)(w >> 8);
            sv->vx = x + row;
            sv->vy = y + column;
            sv->vz = (short)((w >> 24) + layer);
            dispatch_engine_2c690(sv);
            dispatch_engine_2c6e0();
            top = c1 + ((x * (c0 - c1)) >> 6);
            *(unsigned long *)&sv->vz = shade[(top + (((c3 + ((x * (c2 - c3)) >> 6)) - top) * (0x20 - y) >> 6)) >> 2];
            dispatch_engine_2c760(sv);
            sv++;
            w = *stream++;
        } while (--count);
    } else {
        do {
            x = (char)w;
            y = (short)(w >> 8);
            sv->vx = row - x + 0x40;
            sv->vy = y + column;
            sv->vz = (short)((w >> 24) + layer);
            dispatch_engine_2c690(sv);
            dispatch_engine_2c6e0();
            top = c0 + x * (c1 - c0) / 64;
            *(unsigned long *)&sv->vz = shade[(top + ((c2 + x * (c3 - c2) / 64) - top) * (0x20 - y) / 64) >> 2];
            dispatch_engine_2c760(sv);
            sv++;
            w = *stream++;
        } while (--count);
    }
}

void dispatch_local_2ef60(MeshVisit *visit)
{
    Mat *m;
    MeshChild *child;
    MeshRef *ref;
    SVec ang;
    long theta, orbit, key, x, z, c, s, tx, tz, curve;

    m = &g_scratch_70f30->m1;
    child = visit->child_008;
    theta = visit->transform_054[4];
    orbit = ((child->angle_008 >> 12) & 0xfff) + theta;
    key = orbit | 0x4000;
    if (key != visit->cached_044) {
        ref = &visit->header_004[child->ref_00c];
        x = (ref->x - visit->transform_054[0]) >> 6;
        z = (ref->z - visit->transform_054[2]) >> 6;
        c = g_sine_table_70f34[(theta - 0x400) & 0xfff];
        s = g_sine_table_70f34[theta & 0xfff];
        tx = (x * s - z * c) >> 6;
        tz = (z * s + x * c) >> 6;
        if (visit->variant_025 == 1) {
            tx += g_sine_table_70f34[(orbit + 0x400) & 0xfff] * 652;
            curve = orbit;
        } else {
            tx += g_sine_table_70f34[(orbit - 0x400) & 0xfff] * 652;
            curve = orbit - 0x800;
        }
        tz += g_sine_table_70f34[curve & 0xfff] * 652;
        m->t[0] = (tx >> 12) << 1;
        m->t[2] = (visit->layer_z_03c + (tz >> 12)) * 2;
        m->t[1] = ((visit->column_01c << 6) - visit->transform_054[1]) * 2;
        ang.vx = (short)visit->transform_054[3];
        ang.vy = ang.vz = 0;
        dispatch_engine_2c920(&ang, m);
        dispatch_engine_2c940(m, g_mesh_scale_5ceb0);
        dispatch_engine_2c710(m);
        dispatch_engine_2c700(m);
        visit->cached_044 = key;
        visit->cached_040 = visit->cell_02a;
        visit->anchor_row_050 = child->row_00e & 0x3fff;
        g_mesh_cached_column_6ffb8 = visit->column_01c;
    }
}

void dispatch_local_2f120(unsigned long *stream, MeshVisit *visit, int count)
{
    SVec *out;
    SVec v;
    long w, a0, r0, cy;
    short ang, d;

    out = g_scratch_70f30->v;
    r0 = 0x28c - visit->layer_y_038;
    a0 = ((visit->anchor_row_050 - visit->row_018 - 0x10) << 6) - visit->cached_044;
    cy = (visit->column_01c - g_mesh_cached_column_6ffb8) << 5;
    w = *stream++;
    ang = ((0x40 - (char)w) << 1) - a0;
    d = (w >> 24) - r0;
    v.vx = (short)((g_sine_table_70f34[ang & 0xfff] * d) >> 12);
    v.vy = (short)(((w >> 8) + cy) * 2 + 0x20);
    v.vz = (short)((g_sine_table_70f34[(ang - 0x400) & 0xfff] * d) >> 12);
    dispatch_engine_2c690(&v);
    dispatch_engine_2c6e0();
    w = *stream++;
    while (--count) {
        dispatch_engine_2c760(out);
        out++;
        ang = ((0x40 - (char)w) << 1) - a0;
        d = (w >> 24) - r0;
        v.vx = (short)((g_sine_table_70f34[ang & 0xfff] * d) >> 12);
        v.vy = (short)(((w >> 8) + cy) * 2 + 0x20);
        v.vz = (short)((g_sine_table_70f34[(ang - 0x400) & 0xfff] * d) >> 12);
        dispatch_engine_2c690(&v);
        dispatch_engine_2c6e0();
        w = *stream++;
    }
    dispatch_engine_2c760(out);
    v.vx = (short)(-(g_sine_table_70f34[(0x40 - a0) & 0xfff] * r0) >> 12);
    v.vy = (short)(cy + 0x20);
    v.vz = (short)(-(g_sine_table_70f34[(-0x3c0 - a0) & 0xfff] * r0) >> 12);
    dispatch_engine_2c690(&v);
    dispatch_engine_2c6e0();
    dispatch_engine_2c7b0(&visit->depth_010);
}

void dispatch_local_2f340(unsigned long *stream, MeshVisit *visit, int count)
{
    SVec *sv;
    unsigned long col;
    long w, anchor, a0, r0, top;
    short c0, c1, c2, c3, x, y, ang, d;

    col = g_shade_corners_70d50[visit->flags_02c >> 4];
    c0 = col & 0xff;
    c1 = (col >> 8) & 0xff;
    c2 = (col >> 16) & 0xff;
    c3 = col >> 24;
    sv = g_scratch_70f30->v;
    w = *stream++;
    anchor = visit->anchor_row_050;
    if (visit->variant_025 == 1) {
        r0 = 0x28c - visit->layer_y_038;
        a0 = ((anchor - visit->row_018 - 0x10) << 6) - visit->cached_044;
        if (visit->winding_024) {
            do {
                x = (char)w;
                ang = x - a0;
                d = (w >> 24) - r0;
                y = (short)(w >> 8);
                sv->vx = (short)((g_sine_table_70f34[ang & 0xfff] * d) >> 12);
                sv->vz = (short)((g_sine_table_70f34[(ang - 0x400) & 0xfff] * d) >> 12);
                sv->vy = y;
                dispatch_engine_2c690(sv);
                dispatch_engine_2c6e0();
                top = c1 + ((x * (c0 - c1)) >> 6);
                *(unsigned long *)&sv->vz = g_shade_table_6f4e8[0][(top + (((c3 + ((x * (c2 - c3)) >> 6)) - top) * (0x20 - y) >> 6)) >> 2];
                dispatch_engine_2c760(sv);
                w = *stream++;
                sv++;
            } while (--count);
        } else {
            do {
                x = (char)w;
                ang = 0x40 - x - a0;
                d = (w >> 24) - r0;
                y = (short)(w >> 8);
                sv->vx = (short)((g_sine_table_70f34[ang & 0xfff] * d) >> 12);
                sv->vz = (short)((g_sine_table_70f34[(ang - 0x400) & 0xfff] * d) >> 12);
                sv->vy = y;
                dispatch_engine_2c690(sv);
                dispatch_engine_2c6e0();
                top = c0 + ((x * (c1 - c0)) >> 6);
                *(unsigned long *)&sv->vz = g_shade_table_6f4e8[0][(top + (((c2 + ((x * (c3 - c2)) >> 6)) - top) * (0x20 - y) >> 6)) >> 2];
                dispatch_engine_2c760(sv);
                w = *stream++;
                sv++;
            } while (--count);
        }
    } else {
        a0 = ((visit->row_018 - anchor + 0x11) << 6) - visit->cached_044;
        r0 = -0x28c - visit->layer_y_038;
        if (visit->winding_024 == 0) {
            do {
                x = (char)w;
                ang = x - a0;
                d = r0 - (w >> 24);
                y = (short)(w >> 8);
                sv->vx = (short)((g_sine_table_70f34[ang & 0xfff] * d) >> 12);
                sv->vz = (short)((g_sine_table_70f34[(ang - 0x400) & 0xfff] * d) >> 12);
                sv->vy = y;
                dispatch_engine_2c690(sv);
                dispatch_engine_2c6e0();
                top = c0 + ((x * (c1 - c0)) >> 6);
                *(unsigned long *)&sv->vz = g_shade_table_6f4e8[0][(top + (((c2 + ((x * (c3 - c2)) >> 6)) - top) * (0x20 - y) >> 6)) >> 2];
                dispatch_engine_2c760(sv);
                w = *stream++;
                sv++;
            } while (--count);
        } else {
            do {
                x = (char)w;
                ang = 0x40 - x - a0;
                d = r0 - (w >> 24);
                y = (short)(w >> 8);
                sv->vx = (short)((g_sine_table_70f34[ang & 0xfff] * d) >> 12);
                sv->vz = (short)((g_sine_table_70f34[(ang - 0x400) & 0xfff] * d) >> 12);
                sv->vy = y;
                dispatch_engine_2c690(sv);
                dispatch_engine_2c6e0();
                top = c1 + ((x * (c0 - c1)) >> 6);
                *(unsigned long *)&sv->vz = g_shade_table_6f4e8[0][(top + (((c3 + ((x * (c2 - c3)) >> 6)) - top) * (0x20 - y) >> 6)) >> 2];
                dispatch_engine_2c760(sv);
                w = *stream++;
                sv++;
            } while (--count);
        }
    }
}

typedef struct CamActor {
    long x_000, y_004, z_008;
    short unknown_00c;
    short depth_00e;
    unsigned char unknown_010[0x94 - 0x10];
    unsigned char facing_094;
} CamActor;
typedef struct CellRef {
    unsigned short ref_000;
    unsigned short unknown_002;
} CellRef;

extern CamActor *g_frame_global_734f4;
extern long g_render_transform_70c60[8];
extern long g_camera_x_70ca0, g_camera_y_70ca4;
extern long g_camera_offset_x_70cb8, g_camera_offset_y_70cbc, g_camera_offset_z_70cc0;
extern long g_camera_angle_y_70cd4;
extern long g_camera_center_x_70cdc, g_camera_center_y_70ce0;
extern long g_camera_focal_70ce4;
extern unsigned char g_mode_table_value_70d00;
extern unsigned long g_camera_flags_70d10;
extern short g_element_width_70d14;
extern long g_camera_z_base_70d1c;
extern long *g_rebuilt_row_pointers_70dcc[16];
extern short g_rebuilt_row_offsets_70e0c[32];
extern char g_group_count_70ece;
extern CellRef *g_header_pointer_70d54;
extern MeshChild *g_header_pointer_70d58;
void dispatch_engine_2c9d0(long x, long y);
void dispatch_engine_2c9f0(long focal);

void dispatch_local_2f840(void)
{
    MeshChild *desc;
    short *trig;
    long z, layer, row, column, start, key, packed, base, offset, phase, angle;
    long x0, z0, s, c, turn, zs, xs, d, ox, oz;

    z = g_frame_global_734f4->z_008 >> 16;
    column = g_frame_global_734f4->y_004 >> 16;
    offset = 0;
    layer = ((short)z + 0x40) >> 7;
    row = g_camera_x_70ca0 >> 6;
    column = (short)column >> 6;
    if (layer < 0)
        layer = 0;
    else if (layer > g_group_count_70ece - 1)
        layer = g_group_count_70ece - 1;
    start = g_rebuilt_row_offsets_70e0c[layer + 16];
    row -= start;
    if (row < 0)
        row = 0;
    else if (row > g_rebuilt_row_offsets_70e0c[layer] - 1)
        row = g_rebuilt_row_offsets_70e0c[layer] - 1;
    if (column < 0)
        column = 0;
    else if (column > g_element_width_70d14 - 1)
        column = g_element_width_70d14 - 1;
    key = g_rebuilt_row_pointers_70dcc[layer][row * g_element_width_70d14 + column];
    row += start;
    desc = &g_header_pointer_70d58[g_header_pointer_70d54[(key >> 11) & 0x1fff].ref_000];
    packed = desc->angle_008;
    base = ((unsigned long)packed >> 24) << 4;
    packed &= 0xfff;
    if (g_mode_table_value_70d00)
        offset = 0x20;
    phase = g_camera_x_70ca0 & 0x3f;
    angle = ((((((packed - base) << 20) >> 20) * phase) >> 6) + offset + base) & 0xfff;
    x0 = desc->x;
    z0 = desc->z;
    trig = g_sine_table_70f34;
    if (!(desc->row_00e & 0xc000)) {
        row = (row - (desc->row_00e & 0x3fff)) << 6;
        x0 += trig[packed & 0xfff] * row;
        z0 += trig[(packed - 0x400) & 0xfff] * row;
    }
    s = trig[angle & 0xfff];
    x0 += s * phase;
    c = trig[(angle - 0x400) & 0xfff];
    z0 += c * phase;
    turn = g_camera_angle_y_70cd4 + 0x400;
    z -= g_camera_z_base_70d1c;
    zs = ((0x1000 - trig[(turn - 0x400) & 0xfff]) * z) >> 6;
    xs = (trig[turn & 0xfff] * z) >> 6;
    if (g_camera_flags_70d10 & 0x10) {
        d = g_camera_x_70ca0 - (g_frame_global_734f4->x_000 >> 16);
        zs -= (trig[(g_camera_angle_y_70cd4 - 0x400) & 0xfff] * d) >> 6;
        xs -= ((0x1000 - trig[g_camera_angle_y_70cd4 & 0xfff]) * d) >> 6;
    }
    x0 += (xs * s - zs * c) >> 6;
    z0 += (xs * c + zs * s) >> 6;
    ox = g_camera_offset_x_70cb8 >> 10;
    oz = g_camera_offset_z_70cc0 >> 10;
    g_render_transform_70c60[0] = x0 + ((ox * s - oz * c) >> 6);
    g_render_transform_70c60[2] = z0 + ((ox * c + oz * s) >> 6);
    g_render_transform_70c60[1] = (short)(g_camera_offset_y_70cbc >> 16) + g_camera_y_70ca4;
    if (angle == 1)
        angle = 0;
    g_render_transform_70c60[3] = g_camera_angle_x_70cd0;
    g_render_transform_70c60[4] = g_camera_angle_y_70cd4 - angle;
    g_render_transform_70c60[7] = ((g_camera_y_70ca4 - 0x20) >> 6) + 2;
    g_render_transform_70c60[6] = (g_camera_x_70ca0 - 0x20) >> 6;
    dispatch_engine_2c9d0(g_camera_center_x_70cdc, g_camera_center_y_70ce0);
    dispatch_engine_2c9f0(g_camera_focal_70ce4);
}

extern long g_camera_z_70ca8;
extern long g_camera_grid_x_70cac, g_camera_grid_y_70cb0, g_camera_grid_z_70cb4;
extern long g_camera_prior_x_70cc4, g_camera_prior_y_70cc8;
extern long g_frame_global_70ce8, g_frame_global_70cec;
extern char g_camera_iterations_70cfd, g_camera_bounds_id_70cff;
extern short g_camera_adjust_x_70d02, g_camera_adjust_y_70d04, g_camera_adjust_z_70d06;
extern short g_camera_depth_low_70d0c, g_camera_depth_high_70d0e;
extern long g_camera_z_offset_70d18;
extern unsigned short g_camera_threshold_x_5e058[];
extern unsigned short g_camera_threshold_y_5e080[];
extern unsigned short g_camera_threshold_z_5e0a8[];
int dispatch_local_20ea0(short *bounds, int id, int mode, long x);

void dispatch_local_2fb70(void)
{
    CamActor *a;
    unsigned short *p;
    unsigned short flags, wrap;
    long x, y, z, d, sign, step, lim, scale;
    short depth;
    int bounded, i;
    short bounds[8];

    g_camera_prior_y_70cc8 = g_camera_y_70ca4;
    x = g_camera_x_70ca0;
    g_camera_prior_x_70cc4 = x;
    bounded = 0;
    if (g_camera_flags_70d10 & 0x80) {
        bounded = dispatch_local_20ea0(bounds, g_camera_bounds_id_70cff, 0x18, x);
        x = g_camera_x_70ca0;
    }
    a = g_frame_global_734f4;
    for (i = 0; i < g_camera_iterations_70cfd; i++) {
        if (!(g_camera_flags_70d10 & 0x100)) {
            if (g_camera_flags_70d10 & 2) {
                d = (short)(a->x_000 >> 16) - x;
                wrap = g_camera_flags_70d10 & 1;
                if (wrap)
                    d = (d << 20) >> 20;
                d += g_camera_center_x_70cdc;
                d = d * 2 - 0x140;
                if (wrap)
                    d = (d << 19) >> 19;
                lim = g_camera_adjust_x_70d02;
                if (a->facing_094 == 0)
                    lim = -lim;
                d -= lim;
                sign = d;
                if (d < 0)
                    d = -d;
                p = g_camera_threshold_x_5e058;
                while (*p++ < (unsigned short)d)
                    ;
                step = p - g_camera_threshold_x_5e058 - 1;
                if (wrap) {
                    if (sign > 0)
                        x = ((x + step - 0x80) & 0xfff) + 0x80;
                    else
                        x = ((x - step - 0x80) & 0xfff) + 0x80;
                    g_camera_x_70ca0 = x;
                } else if (step) {
                    if (sign > 0) {
                        x += step;
                        g_camera_x_70ca0 = x;
                        if (x > g_frame_global_70cec) {
                            x = g_frame_global_70cec;
                            g_camera_x_70ca0 = x;
                        }
                    } else {
                        x -= step;
                        g_camera_x_70ca0 = x;
                        if (x < g_frame_global_70ce8) {
                            x = g_frame_global_70ce8;
                            g_camera_x_70ca0 = x;
                        }
                    }
                }
            } else {
                x = (short)(a->x_000 >> 16);
                g_camera_x_70ca0 = x;
                if (x > g_frame_global_70cec) {
                    x = g_frame_global_70cec;
                    g_camera_x_70ca0 = x;
                } else if (x < g_frame_global_70ce8) {
                    x = g_frame_global_70ce8;
                    g_camera_x_70ca0 = x;
                }
            }
        }
        if (g_camera_flags_70d10 & 8) {
            z = g_camera_z_70ca8;
            d = ((short)(a->z_008 >> 16) - z + g_camera_z_offset_70d18 - g_camera_focal_70ce4) * 2
                + g_camera_adjust_z_70d06;
            sign = d;
            if (d < 0)
                d = -d;
            p = g_camera_threshold_z_5e0a8;
            while (*p++ < (unsigned short)d)
                ;
            step = p - g_camera_threshold_z_5e0a8 - 1;
            if (step) {
                if (sign > 0)
                    z += step;
                else
                    z -= step;
            }
            g_camera_z_70ca8 = z;
            if (step & (i > 0))
                continue;
        } else {
            g_camera_z_70ca8 = (short)(a->z_008 >> 16) + g_camera_z_offset_70d18 - g_camera_focal_70ce4;
        }
        flags = g_camera_flags_70d10;
        if (!(flags & 0x200)) {
            if (flags & 4) {
                y = g_camera_y_70ca4;
                d = ((short)(a->y_004 >> 16) - y + g_camera_center_y_70ce0) * 2 - 0x168;
                if (g_camera_flags_70d10 & 0x40) {
                    depth = a->depth_00e;
                    if (depth < g_camera_depth_low_70d0c) {
                        scale = (g_camera_depth_low_70d0c - depth + 0x10) << 4;
                        if (scale > 0x200)
                            scale = 0x200;
                        d = (scale * d) >> 8;
                    }
                    if (depth > g_camera_depth_high_70d0e) {
                        scale = (depth - g_camera_depth_high_70d0e + 0x10) << 4;
                        if (scale > 0x200)
                            scale = 0x200;
                        d = (scale * d) >> 8;
                    }
                }
                d += g_camera_adjust_y_70d04;
                sign = d;
                if (d < 0)
                    d = -d;
                p = g_camera_threshold_y_5e080;
                while (*p++ < (unsigned short)d)
                    ;
                step = p - g_camera_threshold_y_5e080 - 1;
                if (step) {
                    if (sign > 0) {
                        y += step;
                        lim = (g_element_width_70d14 << 6) - 0x40;
                        if (y > lim)
                            y = lim;
                    } else {
                        y -= step;
                        if (y < 0x7c)
                            y = 0x7c;
                    }
                }
                if (bounded) {
                    if (y < bounds[0])
                        y = bounds[0];
                    else if (y > bounds[2])
                        y = bounds[2];
                }
            } else {
                y = (short)(a->y_004 >> 16);
                if (bounded) {
                    if (y < bounds[0])
                        y = bounds[0];
                    else if (y > bounds[2])
                        y = bounds[2];
                } else {
                    lim = (g_element_width_70d14 << 6) - 0x40;
                    if (y > lim)
                        y = lim;
                }
            }
            g_camera_y_70ca4 = y;
        }
    }
    g_camera_grid_y_70cb0 = g_camera_y_70ca4 >> 6;
    g_camera_grid_x_70cac = x >> 6;
    g_camera_grid_z_70cb4 = g_camera_z_70ca8 >> 6;
}

typedef struct CollisionActor {
    long unknown_000, unknown_004, z_008;
} CollisionActor;
typedef void (*CollisionAction)(int arg);
extern unsigned short *g_configuration_dword_713bc;
extern CollisionAction *g_configuration_dword_713c0;
extern CollisionActor *g_native_pointer_7217c;
extern short g_pipeline_word_71c5c;
extern short g_pipeline_word_71bc4;
extern unsigned long g_pipeline_dword_71840;

void eng1_2ff50(int index)
{
    int w;
    short lo, hi;

    w = g_configuration_dword_713bc[index];
    if (w & 0x8000) {
        g_configuration_dword_713c0[(w >> 8) & 0x7f](w & 0xff);
        return;
    }
    if (w & 0x10) {
        lo = (g_native_pointer_7217c->z_008 >> 16) & 0x7f80;
        hi = lo + 0x80;
        if (g_pipeline_word_71c5c > lo)
            g_pipeline_word_71c5c = lo;
        if (g_pipeline_word_71bc4 < hi)
            g_pipeline_word_71bc4 = hi;
        g_pipeline_dword_71840++;
    }
    if (w & 0x20) {
        lo = ((g_native_pointer_7217c->z_008 >> 16) - 0x40) & 0x7f80;
        hi = lo + 0x80;
        if (g_pipeline_word_71c5c > lo)
            g_pipeline_word_71c5c = lo;
        if (g_pipeline_word_71bc4 < hi)
            g_pipeline_word_71bc4 = hi;
        g_pipeline_dword_71840++;
    }
}
