/* ENG1.DLL 0x24090..0x255c0: boss script reader, boss/debris actor handlers (C unit), in address order.
   Includes the already-matched 0x253b0 reset_render_parameter at its address. Lane x7 scratch, 2026-10-20. */
#include "actor_region.h"

long eng1_24090(void)
{
    long op = *g_script_cursor_6fcf4++;
    while (op <= 0) {
        if (op == 0) {
            g_script_low_6fe34 = *g_script_cursor_6fcf4++;
            g_script_high_6fe38 = *g_script_cursor_6fcf4++;
        }
        if (op < 0)
            g_script_cursor_6fcf4 = g_script_start_6fcf8;
        op = *g_script_cursor_6fcf4++;
    }
    if (g_script_low_6fe34 < random_limit(g_script_high_6fe38) + 1 ||
        g_script_low_6fe34 == g_script_high_6fe38)
        return op;
    --g_script_cursor_6fcf4;
    return 0;
}

void eng1_24120(Actor *p)
{
    eng1_241f0(p, -15, -15, 12);
    eng1_241f0(p, 15, -15, 12);
    eng1_241f0(p, -15, -15, 9);
    eng1_241f0(p, 15, -15, 9);
    eng1_241f0(p, 0, -35, 3);
    eng1_241f0(p, 0, -45, 5);
    eng1_241f0(p, -20, -50, 10);
    eng1_241f0(p, 20, -50, 10);
    eng1_241f0(p, -30, -50, 7);
    eng1_241f0(p, 30, -50, 7);
    if (p->direction_x)
        eng1_241f0(p, -50, -60, 4);
    else
        eng1_241f0(p, 50, -60, 4);
    eng1_241f0(p, 0, -65, 2);
}

void eng1_241f0(Actor *p, int dx, int dy, int sprite)
{
    Actor *child = spawn_kind(p->x - (dx << 16), p->y + (dy << 16), p->z, 0x2030);
    if (child)
        g_alt_callbacks[48].initialize(child);
    if (child) {
        child->sprite = sprite + 0x56;
        if (sprite != 5) {
            if (dx < 0)
                child->render_flags |= 0x10;
        } else if (dx > 0)
            child->render_flags |= 0x10;
    }
}

void eng1_24270(Actor *p)
{
    g_boss_script_index_71e24 = 0xffff;
    if (p->record_offset)
        p->state = *(unsigned short *)(g_kind_info[p->profile].owner + p->record_offset + 0x0e);
    g_boss_71e28 = p;
    advance_frame(p);
    if (p->state == 9)
        select_frame(p, 0x20);
    else
        p->state = 0;
    eng1_interpret_signed_pairs(p);
    p->direction_x = 0xff;
    p->flags = 2;
    p->render_flags = 0x80080000;
    p->unknown_124 = 6;
    p->parameter = 0;
    p->hits = (char)g_engine_interface.context_004->unknown_000[0x0e] * 2 + 6;
    eng1_native_code_target_002a290(p);
}

void eng1_24320(Actor *p)
{
    eng1_interpret_signed_pairs(p);
    g_boss_state_6ffda = p->state;
    if (p->state == 8)
        p->z -= 0x40000;
    if (!eng1_29c20(p)) {
        p->x = p->saved_x;
        p->y = p->saved_y;
        p->z = p->saved_z;
    }
    p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10);
    if (g_eng1_setup_dword_72188) {
        if (g_eng1_setup_word_72012 & 0x20) {
            g_native_word_72168 |= 0x20;
            eng1_24c60(p);
        }
        if (p->state < 7 && eng1_26d50(p)) {
            g_boss_threshold_72184 = 0x100 / ((char)g_engine_interface.context_004->unknown_000[0x0e] * 2 + 6) + 1;
            if (--p->hits <= 0)
                g_boss_threshold_72184 = 300;
            if (g_native_word_70832 - g_boss_threshold_72184 > 0) {
                select_frame(g_boss_71e28, 0x1a);
                return;
            }
            select_frame(g_boss_71e28, 0x1d);
            g_configuration_dword_713bc[11] = 0x10;
            g_configuration_dword_713bc[12] = 0x20;
            return;
        }
    }
    if (eng1_2a710(&g_collision_context->bounds, p))
        eng1_native_code_target_00266a0(p, 3, 0);
    if (g_pipeline_dword_71a18 && g_pipeline_dword_71a18->child &&
        g_pipeline_dword_71a18->child->kind == g_pipeline_dword_71a18->unknown_108 &&
        !g_eng1_setup_dword_7201c && eng1_2a660(g_pipeline_dword_71a18->child, p)) {
        eng1_20000(g_pipeline_dword_71a18->child);
        remove_actor(g_pipeline_dword_71a18->child);
    }
}

