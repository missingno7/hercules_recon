/* ENG3.DLL 0x20010..0x22e40 (lane y6m merge of lane y6 and its sub-lanes, 2026-10-20).
 * Contents in address order: the tail of the engine-interface wrapper run 0x20010..0x20190 (its head,
 * 0x1f830.., is recorded in calibration/eng3/interface_wrappers.c, actor_services_1e000.c and the TITLE
 * u0c1f0 twin), the track renderer/camera code 0x20190..0x22b30, and the tile/spawn services
 * 0x22b30..0x22e40 (ENG1 0x30010.. / TITLE r17ad0.c family).  One C++ unit (VC5 /TP base/index order),
 * functions in extern "C".  No system or count-only headers (owner ruling 2026-10-20).
 * Record layouts are measured caller/callee views; field names are provisional. */

extern "C" {

typedef struct Actor Actor;
struct Actor {
    long x, y, z;
    unsigned char unknown_0c[0x1f - 0x0c];
    unsigned char unknown_1f;
    unsigned char unknown_20[0x38 - 0x20];
    unsigned short profile;
    unsigned char unknown_3a[0x94 - 0x3a];
    unsigned char unknown_94, unknown_95, unknown_96, unknown_97;
    unsigned char unknown_98[0x9c - 0x98];
    long origin_x;
    unsigned char unknown_a0[0x120 - 0xa0];
    long *unknown_120;                      /* count-prefixed list of linked actors */
};

/* 16-byte track cell: g_eng3_cells_40540[layer][row] and the rebuilt g_eng3_track_40794. */
typedef struct Eng3Cell {
    long x;
    long z;
    short angle_08;
    short angle_0a;
    unsigned short unknown_0c;
    unsigned char curve;
    unsigned char pivot;
} Eng3Cell;

/* 16-byte layer header (g_eng3_layers_40674); the three shorts are self-relative offsets. */
typedef struct Eng3Layer {
    unsigned char link;
    unsigned char width;
    unsigned char extent;
    unsigned char depth;
    unsigned short rotation;
    short entries;
    short neighbours;
    short shape;
    unsigned char unknown_0c[4];
} Eng3Layer;

/* 12-byte per-layer visible range (g_eng3_ranges_4078c). */
typedef struct Eng3Range {
    long tick;
    long first;
    long end;
} Eng3Range;

typedef struct Eng3Matrix {
    short m[3][3];
    long t[3];
} Eng3Matrix;

/* 9-long camera transform at 0x405a0, copied into every visit. */
typedef struct Eng3View {
    long x, y, z;
    long pitch, yaw;
    long unknown_14;
    long unknown_18;
    long unknown_1c;
    long column;
} Eng3View;

typedef struct Eng3Visit {
    unsigned char unknown_00[8];
    Eng3Cell *cell;
    unsigned char unknown_0c[4];
    short depth;
    short unknown_12;
    long column;
    long lateral;
    long row;
    unsigned long packed;
    unsigned char winding;
    unsigned char curve;
    unsigned char shift;
    unsigned char limit;
    unsigned char mode;
    unsigned char layer;
    unsigned char lane;
    unsigned char side;
    unsigned char unknown_2c;
    unsigned char stop;
    short start;
    unsigned short flags;
    unsigned char unknown_32[6];
    long tick;
    unsigned char unknown_3c[8];
    long angle;
    long pivot;
    Eng3View view;
} Eng3Visit;

typedef struct Eng3LevelInfo {
    unsigned char unknown_00[3];
    unsigned char mode;
    unsigned char unknown_04[8];
    unsigned char limit;
    unsigned char shift;
    unsigned char unknown_0e[0x20 - 0x0e];
} Eng3LevelInfo;

typedef struct Eng3Context {
    signed char level;
    unsigned char unknown_01[3];
    unsigned char state;
    unsigned char unknown_05[0x38 - 0x05];
    long tick;
    unsigned char unknown_3c[4];
    long hidden;
} Eng3Context;

typedef struct Eng3Interface {
    unsigned long signature;
    Eng3Context *context;
    unsigned long unknown_08[2];
    unsigned char *scratch;
    short *sine;
    unsigned long unknown_018[(0x2bc - 0x018) / 4];
    void (*slot_2bc)(unsigned long);
    unsigned long unknown_2c0[3];
    void (*slot_2cc)(void *);
    unsigned long unknown_2d0[7];
    void (*slot_2ec)(void *);
    void (*slot_2f0)(void *);
    unsigned long unknown_2f4[3];
    void (*slot_300)(void *);
    unsigned long unknown_304[3];
    void (*slot_310)(void *, void *);
    unsigned long unknown_314[4];
    void (*slot_324)(void);
    unsigned long unknown_328[2];
    void (*slot_330)(void);
    unsigned long unknown_334[10];
    void (*slot_35c)(void *, void *);
    unsigned long unknown_360[2];
    void (*slot_368)(void *);
    void (*slot_36c)(void);
    void (*slot_370)(void);
    void (*slot_374)(void *, void *, void *);
    unsigned long unknown_378[3];
    void (*slot_384)(void *);
    int (*slot_388)(int, int, int, int, int, int, int);
    unsigned long unknown_38c;
    void (*slot_390)(void *, void *, void *);
    unsigned long unknown_394[19];
    void (*slot_3e0)(unsigned long);
    void (*slot_3e4)(void *);
} Eng3Interface;

typedef struct Eng3Frame {
    unsigned char unknown_00[0x88];
    unsigned long ot[1];
} Eng3Frame;

typedef struct Eng3Vertex {
    long xy;
    unsigned long rgb;
} Eng3Vertex;

typedef union Eng3Color {
    unsigned long rgb;
    struct {
        unsigned char r, g, b, code;
    } c;
} Eng3Color;

typedef struct Eng3PolyFT4 {
    unsigned long tag;
    unsigned long color;
    long xy0;
    unsigned long uv0;
    long xy1;
    unsigned long uv1;
    long xy2;
    unsigned short uv2, pad2;
    long xy3;
    unsigned short uv3, pad3;
} Eng3PolyFT4;

typedef struct Eng3PolyGT4 {
    unsigned long tag;
    Eng3Color c0;
    long xy0;
    unsigned long uv0;
    unsigned long rgb1;
    long xy1;
    unsigned long uv1;
    unsigned long rgb2;
    long xy2;
    unsigned short uv2, pad2;
    unsigned long rgb3;
    long xy3;
    unsigned short uv3, pad3;
} Eng3PolyGT4;

typedef struct Eng3UV {
    unsigned long uv0;
    unsigned long uv1;
    unsigned long uv23;
} Eng3UV;

typedef struct ActionRecord8 {
    void (*fn)(Actor *);
    unsigned long unknown;
} ActionRecord8;

typedef struct ActionRecord12 {
    void (*fn)(Actor *);
    unsigned long unknown[2];
} ActionRecord12;

extern Eng3Interface g_engine_interface;
extern Eng3Matrix g_eng3_identity_39490;
extern long g_eng3_scale_39518[];
extern Eng3LevelInfo g_eng3_levels_39870[];
extern long g_eng3_unknown_3fb88;
extern long g_eng3_unknown_3fb8c;
extern long g_eng3_unknown_38520;
extern Eng3Cell *g_eng3_cells_40540[];
extern Eng3View g_eng3_view_405a0;
extern long g_eng3_camera_x_405e0;
extern long g_eng3_view_column_405ec;
extern long g_eng3_view_angle_40614;
extern signed char g_eng3_layer_40638;
extern signed char g_eng3_spread_4063b;
extern signed char g_eng3_spread_4063c;
extern short g_eng3_height_40640;
extern void (*g_eng3_after_draw_4064c)(void);
extern unsigned char *g_eng3_uvs_40658;
extern short *g_eng3_shapes_4065c;
extern Eng3Layer *g_eng3_layers_40674;
extern unsigned short *g_eng3_cell_flags_4067c;
extern long *g_eng3_lanes_40684[];
extern short g_eng3_lane_rows_40704[];
extern short g_eng3_lane_columns_40744[];
extern Eng3Range *g_eng3_ranges_4078c;
extern Eng3Cell *g_eng3_track_40794;
extern ActionRecord12 *g_eng3_game_40b8c;
extern ActionRecord8 *g_eng3_middle_40b98;
extern ActionRecord8 *g_eng3_host_40bb4;
extern void (**g_eng3_tile_actions_40bd4)(int);
extern unsigned short *g_eng3_tiles_40c38;
extern Eng3Frame *g_eng3_frame_41448;
extern unsigned long g_eng3_colors_41460[];
extern unsigned char *g_eng3_prim_43164;
extern Actor *g_eng3_actor_431d4;

extern void eng3_slot_190(long a, long b, long c);
extern void eng3_slot_1a0(void);
extern void eng3_slot_1b0(void *matrix);
extern void eng3_slot_1b4(void *matrix);
extern void eng3_slot_1c0(long *out);
extern void eng3_slot_1d4(void *prim);
extern void eng3_slot_1dc(void *prim);
extern void eng3_slot_290(short *angles, void *matrix);
extern void eng3_slot_294(void *matrix, long *scale);
extern int dispatch_local_2a170(long x, long y, long z, int mode);
extern Actor *eng3_1cb00(long x, long y, long z, int kind);
extern Actor *create_eng3_middle_actor(long x, long y, long z, int kind);
extern Actor *create_eng3_kind_actor(long x, long y, long z, int kind);
extern void append_chain(Actor *owner, Actor *child);

int eng3_20b60(Eng3Visit *visit);
int eng3_21200(Eng3Visit *visit);
int eng3_206f0(Eng3Visit *visit, int lane);
int eng3_20c50(long *stream, Eng3Visit *visit, int count);
int eng3_20ed0(long *stream, Eng3Visit *visit, int count);
int eng3_21390(long *stream, Eng3Visit *visit, int count);
int eng3_21a10(long *stream, Eng3Visit *visit, int count);
void eng3_204c0(Eng3Visit *visit, unsigned char *entries, int layer, int start, int count, int force);

/* GTE-style vertex/depth wrappers and the debug hook (projectors 0x20c50..0x21a10). */
extern void eng3_slot_194(short *vec);
extern void eng3_slot_1a4(void);
extern void eng3_slot_1a8(void);
extern void eng3_slot_1c4(short *vec);
extern void eng3_slot_1c8(void *out);
extern void eng3_slot_1e4(short *out);
extern void eng3_empty_2390(const char *);

/* 0x40678: per-cell corner shade bytes (one byte per quad corner), indexed by visit->flags >> 4
 * (project name from calibration/eng3/render_unit_22f00.cpp). */
extern unsigned long *g_eng3_shades_40678;

/* camera state (0x22070 view setup, 0x222c0 follow) */
extern unsigned short g_eng3_damp_x_399f0[];
extern unsigned short g_eng3_damp_y_39a18[];
extern unsigned short g_eng3_damp_z_39a40[];
extern long g_eng3_angle_3fc78;
extern long g_eng3_unknown_40580;
extern long g_eng3_camera_y_405e4;
extern long g_eng3_camera_z_405e8;
extern long g_eng3_view_row_405f0;
extern long g_eng3_view_depth_405f4;
extern long g_eng3_offset_x_405f8;
extern long g_eng3_offset_y_405fc;
extern long g_eng3_offset_z_40600;
extern long g_eng3_prior_x_40604;
extern long g_eng3_prior_y_40608;
extern long g_eng3_center_x_4061c;
extern long g_eng3_center_y_40620;
extern long g_eng3_focal_40624;
extern signed char g_eng3_iterations_x_40635;
extern signed char g_eng3_iterations_y_40636;
extern signed char g_eng3_iterations_z_40637;
extern signed char g_eng3_bounds_id_40639;
extern unsigned char g_eng3_camera_flags_4063a;
extern short g_eng3_adjust_y_40642;
extern short g_eng3_adjust_z_40644;

extern void eng3_slot_2b8(long a, long b);
extern long eng3_1b2e0(long x, long y, short z, int layer);
extern int eng3_1b650(short *bounds, int id, int mode, long x);

/* track rebuild: layer count byte, allocator wrappers */
extern signed char g_eng3_layer_count_40786;
extern void eng3_slot_040(void *block);
extern void *eng3_slot_078(void **block, unsigned long size, unsigned long flags);

/* 0x22de0 (TITLE 0x17b90 / ENG1 0x303d0 twin) callees, by their recorded symbols */
extern void title_02dc0(Actor *actor);
extern void title_04410(Actor *actor);
extern int eng3_24a80(Actor *actor);


void eng3_slot_2bc(unsigned long a)
{
    g_engine_interface.slot_2bc(a);
}

void eng3_slot_2cc(void *a)
{
    g_engine_interface.slot_2cc(a);
}

void eng3_slot_2ec(void *a)
{
    g_engine_interface.slot_2ec(a);
}

void eng3_slot_2f0(void *a)
{
    g_engine_interface.slot_2f0(a);
}

void eng3_slot_300(void *a)
{
    g_engine_interface.slot_300(a);
}

void eng3_slot_310(void *a, void *b)
{
    g_engine_interface.slot_310(a, b);
}

void eng3_slot_324(void)
{
    g_engine_interface.slot_324();
}

void eng3_slot_330(void)
{
    g_engine_interface.slot_330();
}

void eng3_slot_35c(void *a, void *b)
{
    g_engine_interface.slot_35c(a, b);
}

void eng3_slot_368(void *a)
{
    g_engine_interface.slot_368(a);
}

void eng3_slot_36c(void)
{
    g_engine_interface.slot_36c();
}

void eng3_slot_370(void)
{
    g_engine_interface.slot_370();
}

void eng3_slot_374(void *a, void *b, void *c)
{
    g_engine_interface.slot_374(a, b, c);
}

void eng3_slot_384(void *a)
{
    g_engine_interface.slot_384(a);
}

/* 0x20120: interface slot 0x388 (recorded twin TITLE 0xc8b0; ENG3 link name _snd_play in the
 * /Od sound unit calibration/eng1/sound_od.c). */
int snd_play(int a, int b, int c, int d, int e, int f, int g)
{
    return g_engine_interface.slot_388(a, b, c, d, e, f, g);
}

void eng3_slot_390(void *a, void *b, void *c)
{
    g_engine_interface.slot_390(a, b, c);
}

void eng3_slot_3e0(unsigned long a)
{
    g_engine_interface.slot_3e0(a);
}

void eng3_slot_3e4(void *a)
{
    g_engine_interface.slot_3e4(a);
}

void eng3_20190(void)
{
    unsigned char *list;
    Eng3Visit visit;
    int spread;
    int k;
    int v;
    int link;
    int width;
    int w;
    Eng3Layer *layer;
    unsigned char *entries;

    *(Eng3Matrix *)g_engine_interface.scratch = g_eng3_identity_39490;
    *(Eng3Matrix *)(g_engine_interface.scratch + 0x20) = g_eng3_identity_39490;
    visit.view = g_eng3_view_405a0;
    spread = g_eng3_spread_4063b + g_eng3_spread_4063c;
    visit.tick = g_engine_interface.context->tick << 8 >> 8;
    visit.limit = g_eng3_levels_39870[g_engine_interface.context->level].limit;
    visit.shift = g_eng3_levels_39870[g_engine_interface.context->level].shift;
    visit.mode = g_eng3_levels_39870[g_engine_interface.context->level].mode;
    switch ((g_eng3_view_angle_40614 >> 8) & 0xf) {
    case 0: case 1: case 2:
    case 13: case 14: case 15:
        visit.start = g_eng3_view_column_405ec - 1;
        break;
    case 5: case 6: case 7: case 8: case 9: case 10:
        visit.start = g_eng3_view_column_405ec - spread + 4;
        break;
    case 3: case 4:
    case 11: case 12:
        visit.start = g_eng3_view_column_405ec - spread / 2 + 4;
        break;
    }
    entries = (unsigned char *)&g_eng3_layers_40674[g_eng3_layer_40638].entries;
    entries += *(short *)entries;
    eng3_204c0(&visit, entries, g_eng3_layer_40638, visit.start, spread, 0);
    list = (unsigned char *)&g_eng3_layers_40674[g_eng3_layer_40638].neighbours;
    list += *(short *)list;
    while ((k = *list++) != 0xff) {
        layer = &g_eng3_layers_40674[k];
        entries = (unsigned char *)&layer->entries;
        entries += *(short *)entries;
        if ((unsigned int)(spread - layer->width + visit.start) < (unsigned int)g_eng3_lane_rows_40704[entries[1]]) {
            v = spread - layer->width + visit.start;
            if (v > spread)
                eng3_204c0(&visit, entries, k, v - spread, spread, 0);
            else
                eng3_204c0(&visit, entries, k, 0, v, 0);
        }
    }
    layer = &g_eng3_layers_40674[g_eng3_layer_40638];
    link = layer->link;
    if (link != 0x7f) {
        width = layer->width;
        if (visit.start < layer->extent) {
            entries = (unsigned char *)&g_eng3_layers_40674[link].entries;
            entries += *(short *)entries;
            eng3_204c0(&visit, entries, link, width + visit.start, spread, width);
        }
        list = (unsigned char *)&g_eng3_layers_40674[link].neighbours;
        list += *(short *)list;
        while ((k = *list++) != 0xff) {
            if (k == g_eng3_layer_40638)
                continue;
            layer = &g_eng3_layers_40674[k];
            w = layer->width;
            entries = (unsigned char *)&layer->entries;
            entries += *(short *)entries;
            if (spread - w + visit.start < g_eng3_lane_rows_40704[entries[1]]) {
                v = width + spread - w + visit.start;
                if (v > spread)
                    eng3_204c0(&visit, entries, k, v - spread, spread, 0);
                else
                    eng3_204c0(&visit, entries, k, 0, v, 0);
            }
        }
    }
    if (g_eng3_after_draw_4064c != 0)
        g_eng3_after_draw_4064c();
    if (g_engine_interface.context->state == 0) {
        g_eng3_unknown_3fb88 = 0;
        g_eng3_unknown_3fb8c = 0;
        g_eng3_unknown_38520 = 0x1e0;
    }
}

void eng3_204c0(Eng3Visit *visit, unsigned char *entries, int layer, int start, int count, int force)
{
    unsigned char *p;
    int n;
    int end;
    int i;
    int j;
    int limit;
    int lane;
    int lateral;
    int rows;
    int columns;
    long offset;
    long value;
    long *base;

    if (g_eng3_layer_40638 != layer && force == 0 && start > g_eng3_layers_40674[layer].depth)
        return;
    visit->layer = layer;
    n = *entries++;
    g_eng3_ranges_4078c[layer].first = start;
    end = start + count;
    g_eng3_ranges_4078c[layer].end = end;
    for (visit->row = start; visit->row < end; visit->row++) {
        p = entries;
        rows = g_eng3_lane_rows_40704[*p];
        columns = g_eng3_lane_columns_40744[*p];
        visit->cell = g_eng3_cells_40540[layer] + visit->row;
        offset = columns * visit->row;
        for (i = 0; i < n; i++) {
            lane = *p++;
            lateral = *(signed char *)p++;
            if ((unsigned long)visit->row >= (unsigned long)rows)
                continue;
            g_eng3_ranges_4078c[layer].tick = g_engine_interface.context->tick;
            visit->lateral = lateral << 5;
            visit->lane = lane;
            visit->curve = visit->cell->curve;
            if (i == 0) {
                if (visit->curve)
                    eng3_21200(visit);
                else
                    eng3_20b60(visit);
            }
            base = g_eng3_lanes_40684[lane] + offset;
            limit = 8;
            visit->stop = 0;
            visit->side = 0;
            for (j = 0; j < limit; j++) {
                visit->column = visit->view.column + j;
                if ((unsigned long)visit->column >= (unsigned long)columns)
                    break;
                value = base[visit->column];
                if ((value & 1) == 0) {
                    visit->packed = value;
                    if (eng3_206f0(visit, lane))
                        return;
                    if (visit->stop)
                        limit = j + 1;
                } else {
                    j += (value >> 1) & 0x1f;
                }
            }
            limit = 9;
            visit->stop = 0;
            visit->side = 1;
            for (j = 1; j < limit; j++) {
                visit->column = visit->view.column - j;
                if ((unsigned long)visit->column >= (unsigned long)columns)
                    break;
                value = base[visit->column];
                if ((value & 1) == 0) {
                    visit->packed = value;
                    if (eng3_206f0(visit, lane))
                        return;
                    if (visit->stop)
                        limit = j + 1;
                } else {
                    j += (value >> 6) & 0x1f;
                }
            }
        }
    }
}

int eng3_206f0(Eng3Visit *visit, int lane)
{
    unsigned char *p;
    int count;
    int remaining;
    int mode;
    int depth;
    unsigned long shade;
    unsigned char *ot;
    unsigned char *uvs;
    unsigned char *verts;
    unsigned char *packet;
    unsigned long packed;
    int index;
    long face;
    unsigned long uv0;
    unsigned long uv1;
    unsigned long uv;
    unsigned short clut;
    unsigned long uv23;
    unsigned long *head;
    unsigned char *v0;
    unsigned char *v1;
    unsigned char *v2;
    unsigned char *v3;
    Eng3UV *e;
    unsigned long color;
    long xy0;
    long xy1;
    long xy2;
    long xy3;

    mode = 0;
    if (g_engine_interface.context->hidden != 0)
        return 0;
    packed = visit->packed;
    visit->winding = (packed >> 24) & 1;
    index = (packed >> 1) & 0x3ff;
    p = (unsigned char *)&g_eng3_shapes_4065c[(g_eng3_shapes_4065c[index * 2 - 2] + index) * 2 - 1];
    count = p[0];
    if (count == 0)
        return 0;
    remaining = p[1];
    p += 2;
    visit->flags = g_eng3_cell_flags_4067c[((packed >> 11) & 0x1fff) * 2 + 1];
    if (visit->curve) {
        if (visit->flags & 8) {
            if (eng3_21390((long *)p, visit, count))
                return 0;
        } else {
            if (eng3_21a10((long *)p, visit, count))
                return 0;
            mode = 1;
        }
    } else {
        if (visit->flags & 8) {
            if (eng3_20c50((long *)p, visit, count))
                return 0;
        } else {
            if (eng3_20ed0((long *)p, visit, count))
                return 0;
            mode = 1;
        }
    }
    depth = 0x7ff - visit->depth;
    if (depth < 0x80)
        depth = 0x80;
    else if (depth > 0x7fe)
        depth = 0x7fe;
    ot = (unsigned char *)&g_eng3_frame_41448->ot[depth];
    p += count * 4 - 4;
    packet = g_eng3_prim_43164;
    uvs = g_eng3_uvs_40658;
    verts = g_engine_interface.scratch + 0x40;
    depth = (visit->depth - 0x40) >> visit->shift;
    if (depth < 1)
        depth = 1;
    else if (depth > visit->limit)
        depth = visit->limit;
    shade = (depth << 22) + 0x78300000;
    switch (mode) {
    case 1: {
        long normal;

        do {
            p += 8;
            face = *(long *)p;
            v0 = verts + (face & 0x3f) * 8;
            face >>= 3;
            v1 = verts + (face & 0x1f8);
            face >>= 6;
            v2 = verts + (face & 0x1f8);
            face >>= 6;
            v3 = verts + (face & 0x1f8);
            face >>= 9;
            eng3_slot_190(*(long *)v0, *(long *)v1, *(long *)v2);
            eng3_slot_1a0();
            eng3_slot_1c0(&normal);
            if (visit->winding == 0) {
                if (normal < 0)
                    continue;
            } else if (normal > 0) {
                continue;
            }
            eng3_slot_1dc(packet);
            ((Eng3PolyGT4 *)packet)->xy3 = *(long *)v3;
            ((Eng3PolyGT4 *)packet)->c0.rgb = *(unsigned long *)(v0 + 4);
            ((Eng3PolyGT4 *)packet)->rgb1 = *(unsigned long *)(v1 + 4);
            ((Eng3PolyGT4 *)packet)->rgb2 = *(unsigned long *)(v2 + 4);
            ((Eng3PolyGT4 *)packet)->rgb3 = *(unsigned long *)(v3 + 4);
            if (face & 0x80) {
                uv = *(unsigned long *)(p + 4);
                clut = (unsigned short)(uv | 0xff00);
                ((Eng3PolyGT4 *)packet)->uv2 = clut;
                ((Eng3PolyGT4 *)packet)->uv0 = shade | clut;
                ((Eng3PolyGT4 *)packet)->uv3 = clut;
                ((Eng3PolyGT4 *)packet)->uv1 = clut | 0x9c0000;
                head = (unsigned long *)(ot - ((uv >> 22) & 0x3fc));
                ((Eng3PolyGT4 *)packet)->tag = *head;
                *head = (unsigned long)packet;
            } else {
                uv = *(unsigned long *)(p + 4);
                e = (Eng3UV *)(uvs + (uv & 0xffff));
                uv1 = e->uv1;
                uv23 = e->uv23;
                uv0 = e->uv0;
                ((Eng3PolyGT4 *)packet)->uv2 = (unsigned short)uv23;
                ((Eng3PolyGT4 *)packet)->uv0 = uv0 | shade;
                ((Eng3PolyGT4 *)packet)->uv1 = uv1;
                ((Eng3PolyGT4 *)packet)->uv3 = (unsigned short)(uv23 >> 16);
                head = (unsigned long *)(ot - ((uv >> 22) & 0x3fc));
                ((Eng3PolyGT4 *)packet)->tag = *head;
                *head = (unsigned long)packet;
            }
            if (face & 0x20)
                ((Eng3PolyGT4 *)packet)->c0.c.code = 0x3e;
            else
                ((Eng3PolyGT4 *)packet)->c0.c.code = 0x3c;
            packet += sizeof(Eng3PolyGT4);
        } while (--remaining);
        g_eng3_prim_43164 = packet;
        return 0;
    }
    case 0: {
        long normal;

        color = g_eng3_colors_41460[(unsigned char)(visit->flags >> 6) + ((visit->flags & 7) << 6)] | 0x2c000000;
        do {
            p += 8;
            face = *(long *)p;
            xy0 = *(long *)(verts + (face & 0x3f) * 8);
            face >>= 3;
            xy1 = *(long *)(verts + (face & 0x1f8));
            face >>= 6;
            xy2 = *(long *)(verts + (face & 0x1f8));
            face >>= 6;
            eng3_slot_190(xy0, xy1, xy2);
            eng3_slot_1a0();
            xy3 = *(long *)(verts + (face & 0x1f8));
            face >>= 9;
            eng3_slot_1c0(&normal);
            if (visit->winding == 0) {
                if (normal < 0)
                    continue;
            } else if (normal > 0) {
                continue;
            }
            eng3_slot_1d4(packet);
            ((Eng3PolyFT4 *)packet)->color = ((face & 0x20) << 20) | color;
            ((Eng3PolyFT4 *)packet)->xy3 = xy3;
            if (face & 0x80) {
                uv = *(unsigned long *)(p + 4);
                clut = (unsigned short)(uv | 0xff00);
                ((Eng3PolyFT4 *)packet)->uv2 = clut;
                ((Eng3PolyFT4 *)packet)->uv0 = shade | clut;
                ((Eng3PolyFT4 *)packet)->uv3 = clut;
                ((Eng3PolyFT4 *)packet)->uv1 = clut | 0x9c0000;
                head = (unsigned long *)(ot - ((uv >> 22) & 0x3fc));
                ((Eng3PolyFT4 *)packet)->tag = *head;
                *head = (unsigned long)packet;
            } else {
                uv = *(unsigned long *)(p + 4);
                e = (Eng3UV *)(uvs + (uv & 0xffff));
                uv1 = e->uv1;
                uv23 = e->uv23;
                uv0 = e->uv0;
                ((Eng3PolyFT4 *)packet)->uv2 = (unsigned short)uv23;
                ((Eng3PolyFT4 *)packet)->uv0 = uv0 | shade;
                ((Eng3PolyFT4 *)packet)->uv1 = uv1;
                ((Eng3PolyFT4 *)packet)->uv3 = (unsigned short)(uv23 >> 16);
                head = (unsigned long *)(ot - ((uv >> 22) & 0x3fc));
                ((Eng3PolyFT4 *)packet)->tag = *head;
                *head = (unsigned long)packet;
            }
            packet += sizeof(Eng3PolyFT4);
        } while (--remaining);
        break;
    }
    }
    g_eng3_prim_43164 = packet;
    return 0;
}

int eng3_20b60(Eng3Visit *visit)
{
    Eng3Matrix *m;
    long a;

    m = (Eng3Matrix *)g_engine_interface.scratch;
    {
        short *sine;
        long s;
        long x;
        long c;
        long tx;
        long yaw;
        long tz;
        Eng3Cell *cell;
        long z;

        cell = visit->cell;
        yaw = visit->view.yaw;
        x = (cell->x - visit->view.x) >> 12;
        z = (cell->z - visit->view.z) >> 12;
        sine = g_engine_interface.sine;
        c = sine[(yaw - 0x400) & 0xfff];
        s = sine[yaw & 0xfff];
        tz = ((x * c + z * s) >> 12) + g_eng3_height_40640;
        tx = (x * s - z * c) >> 11;
        m->t[0] = tx & ~1;
        m->t[1] = 0;
        m->t[2] = tz * 2;
        a = -(cell->angle_08 + visit->view.yaw);
    }
    {
        short angles[3];

        angles[0] = (short)visit->view.pitch;
        angles[2] = 0;
        angles[1] = (short)a;
        eng3_slot_290(angles, m);
    }
    eng3_slot_294(m, g_eng3_scale_39518);
    eng3_slot_1b0(m);
    visit->angle = a;
    eng3_slot_1b4(m);
    return 0;
}

int eng3_20c50(long *stream, Eng3Visit *visit, int count)
{
    short v[3];
    long x;
    long y;
    long z;
    long value;
    unsigned char *out;

    x = (visit->column << 7) - 0x2c0;
    y = visit->lateral - ((signed char)(visit->packed >> 24) & ~1) * 2 - g_eng3_view_405a0.y;
    z = (visit->row - visit->cell->pivot) << 7;
    v[0] = x;
    v[1] = y;
    v[2] = z;
    eng3_slot_194(v);
    eng3_slot_1a8();
    eng3_slot_1e4(&visit->depth);
    if (visit->depth < 0)
        return -1;
    if (visit->stop == 0) {
        eng3_slot_1c8(v);
        if (visit->side) {
            if (v[0] < 0)
                visit->stop = 1;
            if (v[0] > 0x160)
                visit->stop = 1;
        } else {
            if (v[0] > 0x140)
                visit->stop = 1;
            if (v[0] < -0x20)
                visit->stop = 1;
        }
    } else {
        eng3_empty_2390(".");
    }
    out = g_engine_interface.scratch + 0x38;
    if (visit->winding) {
        value = *stream++;
        v[0] = x + (signed char)value * 2;
        v[1] = ((value >> 7) & ~1) + y;
        v[2] = ((value >> 23) & ~1) + z;
        eng3_slot_1c8(out);
        out += 8;
        eng3_slot_194(v);
        while (--count) {
            eng3_slot_1a8();
            value = *stream++;
            v[0] = x + (signed char)value * 2;
            v[1] = ((value >> 7) & ~1) + y;
            v[2] = ((value >> 23) & ~1) + z;
            eng3_slot_1c8(out);
            out += 8;
            eng3_slot_194(v);
        }
    } else {
        value = *stream++;
        v[0] = x - (signed char)value * 2;
        v[1] = ((value >> 7) & ~1) + y;
        v[2] = ((value >> 23) & ~1) + z;
        eng3_slot_1c8(out);
        out += 8;
        eng3_slot_194(v);
        while (--count) {
            eng3_slot_1a8();
            value = *stream++;
            v[0] = x - (signed char)value * 2;
            v[1] = ((value >> 7) & ~1) + y;
            v[2] = ((value >> 23) & ~1) + z;
            eng3_slot_1c8(out);
            out += 8;
            eng3_slot_194(v);
        }
    }
    eng3_slot_1a8();
    eng3_slot_1c8(out);
    return 0;
}

int eng3_20ed0(long *stream, Eng3Visit *visit, int count)
{
    short v[3];
    short c[4];
    long x;
    long y;
    long z;
    long value;
    unsigned long corners;
    unsigned long *colors;
    unsigned char *out;
    long left;

    out = g_engine_interface.scratch + 0x40;
    value = *stream++;
    x = (visit->column << 7) - 0x2c0;
    y = visit->lateral - ((signed char)(visit->packed >> 24) & ~1) * 2 - g_eng3_view_405a0.y;
    z = (visit->row - visit->cell->pivot) << 7;
    v[0] = x;
    v[1] = y;
    v[2] = z;
    eng3_slot_194(v);
    eng3_slot_1a8();
    corners = g_eng3_shades_40678[visit->flags >> 4];
    colors = &g_eng3_colors_41460[(visit->flags & 7) * 64];
    c[0] = corners & 0xff;
    c[1] = (corners >> 8) & 0xff;
    c[2] = (corners >> 16) & 0xff;
    c[3] = (corners >> 24) & 0xff;
    eng3_slot_1e4(&visit->depth);
    if (visit->depth < 0)
        return -1;
    eng3_slot_1c8(v);
    if (visit->side) {
        if (v[0] < 0)
            visit->stop = 1;
        if (v[0] > 0x160)
            visit->stop = 1;
    } else {
        if (v[0] > 0x140)
            visit->stop = 1;
        if (v[0] < -0x20)
            visit->stop = 1;
    }
    if (visit->winding) {
        do {
            short s, w;
            s = (signed char)value * 2;
            w = (value >> 23) & ~1;
            v[0] = s + x;
            v[1] = ((value >> 7) & ~1) + y;
            v[2] = w + z;
            eng3_slot_194(v);
            eng3_slot_1a8();
            s += 0x40;
            left = c[1] + (c[0] - c[1]) * s / 128;
            ((unsigned long *)out)[1] = colors[(left + (c[3] + (c[2] - c[3]) * s / 128 - left) * w / 128) >> 2];
            eng3_slot_1c8(out);
            out += 8;
            value = *stream++;
        } while (--count);
    } else {
        do {
            short s, w;
            s = (signed char)value * 2;
            w = (value >> 23) & ~1;
            v[0] = x - s;
            v[1] = ((value >> 7) & ~1) + y;
            v[2] = w + z;
            eng3_slot_194(v);
            eng3_slot_1a8();
            s += 0x40;
            left = c[0] + (c[1] - c[0]) * s / 128;
            ((unsigned long *)out)[1] = colors[(left + (c[2] + (c[3] - c[2]) * s / 128 - left) * w / 128) >> 2];
            eng3_slot_1c8(out);
            out += 8;
            value = *stream++;
        } while (--count);
    }
    return 0;
}

int eng3_21200(Eng3Visit *visit)
{
    long tx;
    long c;
    long tz;
    long b;
    long x;
    Eng3Cell *pivot;
    long s;
    Eng3Cell *cell;
    long z;
    long a;
    short angles[3];
    Eng3Matrix *m;
    long yaw;

    cell = visit->cell;
    yaw = visit->view.yaw;
    m = (Eng3Matrix *)(g_engine_interface.scratch + 0x20);
    a = -(cell->angle_0a + yaw);
    pivot = g_eng3_cells_40540[visit->layer] + cell->pivot;
    x = (pivot->x - visit->view.x) >> 6;
    z = (pivot->z - visit->view.z) >> 6;
    c = g_engine_interface.sine[(yaw - 0x400) & 0xfff];
    s = g_engine_interface.sine[yaw & 0xfff];
    tz = (x * c + z * s) >> 6;
    tx = (x * s - z * c) >> 6;
    if (visit->curve == 1) {
        tx += g_engine_interface.sine[a & 0xfff] * 1304;
        b = a + 0x400;
    } else {
        tx += g_engine_interface.sine[(a - 0x800) & 0xfff] * 1304;
        b = a - 0x400;
    }
    m->t[2] = ((g_engine_interface.sine[b & 0xfff] * 1304 + tz + (g_eng3_height_40640 << 12)) >> 11) & ~1;
    m->t[0] = (tx >> 11) & ~1;
    m->t[1] = 0;
    angles[2] = 0;
    angles[1] = 0;
    angles[0] = (short)visit->view.pitch;
    eng3_slot_290(angles, m);
    eng3_slot_294(m, g_eng3_scale_39518);
    eng3_slot_1b4(m);
    eng3_slot_1b0(m);
    visit->angle = a & 0xfff;
    visit->pivot = cell->pivot;
    return 0;
}

int eng3_21390(long *stream, Eng3Visit *visit, int count)
{
    long radius;
    long base;
    long radius2;
    long base2;
    long pivot;
    long angle;
    short v[4];
    short c[4];
    long y;
    long value;
    short a;
    short r;
    unsigned char *out;

    angle = visit->angle;
    pivot = visit->pivot;
    y = visit->lateral - ((signed char)(visit->packed >> 24) & ~1) * 2 - g_eng3_view_405a0.y;
    out = g_engine_interface.scratch + 0x38;
    value = *stream++;
    c[1] = y;
    if (visit->curve == 1) {
        radius = 0x7d8 - visit->column * 128;
        base = ((visit->row - pivot + 16) << 6) + angle;
        c[0] = -(g_engine_interface.sine[(base - 0x400) & 0xfff] * radius) >> 12;
        c[2] = -(g_engine_interface.sine[base & 0xfff] * radius) >> 12;
        if (visit->winding) {
            a = value >> 24;
            a = a + base;
            r = (signed char)value * 2;
            r = r - radius;
            v[0] = (g_engine_interface.sine[(a - 0x400) & 0xfff] * r) >> 12;
            v[2] = (g_engine_interface.sine[a & 0xfff] * r) >> 12;
            v[1] = ((value >> 7) & ~1) + y;
            eng3_slot_1c8(out);
            value = *stream++;
            eng3_slot_194(v);
            out += 8;
            while (--count) {
                eng3_slot_1a8();
                a = value >> 24;
                a = a + base;
                r = (signed char)value * 2;
                r = r - radius;
                v[0] = (g_engine_interface.sine[(a - 0x400) & 0xfff] * r) >> 12;
                v[2] = (g_engine_interface.sine[a & 0xfff] * r) >> 12;
                v[1] = ((value >> 7) & ~1) + y;
                eng3_slot_1c8(out);
                value = *stream++;
                eng3_slot_194(v);
                out += 8;
            }
        } else {
            a = value >> 24;
            a = a + base;
            r = (signed char)value * 2;
            r = -radius - r;
            v[0] = (g_engine_interface.sine[(a - 0x400) & 0xfff] * r) >> 12;
            v[2] = (g_engine_interface.sine[a & 0xfff] * r) >> 12;
            v[1] = ((value >> 7) & ~1) + y;
            value = *stream++;
            eng3_slot_1c8(out);
            eng3_slot_194(v);
            out += 8;
            while (--count) {
                eng3_slot_1a8();
                a = value >> 24;
                a = a + base;
                r = (signed char)value * 2;
                r = -radius - r;
                v[0] = (g_engine_interface.sine[(a - 0x400) & 0xfff] * r) >> 12;
                v[2] = (g_engine_interface.sine[a & 0xfff] * r) >> 12;
                v[1] = ((value >> 7) & ~1) + y;
                value = *stream++;
                eng3_slot_1c8(out);
                eng3_slot_194(v);
                out += 8;
            }
        }
    } else {
        radius2 = -600 - visit->column * 128;
        base2 = ((pivot - visit->row - 16) << 6) + angle;
        c[0] = (g_engine_interface.sine[(base2 - 0x400) & 0xfff] * radius2) >> 12;
        c[2] = (g_engine_interface.sine[base2 & 0xfff] * radius2) >> 12;
        if (visit->winding) {
            a = value >> 24;
            a = base2 - a;
            r = (signed char)value * 2;
            r = radius2 - r;
            v[0] = (g_engine_interface.sine[(a - 0x400) & 0xfff] * r) >> 12;
            v[2] = (g_engine_interface.sine[a & 0xfff] * r) >> 12;
            v[1] = ((value >> 7) & ~1) + y;
            value = *stream++;
            eng3_slot_1c8(out);
            eng3_slot_194(v);
            out += 8;
            while (--count) {
                eng3_slot_1a8();
                a = value >> 24;
                a = base2 - a;
                r = (signed char)value * 2;
                r = radius2 - r;
                v[0] = (g_engine_interface.sine[(a - 0x400) & 0xfff] * r) >> 12;
                v[2] = (g_engine_interface.sine[a & 0xfff] * r) >> 12;
                v[1] = ((value >> 7) & ~1) + y;
                value = *stream++;
                eng3_slot_1c8(out);
                eng3_slot_194(v);
                out += 8;
            }
        } else {
            a = value >> 24;
            a = base2 - a;
            r = (signed char)value * 2;
            r = radius2 + r;
            v[0] = (g_engine_interface.sine[(a - 0x400) & 0xfff] * r) >> 12;
            v[2] = (g_engine_interface.sine[a & 0xfff] * r) >> 12;
            v[1] = ((value >> 7) & ~1) + y;
            value = *stream++;
            eng3_slot_1c8(out);
            eng3_slot_194(v);
            out += 8;
            while (--count) {
                eng3_slot_1a8();
                a = value >> 24;
                a = base2 - a;
                r = (signed char)value * 2;
                r = radius2 + r;
                v[0] = (g_engine_interface.sine[(a - 0x400) & 0xfff] * r) >> 12;
                v[2] = (g_engine_interface.sine[a & 0xfff] * r) >> 12;
                v[1] = ((value >> 7) & ~1) + y;
                value = *stream++;
                eng3_slot_1c8(out);
                eng3_slot_194(v);
                out += 8;
            }
        }
    }
    eng3_slot_1a8();
    eng3_slot_1c8(out);
    eng3_slot_194(c);
    eng3_slot_1a8();
    eng3_slot_1e4(&visit->depth);
    return visit->depth < 0 ? -1 : 0;
}

/* Curved-track vertex projector (sibling of the straight 0x20ed0): rotates each packed stream
 * vertex about the curve pivot, projects it, and stores a colour bilinearly interpolated from the
 * four corner shades; then projects the cell centre and records its depth.  sv[0] holds the
 * corner shades, sv[1] the centre vector. */
int eng3_21a10(long *stream, Eng3Visit *visit, int count)
{
    short sv[2][4];
    short *v;
    unsigned long *colors;
    unsigned long corner;
    long w;
    long ybase;
    long pivot;

    corner = g_eng3_shades_40678[visit->flags >> 4];
    colors = &g_eng3_colors_41460[(visit->flags & 7) << 6];
    sv[0][0] = corner & 0xff;
    sv[0][1] = (corner >> 8) & 0xff;
    sv[0][2] = (corner >> 16) & 0xff;
    sv[0][3] = (corner >> 24) & 0xff;
    v = (short *)(g_engine_interface.scratch + 0x40);
    w = *stream++;
    ybase = visit->lateral - (((signed char)(visit->packed >> 24) >> 1) << 2) - g_eng3_view_405a0.y;
    pivot = visit->pivot;
    if (visit->curve == 1) {
        long r, a;
        short x, t, ang, rad;
        long lo, b0, d0, b1, d1;

        r = 0x7d8 - visit->column * 128;
        a = ((visit->row - pivot + 0x10) << 6) + visit->angle;
        sv[1][0] = -(g_engine_interface.sine[(a - 0x400) & 0xfff] * r) >> 12;
        sv[1][2] = -(g_engine_interface.sine[a & 0xfff] * r) >> 12;
        if (visit->winding) {
            b0 = sv[0][1];
            d0 = sv[0][0] - sv[0][1];
            b1 = sv[0][3];
            d1 = sv[0][2] - sv[0][3];
            do {
                x = (signed char)w * 2;
                t = w >> 24;
                ang = a + t;
                rad = x - r;
                v[0] = g_engine_interface.sine[(ang - 0x400) & 0xfff] * rad >> 12;
                v[2] = g_engine_interface.sine[ang & 0xfff] * rad >> 12;
                v[1] = ((w >> 8) << 1) + ybase;
                eng3_slot_194(v);
                eng3_slot_1a8();
                x += 0x40;
                lo = x * d0 / 128 + b0;
                *(unsigned long *)(v + 2) = colors[((d1 * x / 128 - lo + b1) * t / 64 + lo) >> 2];
                eng3_slot_1c8(v);
                w = *stream++;
                v += 4;
            } while (--count);
        } else {
            b0 = sv[0][0];
            d0 = sv[0][1] - sv[0][0];
            b1 = sv[0][2];
            d1 = sv[0][3] - sv[0][2];
            do {
                x = (signed char)w * 2;
                t = w >> 24;
                ang = a + t;
                rad = -(r + x);
                v[0] = g_engine_interface.sine[(ang - 0x400) & 0xfff] * rad >> 12;
                v[2] = g_engine_interface.sine[ang & 0xfff] * rad >> 12;
                v[1] = ((w >> 8) << 1) + ybase;
                eng3_slot_194(v);
                eng3_slot_1a8();
                x += 0x40;
                lo = x * d0 / 128 + b0;
                *(unsigned long *)(v + 2) = colors[((d1 * x / 128 - lo + b1) * t / 64 + lo) >> 2];
                eng3_slot_1c8(v);
                w = *stream++;
                v += 4;
            } while (--count);
        }
    } else {
        long r, a;
        short x, t, ang, rad;
        long lo, b0, d0, b1, d1;

        r = -0x258 - visit->column * 128;
        a = ((pivot - visit->row - 0x10) << 6) + visit->angle;
        sv[1][0] = g_engine_interface.sine[(a - 0x400) & 0xfff] * r >> 12;
        sv[1][2] = g_engine_interface.sine[a & 0xfff] * r >> 12;
        if (visit->winding) {
            b0 = sv[0][1];
            d0 = sv[0][0] - sv[0][1];
            b1 = sv[0][3];
            d1 = sv[0][2] - sv[0][3];
            do {
                x = (signed char)w * 2;
                t = w >> 24;
                ang = a - t;
                rad = r - x;
                v[0] = g_engine_interface.sine[(ang - 0x400) & 0xfff] * rad >> 12;
                v[2] = g_engine_interface.sine[ang & 0xfff] * rad >> 12;
                v[1] = ((w >> 8) << 1) + ybase;
                eng3_slot_194(v);
                eng3_slot_1a8();
                x += 0x40;
                lo = x * d0 / 128 + b0;
                *(unsigned long *)(v + 2) = colors[((d1 * x / 128 - lo + b1) * t / 64 + lo) >> 2];
                eng3_slot_1c8(v);
                w = *stream++;
                v += 4;
            } while (--count);
        } else {
            b0 = sv[0][0];
            d0 = sv[0][1] - sv[0][0];
            b1 = sv[0][2];
            d1 = sv[0][3] - sv[0][2];
            do {
                x = (signed char)w * 2;
                t = w >> 24;
                ang = a - t;
                rad = r + x;
                v[0] = g_engine_interface.sine[(ang - 0x400) & 0xfff] * rad >> 12;
                v[2] = g_engine_interface.sine[ang & 0xfff] * rad >> 12;
                v[1] = ((w >> 8) << 1) + ybase;
                eng3_slot_194(v);
                eng3_slot_1a8();
                x += 0x40;
                lo = x * d0 / 128 + b0;
                *(unsigned long *)(v + 2) = colors[((d1 * x / 128 - lo + b1) * t / 64 + lo) >> 2];
                eng3_slot_1c8(v);
                w = *stream++;
                v += 4;
            } while (--count);
        }
    }
    eng3_slot_194(sv[1]);
    eng3_slot_1a4();
    eng3_slot_1c4(sv[1]);
    if (sv[1][2] < 0x20)
        return -1;
    visit->depth = sv[1][2] >> 2;
    return 0;
}

void eng3_22070(void)
{
    long px;
    long pz;
    long ox;
    long oz;
    long sn;
    long frac;
    long cs;
    int layer;
    long x;
    long y;
    long top;
    long column;
    long row;
    long grid_y;
    long a0;
    long a1;
    long angle;
    long d;
    Eng3Cell *cells;
    Eng3Cell *cell;
    unsigned short delta;
    unsigned short *p;

    x = g_eng3_camera_x_405e0;
    y = g_eng3_camera_y_405e4;
    layer = g_eng3_layer_40638;
    column = x >> 7;
    row = column;
    g_eng3_unknown_40580 = -0x8a;
    grid_y = y >> 7;
    if (row < 0)
        row = 0;
    cells = g_eng3_cells_40540[layer];
    a0 = -cells[row].angle_08;
    frac = x & 0x7f;
    top = y - 0x74;
    cell = &cells[row + 1];
    a1 = -cell->angle_08;
    angle = (((a1 - a0) << 20 >> 20) * frac >> 7) + a0;
    px = cell->x;
    pz = cell->z;
    if (cell->curve == 0) {
        d = (column - cell->pivot + 1) << 7;
        px += g_engine_interface.sine[(a1 - 0x400) & 0xfff] * d;
        pz += g_engine_interface.sine[a1 & 0xfff] * d;
    }
    ox = g_eng3_offset_x_405f8 >> 10;
    oz = g_eng3_offset_z_40600 >> 10;
    cs = g_engine_interface.sine[(angle - 0x400) & 0xfff];
    sn = g_engine_interface.sine[angle & 0xfff];
    g_eng3_view_405a0.z = sn * frac + pz + (((ox * cs + oz * sn) >> 6)
        + g_engine_interface.sine[(angle + 0x400) & 0xfff] * g_eng3_camera_z_405e8);
    g_eng3_view_405a0.y = (g_eng3_offset_y_405fc >> 16) + top;
    g_eng3_view_405a0.x = cs * frac + (g_eng3_camera_z_405e8 * sn + px + ((ox * sn - oz * cs) >> 6));
    d = (angle << 20 >> 20) - (g_eng3_angle_3fc78 << 20 >> 20);
    if (d < 0)
        d = -d;
    delta = (unsigned short)d;
    for (p = g_eng3_damp_y_39a18; *p++ < delta;)
        ;
    g_eng3_angle_3fc78 = angle;
    g_eng3_view_405a0.yaw = angle + g_eng3_view_angle_40614;
    g_eng3_view_405a0.unknown_18 = row;
    g_eng3_view_405a0.pitch = 0;
    g_eng3_view_405a0.unknown_14 = 0;
    g_eng3_view_405a0.unknown_1c = grid_y;
    g_eng3_view_405a0.column = ((g_eng3_camera_z_405e8 + 0x40) >> 7) + 6;
    eng3_slot_2b8(g_eng3_center_x_4061c, g_eng3_center_y_40620 - 0x8a);
    eng3_slot_2bc(g_eng3_focal_40624);
}

void eng3_222c0(void)
{
    short bounds[7];
    int bounded;
    int count;
    short adj;
    long d;
    long step;
    long z;
    unsigned short a;
    unsigned short *p;
    unsigned short h;

    bounded = 0;
    g_eng3_prior_x_40604 = g_eng3_camera_x_405e0;
    g_eng3_prior_y_40608 = g_eng3_camera_y_405e4;
    if (g_eng3_camera_flags_4063a & 0x20) {
        bounded = eng3_1b650(bounds, g_eng3_bounds_id_40639, 0x1a, g_eng3_camera_x_405e0);
        g_eng3_spread_4063c = (signed char)bounds[5];
    } else {
        g_eng3_spread_4063c = 0;
    }
    for (count = g_eng3_iterations_x_40635; count > 0; count--) {
        adj = g_eng3_height_40640;
        if (g_eng3_camera_flags_4063a & 2) {
            d = ((short)(g_eng3_actor_431d4->x >> 16) - g_eng3_camera_x_405e0 + g_eng3_center_x_4061c) * 2 - 0x140;
            if (g_eng3_camera_flags_4063a & 1)
                d = d << 19 >> 19;
            d -= adj >> 1;
            a = d < 0 ? -d : d;
            for (p = g_eng3_damp_x_399f0; *p++ < a;)
                ;
            step = p - g_eng3_damp_x_399f0 - 1;
            if (g_eng3_camera_flags_4063a & 1) {
                if (d > 0)
                    g_eng3_camera_x_405e0 += step;
                else
                    g_eng3_camera_x_405e0 -= step;
                g_eng3_camera_x_405e0 = ((g_eng3_camera_x_405e0 - 0x100) & 0x1fff) + 0x100;
            } else if (step != 0) {
                if (d > 0)
                    g_eng3_camera_x_405e0 += step;
                else
                    g_eng3_camera_x_405e0 -= step;
            }
        } else {
            g_eng3_camera_x_405e0 = (g_eng3_actor_431d4->x >> 16) - (adj >> 1);
        }
    }
    adj = g_eng3_adjust_y_40642;
    for (count = g_eng3_iterations_y_40636; count > 0; count--) {
        if (g_eng3_camera_flags_4063a & 4) {
            d = adj + ((short)(g_eng3_actor_431d4->y >> 16) - g_eng3_camera_y_405e4 + g_eng3_center_y_40620) * 2 - 0x168;
            a = d < 0 ? -d : d;
            for (p = g_eng3_damp_y_39a18; *p++ < a;)
                ;
            step = p - g_eng3_damp_y_39a18 - 1;
            if (step != 0) {
                if (d > 0)
                    g_eng3_camera_y_405e4 += step;
                else
                    g_eng3_camera_y_405e4 -= step;
            }
        } else {
            g_eng3_camera_y_405e4 = (g_eng3_actor_431d4->y >> 16) + adj;
        }
    }
    h = eng3_1b2e0(g_eng3_camera_x_405e0 - ((g_engine_interface.context->level == 8) << 7),
        g_eng3_camera_y_405e4 - 0x14, (short)g_eng3_camera_z_405e8, g_eng3_layer_40638) >> 16;
    if (h != 0x7fff)
        g_eng3_camera_y_405e4 -= h;
    z = (short)(g_eng3_actor_431d4->z >> 16);
    if (bounded) {
        if (z < bounds[0])
            z = bounds[0];
        else if (z > bounds[2])
            z = bounds[2];
    }
    adj = g_eng3_adjust_z_40644;
    for (count = g_eng3_iterations_z_40637; count > 0; count--) {
        if (g_eng3_camera_flags_4063a & 8) {
            d = adj + (z - g_eng3_camera_z_405e8) * 2;
            a = d < 0 ? -d : d;
            for (p = g_eng3_damp_z_39a40; *p++ < a;)
                ;
            step = p - g_eng3_damp_z_39a40 - 1;
            if (step != 0) {
                if (d > 0)
                    g_eng3_camera_z_405e8 += step;
                else
                    g_eng3_camera_z_405e8 -= step;
            }
        } else {
            g_eng3_camera_z_405e8 = z;
        }
    }
    g_eng3_view_column_405ec = g_eng3_camera_x_405e0 >> 7;
    g_eng3_view_row_405f0 = g_eng3_camera_y_405e4 >> 7;
    g_eng3_view_depth_405f4 = g_eng3_camera_z_405e8 >> 7;
}

short eng3_22620(Actor *actor, int layer)
{
    int old;
    long dx;

    old = (short)(actor->x >> 16);
    dx = g_eng3_layers_40674[layer].width << 7;
    actor->profile = layer;
    actor->x -= dx << 16;
    actor->origin_x -= dx << 16;
    if (actor == g_eng3_actor_431d4) {
        g_eng3_camera_x_405e0 += (short)(actor->x >> 16) - old;
        g_eng3_layer_40638 = layer;
    }
    return 0;
}

void eng3_22690(int index, long x, long z, long angle, int pivot, int curve)
{
    g_eng3_track_40794[index].x = x & 0xfffff000;
    g_eng3_track_40794[index].z = z & 0xfffff000;
    g_eng3_track_40794[index].angle_08 = -angle | 0x8000;
    g_eng3_track_40794[index].angle_0a = -angle;
    g_eng3_track_40794[index].pivot = pivot;
    g_eng3_track_40794[index].curve = curve;
}

void eng3_22710(int index, long x, long z, long angle, int pivot, int curve)
{
    g_eng3_track_40794[index].x = x & 0xfffff000;
    g_eng3_track_40794[index].z = z & 0xfffff000;
    g_eng3_track_40794[index].angle_08 = -angle | 0x8000;
    g_eng3_track_40794[index].angle_0a = g_eng3_track_40794[pivot - 1].angle_08;
    g_eng3_track_40794[index].pivot = pivot;
    g_eng3_track_40794[index].curve = curve;
}

void eng3_22790(void)
{
    int layer;
    int rows;
    int row;
    int start;
    int length;
    int count;
    int last;
    int turn;
    int step;
    int offset;
    long x;
    long z;
    long angle;
    long previous;
    signed char *shape;
    int link;
    short *ep;
    Eng3Cell *cell;

    if (g_eng3_ranges_4078c != 0) {
        eng3_slot_040(g_eng3_ranges_4078c);
    }
    eng3_slot_078((void **)&g_eng3_ranges_4078c, g_eng3_layer_count_40786 * sizeof(Eng3Range), 0);
    for (layer = 0; layer < g_eng3_layer_count_40786; layer++) {
        ep = &g_eng3_layers_40674[layer].entries;
        rows = g_eng3_lane_rows_40704[((unsigned char *)ep + *ep)[1]];
        if (g_eng3_cells_40540[layer] != 0) {
            eng3_slot_040(g_eng3_cells_40540[layer]);
        }
        g_eng3_track_40794 = (Eng3Cell *)eng3_slot_078((void **)&g_eng3_cells_40540[layer], rows * 16, 0);
        g_eng3_ranges_4078c[layer].tick = -1;
        ep = &g_eng3_layers_40674[layer].shape;
        shape = (signed char *)ep + *ep;
        link = g_eng3_layers_40674[layer].link;
        if (link == 0x7f) {
            x = 0;
            z = 0;
            eng3_22690(0, x, z, 0, 0, 0);
            angle = 0;
        } else {
            cell = &g_eng3_cells_40540[link][g_eng3_layers_40674[layer].width];
            angle = -cell->angle_08;
            x = cell->x;
            z = cell->z;
            if (cell->curve != 0) {
                offset = 0;
            } else {
                offset = g_eng3_layers_40674[layer].width - cell->pivot;
            }
            x += (g_engine_interface.sine[(angle - 0x400) & 0xfff] * offset) << 7;
            z += (g_engine_interface.sine[angle & 0xfff] * offset) << 7;
            angle += g_eng3_layers_40674[layer].rotation;
            eng3_22690(0, x, z, angle, 0, 0);
        }
        last = 0;
        length = 0;
        count = 0;
        step = 0x20;
        start = 1;
        for (row = 1; row < rows; row++) {
            previous = angle;
            turn = shape[row];
            length += 0x80;
            if (angle + turn == angle) {
                eng3_22690(row, x, z, angle, last, 0);
                start = row + 1;
                x &= 0xfffff000;
                z &= 0xfffff000;
                step = 0x20;
                if (count++ > 0x20) {
                    last = row;
                    x += g_engine_interface.sine[(angle - 0x400) & 0xfff] * (length - 2);
                    z += g_engine_interface.sine[angle & 0xfff] * (length - 2);
                    length = 0;
                    count = 0;
                }
            } else if (shape[row] == shape[row + 1]) {
                angle += step * turn;
                step = 0x40;
                x += g_engine_interface.sine[(previous - 0x400) & 0xfff] * (length - 4);
                z += g_engine_interface.sine[previous & 0xfff] * (length - 4);
                eng3_22710(row, x, z, angle, start, turn);
                length = 4;
            } else {
                angle += turn * 24;
                step = 0x18;
                x += g_engine_interface.sine[(previous - 0x400) & 0xfff] * (length - 0x10);
                z += g_engine_interface.sine[previous & 0xfff] * (length - 0x10);
                eng3_22710(row, x, z, angle, row, 0);
                last = row;
                length = 0;
            }
        }
    }
}

void eng3_22b30(int index, Actor *actor)
{
    int tile;

    tile = g_eng3_tiles_40c38[index];
    if (tile & 0x8000) {
        g_eng3_tile_actions_40bd4[(tile >> 8) & 0x7f](tile & 0xff);
        return;
    }
    if (tile & 0x100) {
        if (actor->profile != (tile & 0xff))
            eng3_22620(actor, tile & 0xff);
    }
}

int eng3_22b90(long x, long y, long z, unsigned short mask)
{
    unsigned short tile;

    tile = g_eng3_tiles_40c38[dispatch_local_2a170(x, y, z, 0)];
    if (tile & 0x8000)
        return 0;
    return tile & mask;
}

Actor *eng3_22bd0(long x, long y, long z, int kind, int profile)
{
    Actor *actor;

    if (kind & 0x6000) {
        if (kind & 0x4000) {
            actor = create_eng3_middle_actor(x, y, z, kind);
            if (actor != 0) {
                actor->profile = profile;
                g_eng3_middle_40b98[kind & 0xfff].fn(actor);
            }
        } else {
            actor = create_eng3_kind_actor(x, y, z, kind);
            if (actor != 0) {
                actor->profile = profile;
                g_eng3_host_40bb4[kind & 0xfff].fn(actor);
            }
        }
    } else {
        actor = eng3_1cb00(x, y, z, kind);
        if (actor != 0) {
            actor->profile = profile;
            g_eng3_game_40b8c[kind & 0xfff].fn(actor);
        }
    }
    return actor;
}

void eng3_22cb0(Actor *owner, long x, long y, long z, int kind)
{
    Actor *actor;

    if (kind & 0x6000) {
        if (kind & 0x4000) {
            actor = create_eng3_middle_actor(x, y, z, kind);
            if (actor != 0) {
                actor->unknown_94 = owner->unknown_94;
                actor->unknown_95 = owner->unknown_95;
                actor->unknown_96 = owner->unknown_96;
                actor->profile = owner->profile;
                g_eng3_middle_40b98[kind & 0xfff].fn(actor);
            }
        } else {
            actor = create_eng3_kind_actor(x, y, z, kind);
            if (actor != 0) {
                actor->profile = owner->profile;
                g_eng3_host_40bb4[kind & 0xfff].fn(actor);
            }
        }
    } else {
        actor = eng3_1cb00(x, y, z, kind);
        if (actor != 0) {
            actor->unknown_94 = owner->unknown_94;
            actor->unknown_95 = owner->unknown_95;
            actor->unknown_96 = owner->unknown_96;
            actor->profile = owner->profile;
            append_chain(owner, actor);
            g_eng3_game_40b8c[kind & 0xfff].fn(actor);
        }
    }
}

/* 0x22de0: same source as TITLE 0x17b90 (calibration/title/r17ad0.c) and ENG1 0x303d0. */
void title_17b90(Actor *actor)
{
    long *list;
    int count;
    Actor *item;

    if (eng3_24a80(actor) == -1) {
        actor->unknown_1f |= 8;
        return;
    }
    list = actor->unknown_120;
    count = *list++;
    while (count--) {
        item = (Actor *)*list++;
        title_04410(item);
        eng3_24a80(item);
        title_02dc0(item);
    }
}

}
