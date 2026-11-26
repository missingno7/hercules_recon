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

/* ENG1 native entries 0x26d50 and 0x26f20.
 * 0x26d50 reuses the measured 0x71be0 actor-pointer set and the existing
 * bounds helpers. 0x26f20 scans the count-plus-pointer list at 0x6fe78,
 * calling 0x29470 for the first record with a null link at +0x12c, then sets
 * its high flag bit. 0x6fe78 aliases the existing minimum four-byte
 * g_level_actors_6fe78 cell; later pointer slots are accessed as observed,
 * but their capacity is not claimed here.
 */
Actor *__cdecl eng1_26d50(Actor *query)
{
    Query2a170Bounds query_bounds;
    Query2a170Bounds actor_bounds;
    unsigned long *set = (unsigned long *)(void *)
        (g_actor_pipeline_storage_71440.byte_cells + 0x7a0);
    int index;
    Actor *record;

    if (set[0] == 0)
        return 0;
    if (!actor_bounds_2a880(query, &query_bounds, 0UL))
        return 0;
    index = 0;
    while (index < (long)set[0]) {
        record = (Actor *)set[++index];
        if (actor_bounds_2a880(record, &actor_bounds, 0UL) &&
            bounds_overlap(&query_bounds, &actor_bounds))
            return record;
    }
    return 0;
}

extern long g_level_actors_6fe78[];
extern void __cdecl append_chain(void *, void *);

Actor *__cdecl eng1_26f20(void *argument)
{
    int count;
    int index;
    Actor *record;

    count = (int)g_level_actors_6fe78[0];
    index = 0;
    while (index < count) {
        record = (Actor *)g_level_actors_6fe78[++index];
        if (*(void **)((unsigned char *)record + 0x12c) == 0) {
            append_chain(argument, record);
            *(unsigned long *)((unsigned char *)record + 0x54) |= 0x80000000UL;
            return record;
        }
    }
    return 0;
}

/* Native neighbours reset_placement..show_sprite_7a bracket these handlers.
 * Two indexed extents (22fc0,237b0) are compiler switch tables, not entries. */
#include "cluster_view.h"
#include <stdlib.h>
extern ClusterCallbacks *g_configuration_dword_71370,*g_configuration_dword_7139c;
extern ClusterActor *spawn_normal(long,long,long,int);
extern ClusterActor *spawn_kind(long,long,long,int);
extern int remove_actor(ClusterActor *);
extern void release_actor(ClusterActor *);
extern void finish_actor(ClusterActor *);
extern void select_frame(ClusterActor *,int);
extern ClusterActor **acquire_pending_action_group(void);
extern ClusterActor *append_kind_action(ClusterActor **,long,long,long,int);
extern void eng1_32130(ClusterActor *);
extern void eng1_131c0(long,long,long,int);
extern long eng1_native_code_target_00266a0(ClusterActor *,int,int);
extern ClusterActor *g_collision_context;
extern int eng1_20030(void *,ClusterActor *);

void eng1_native_code_target_021f60(ClusterActor *p)
{
    long scroll;
    ClusterActor **group,**cursor;
    ClusterActor *q,*a;
    int i,j;
    LinkedSoundFx_6da0(0x1c7,0);
    scroll=g_camera_x_70ca0-g_camera_offset_70cc4;
    p->state=1;
    group=g_cluster_actor_6fe3c->group;
    cursor=group+2;
    a=*cursor;
    a->x=g_frame_owner_72860->x-scroll-0x100000;
    a->y=p->y-0x80000;
    a->z=p->z;
    a->sprite=5;
    LinkedSoundFx_6da0(0x1c8,0);
    a=group[5];
    a->x=g_frame_owner_72860->x-scroll+0x100000;
    a->y=p->y-0x80000;
    a->z=p->z;
    a->sprite=5;
    g_cluster_left_6fdf0.state=0;
    g_cluster_right_6fdfc.state=0;
    g_cluster_left_6fdf0.index=-1;
    g_cluster_right_6fdfc.target=g_cluster_left_6fdf0.target;
    g_cluster_right_6fdfc.index=-1;
    i=g_configuration_word_713b4;
    if(i>0) {
    q=g_frame_owner_72860;
    do {
        if(q->kind==0x2d) {
            for(j=0;j<2;++j) {
                if(cursor[j*3]->x>=q->x-0x200000 && cursor[j*3]->x<=q->x+0x200000 &&
                   g_cluster_groups_6fd00[q->index].phase==6 && g_cluster_count_6fe20==1)
                    g_eng1_setup_dword_72014=1;
            }
        }
        ++q;
    } while(--i);
    }
    eng1_22280(p);
}