void eng1_244d0(Actor *p)
{
    RelativeMeasure m;
    measure_absolute_vector((FixedPosition *)p, (FixedPosition *)g_collision_context, &m);
    switch (p->state) {
    case 0:
        if (g_collision_context->x < p->x && !p->direction_x) {
            select_frame(p, 0x14);
            return;
        }
        if (g_collision_context->x > p->x && p->direction_x == 0xff) {
            select_frame(p, 0x14);
            return;
        }
        switch (eng1_24680()) {
        case 1:
            break;
        case 2:
            select_frame(p, 1);
            return;
        case 3:
            select_frame(p, 2);
            return;
        case 6:
            if (!p->unknown_130) {
                select_frame(p, 0x0f);
                spawn_actor_with_owner(p, p->x, p->y, p->z, 0x41);
                p->delay = g_boss_script_5a060[++g_boss_script_index_71e24];
                return;
            }
            break;
        }
        select_frame(p, 0);
        return;
    case 2:
        if (g_collision_context->x < p->x)
            p->direction_x = 0xff;
        else
            p->direction_x = 0;
        if (p->delay) {
            --p->delay;
            return;
        }
        if (m.x < 0x50)
            select_frame(p, 0x17);
        else
            select_frame(p, 0x11);
        p->motion_x.target = eng1_2bef0(eng3_1f7b0(m.x, m.y)) << 4;
        return;
    }
}

unsigned short eng1_24680(void)
{
    unsigned short value = g_boss_script_5a060[++g_boss_script_index_71e24];
    if (!value) {
        g_boss_script_index_71e24 = g_boss_script_5a060[g_boss_script_index_71e24 + 1];
        value = g_boss_script_5a060[g_boss_script_index_71e24];
    }
    return value;
}

void eng1_246c0(Actor *p)
{
    p->state = 0;
    p->render_flags |= 0x10000000;
    p->sprite = 0x18;
    p->scale = 0x100;
    p->direction_z = 0xff;
    p->direction_y = 0;
    p->motion_y.velocity = p->motion_y.target = 0x100000;
    p->motion_y.unknown = 0;
    p->motion_y.rate = random_limit(0x2000) + 0x3000;
    p->motion_z.velocity = p->motion_z.target = ((dispatch_engine_2c470() & 0x1ff) + 0xa80) << 8;
    p->motion_z.rate = 0x4000;
    p->motion_z.unknown = 0;
    p->motion_x.velocity = p->motion_x.target = 0;
    p->motion_x.rate = random_limit(0x2000) + 0x3000;
    p->motion_x.unknown = 0;
}

void eng1_24780(Actor *p)
{
    Offset2 offset;
    if (p->state) {
        p->angle += 0x40;
        if (eng1_2bd20(p) & 1) {
            g_pipeline_dword_71680[++g_pipeline_dword_71680[0]] = 0xffc00000;
            eng1_24980(p);
            remove_actor(p);
            return;
        }
    } else if (p->unknown_12c) {
        switch (g_boss_71e28->state) {
        case 2:
        case 4:
        case 5:
            eng1_31000(g_boss_71e28, &offset);
            p->x = g_boss_71e28->x + (offset.x << 16);
            p->y = g_boss_71e28->y + (offset.y << 16);
            p->direction_x = g_boss_71e28->direction_x;
            break;
        case 6:
            g_boss_71e28->state = 0;
            p->motion_x.target = p->motion_x.velocity = g_boss_71e28->motion_x.target;
            p->motion_z.velocity = p->motion_z.target += g_boss_71e28->motion_x.target / 6;
            unlink_12c(p);
            p->state = 5;
            break;
        case 7:
        case 8:
            eng1_24980(p);
            remove_actor(p);
            return;
        }
    } else {
        eng1_24980(p);
        remove_actor(p);
        return;
    }
    if (g_pipeline_word_6fec4 && eng1_2a710((Bounds *)g_pipeline_actor_slots_71440, p)) {
        eng1_24980(p);
        switch (g_boss_drop_count_6ffe4) {
        case 0:
            eng1_131c0(p->x, p->y, p->z, 2);
            break;
        case 1:
            eng1_131c0(p->x, p->y, p->z, 3);
            break;
        case 2:
            eng1_131c0(p->x, p->y, p->z, 4);
            break;
        default:
            eng1_131c0(p->x, p->y, p->z, 1);
            break;
        }
        ++g_boss_drop_count_6ffe4;
        remove_actor(p);
    }
    if (eng1_2a710(&g_collision_context->bounds, p))
        eng1_native_code_target_00266a0(p, 3, 0);
    if (eng1_native_code_target_002aa30(p, 0x100, 0x100))
        remove_actor(p);
}

