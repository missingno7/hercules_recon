/* ENG1 26f70/27b00: first free slot from the current cursor, then the wrap range.
 * Success stores the found index, without advancing it or marking a reservation.
 * Sparse views recover only the kind offset and measured slot strides.
 * Direct global expressions preserve the original reload/ordering behavior.
 */
#include <stdlib.h> /* Plausible CRT context for the actor-pool unit; measured, not proven original. */
typedef unsigned short ActorKind;
typedef struct AlternatePoolSlot {
    unsigned char before_kind[0x2e];
    unsigned short kind;
    unsigned char after_kind[0x94-0x30];
} AlternatePoolSlot;
typedef struct NormalPoolSlot {
    unsigned char before_kind[0x2e];
    unsigned short kind;
    unsigned char after_kind[0x134-0x30];
} NormalPoolSlot;
typedef char alternate_stride_is_94[sizeof(AlternatePoolSlot)==0x94?1:-1];
typedef char normal_stride_is_134[sizeof(NormalPoolSlot)==0x134?1:-1];
typedef char int_is_32[sizeof(int)==4?1:-1];
extern NormalPoolSlot *g_normal_actor_pool;
extern AlternatePoolSlot *g_alternate_actor_pool;
extern unsigned short g_configuration_word_71430;
extern unsigned short g_configuration_word_7138c;
extern unsigned short g_configuration_word_71374;
extern unsigned short g_configuration_word_713c4;
extern unsigned short g_configuration_word_71378;
extern unsigned short g_configuration_word_713ba;
extern unsigned short g_configuration_word_71308;
extern unsigned short g_configuration_word_713b4;
extern unsigned short g_configuration_word_713b6;
extern unsigned short g_configuration_word_713a8;

NormalPoolSlot *allocate_normal_actor(ActorKind kind)
{
    int i;
    if (kind & 0x8000) {
        for (i=g_configuration_word_71308; i<g_configuration_word_713b4; ++i)
            if (g_normal_actor_pool[i].kind==0) break;
        if (i<g_configuration_word_713b4) {
            g_configuration_word_71308=(unsigned short)i;
            return &g_normal_actor_pool[i];
        }
        for(i=g_configuration_word_713a8;i<g_configuration_word_71308;++i)
            if(g_normal_actor_pool[i].kind==0)break;
        if(i<g_configuration_word_71308) {
            g_configuration_word_71308=(unsigned short)i;
            return &g_normal_actor_pool[i];
        }
        return 0;
    }
    for(i=g_configuration_word_713c4;i<g_configuration_word_713a8;++i)
        if(g_normal_actor_pool[i].kind==0)break;
    if(i<g_configuration_word_713a8) {
        g_configuration_word_713c4=(unsigned short)i;
        return &g_normal_actor_pool[i];
    }
    for(i=g_configuration_word_71378;i<g_configuration_word_713c4;++i)
        if(g_normal_actor_pool[i].kind==0)break;
    if(i<g_configuration_word_713c4) {
        g_configuration_word_713c4=(unsigned short)i;
        return &g_normal_actor_pool[i];
    }
    return 0;
}

AlternatePoolSlot *allocate_alternate_actor(ActorKind kind)
{
    int i;
    if (kind & 0x8000) {
        for (i=g_configuration_word_71374; i<g_configuration_word_713ba; ++i)
            if (g_alternate_actor_pool[i].kind==0) break;
        if (i<g_configuration_word_713ba) {
            g_configuration_word_71374=(unsigned short)i;
            return &g_alternate_actor_pool[i];
        }
        for(i=g_configuration_word_713b6;i<g_configuration_word_71374;++i)
            if(g_alternate_actor_pool[i].kind==0)break;
        if(i<g_configuration_word_71374) {
            g_configuration_word_71374=(unsigned short)i;
            return &g_alternate_actor_pool[i];
        }
        return 0;
    }
    for(i=g_configuration_word_71430;i<g_configuration_word_713b6;++i)
        if(g_alternate_actor_pool[i].kind==0)break;
    if(i<g_configuration_word_713b6) {
        g_configuration_word_71430=(unsigned short)i;
        return &g_alternate_actor_pool[i];
    }
    for(i=g_configuration_word_7138c;i<g_configuration_word_71430;++i)
        if(g_alternate_actor_pool[i].kind==0)break;
    if(i<g_configuration_word_71430) {
        g_configuration_word_71430=(unsigned short)i;
        return &g_alternate_actor_pool[i];
    }
    return 0;
}


typedef union WorklistRecordFlags {
    unsigned short word;
    struct { unsigned char low, high; } bytes;
} WorklistRecordFlags;

typedef struct WorklistForestRecord {
    long x, y, z;
    WorklistRecordFlags flags;
    unsigned short state, kind, unknown_12;
} WorklistForestRecord;

typedef struct WorklistKindInfo {
    unsigned char *owner;
    long count;
} WorklistKindInfo;

typedef struct WorklistNormalDescriptor {
    unsigned long d00, d04, d08;
    unsigned char b0c;
    unsigned char unknown_0d[3];
} WorklistNormalDescriptor;

typedef struct WorklistAlternateDescriptor {
    unsigned long d00;
    unsigned char b04, b05, b06, b07;
} WorklistAlternateDescriptor;