void eng1_native_code_target_0220c0(ClusterActor *p)
{
    ClusterActor *a;
    if(g_cluster_count_6fe20>0) {
        if(!g_cluster_hit_6fe10) select_frame(p,0);
        else {
            g_cluster_hit_6fe10=0;
            --g_cluster_count_6fe20;
            g_boss_threshold_72184=0x20;
        }
    } else {
        a=spawn_normal(0,0x2f00000,0x2780000,0x31);
        if(a) g_configuration_dword_71370->slot[0x24c/4](a);
    }
    finish_actor(p);
}

void eng1_22190(ClusterActor *p)
{
    ClusterActor **group;
    ClusterActor *a;
    int i;
    g_cluster_actor_6fe3c=spawn_normal(p->x,p->y,p->z,0x2f);
    if(g_cluster_actor_6fe3c) g_configuration_dword_71370->slot[0x234/4](g_cluster_actor_6fe3c);
    if(!g_cluster_actor_6fe3c) return;
    group=acquire_pending_action_group();
    if(!group) return;
    g_cluster_actor_6fe3c->group=group;
    for(i=0;i<6;++i) {
        a=append_kind_action(group,0,0,0,0x2032);
        if(a) {
            a->flags|=0x20000;
            a->render_flags|=0x10000045;
            a->opacity=0xff;
            a->sprite=6;
            a->entries=g_cluster_program_58d78;
            if(i!=0 && i!=3) a->mode=2;
            if(i==1 || i==4) a->scale*=2;
        }
    }
}

void eng1_22280(ClusterActor *p)
{
    ClusterActor **group=g_cluster_actor_6fe3c->group;
    long x=g_frame_owner_72860->x;
    ClusterActor *a;
    if(g_cluster_left_6fdf0.state>=0) {
        a=group[1]; a->x=x+0x210000; a->y=0x26f0000; a->z=0x2200000; a->sprite=6;
        a=group[3]; a->x=x+0x210000; a->y=0x26f0000; a->z=0x2200000; a->sprite=6;
        eng1_22440(p,2,&g_cluster_left_6fdf0);
        if(g_cluster_left_6fdf0.state<0 && g_cluster_left_6fdf0.index>=0)
            ++g_cluster_groups_6fd00[g_cluster_left_6fdf0.index].phase;
    }
    if(g_cluster_right_6fdfc.state>=0) {
        a=group[4]; a->x=x+0x8c0000; a->y=0x26f0000; a->z=0x2200000; a->sprite=6;
        a=group[6]; a->x=x+0x8c0000; a->y=0x26f0000; a->z=0x2200000; a->sprite=6;
        eng1_22440(p,5,&g_cluster_right_6fdfc);
        if(g_cluster_right_6fdfc.state<0 && g_cluster_left_6fdf0.index<0 && g_cluster_right_6fdfc.index>=0)
            ++g_cluster_groups_6fd00[g_cluster_right_6fdfc.index].phase;
    }
    if(g_cluster_left_6fdf0.state!=g_cluster_right_6fdfc.state) {
        if(g_cluster_left_6fdf0.state>0) g_cluster_right_6fdfc.state=-1;
        else if(g_cluster_right_6fdfc.state>0) g_cluster_left_6fdf0.state=-1;
    }
    if(g_cluster_left_6fdf0.state<0) eng1_22400(1);
    if(g_cluster_right_6fdfc.state<0) eng1_22400(4);
    if(g_cluster_left_6fdf0.state<0 && g_cluster_right_6fdfc.state<0) eng1_native_code_target_0220c0(p);
}

void eng1_22400(int index)
{
    ClusterActor **group=g_cluster_actor_6fe3c->group;
    ClusterActor *a;
    a=group[index]; a->x=a->y=a->z=0;
    a=group[index+1]; a->x=a->y=a->z=0;
    a=group[index+2]; a->x=a->y=a->z=0;
}