void eng1_24980(Actor *p)
{
    int i, n;
    long offset, rise;
    Actor *child;
    spawn_at_group_300c0(p->x, p->y - 0x200000, p->z, 0x2027, 0);
    play_sound(0xc150, p);
    n = random_limit(11) + 8;
    for (i = 0; i < n; i++) {
        offset = (random_limit(0x40) - 0x20) << 16;
        rise = random_limit(0x30) << 16;
        child = spawn_at_group_300c0(p->x + offset, p->y - rise, p->z, 0x4028, 0);
        if (child) {
            child->render_flags |= 0x10000000;
            if (random_limit(2))
                child->sprite = 9;
            else
                child->sprite = 10;
            child->scale = random_limit(0xa0 - n * 3) + (0x48 - n * 3);
            child->direction_x = -random_limit(2);
            child->direction_z = -random_limit(2);
            child->direction_y = 0xff;
            child->motion_y.velocity = child->motion_y.target = ((dispatch_engine_2c470() & 0x1ff) + 0x800) << 8;
            child->motion_y.rate = 0;
            child->motion_y.unknown = random_limit(0x2000) + 0x4000;
            child->motion_z.velocity = child->motion_z.target = ((dispatch_engine_2c470() & 0xff) + 0x200) << 8;
            child->motion_z.rate = random_limit(0x1000) + 0x2000;
            child->motion_z.unknown = 0;
            child->motion_x.velocity = child->motion_x.target = ((dispatch_engine_2c470() & 0x1ff) + 0x100) << 8;
            child->motion_x.rate = random_limit(0x1000) + 0x1000;
            child->motion_x.unknown = 0;
            child->state = 0;
        }
    }
}

void eng1_24b60(Actor *p)
{
    unsigned int result;
    long step;
    p->angle = 0x400 - ((p->z >> 13) & 0x3ff);
    p->angle_72 = 0x400 - ((p->x >> 13) & 0x3ff);
    result = eng1_2bd20(p);
    if (result & 2) {
        remove_actor_variant(p);
        return;
    }
    if (p->state < random_limit(2) + 1 && (result & 1)) {
        play_sound((random_limit(2) + 0x153) | 0xc000, p);
        p->state++;
    }
    step = (p->scale >> 5) + 5;
    if (p->direction_x)
        step = ~step + 1;
    if (eng1_30050((step << 16) + p->x, p->y, p->z))
        p->direction_x = ~p->direction_x;
    if (p->z < -0x400000 || p->y > 0x5800000 || eng1_native_code_target_002aa30(p, 0x80, 0x190))
        remove_actor_variant(p);
}

