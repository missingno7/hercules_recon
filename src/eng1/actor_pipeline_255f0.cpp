/* ENG1.DLL 0x255f0..0x26700: per-frame actor pipeline, forest record producers, hit report (C++ unit),
   in address order; ends with the already-matched 0x26700 attach_entries. Lane x7 scratch, 2026-10-20. */
extern "C" {
#include "actor_region.h"

void dispatch_local_255f0(void)
{
    g_pipeline_dword_71844 = 0;
    g_pipeline_word_713ae = 0;
    eng1_call_274e0();
    eng1_call_1fdf0();
    eng1_call_282e0();
    eng1_call_2af80();
    eng1_call_279f0();
    eng1_call_27fa0();
}

void dispatch_local_25620(void)
{
    g_pipeline_word_71ca0 = 0;
    g_pipeline_word_71740 = 0;
    g_pipeline_word_71720 = 0;
    g_pipeline_word_71cc0 = 0;
    g_pipeline_dword_71680[0] = 0;
    g_pipeline_word_71c60 = 0;
    g_actor_worklist_714c0[0] = 0;
    g_actor_worklist_71760[0] = 0;
    g_actor_worklist_717c0[0] = 0;
    g_actor_worklist_716c0[0] = 0;
    g_actor_worklist_71580[0] = 0;
    g_actor_worklist_71a20[0] = 0;
    g_actor_worklist_71be0[0] = 0;
    g_actor_worklist_71b20[0] = 0;
    g_pipeline_dword_71a18 = 0;
    eng1_call_01e50();
    eng1_call_12960();
    eng1_call_26040();
    eng1_call_1fd20();
    dispatch_local_262a0();
    g_pipeline_word_71376 = 5;
}

void engine_shutdown(void)
{
    reset_normal_actor_pool();
    reset_middle_actor_pool();
    reset_alternate_actor_pool();
    g_configuration_word_713c4 = g_configuration_word_71378;
    g_configuration_word_713b8 = g_configuration_word_713aa;
    g_configuration_word_71430 = g_configuration_word_7138c;
}

void dispatch_local_256e0(void)
{
    int i;
    unsigned short kind, flags;
    AltActor *alt;
    MiddleActor *mid;
    Actor *actor;
    for (i = g_configuration_word_7138c; i < g_configuration_word_713b6; i++) {
        alt = &g_alternate_actor_pool[i];
        kind = alt->kind;
        if (kind) {
            flags = (unsigned short)alt->flags;
            if (flags & 4) {
                if (flags & 1)
                    g_alt_callbacks[kind & 0xfff].update((Actor *)alt);
                if (flags & 2)
                    g_alt_callbacks[kind & 0xfff].update((Actor *)alt);
            }
        }
    }
    for (i = g_configuration_word_713aa; i < g_configuration_word_713a0; i++) {
        mid = &g_middle_actor_pool[i];
        kind = mid->kind;
        if (kind) {
            flags = (unsigned short)mid->flags;
            if (flags & 4) {
                if (!(flags & 0x800)) {
                    mid->saved_x = mid->x;
                    mid->saved_y = mid->y;
                }
                if (flags & 1)
                    g_configuration_dword_7137c[kind & 0xfff].update((Actor *)mid);
                if (flags & 2)
                    g_configuration_dword_7137c[kind & 0xfff].update((Actor *)mid);
            }
        }
    }
    for (i = g_configuration_word_71378; i < g_configuration_word_713a8; i++) {
        actor = &g_normal_actor_pool[i];
        if (actor->kind) {
            flags = (unsigned short)actor->flags;
            if (flags & 4) {
                if (!(flags & 0x800)) {
                    actor->saved_x = actor->x;
                    actor->saved_y = actor->y;
                    actor->saved_z = actor->z;
                }
                if (flags & 1)
                    g_callbacks[actor->kind].update(actor);
                if (flags & 2)
                    g_callbacks[actor->kind].draw(actor);
            }
        }
    }
}

void dispatch_local_258e0(void)
{
    int i;
    AltActor *alt;
    MiddleActor *mid;
    Actor *actor;
    unsigned short kind;
    unsigned long flags;
    g_pipeline_dword_71840 = 0;
    g_pipeline_word_71bc4 = (short)(g_normal_actor_pool->z >> 16);
    g_pipeline_word_71c5c = (short)(g_normal_actor_pool->z >> 16);
    g_pipeline_dword_716a8 = (g_sine_table_5a848[(g_engine_interface.context_004->tick_038 & 0x7f) << 3] << 6) >> 16;
    g_pipeline_dword_71718 = (g_sine_table_5a848[((g_engine_interface.context_004->tick_038 + 0x10) << 4) & 0x3ff] * 80) >> 16;
    g_pipeline_word_71ca0 = 0;
    g_pipeline_word_71740 = 0;
    g_pipeline_word_71720 = 0;
    g_pipeline_word_71cc0 = 0;
    g_pipeline_dword_71680[0] = 0;
    g_pipeline_word_71c60 = 0;
    eng1_call_26480();
    g_actor_worklist_714c0[0] = 0;
    g_actor_worklist_71760[0] = 0;
    g_actor_worklist_717c0[0] = 0;
    g_actor_worklist_716c0[0] = 0;
    g_actor_worklist_71580[0] = 0;
    g_actor_worklist_71a20[0] = 0;
    g_actor_worklist_71be0[0] = 0;
    g_actor_worklist_71b20[0] = 0;
    for (i = 0; i < 81; i++)
        g_resource_state_snapshot_719c0[i] = g_actor_resources[i].state;
    for (i = g_configuration_word_7138c; i < g_configuration_word_713b6; i++) {
        alt = &g_alternate_actor_pool[i];
        kind = alt->kind;
        if (kind) {
            if (g_resource_state_snapshot_719c0[alt->category] == 4)
                release_actor((Actor *)alt);
            else {
                flags = alt->flags;
                if (flags & 0x40)
                    g_actor_worklist_71a20[++g_actor_worklist_71a20[0]] = (long)alt;
                if (flags & 0x200)
                    g_actor_worklist_71580[++g_actor_worklist_71580[0]] = (long)alt;
                if (flags & 1)
                    g_alt_callbacks[kind & 0x1ff].update((Actor *)alt);
            }
        }
    }
    for (i = 0; i < g_configuration_word_713a0; i++) {
        mid = &g_middle_actor_pool[i];
        kind = mid->kind;
        if (kind) {
            if (g_resource_state_snapshot_719c0[mid->category] == 4)
                remove_actor_variant((Actor *)mid);
            else {
                flags = mid->flags;
                if (!(flags & 0x800)) {
                    mid->saved_x = mid->x;
                    mid->saved_y = mid->y;
                }
                if (flags & 0x40000)
                    g_actor_worklist_714c0[++g_actor_worklist_714c0[0]] = (long)mid;
                if (flags & 0x100000)
                    g_actor_worklist_71b20[++g_actor_worklist_71b20[0]] = (long)mid;
                if (flags & 0x200)
                    g_actor_worklist_71580[++g_actor_worklist_71580[0]] = (long)mid;
                if (flags & 0x800000)
                    g_actor_worklist_71be0[++g_actor_worklist_71be0[0]] = (long)mid;
                if (flags & 1)
                    g_configuration_dword_7137c[kind & 0x1ff].update((Actor *)mid);
            }
        }
    }
    for (i = g_configuration_word_71378; i < g_configuration_word_713a8; i++) {
        actor = &g_normal_actor_pool[i];
        if (actor->kind) {
            if (g_resource_state_snapshot_719c0[actor->category] == 4)
                remove_actor(actor);
            else {
                flags = actor->flags;
                if (!(flags & 0x800)) {
                    actor->saved_x = actor->x;
                    actor->saved_y = actor->y;
                    actor->saved_z = actor->z;
                }
                if (flags & 0x100000)
                    g_actor_worklist_71b20[++g_actor_worklist_71b20[0]] = (long)actor;
                if (flags & 0x80)
                    g_actor_worklist_716c0[++g_actor_worklist_716c0[0]] = (long)actor;
                if (flags & 0x200)
                    g_actor_worklist_71580[++g_actor_worklist_71580[0]] = (long)actor;
                if (flags & 0x400000)
                    g_actor_worklist_717c0[++g_actor_worklist_717c0[0]] = (long)actor;
                if (flags & 0x800000)
                    g_actor_worklist_71be0[++g_actor_worklist_71be0[0]] = (long)actor;
                if (!(flags & 0x10)) {
                    if (actor->lifetime)
                        actor->lifetime--;
                    else
                        actor->lifetime_flag = 0;
                } else
                    g_actor_worklist_71760[++g_actor_worklist_71760[0]] = (long)actor;
                if (flags & 0x240000)
                    g_actor_worklist_714c0[++g_actor_worklist_714c0[0]] = (long)actor;
                if (flags & 1)
                    g_callbacks[actor->kind & 0x1ff].update(actor);
            }
        }
    }
}

void dispatch_local_25d00(void)
{
    int i, count;
    unsigned short kind;
    AltActor *alt;
    MiddleActor *mid;
    Actor *actor;
    g_actor_worklist_71460[0] = 0;
    g_actor_worklist_71ce0[0] = 0;
    g_actor_worklist_71900[0] = 0;
    actor_bounds_2a880(g_collision_context, &g_collision_context->bounds, 1);
    g_pipeline_word_6fec4 = actor_bounds_2a880(g_collision_context, (Bounds *)g_pipeline_actor_slots_71440, 2);
    g_pipeline_word_6fec0 = 0;
    if (g_pipeline_word_71376)
        g_pipeline_word_71376--;
    i = g_configuration_word_713aa;
    count = g_configuration_word_713a0;
    for (; i < count; i++) {
        mid = &g_middle_actor_pool[i];
        kind = mid->kind & 0x3fff;
        if (kind && kind < 10)
            g_actor_worklist_71900[++g_actor_worklist_71900[0]] = (long)mid;
    }
    for (i = g_configuration_word_71378; i < g_configuration_word_713a8; i++) {
        actor = &g_normal_actor_pool[i];
        kind = actor->kind;
        if (kind && ((unsigned char)actor->flags & 2))
            g_callbacks[kind].draw(actor);
    }
    for (i = 0; i < g_configuration_word_713a0; i++) {
        mid = &g_middle_actor_pool[i];
        kind = mid->kind;
        if (kind && ((unsigned char)mid->flags & 2))
            g_configuration_dword_7137c[kind & 0x1ff].update((Actor *)mid);
    }
    for (i = g_configuration_word_7138c; i < g_configuration_word_713b6; i++) {
        alt = &g_alternate_actor_pool[i];
        kind = alt->kind;
        if (kind) {
            if (alt->flags & 0x10000)
                g_actor_worklist_71ce0[++g_actor_worklist_71ce0[0]] = (long)alt;
            if ((unsigned char)alt->flags & 2)
                g_alt_callbacks[kind & 0x1ff].update((Actor *)alt);
        }
    }
    i = 0;
    while (i < g_actor_worklist_71ce0[0]) {
        alt = (AltActor *)g_actor_worklist_71ce0[++i];
        g_alt_callbacks[alt->kind & 0x1ff].update((Actor *)alt);
    }
    actor = g_pipeline_actor_7200c;
    kind = actor->kind;
    if (kind)
        g_callbacks[kind].draw(actor);
    if (g_engine_interface.context_004->unknown_000[0] != 9)
        eng1_call_26730();
    g_pipeline_dword_71ae0 = 0;
    if (g_level_profiles_5df08[(char)g_engine_interface.context_004->unknown_000[0]].wrap_x) {
        actor = g_normal_actor_pool;
        for (i = 0; i < g_configuration_word_713b4; i++) {
            actor->x = ((actor->x - 0x400000) & 0xfffffff) + 0x400000;
            actor++;
        }
        mid = g_middle_actor_pool;
        for (i = 0; i < g_configuration_word_71432; i++) {
            mid->x = ((mid->x - 0x400000) & 0xfffffff) + 0x400000;
            mid++;
        }
        alt = g_alternate_actor_pool;
        for (i = 0; i < g_configuration_word_713ba; i++) {
            alt->x = ((alt->x - 0x400000) & 0xfffffff) + 0x400000;
            alt++;
        }
    }
}

void eng1_call_26040(void)
{
    int count_2019 = 0, count_2018 = 0;
    SpatialType *types = g_configuration_dword_71398;
    int group, i;
    unsigned char *block;
    unsigned char *allocated;
    ForestPlacement *place;
    ForestRecord *record;
    SpatialType *type;
    if (!types)
        return;
    if (!g_pipeline_word_713ae) {
        g_pipeline_word_713ae = g_root_count_70ed0;
        for (group = 0; group < g_pipeline_word_713ae; group++) {
            block = g_root_group_rows_70d5c[group];
            g_kind_info[group].owner = 0;
            g_kind_info[group].count = *(unsigned short *)(block + 8);
            if (!g_kind_info[group].count)
                continue;
            allocated = (unsigned char *)engine_pool_allocate((void **)&g_kind_info[group].owner, (g_kind_info[group].count + 1) * 20, 0);
            if (!allocated)
                continue;
            record = (ForestRecord *)(allocated + 20);
            block = g_root_group_rows_70d5c[group];
            place = (ForestPlacement *)(block + 10 + *(short *)(block + 10));
            for (i = 0; i < g_kind_info[group].count; i++) {
                type = &types[place->type];
                record->x = (place->x + type->x) << 16;
                record->y = (place->y + type->y) << 16;
                record->z = (place->z + type->z) << 16;
                record->flags = type->flags;
                record->state = 0;
                record->kind = type->kind;
                if (record->kind == 0x2018)
                    count_2018++;
                if (record->kind == 0x2019)
                    count_2019++;
                record++;
                place++;
            }
        }
        *(unsigned short *)&g_engine_interface.context_004->unknown_000[0x16] = count_2019;
        *(unsigned short *)&g_engine_interface.context_004->unknown_000[0x1a] = count_2018;
        return;
    }
    for (group = 0; group < g_pipeline_word_713ae; group++) {
        if (!g_kind_info[group].owner)
            continue;
        record = (ForestRecord *)(g_kind_info[group].owner + 20);
        block = g_root_group_rows_70d5c[group];
        place = (ForestPlacement *)(block + 10 + *(short *)(block + 10));
        for (i = 0; i < g_kind_info[group].count; i++) {
            type = &types[place->type];
            if (type->flags & 0x200) {
                record->x = (place->x + type->x) << 16;
                record->y = (place->y + type->y) << 16;
                record->z = (place->z + type->z) << 16;
                record->flags = type->flags;
                record->state = 0;
                record->kind = type->kind;
            }
            record++;
            place++;
        }
    }
}

void dispatch_local_262a0(void)
{
    unsigned short kind = g_collision_context->profile;
    long delta = g_camera_x_70ca0 - g_camera_center_x_70cdc;
    ForestRecord *record;
    int i;
    long x;
    unsigned short flags, type;
    record = (ForestRecord *)g_kind_info[kind].owner;
    if (!record)
        return;
    record++;
    for (i = 0; i < g_kind_info[kind].count; i++, record++) {
        if (g_frame_global_734f4 == g_pipeline_actor_7200c && (record->flags & 0x400))
            continue;
        if ((record->flags & 0xc800) != 0x800)
            continue;
        type = record->kind & 0x7000;
        if (type == 0)
            eng1_call_270c0(record, 0);
        if (type == 0x4000)
            eng1_call_27720(record, 0);
        if (type == 0x2000)
            eng1_call_27c40(record, 0);
    }
    record = (ForestRecord *)(g_kind_info[kind].owner + 20);
    for (i = 0; i < g_kind_info[kind].count; i++, record++) {
        if (g_frame_global_734f4 == g_pipeline_actor_7200c && (record->flags & 0x400))
            continue;
        x = (record->x >> 16) - delta;
        flags = record->flags;
        if (flags & 0x40) {
            if (x + 500 < 0 || x - 500 > 320)
                continue;
            if (flags & 0xc000)
                continue;
            type = record->kind & 0x7000;
            if (type == 0)
                eng1_call_270c0(record, 0);
            if (type == 0x4000)
                eng1_call_27720(record, 0);
            if (type == 0x2000)
                eng1_call_27c40(record, 0);
        } else {
            if (x + 386 < 0 || x - 380 > 320)
                continue;
            if (flags & 0xc000)
                continue;
            type = record->kind & 0x7000;
            if (type == 0)
                eng1_call_270c0(record, 0);
            if (type == 0x4000)
                eng1_call_27720(record, 0);
            if (type == 0x2000)
                eng1_call_27c40(record, 0);
        }
    }
}

void eng1_call_26480(void)
{
    unsigned short group = g_active_category_70cfe;
    long delta = g_camera_x_70ca0 - g_camera_center_x_70cdc;
    ForestRecord *record;
    int i;
    long x;
    unsigned short flags, type, kind;
    record = (ForestRecord *)g_kind_info[group].owner;
    if (!record)
        return;
    record++;
    for (i = 0; i < g_kind_info[group].count; i++, record++) {
        if (g_frame_global_734f4 == g_pipeline_actor_7200c && (record->flags & 0x400))
            continue;
        x = (record->x >> 16) - delta;
        flags = record->flags;
        if (flags & 0x40) {
            if (x + 500 < 0 || x - 500 > 320)
                continue;
            if (x + 32 > 0 && x - 32 < 320)
                continue;
            if (flags & 0xc000)
                continue;
            type = record->kind & 0x7000;
            if (type == 0)
                eng1_call_270c0(record, 0);
            if (type == 0x4000)
                eng1_call_27720(record, 0);
            if (type == 0x2000)
                eng1_call_27c40(record, 0);
        } else {
            if (x + 256 < 0 || x - 256 > 320)
                continue;
            if (x + 32 > 0 && x - 32 < 320) {
                if (x - 120 < 0 || x - 180 > 0 || (flags & 0xc000))
                    continue;
                kind = record->kind;
                if ((kind & 0x7000) == 0x1000)
                    eng1_native_code_target_028290(kind);
            } else {
                if (flags & 0xc000)
                    continue;
                type = record->kind & 0x7000;
                if (type == 0)
                    eng1_call_270c0(record, 0);
                if (type == 0x4000)
                    eng1_call_27720(record, 0);
                if (type == 0x2000)
                    eng1_call_27c40(record, 0);
            }
        }
    }
}

ForestRecord *eng1_26640(Actor *p)
{
    int kind = p->kind;
    ForestRecord *self = (ForestRecord *)(g_kind_info[p->profile].owner + p->record_offset);
    ForestRecord *record;
    int i;
    if (!g_kind_info[0].owner)
        return 0;
    record = (ForestRecord *)(g_kind_info[0].owner + 20);
    for (i = 0; i < g_kind_info[0].count; i++, record++) {
        if (record != self && record->kind == kind)
            return record;
    }
    return 0;
}

long eng1_native_code_target_00266a0(Actor *p, int priority, int value)
{
    RelativeMeasure m;
    priority++;
    if (priority >= g_eng1_sim_state_6feb4) {
        g_eng1_sim_state_6feb4 = priority;
        measure_relative_vector((FixedPosition *)p, (FixedPosition *)g_collision_context, &m);
        g_hit_direction_6feb8 = m.direction;
        g_hit_value_6febc = value;
    }
    return g_hit_result_72008;
}

void attach_entries(Actor *p, Entry *entries)
{
    Entry *e;
    unsigned short count = 0;
    p->entries = entries;
    for (e = entries; e->tag != 0xffff; e++)
        count++;
    p->entry_count = count;
}
}