void eng1_22440(ClusterActor *p,int index,ClusterState *s)
{
    ClusterActor **group=g_cluster_actor_6fe3c->group;
    long x=g_frame_owner_72860->x;
    ClusterActor *a=group[index],*b,*q;
    long tx;
    int i;
    switch(s->state) {
    case 2:
        tx=s->target->x;
        if(index<4) tx-=0x80000; else tx+=0x80000;
        a->x+=signed_step(a->x,tx,2);
        a->y+=signed_step(a->y,s->target->y,2);
        a->z+=signed_step(a->z,g_frame_owner_72860->z+0xa40000,2);
        b=group[index+1];
        b->x=x-0x1f0000;
        if(index>3) b->x=x+0xcc0000;
        b->y=0x29f0000; b->z=0x2200000;
        if(!(g_cluster_tick_6fe30&1)) {
            q=spawn_kind(b->x,0x29f0000,0x2200000,0x202b);
            if(q) g_configuration_dword_7139c->slot[0x158/4](q);
        }
        if(g_cluster_timer_6fe2c<=0) {
            s->state=-1; select_frame(p,1);
            RemoveFxIdLinks_7f8e(0x1c7,0);
            if(!random_limit(2)) LinkedSoundFx_6da0(0x1c3,0);
            else LinkedSoundFx_6da0(0x1c4,0);
            g_cluster_hit_6fe10=1;
        }
        break;
    case 1:
        a->y-=0x40000;
        if(!(g_cluster_tick_6fe30&1)) {
            q=spawn_kind(a->x+eng1_21630(0x20),a->y+eng1_21630(0x20),a->z,0x202a);
            if(q) g_configuration_dword_7139c->slot[0x150/4](q);
        }
        if(a->y<0x2400000) {
            s->state=-1;
            if(!g_cluster_groups_6fd00[s->index].bottom) {
                RemoveFxIdLinks_7f8e(0x1c7,0);
                b=g_cluster_groups_6fd00[s->index].top;
                RemoveFxLinks_7ef6(b);
                g_cluster_groups_6fd00[s->index].top=eng1_22fe0(b->x,0x2c00000,0xb20000,7,s->index);
                g_cluster_groups_6fd00[s->index].top->sprite=0xb;
                g_cluster_groups_6fd00[s->index].middle=eng1_22fe0(b->x,0x2800000,0xb20000,8,s->index);
                g_cluster_groups_6fd00[s->index].middle->sprite=2;
                g_cluster_groups_6fd00[s->index].lower=eng1_22fe0(b->x,0x2400000,0xb20000,9,s->index);
                g_cluster_groups_6fd00[s->index].lower->sprite=3;
                g_cluster_groups_6fd00[s->index].bottom=eng1_22fe0(b->x,0x2000000,0xb20000,10,s->index);
                g_cluster_groups_6fd00[s->index].bottom->sprite=4;
                remove_actor(b);
                if(!(s->index&1)) eng1_131c0(g_cluster_groups_6fd00[s->index].bottom->x,g_cluster_groups_6fd00[s->index].bottom->y,g_frame_owner_72860->z,0x289);
                eng1_32130(g_cluster_groups_6fd00[s->index].bottom);
                g_cluster_groups_6fd00[s->index].bottom->unknown_68=-1;
                LinkedSoundFx_6da0(0xd1c0,g_cluster_groups_6fd00[s->index].bottom);
            }
        }
        break;
    case 0:
        a->z-=0x200000;
        for(i=0;i<g_configuration_word_713b4;++i) {
            q=g_frame_owner_72860+i;
            switch(q->kind) {
            case 0x32:
                if(a->z<g_frame_owner_72860->z+0x80000 && a->x>=q->x-0xf0000 && a->x<=q->x+0xf0000) q->hits=0;
                break;
            case 0x30:
                if(a->z<g_frame_owner_72860->z+0x840000 && a->x>=q->x-0x2c0000 && a->x<=q->x+0x2c0000 && !g_cluster_groups_6fd00[q->index].phase) {
                    s->state=1; s->index=q->index;
                    LinkedSoundFx_6da0(0x51c9,g_cluster_groups_6fd00[q->index].top);
                }
                break;
            case 0x2d:
                if(a->z<g_frame_owner_72860->z+0xa40000 && a->x>=q->x-0x200000 && a->x<=q->x+0x200000 && g_cluster_groups_6fd00[q->index].phase==6) {
                    s->state=2; s->index=q->index; s->target=q;
                    g_cluster_timer_6fe2c=0x1e;
                    RemoveFxLinks_7ef6(g_cluster_groups_6fd00[q->index].top);
                    LinkedSoundFx_6da0(0x51ce,q);
                    if(g_cluster_once_6fe14) {g_cluster_once_6fe14=0; title_0c4d0(0x26);}
                }
                break;
            }
            if(s->state) break;
        }
        if(!s->state) {
            for(i=0;i<g_configuration_word_713b4;++i) {
                q=g_frame_owner_72860+i;
                if(q->kind==1 && g_frame_owner_72860->y>=0x2d00000 && a->z<g_frame_owner_72860->z+0x80000 && a->x>=q->x-0x100000 && a->x<=q->x+0x100000) {
                    RemoveFxIdLinks_7f8e(0x1c7,0);
                    RemoveFxLinks_7ef6(g_cluster_groups_6fd00[q->index].top);
                    eng1_native_code_target_00266a0(p,3,0);
                    if(!random_limit(4)) LinkedSoundFx_6da0(0x1c5,0);
                    s->state=-1;
                }
            }
        }
        if(a->z<g_frame_owner_72860->z-0x200000) {RemoveFxIdLinks_7f8e(0x1c7,0); s->state=-1;}
        break;
    }
    if(!(g_cluster_tick_6fe30&1)) {
        q=spawn_kind(a->x,a->y,a->z+0x60000,0x202b);
        if(q) g_configuration_dword_7139c->slot[0x158/4](q);
    }
}

