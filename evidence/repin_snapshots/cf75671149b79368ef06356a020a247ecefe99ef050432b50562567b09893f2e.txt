/* ENG1 relocation-free handlers and geometry tests, 0x201b0..0x2c060.
 * Provisional shared Actor/Bounds3D layouts; not a recovered original translation unit. */
typedef struct Bounds3D {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds3D;

typedef struct Entry {
    unsigned char unknown_00[0x0e];
    unsigned short tag;
} Entry;

typedef struct Actor Actor;
struct Actor {
    long x, y, z;
    unsigned char unknown_0c[0x22 - 0x0c];
    unsigned char category, mode;
    unsigned char unknown_24[0x34 - 0x24];
    unsigned short sprite, unknown_36;
    unsigned char unknown_38[0x3a - 0x38];
    short scale;
    unsigned char unknown_3c[0x4c - 0x3c];
    long parameter;
    unsigned long flags, render_flags;
    unsigned char unknown_58[0x5c - 0x58];
    Entry *entries;
};

int overlap_wrapped_bounds(Bounds3D *a, Bounds3D *b)
{
    int d = a->min_x - b->min_x;
    int r;
    if (d > 0x800) { a->min_x -= 4096; a->max_x -= 4096; }
    if (d < -0x800) { b->min_x -= 4096; b->max_x -= 4096; }
    r = (b->max_y < a->min_y);
    r |= (a->max_y < b->min_y);
    r |= (b->max_z < a->min_z);
    r |= (a->max_z < b->min_z);
    r |= (b->max_x < a->min_x);
    r |= (a->max_x < b->min_x);
    return !r;
}

void set_wide_scale(Actor *p)
{ p->scale = 0x200; }

void reset_placement(Actor *p)
{
    p->flags = 2; p->category = 0; p->render_flags = 0xb0000000; p->mode = 0;
    p->sprite = 0; p->x = 0; p->y = 0; p->z = 0;
}

long signed_step(long a, long b, long step)
{
    long from = a >> 16, to = b >> 16;
    if (from < to) return step << 16;
    if (from > to) return -(step << 16);
    return 0;
}

void set_flags_two(Actor *p)
{ p->flags = 2; }

void show_sprite_82(Actor *p)
{ p->sprite = 0x82; p->flags |= 2; p->render_flags |= 0x80000005; }

void show_sprite_7a(Actor *p)
{ p->sprite = 0x7a; p->flags |= 2; p->render_flags |= 0x80000005; }

void reset_render_parameter(Actor *p)
{ p->render_flags = 0; p->parameter = 0x3c; }

void attach_entries(Actor *p, Entry *entries)
{
    Entry *e;
    unsigned short count = 0;
    p->entries = entries;
    for (e = entries; e->tag != 0xffff; e++)
        count++;
    p->unknown_36 = count;
}

int wrapped_ratio(int a, int b, int divisor)
{
    int delta = (a - (b & 0x3ff)) << 22;
    delta >>= 22;
    if (divisor != 0) return delta / divisor;
    return 0;
}

/* New native entries are placed in this candidate unit by their surrounding
 * recorded owners: 0x20d30 lies between actor_handlers_mid rows 0x201b0 and
 * 0x21310; 0x26cd0..0x26f20 lies after its 0x26700 row and before the
 * actor-pool row at 0x26f70. This is a source-owner hypothesis, not a claim
 * that the original compiler used this historical translation-unit boundary.
 * The older standalone providers for 0x20d30 and 0x26de0 must be removed or
 * excluded when this candidate is integrated, to avoid duplicate definitions.
 */
typedef unsigned char grid_u8;
typedef unsigned short grid_u16;
typedef unsigned long grid_u32;
typedef long grid_s32;
extern signed char g_active_row_70d01;
extern signed char g_group_count_70ece;
extern short g_element_width_70d14;
extern unsigned short g_rebuilt_row_offsets_70e0c[32];
extern grid_u8 *g_rebuilt_row_pointers_70dcc[16];
extern grid_u8 *g_header_pointer_70d40;

/* Native coefficient view: four signed DWORDs per index, first/fourth lanes.
 * No storage is defined here. The full overlapping data owner remains unresolved;
 * this declaration is a native read-shape hypothesis, not a binding certificate.
 */
extern long g_grid_query_coefficients_5e0d0[][4];

short __cdecl grid_query_20d30(long x, long y, long z, unsigned long step)
{
    long layer;
    long z_low;
    long x_low, y_low;
    unsigned long column_x, column_y;
    short width;
    unsigned long record;
    unsigned char *sample;
    unsigned long packed;
    unsigned long ix, iy, iz;

    if (g_active_row_70d01 == -1)
        return 0;
    z_low = z & 0xffff;
    layer = (z_low + 0x40) >> 7;
    if (layer < 0)
        layer = 0;
    else if (layer > g_group_count_70ece - 1)
        layer = g_group_count_70ece - 1;
    x_low = x;
    x_low += (unsigned short)(g_rebuilt_row_offsets_70e0c[layer + 16] * -64);
    x_low &= 0xffff;
    column_x = (unsigned long)x_low >> 6;
    if ((unsigned short)column_x >= g_rebuilt_row_offsets_70e0c[layer])
        return 0;
    y_low = y;
    y_low += 0x20;
    y_low &= 0xffff;
    column_y = (unsigned long)y_low >> 6;
    width = g_element_width_70d14;
    if ((unsigned short)column_y >= (unsigned short)width)
        return 0;
    column_x *= width;
    column_x += column_y;
    record = ((unsigned long *)g_rebuilt_row_pointers_70dcc[layer])[column_x];
    record = (record >> 24) & 0x7f;
    if (!record)
        return 0;
    sample = g_header_pointer_70d40 + record * 4 - 4;
    sample += *(long *)sample;
    packed = *(unsigned short *)sample;
    sample += 2;
    if (packed) {
        ix = packed & 15;
        iy = (unsigned char)packed;
        iy >>= 4;
        iz = packed >> 8;
        if (ix)
            sample += (g_grid_query_coefficients_5e0d0[ix][0] * (x_low & 63)) >> 8;
        if (iy)
            sample += ((g_grid_query_coefficients_5e0d0[iy][0] * (y_low & 63)) >> 8)
                    * g_grid_query_coefficients_5e0d0[ix][3];
        if (iz) {
            long term;
            term = g_grid_query_coefficients_5e0d0[iz][0] * (((z_low + 63) >> 1) & 63);
            term >>= 8;
            term *= g_grid_query_coefficients_5e0d0[ix][3];
            term *= g_grid_query_coefficients_5e0d0[iy][3];
            sample += term;
        }
    }
    return *(signed char *)sample;
}

typedef struct Query2a170Bounds Query2a170Bounds;
struct Query2a170Bounds { short mn[3]; short mx[3]; };
typedef union MidActorPipelineStorage MidActorPipelineStorage;
union MidActorPipelineStorage {
    unsigned char byte_cells[0x8c0];
    unsigned long dword_cells[0x230];
};
typedef struct MidEngineInterfacePrefix MidEngineInterfacePrefix;
struct MidEngineInterfacePrefix { unsigned long signature_000; void *context_004; };
extern MidActorPipelineStorage g_actor_pipeline_storage_71440;
extern MidEngineInterfacePrefix g_engine_interface;
extern unsigned short g_pipeline_word_6fec4;
extern unsigned short g_native_word_72168;
extern int __cdecl actor_bounds_2a880(void *, Query2a170Bounds *, unsigned long);
extern int __cdecl eng1_2a570(Bounds3D *, Actor *);
extern int __cdecl point_in_box_2a810(unsigned long, unsigned long, unsigned long, const Query2a170Bounds *);
extern int __cdecl bounds_overlap(Query2a170Bounds *, Query2a170Bounds *);
extern unsigned long __cdecl eng1_1a9b0(Query2a170Bounds *);
extern unsigned long __cdecl eng1_19b40(unsigned long);
extern void __cdecl eng1_1aba0(void);
extern void __cdecl eng1_1ab50(void);
extern void __cdecl eng1_30290(int, unsigned long);
#define g_actor_worklist_71900 ((unsigned long *)(void *)(g_actor_pipeline_storage_71440.byte_cells + 0x4c0))


void *__cdecl eng1_26cd0(void *query)
{
    Bounds3D parsed;
    int index;
    void *record;
    unsigned long *set = (unsigned long *)(void *)
        (g_actor_pipeline_storage_71440.byte_cells + 0x7a0);
    if (set[0] == 0)
        return 0;
    if (!actor_bounds_2a880(query, (Query2a170Bounds *)(void *)&parsed, 0))
        return 0;
    index = 0;
    while (index < (long)set[0]) {
        record = (void *)set[++index];
        if (eng1_2a570(&parsed, (Actor *)record))
            return record;
    }
    return 0;
}

unsigned long __cdecl eng1_26de0(void *query_actor)
{
    Query2a170Bounds query_bounds;
    unsigned long selection;
    long index;
    unsigned long *worklist = g_actor_worklist_71900;
    unsigned long *position;

    if (!actor_bounds_2a880(query_actor, &query_bounds, 1UL))
        return 0;
    selection = eng1_1a9b0(&query_bounds);
    if (selection != 0)
        return selection;
    index = 0;
    while (index < (long)worklist[0]) {
        position = (unsigned long *)worklist[++index];
        if (point_in_box_2a810(position[0], position[1], position[2], &query_bounds))
            return (unsigned long)position;
    }
    if (g_pipeline_word_6fec4 != 0 &&
        bounds_overlap((Query2a170Bounds *)(void *)g_actor_pipeline_storage_71440.byte_cells,
                       &query_bounds))
        return 0x80UL;
    return 0;
}

int __cdecl eng1_26e90(void *actor, unsigned short amount, int mode)
{
    unsigned long result;
    unsigned long allocated;

    result = eng1_26de0(actor);
    if (result != 0) {
        if (result == 0x80) {
            allocated = eng1_19b40(
                (unsigned long)(~g_native_word_72168) & 0x5f);
            if (allocated != 0) {
                if (mode)
                    eng1_1aba0();
                else
                    eng1_1ab50();
                *(unsigned short *)((unsigned char *)actor + 0xee) -= (unsigned short)allocated;
                return 1;
            }
        } else {
            eng1_30290(result, 0);
            *(unsigned short *)((unsigned char *)actor + 0xee) -= amount;
            return 1;
        }
    }
    return 0;
}