void eng1_24c60(Actor *p)
{
    int i, n;
    long offset;
    Actor *child;
    if (--p->parameter < 0) {
        switch ((char)g_engine_interface.context_004->unknown_000[0x0e]) {
        case 0:
            p->parameter = random_limit(2) + 2;
            break;
        case 1:
            p->parameter = random_limit(3) + 3;
            break;
        default:
            p->parameter = 999;
            break;
        }
        child = spawn_at_group_300c0(g_collision_context->x, p->y - 0x1000000, p->z, 0x4029, 0);
        if (child) {
            child->render_flags |= 0x10000040;
            child->sprite = g_debris_sprites_601ac[random_limit(8)];
            child->direction_y = 0xff;
            child->direction_z = 0xff;
            child->motion_y.target = ((dispatch_engine_2c470() & 0x1ff) + 0xc00) << 8;
            child->motion_y.velocity = 0;
            child->motion_y.rate = 0;
            child->motion_y.unknown = random_limit(0x2000) + 0x8000;
            child->motion_z.target = 0xf0000;
            child->motion_z.velocity = 0;
            child->motion_z.rate = 0;
            child->motion_z.unknown = 0;
            child->state = 0;
        }
    }
    n = random_limit(7) + 10;
    for (i = 0; i < n; i++) {
        offset = random_limit(0x80) - 0x40;
        child = spawn_at_group_300c0((offset << 16) + g_collision_context->x, p->y - 0x1000000, p->z, 0x4028, 0);
        if (child) {
            child->render_flags |= 0x10000000;
            if (random_limit(2))
                child->sprite = 9;
            else
                child->sprite = 10;
            child->delay = random_limit(0x1b) + 0x0b;
            child->sprite = g_debris_sprites_5ef60[child->delay];
            child->scale = random_limit(0xa0 - n * 3) + 0x40 - n * 3;
            child->direction_x = -random_limit(2);
            child->direction_z = 0xff;
            child->direction_y = 0xff;
            child->motion_y.target = ((dispatch_engine_2c470() & 0x1ff) + 0xa00) << 8;
            child->motion_y.velocity = 0;
            child->motion_y.rate = 0;
            child->motion_y.unknown = random_limit(0x2000) + 0x4000;
            child->motion_z.velocity = child->motion_z.target = (dispatch_engine_2c470() & 0x1ff) << 8;
            child->motion_z.rate = random_limit(0x1000);
            child->motion_z.unknown = 0;
            child->motion_x.velocity = child->motion_x.target = (dispatch_engine_2c470() & 0x1ff) << 8;
            child->motion_x.rate = random_limit(0x1000);
            child->motion_x.unknown = 0;
            child->spin_x.value = random_limit(8) << 2;
            child->spin_y.value = random_limit(8) << 2;
            child->spin_z.value = random_limit(8) << 2;
            child->state = 0;
        }
    }
}

void eng1_24f50(Actor *p)
{
    p->angle += p->spin_x.step;
    if (g_boss_71e28->state != 7 && g_boss_71e28->state != 8 && eng1_2a660(g_boss_71e28, p)) {
        g_boss_threshold_72184 = 0x100 / ((char)g_engine_interface.context_004->unknown_000[0x0e] * 2 + 6) * 2 + 3;
        g_boss_71e28->hits -= 2;
        if (g_boss_71e28->hits <= 0)
            g_boss_threshold_72184 = 300;
        if (g_native_word_70832 - g_boss_threshold_72184 > 0)
            select_frame(g_boss_71e28, 0x1a);
        else {
            select_frame(g_boss_71e28, 0x1d);
            g_configuration_dword_713bc[11] = 0x10;
            g_configuration_dword_713bc[12] = 0x20;
        }
    }
    p->y += 0x180000;
    p->saved_y += 0x180000;
    if (eng1_2bd20(p) & 1) {
        if (p->motion_z.rate == 0)
            play_sound(0x4150, p);
        if (p->motion_z.rate == 0x4000)
            play_sound(0x4151, p);
        if (p->motion_z.rate == 0x8000)
            play_sound(0x4152, p);
        p->motion_z.velocity += 0x20000;
        p->motion_z.rate += 0x4000;
        p->spin_x.value += 0x20;
    }
    p->y -= 0x180000;
    p->saved_y -= 0x180000;
    if (eng1_2a710(&g_collision_context->bounds, p))
        eng1_native_code_target_00266a0(p, 3, 0);
    if (g_pipeline_dword_71a18 && g_pipeline_dword_71a18->child &&
        g_pipeline_dword_71a18->child->kind == g_pipeline_dword_71a18->unknown_108 &&
        !g_eng1_setup_dword_7201c && eng1_2a660(g_pipeline_dword_71a18->child, p)) {
        eng1_20000(g_pipeline_dword_71a18->child);
        remove_actor(g_pipeline_dword_71a18->child);
    }
    if (p->z < -0x800000 || p->z > 0x1f00000 || eng1_2a9e0(p, 0x80))
        remove_actor_variant(p);
}