void eng1_native_code_target_022b90(ClusterActor *p)
{
    p->flags|=0x20000; p->render_flags|=0x80000000;
    p->angle=random_limit(12)-6;
    p->angle_y=-4-random_limit(8); p->angle_z=0;
    p->sprite=random_limit(8)+1; p->scale=0x100;
    p->parameter=0; p->bounce=0;
    if(!random_limit(2)) p->render_flags|=0x10;
}

void eng1_native_code_target_022c20(ClusterActor *p)
{
    p->scale-=4;
    if(p->scale<=5) {release_actor(p); return;}
    p->x+=(long)p->angle<<16;
    p->y+=(long)p->angle_y<<16;
    p->z+=(long)p->angle_z<<16;
    ++p->angle_y;
    if(p->parameter==2) {
        if(!(g_cluster_sounds_6fe50++&15)) {
            switch(random_limit(3)) {
            case 0: LinkedSoundFx_6da0(0xd1e7,p); break;
            case 1: LinkedSoundFx_6da0(0xd1e8,p); break;
            case 2: LinkedSoundFx_6da0(0xd1e9,p); break;
            }
        }
        if(p->y>=0x2d80000) {
            p->angle_y>>=1;
            p->angle>>=1; p->angle_y=-p->angle_y; p->angle_z>>=1;
            p->y=(p->angle_y+0x2d8)<<16;
            if(++p->bounce>=4) release_actor(p);
        }
    } else if(p->y>=0x2d80000) release_actor(p);
}

void eng1_native_code_target_022d20(ClusterActor *p)
{
    p->flags|=0x20000; p->render_flags|=0x80000007;
    p->sprite=9; p->scale=0x200; p->counter=random_limit(5)+1;
}

void eng1_native_code_target_022d60(ClusterActor *p)
{
    p->y-=(long)p->counter<<15;
    if((g_cluster_tick_6fe30&1)==1) {
        if(++p->sprite>=0x19) release_actor(p);
    }
}

void eng1_native_code_target_022da0(ClusterActor *p)
{
    p->render_flags=0x90000000; p->angle_y=0x200; p->sprite=0xf;
    g_cluster_groups_6fd00[p->index].sound=0;
}

void eng1_native_code_target_022dd0(ClusterActor *p)
{
    switch(g_cluster_groups_6fd00[p->index].phase) {
    case 1: p->vy=0; break;
    case 5:
        p->y+=p->vy<<16;
        if(p->vy<0x20) ++p->vy;
        if(p->y>=0x2d00000) {
            p->vy=-(p->vy/4); p->y=(p->vy+0x2d0)<<16;
            if(!g_cluster_groups_6fd00[p->index].sound) {
                g_cluster_groups_6fd00[p->index].sound=1;
                LinkedSoundFx_6da0(0x59cb,p);
            }
            if(p->vy>=-2) ++g_cluster_groups_6fd00[p->index].phase;
        }
        break;
    case 7:
        p->vx=random_limit(4)-2; p->vy=-8-random_limit(8); p->vz=-10-random_limit(8);
        ++g_cluster_groups_6fd00[p->index].phase;
        LinkedSoundFx_6da0(0x59ca,p);
        break;
    case 8:
        p->x+=p->vx<<16; p->y+=p->vy<<16; p->z+=p->vz<<16;
        if(p->vy<0x20) ++p->vy;
        p->angle_y=(p->angle_y+0x30)&0xfff;
        if(p->y>=0x2d00000) {
            p->vx/=2; p->vz/=2; p->vy=-(p->vy/2);
            p->y=(p->vy+0x2d0)<<16;
            if(p->vy>=-2) remove_actor(p);
        }
        break;
    }
}

ClusterActor *eng1_22fe0(long x,long y,long z,int sprite,int index)
{
    ClusterActor *p=spawn_normal(x,y,z,0x30);
    if(p) g_configuration_dword_71370->slot[0x240/4](p);
    if(p) {
        p->category=0; p->render_flags=0x90000040;
        p->sprite=sprite; p->index=index;
    }
    return p;
}