typedef struct WorklistActorView {
    long x, y, z;
    unsigned char unknown_00c[0x16];
    unsigned char category, mode;
    unsigned char unknown_024[6];
    unsigned short state;
    unsigned char unknown_02c[2];
    unsigned short kind;
    unsigned char unknown_030[4];
    unsigned short sprite, entry_count, profile, scale;
    unsigned char unknown_03c[0x10];
    long parameter;
    unsigned long flags, render_flags;
    long record_offset;
    unsigned long entries, child;
    unsigned long asset_64;
    unsigned char unknown_068[8];
    unsigned char unknown_070[0x24];
    unsigned char direction_x, direction_y, direction_z;
    unsigned char unknown_097[5];
    long saved_x, saved_y, saved_z;
    unsigned char unknown_0a8[0x68];
    unsigned long asset_110;
    unsigned char unknown_114[4];
    unsigned long asset_118;
    unsigned char unknown_11c[0x18];
} WorklistActorView;

typedef struct WorklistAlternateActorView {
    long x, y, z;
    unsigned char unknown_00c[0x16];
    unsigned char category;
    unsigned char unknown_023[0x0b];
    unsigned short kind;
    unsigned char unknown_030[8];
    unsigned short profile;
    unsigned char unknown_03a[0x16];
    unsigned long flags, render_flags;
    long record_offset;
    unsigned char unknown_05c[8];
    unsigned long asset_64;
    unsigned char unknown_068[0x2c];
} WorklistAlternateActorView;

typedef void (__cdecl *WorklistFactoryCallback)(void *actor);
extern WorklistKindInfo g_kind_info[];
extern WorklistNormalDescriptor *g_configuration_dword_71388;
extern WorklistFactoryCallback *g_configuration_dword_71370;
extern WorklistFactoryCallback *g_configuration_dword_7139c;
extern WorklistAlternateDescriptor *g_configuration_dword_713a4;
extern void __cdecl initialize_normal_actor(WorklistActorView *actor);
extern void __cdecl initialize_alternate_actor(void *actor);

WorklistActorView *eng1_call_270c0(WorklistForestRecord *record,
                                   unsigned long group_index)
{
    unsigned short kind = record->kind;
    WorklistActorView *actor =
        (WorklistActorView *)allocate_normal_actor(kind);

    long coordinate;
    long inherited, render_flags;
    unsigned long callback_kind, group_snapshot;
    unsigned char direction_y_value;
    unsigned char *owner;

    if (actor != 0) {
        initialize_normal_actor(actor);
        record->flags.word |= 0x8000U;
        actor->kind = kind;
        coordinate = record->x;
        direction_y_value = actor->direction_y;
        render_flags = actor->render_flags;
        inherited = direction_y_value;
        actor->x = coordinate;
        actor->saved_x = coordinate;
        coordinate = record->y;
        group_snapshot = group_index;
        actor->y = coordinate;
        actor->saved_y = coordinate;
        coordinate = record->z;
        inherited &= 0x20;
        callback_kind = kind;
        actor->z = coordinate;
        actor->saved_z = coordinate;
        actor->state = record->state;
        actor->direction_x = (unsigned char)-(int)((record->flags.word >> 12) & 1);
        inherited |= actor->direction_x & 0x10;
        actor->render_flags = render_flags | inherited;
        actor->category = g_configuration_dword_71388[record->kind].b0c;
        actor->asset_64 = g_configuration_dword_71388[record->kind].d00;
        actor->asset_110 = g_configuration_dword_71388[record->kind].d04;
        actor->asset_118 = g_configuration_dword_71388[record->kind].d08;
        owner = g_kind_info[group_snapshot].owner;
        actor->profile = (unsigned short)group_snapshot;
        actor->record_offset = (long)((unsigned long)record -
            (unsigned long)owner);
        g_configuration_dword_71370[callback_kind * 3](actor);
    }
    return actor;
}

WorklistAlternateActorView *eng1_call_27c40(
    WorklistForestRecord *record, unsigned long group_index)
{
    unsigned short kind = record->kind;
    unsigned char *owner;
    unsigned long group_snapshot;
    long coordinate, record_delta;
    WorklistAlternateActorView *actor =
        (WorklistAlternateActorView *)allocate_alternate_actor(kind);

    if (actor != 0) {
        initialize_alternate_actor(actor);
        record->flags.word |= 0x8000U;
        actor->kind = kind;
        coordinate = record->x;
        actor->x = coordinate;
        coordinate = record->y;
        group_snapshot = group_index;
        actor->y = coordinate;
        coordinate = record->z;
        actor->z = coordinate;
        owner = g_kind_info[group_snapshot].owner;
        record_delta = (long)((unsigned long)record - (unsigned long)owner);
        actor->profile = (unsigned short)group_snapshot;
        actor->record_offset = record_delta;
        actor->category = g_configuration_dword_713a4[
            kind & 0x0fffU].b06;
        actor->asset_64 = g_configuration_dword_713a4[
            kind & 0x0fffU].d00;
        g_configuration_dword_7139c[(kind & 0x0fffU) * 2](actor);
    }
    return actor;
}
