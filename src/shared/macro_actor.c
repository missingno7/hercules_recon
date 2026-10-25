/* Bounded macro reconstruction: ENG1 0x29160..0x296df.
 * A shared layout hypothesis, not an identified original translation unit.
 * External symbols are unresolved declarations; this file cannot dispatch
 * into an original binary. Addresses exist only in the comparison manifest. */
typedef struct Actor Actor;
typedef struct Step2 { short x, y; } Step2;
typedef struct Step3 { short x, y, z; } Step3;
struct Actor {
    long x, y, z;
    unsigned char unknown_00c[0x22 - 0x0c];
    unsigned char category;
    unsigned char unknown_023[0x2e - 0x23];
    unsigned short kind;
    short frame_ticks, frame_state;
    unsigned char unknown_034[0x50 - 0x34];
    unsigned long flags, render_flags;
    void *unknown_058;
    Actor *attachment, *child;
    void *image;
    void *unknown_068;
    void *buffer;
    unsigned char unknown_070[0x94 - 0x70];
    unsigned char negative_x, negative_y, negative_z, tag;
    short vertical_step;
    unsigned char unknown_09a[0xe4 - 0x9a];
    Actor *prev_scene, *next_scene;
    void *unknown_0ec;
    void *steps;
    long step_index;
    unsigned char unknown_0f8[0x110 - 0xf8];
    void **frames;
    short frame_index;
    short unknown_116;
    void *initial_steps;
    short unknown_11c, active_step;
    unsigned char unknown_120[8];
    unsigned short **command;
    Actor *prev_chain, *next_chain;
};

typedef struct Asset8 {
    void *image;
    unsigned short unknown_004;
    unsigned char category, unknown_007;
} Asset8;
typedef struct Asset16 {
    void *image;
    unsigned char unknown_004[8];
    unsigned char category;
    unsigned char unknown_00d[3];
} Asset16;
typedef void (__cdecl *ActorCallback)(Actor *);
typedef struct Callback8 { ActorCallback init; void *unknown_004; } Callback8;
typedef struct Callback12 { ActorCallback init; void *unknown_004, *unknown_008; } Callback12;
extern Asset8 *g_special_assets, *g_alternate_assets;
extern Asset16 *g_normal_assets;
extern Callback8 *g_special_callbacks, *g_alternate_callbacks;
extern Callback12 *g_normal_callbacks;
extern Actor *g_scene_anchor;
extern unsigned long g_tag_list[];
extern unsigned short **allocate_command(unsigned int opcode);
extern void release_actor(Actor *p);
extern void release_buffer(Actor *p);
extern void refresh_actor(Actor *p);

/* Four pre-existing exact functions are controls, adapted to the shared view. */
void apply_frame_motion_2d(Actor *p)
{
    Step2 *step;
    short x;
    if (p->active_step >= 0) {
        step = (Step2 *)p->steps + p->step_index;
        x = step->x;
        if (p->negative_x) p->x -= x << 16;
        else p->x += x << 16;
        p->y += step->y << 16;
        p->z += p->vertical_step << 16;
    }
}

void apply_frame_motion_3d(Actor *p)
{
    Step3 *step;
    short x, z;
    if (p->active_step >= 0) {
        step = (Step3 *)p->steps + p->step_index;
        x = step->x;
        z = step->z;
        if (p->negative_x) p->x -= x << 16;
        else p->x += x << 16;
        p->y += step->y << 16;
        if (p->negative_z) p->z += z << 16;
        else p->z -= z << 16;
    }
}

void reset_frame_state(Actor *p)
{
    p->frame_ticks = p->frame_state = 0;
}

void advance_frame(Actor *p)
{
    void *image;
    ++p->frame_index;
    do {
        image = p->frames[p->frame_index];
        if (!image) {
            ++p->frame_index;
            p->frame_index = (short)(long)p->frames[p->frame_index];
        }
    } while (!image);
    p->image = image;
}

void restart_steps(Actor *p)
{
    p->steps = p->initial_steps;
    p->step_index = 0;
    p->active_step = 1;
}