void eng1_23040(void)
{
    int i;
    long x;
    ClusterActor *p;
    for(i=0;i<8;++i) {
        x=0xa00000+i*0x2000000;
        g_cluster_groups_6fd00[i].top=eng1_22fe0(x,0x2c00000,0xb20000,0x10,i);
        g_cluster_groups_6fd00[i].top->scale<<=1;
        g_cluster_groups_6fd00[i].bottom=0; g_cluster_groups_6fd00[i].lower=0; g_cluster_groups_6fd00[i].middle=0; g_cluster_groups_6fd00[i].phase=0;
        g_cluster_groups_6fd00[i].marker=spawn_normal(x,0x2200000,0xe20000,0x2d);
        if(g_cluster_groups_6fd00[i].marker) g_configuration_dword_71370->slot[0x21c/4](g_cluster_groups_6fd00[i].marker);
        g_cluster_groups_6fd00[i].marker->index=i;
    }
    for(x=0x200000;x<0x10200000;x+=0x1000000) {
        p=spawn_kind(x,0x2a00000,0x80000,0x2029);
        if(p) g_configuration_dword_7139c->slot[0x148/4](p);
    }
}

void eng1_native_code_target_023120(ClusterActor *p)
{
    ClusterActor *a;
    int i;
    switch(g_cluster_groups_6fd00[p->index].phase) {
    case 1: case 2: case 3: case 4:
        p->vx=random_limit(5)+7;
        if(g_frame_owner_72860->x>p->x) p->vx=-p->vx;
        p->vy=-5-random_limit(5); p->vz=-8-random_limit(8); p->phase=0;
        ++g_cluster_groups_6fd00[p->index].phase;
        break;
    case 5: case 6:
        if(!(g_cluster_tick_6fe30&7)) {
            a=spawn_kind(p->x,p->y,p->z,0x202b);
            if(a) g_configuration_dword_7139c->slot[0x158/4](a);
        }
        p->x+=p->vx<<16; p->y+=p->vy<<16; p->z+=p->vz<<16;
        if(p->vy<0x20) ++p->vy;
        p->angle=(p->angle+(short)p->vy+4)&0xfff;
        if(p->y>=0x2d00000) {
            for(i=0;i<4-p->phase;++i) {
                a=spawn_kind(p->x,p->y,p->z,0x202a);
                if(a) g_configuration_dword_7139c->slot[0x150/4](a);
            }
            if(!p->phase) g_cluster_effect_6fde0=3;
            ++p->phase;
            p->vx/=2; p->vz/=2; p->vy=-(p->vy/2);
            p->y=(p->vy+0x2d0)<<16;
            if(p->vy>=-2) remove_actor(p);
        }
        break;
    }
}

void eng1_native_code_target_023340(ClusterActor *p)
{
    eng1_23390(p,-0x4000000,0x600000,0x640000);
    eng1_23390(p,0x3ff0000,0x600000,0x640000);
    LinkedSoundFx_6da0(0x1e6,0);
    finish_actor(p);
}

void eng1_23390(void *owner,long dx,long dy,long dz)
{
    ClusterActor *p=spawn_normal(g_cluster_origin_6fe24->x+dx,g_cluster_origin_6fe24->y-dy,g_cluster_origin_6fe24->z-dz,0x33);
    if(p) g_configuration_dword_71370->slot[0x264/4](p);
    if(p) {
        p->state=0; p->vx=0;
        if(dx>=0) {p->phase=-1; p->render_flags|=0x10;} else p->phase=1;
        p->program=(long *)0x800; p->vy=p->y;
        LinkedSoundFxAt_75e3(0x59e5,p,owner);
    }
}

