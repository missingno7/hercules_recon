/* Six independently matched frame/list helpers extracted from the preserved
 * ENG1 macro experiment. Shared offsets are a layout hypothesis, not an
 * identified original translation unit. No external calls or data are used. */
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