void select_frame(Actor *p, int index)
{
    p->frame_index = index;
    p->image = p->frames[index];
    p->frame_state = -1;
    p->frame_ticks = 0;
}

void change_kind(Actor *p, unsigned short kind)
{
    if (p->kind & 0x6000) {
        if (p->kind & 0x4000) {
            p->kind = kind;
            p->category = g_special_assets[kind & 0xfff].category;
            p->image = g_special_assets[kind & 0xfff].image;
            g_special_callbacks[kind & 0xfff].init(p);
        } else {
            p->kind = kind;
            p->category = g_alternate_assets[kind & 0xfff].category;
            p->image = g_alternate_assets[kind & 0xfff].image;
            g_alternate_callbacks[kind & 0xfff].init(p);
        }
    } else {
        p->kind = kind;
        p->category = g_normal_assets[kind].category;
        p->image = g_normal_assets[kind].image;
        g_normal_callbacks[kind].init(p);
    }
    reset_frame_state(p);
    refresh_actor(p);
}

int create_command(Actor *p)
{
    unsigned short **handle = allocate_command(0x3781);
    unsigned short *command;
    if (handle) {
        command = *handle;
        p->command = handle;
        p->flags |= 0x20000000;
        command[0] |= 0x8000;
        command[1] = command[2] = command[3] = 0;
        command[4] = command[5] = 0;
        command[6] = command[7] = 0;
        command[8] = command[9] = 0;
        return 1;
    }
    return 0;
}

int count_chain(Actor *p)
{
    int count = 0;
    p = p->next_chain;
    while (p) { ++count; p = p->next_chain; }
    return count;
}

void append_chain(Actor *p, Actor *next)
{
    if (!((p->kind | next->kind) & 0x6000)) {
        while (p->next_chain) p = p->next_chain;
        p->next_chain = next;
        next->prev_chain = p;
    }
}

void unlink_12c(Actor *p)
{
    if (p->prev_chain) p->prev_chain->next_chain = p->next_chain;
    if (p->next_chain) p->next_chain->prev_chain = p->prev_chain;
}

void detach_attachment(Actor *p)
{
    Actor *attachment = p->attachment;
    if (attachment && !(p->flags & 0x20000)) {
        if (p->flags & 0x10000) p = attachment;
        p->attachment = 0;
        if (attachment->flags & 0x10000) release_actor(attachment);
    }
}

void detach_child(Actor *p)
{
    Actor *child = p->child;
    if (child && !(p->flags & 0x40000000)) {
        p->child = 0;
        p->render_flags &= ~0x800000;
        release_actor(child);
    }
}

void detach_buffer(Actor *p)
{
    if (!(p->flags & 0x10000000) && p->buffer) {
        p->buffer = 0;
        release_buffer(p);
    }
}

Actor *insert_scene(Actor *p, Actor *after)
{
    Actor *anchor;
    if (p->flags & 0x80000) return 0;
    if (!after) after = g_scene_anchor;
    anchor = after;
    if (after) {
        if (p->flags & 0x40000) {
            while (after->next_scene && !(after->next_scene->flags & 0x40000))
                after = after->next_scene;
        }
        p->next_scene = after->next_scene;
        if (p->next_scene) p->next_scene->prev_scene = p;
        after->next_scene = p;
        p->prev_scene = after;
    }
    return anchor;
}

void unlink_0e4(Actor *p)
{
    if (!(p->flags & 0x80000)) {
        if (p->prev_scene) p->prev_scene->next_scene = p->next_scene;
        if (p->next_scene) p->next_scene->prev_scene = p->prev_scene;
        p->prev_scene = 0;
        p->next_scene = 0;
    }
}

Actor *find_tag(int tag)
{
    int i, count = g_tag_list[0];
    unsigned long *entry = g_tag_list + 1;
    Actor *p;
    for (i = 0; i < count; ++i) {
        p = (Actor *)*entry++;
        if (p->tag == tag) return p;
    }
    return 0;
}

void hide_and_unlink(Actor *p)
{
    p->render_flags &= 0x7fffffff;
    unlink_12c(p);
    p->prev_chain = 0;
    p->next_chain = 0;
}