void eng1_native_code_target_023460(ClusterActor *p)
{
    ClusterActor *a;
    if(!(g_cluster_tick_6fe30&3)) {if(++p->sprite>=0x86) p->sprite=0x82;}
    if(eng1_20030((unsigned char *)g_collision_context+0x88,p)) {
        eng1_native_code_target_00266a0(p,2,0); p->program=0;
    } else {
        a=spawn_kind(p->x,p->y,p->z,0x202c);
        if(a) g_configuration_dword_7139c->slot[0x160/4](a);
        switch(p->state) {
        case 1:
            p->y+=signed_step(p->y,p->vy,2); p->x+=p->vx<<14;
            p->program=(long *)((long)p->program-abs(p->vx>>2)); p->vx+=p->phase*2;
            break;
        case 0:
            p->y=((0x21c-(p->z>>16))<<11)/464;
            p->y=p->vy-((g_sine_table_70f34[(p->y-0x400)&0xfff]*1920)&0xffff0000);
            p->z-=0x100000;
            if(p->z<=g_frame_owner_72860->z) {
                p->z=g_frame_owner_72860->z; p->state=1; p->render_flags^=0x10;
                g_cluster_choice_6fe18=random_limit(3);
                if(g_cluster_choice_6fe18==g_cluster_last_6fe54) {if(--g_cluster_choice_6fe18<0) g_cluster_choice_6fe18=2;}
                g_cluster_last_6fe54=g_cluster_choice_6fe18;
                switch(g_cluster_choice_6fe18) {
                case 0:p->vy=0x2a80000;break;
                case 1:p->vy=0x2c80000;break;
                case 2:p->vy=0x2d80000;break;
                }
            }
            break;
        }
    }
    if((long)p->program<=0) remove_actor(p);
}

void eng1_native_code_target_023660(ClusterActor *p)
{ if(++p->sprite>=0x82) release_actor(p); }

void eng1_native_code_target_023680(ClusterActor *p)
{
    p->flags=2; p->render_flags|=0x80000000;
    p->counter=random_limit(20)-10; p->index=-5-random_limit(8); p->state=random_limit(50)+5;
}

void eng1_native_code_target_0236e0(ClusterActor *p)
{
    unsigned short angle=p->state;
    angle+=p->angle_z;
    p->angle_z=angle&0xfff;
    p->x+=(long)p->counter<<16; p->y+=(long)p->index<<16; ++p->index;
    if(p->y>=0x2d80000) {
        switch(random_limit(10)) {
        case 0:LinkedSoundFx_6da0(0xd9e0,p);break;
        case 1:LinkedSoundFx_6da0(0xd9e1,p);break;
        case 2:LinkedSoundFx_6da0(0xd9e2,p);break;
        case 3:LinkedSoundFx_6da0(0xd9e3,p);break;
        case 4:LinkedSoundFx_6da0(0xd9e4,p);break;
        }
        p->counter/=2; p->index=-(p->index/2); p->state>>=1;
        p->y=(p->index+0x2d8)<<16;
        if(abs(p->index)<=2) release_actor(p);
    }
}

void eng1_native_code_target_0237d0(ClusterActor *p)
{
    p->flags=2; p->vx=random_limit(20)-10; p->vy=-5-random_limit(8); p->vz=random_limit(50)+5;
    p->index=0; p->counter=90; p->home_x=p->x; p->home_y=p->y;
}

void eng1_native_code_target_023840(ClusterActor *p)
{
    switch(p->index) {
    case 1:
        p->x+=p->vx; p->y+=p->vy+(++p->gravity<<15);
        p->angle_z=p->vz*p->counter/35;
        if(--p->counter<0) {extern int remove_actor_variant(ClusterActor *); remove_actor_variant(p);}
        break;
    case 0:
        p->angle_z+=(short)p->vz;
        p->x+=p->vx<<16; p->y+=p->vy<<16; ++p->vy;
        if(p->y>=0x2d80000 && p->vy) {
            switch(random_limit(5)) {
            case 0:LinkedSoundFx_6da0(0xd1e0,p);break;
            case 1:LinkedSoundFx_6da0(0xd1e1,p);break;
            case 2:LinkedSoundFx_6da0(0xd1e2,p);break;
            case 3:LinkedSoundFx_6da0(0xd1e3,p);break;
            case 4:LinkedSoundFx_6da0(0xd1e4,p);break;
            }
            p->vx/=2; p->vy=-(p->vy/2); p->vz/=2; p->y=(p->vy+0x2d8)<<16;
            if(p->vy<9 && p->vy>-9) {p->vy=0;p->vx=0;p->vz=0;}
        }
        if(--p->counter<=0) {
            p->vx=(p->home_x-p->x)/36; p->vy=(p->home_y-p->y)/36;
            p->index=1; p->counter=35; p->gravity=-18; p->vz=p->angle_z&0x3ff;
        }
        break;
    }
}


#include "resource_runtime_data.h"

/* Native entries 20250..21650 are bracketed by this unit's accepted
 * overlap_wrapped_bounds (201b0) and set_wide_scale (21310), followed by
 * shared effect_state's initialize_backdrop (216b0). Existing providers for
 * the dispatch/resource/query functions remain in their current units.
 * No original translation-unit identity or global capacity is asserted. */