void eng1_25190(Actor *p)
{
    int i, n;
    long dx, dy, dz;
    Actor *child;
    spawn_at_group_300c0(p->x, p->y - 0x200000, p->z, 0x2027, 0);
    play_sound(0xc150, p);
    n = random_limit(8) + 0x18;
    for (i = 0; i < n; i++) {
        dx = (random_limit(0x80) - 0x40) << 16;
        dy = random_limit(0x80) - 0x40;
        if (dy < 0)
            dy = 0;
        dz = random_limit(0x60) - 0x30;
        child = spawn_at_group_300c0(p->x + dx, p->y - (dy << 16), (dz << 16) + p->z, 0x4028, 0);
        if (child) {
            child->render_flags |= 0x10000000;
            if (random_limit(2))
                child->sprite = 9;
            else
                child->sprite = 10;
            child->scale = random_limit(0x80) + (0x90 - n * 2);
            if (i > 0x18)
                child->scale >>= 1;
            child->direction_x = -random_limit(2);
            child->direction_z = -random_limit(2);
            child->direction_y = 0xff;
            child->motion_y.velocity = child->motion_y.target = ((dispatch_engine_2c470() & 0x1ff) + 0xa00) << 8;
            child->motion_y.rate = 0;
            child->motion_y.unknown = random_limit(0x1000) + 0x6000;
            child->motion_z.velocity = child->motion_z.target = ((dispatch_engine_2c470() & 0xff) + 0x200) << 8;
            child->motion_z.rate = random_limit(0x1000) + 0x2000;
            child->motion_z.unknown = 0;
            child->motion_x.velocity = child->motion_x.target = ((dispatch_engine_2c470() & 0x1ff) + 0x100) << 8;
            child->motion_x.rate = random_limit(0x1000) + 0x1000;
            child->motion_x.unknown = 0;
            child->state = 0;
        }
    }
}

void reset_render_parameter(Actor *p)
{
    p->render_flags = 0;
    p->parameter = 0x3c;
}

void eng1_253d0(Actor *p)
{
    long offset;
    Actor *child;
    if (p->parameter) {
        p->parameter--;
        if (random_limit(6))
            return;
        offset = (random_limit(0xa0) - 0x50) << 16;
        child = spawn_at_group_300c0(offset + p->x, p->y - 0x1000000, p->z, 0x4028, 0);
        if (child) {
            child->render_flags |= 0x10000000;
            if (random_limit(2))
                child->sprite = 9;
            else
                child->sprite = 10;
            child->scale = random_limit(0xa0) + 0x40;
            child->direction_x = -random_limit(2);
            child->direction_y = child->direction_z = 0xff;
            child->motion_y.target = ((dispatch_engine_2c470() & 0x1ff) + 0xa00) << 8;
            child->motion_y.velocity = 0;
            child->motion_y.rate = 0;
            child->motion_y.unknown = random_limit(0x2000) + 0x4000;
            child->motion_z.velocity = child->motion_z.target = (dispatch_engine_2c470() & 0x1ff) << 8;
            child->motion_z.rate = random_limit(0x1000);
            child->motion_z.unknown = 0;
            child->motion_x.velocity = child->motion_x.target = (dispatch_engine_2c470() & 0x1ff) << 8;
            child->motion_x.rate = random_limit(0x1000);
            child->motion_x.unknown = 0;
            child->spin_x.value = random_limit(8) << 2;
            child->spin_y.value = random_limit(8) << 2;
            child->spin_z.value = random_limit(8) << 2;
            child->state = 0;
        }
    } else
        release_actor(p);
}

void eng1_25580(Actor *p)
{
    p->flags |= 0x800;
    p->render_flags |= 0x20000600;
    p->sprite = 1;
    p->category = 1;
    p->mode = 4;
    attach_entries(p, g_entries_5a450);
}

void eng1_255c0(Actor *p)
{
    p->angle += 6;
    p->angle_72 += 0x10;
    p->angle_74 += 2;
    if (!p->unknown_12c)
        remove_actor(p);
}