extern void finish_actor(ClusterActor *);
extern void advance_frame(ClusterActor *);
extern void eng1_interpret_signed_pairs(ClusterActor *);
extern void eng1_21aa0(ClusterActor *);
extern void *title_17ad0(long,long,long,int,int);
extern void title_0c4e0(void);
extern void eng1_19240(void);
extern long g_cluster_spawn_timer_6fde4,g_cluster_camera_shake_6fe1c;
extern long g_busy,g_cluster_unknown_6fe28;
extern long g_cluster_unknown_6fe40,g_cluster_unknown_6fe44,g_cluster_unknown_6fe48,g_cluster_unknown_6fe4c;
extern long g_camera_offset_y_70cbc,g_camera_center_y_70ce0,g_camera_z_base_70d1c;
extern short g_camera_adjust_y_70d04;
extern unsigned short g_camera_flags_70d10;
extern long g_countdown;
extern signed char g_camera_iterations_70cfd;
extern long g_eng1_setup_dword_72188;
extern long *g_script_start_6fcf8,*g_script_cursor_6fcf4;
extern long *g_cluster_script_start_6fe0c,*g_cluster_script_cursor_6fcf0;
extern long g_cluster_script_58b08[],g_cluster_script_58ba0[],g_cluster_script_58c40[];
extern long g_cluster_script_58ce0[],g_cluster_script_58d10[],g_cluster_script_58d38[];

long eng1_212d0(int,int);
void eng1_21590(ClusterActor *,int);
void eng1_21650(ClusterActor *,int,int,int);

int eng1_20250(ClusterActor *actor)
{
    Query2a170Bounds bounds;
    long selection;
    int i;
    unsigned long *list=g_actor_worklist_71900;
    unsigned long *p;
    if(!actor_bounds_2a880(actor,&bounds,0)) return 0;
    selection=eng1_1a9b0(&bounds);
    if(selection) return selection;
    i=0;
    while(i<(long)list[0]) {
        p=(unsigned long *)list[++i];
        if(point_in_box_2a810(p[0],p[1],p[2],&bounds)) return (int)p;
    }
    if(g_pipeline_word_6fec4 && overlap_wrapped_bounds((Bounds3D *)(void *)g_actor_pipeline_storage_71440.byte_cells,(Bounds3D *)(void *)&bounds)) return 0x80;
    return 0;
}

void eng1_native_code_target_0021050(void)
{
    g_cluster_unknown_6fe4c=0;g_cluster_unknown_6fe48=0;g_cluster_unknown_6fe44=0;g_cluster_unknown_6fe40=0;
    g_frame_owner_72860->x=0x3b80000;
    g_frame_owner_72860->z=0x4c0000;
    g_cluster_origin_6fe24=spawn_normal(0,0x2f00000,0x2800000,0x2e);
    if(g_cluster_origin_6fe24)g_configuration_dword_71370->slot[0x228/4](g_cluster_origin_6fe24);
    g_cluster_origin_6fe24->sprite=0x3d;
    eng1_22190(g_cluster_origin_6fe24);
    eng1_23040();
    title_17ad0(0,0x2e00000,0x2800000,0x202d,0);
    finish_actor(g_cluster_origin_6fe24);
    advance_frame(g_cluster_origin_6fe24);
    select_frame(g_cluster_origin_6fe24,0);
    g_camera_center_y_70ce0=0x60;
    g_camera_adjust_y_70d04=-0x40;
    g_camera_z_base_70d1c=0;
    g_camera_iterations_70cfd=2;
    g_camera_flags_70d10=0x1f;
    g_cluster_count_6fe20=8;
    g_cluster_spawn_timer_6fde4=0x1f4;
    g_cluster_bosses_6fde8=0;
    g_busy=0;
    g_cluster_effect_6fde0=0;
    g_cluster_camera_shake_6fe1c=0;
    g_cluster_hit_6fe10=0;
    LinkedSoundFx_6da0(0x1c6,0);
    g_cluster_unknown_6fe28=3;
    switch(*(signed char *)((unsigned char *)g_engine_interface.context_004+0xe)) {
    case 0:g_script_start_6fcf8=g_cluster_script_58b08;g_cluster_script_start_6fe0c=g_cluster_script_58ce0;break;
    case 1:g_script_start_6fcf8=g_cluster_script_58ba0;g_cluster_script_start_6fe0c=g_cluster_script_58d10;break;
    case 2:g_script_start_6fcf8=g_cluster_script_58c40;g_cluster_script_start_6fe0c=g_cluster_script_58d38;break;
    }
    g_script_cursor_6fcf4=g_script_start_6fcf8;
    g_cluster_script_cursor_6fcf0=g_cluster_script_start_6fe0c;
    g_eng1_setup_dword_72188=5;
    g_cluster_once_6fe14=1;
}

void eng1_native_code_target_0021210(void)
{
    long x;
    ClusterActor *p;
    if(--g_cluster_spawn_timer_6fde4<=0 && g_cluster_count_6fe20>0) {
        g_cluster_spawn_timer_6fde4=random_limit(120)+60;
        if(g_cluster_bosses_6fde8<3) {
            if(!(g_frame_owner_72860->vx&0xffff0000)) x=eng1_212d0(96,96)>>16;
            else {
                x=random_limit(64)+0x1f0;
                if(g_frame_owner_72860->direction) x=-x;
            }
            p=spawn_normal(g_frame_owner_72860->x+(x<<16),0x2e00000,g_frame_owner_72860->z,0x32);
            if(p) g_configuration_dword_71370->slot[0x258/4](p);
        }
    }
}

long eng1_212d0(int distance,int range)
{
    if(!random_limit(2))distance=-distance;
    return (distance+random_limit(range)+((range>>1)<<16)-(range>>1))<<16;
}

void eng1_native_code_target_021320(ClusterActor *p)
{
    long shake;
    --g_countdown;--g_cluster_tick_6fe30;
    eng1_interpret_signed_pairs(p);
    p->render_flags=(p->render_flags&~0x30UL)|(p->direction&0x10);
    p->x=g_frame_owner_72860->x;
    switch(p->state) {
    case 1:eng1_22280(p);break;
    case 2:eng1_21aa0(p);break;
    }
    shake=g_cluster_camera_shake_6fe1c;
    if(g_cluster_effect_6fde0>0) {--g_cluster_effect_6fde0;shake=0xa0000;}
    g_camera_offset_y_70cbc=shake;
    if(g_camera_offset_y_70cbc && !(g_cluster_tick_6fe30&1))g_camera_offset_y_70cbc=-g_camera_offset_y_70cbc;
}

void eng1_native_code_target_0213c0(ClusterActor *p)
{
    advance_frame(p);select_frame(p,7);
    p->scale=0x200;p->state=0;g_countdown=20;
    LinkedSoundFx_6da0(0x1c1,0);
}

void eng1_native_code_target_021400(ClusterActor *p)
{
    int i;
    eng1_interpret_signed_pairs(p);
    p->render_flags=(p->render_flags&~0x30UL)|(p->direction&0x10);
    p->x=g_frame_owner_72860->x;
    switch(p->state) {
    case 0:
        if(g_countdown<=0) {g_countdown=20;++p->state;}
        break;
    case 1:
        if(!(g_cluster_tick_6fe30&3))eng1_21590(p,1);
        if(g_countdown<=0) {
            g_countdown=20;++p->state;title_0c4e0();title_0c4d0(0x27);
        }
        break;
    case 2:
        if(!(g_cluster_tick_6fe30&1))eng1_21590(p,1);
        if(g_countdown<=0) {
            g_countdown=90;++p->state;
            eng1_21650(p,0,-0x48,0);eng1_21650(p,-0x800,-0x80,5);
            eng1_21650(p,0x800,-0x38,8);eng1_21650(p,-0x400,-0x40,10);
            for(i=0;i<100;++i)eng1_21590(p,2);
            p->render_flags&=0x7fffffff;
            g_cluster_origin_6fe24->render_flags&=0x7fffffff;
            RemoveFxIdLinks_7f8e(0x1c6,0);LinkedSoundFx_6da0(0x1ea,0);
        }
        break;
    case 3:
        if(g_countdown<=0) {eng1_19240();++p->state;}
        break;
    }
}

void eng1_21590(ClusterActor *p,int factor)
{
    ClusterActor *a=spawn_kind(p->x+eng1_21630(0x640),p->y-0x800000,p->z-0x100000,0x202a);
    if(a) g_configuration_dword_7139c->slot[0x150/4](a);
    if(a) {
        a->angle=(random_limit(10)-5)*factor;
        a->angle_y=-(random_limit(8)+3)*factor;
        a->angle_z=-(random_limit(8)+1)*factor;
        a->parameter=factor;
    }
}

long eng1_21630(int range)
{
    return (random_limit(range)+((range>>1)<<16)-(range>>1))<<16;
}

void eng1_21650(ClusterActor *p,int dx,int dy,int delay)
{
    ClusterActor *a=spawn_normal((dx<<16)+p->x,(dy<<16)+p->y,p->z,0x35);
    if(a)g_configuration_dword_71370->slot[0x27c/4](a);
    if(a)a->counter=delay;
}
