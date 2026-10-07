/* Private ordinary-C reconstruction of ENG1 cleanup helpers.
 * Field names stay conservative where PC evidence establishes offsets only. */
typedef struct Actor Actor;

typedef struct ActorReference {
    short count;
    short unknown_002;
} ActorReference;

typedef struct ActorResource {
    ActorReference *references;
    unsigned short release_mode;
    unsigned char unknown_006[14];
} ActorResource;

typedef struct ActorResourceLinkRow {
    unsigned short live_mask;
    unsigned short unknown_002;
    unsigned short next[4];
} ActorResourceLinkRow;

typedef struct ActorTokenReference {
    short count;
    short unknown_002;
} ActorTokenReference;

struct Actor {
    long x, y, z;
    unsigned char unknown_00c[0x1d - 0x0c];
    unsigned char token_high;
    unsigned char token_low;
    unsigned char unknown_01f[3];
    unsigned char category;
    unsigned char unknown_023[0x34 - 0x23];
    short initial_resource_index;
    unsigned char unknown_036[0x40 - 0x36];
    short reference_index_a;
    short reference_index_b;
    unsigned short resource_index;
    short link_key_a;
    short link_key_b;
    unsigned char unknown_04a[0x50 - 0x4a];
    unsigned long flags;
    unsigned long render_flags;
    void *unknown_058;
    Actor *attachment;
    Actor *child;
};

typedef char actor_token_high_at_01d[(unsigned long)&(((Actor *)0)->token_high) == 0x1d ? 1 : -1];
typedef char actor_token_low_at_01e[(unsigned long)&(((Actor *)0)->token_low) == 0x1e ? 1 : -1];
typedef char actor_category_at_022[(unsigned long)&(((Actor *)0)->category) == 0x22 ? 1 : -1];
typedef char actor_initial_resource_index_at_034[(unsigned long)&(((Actor *)0)->initial_resource_index) == 0x34 ? 1 : -1];
typedef char actor_reference_index_a_at_040[(unsigned long)&(((Actor *)0)->reference_index_a) == 0x40 ? 1 : -1];
typedef char actor_reference_index_b_at_042[(unsigned long)&(((Actor *)0)->reference_index_b) == 0x42 ? 1 : -1];
typedef char actor_resource_index_at_044[(unsigned long)&(((Actor *)0)->resource_index) == 0x44 ? 1 : -1];
typedef char actor_link_key_a_at_046[(unsigned long)&(((Actor *)0)->link_key_a) == 0x46 ? 1 : -1];
typedef char actor_link_key_b_at_048[(unsigned long)&(((Actor *)0)->link_key_b) == 0x48 ? 1 : -1];
typedef char actor_render_flags_at_054[(unsigned long)&(((Actor *)0)->render_flags) == 0x54 ? 1 : -1];
typedef char actor_child_at_060[(unsigned long)&(((Actor *)0)->child) == 0x60 ? 1 : -1];

extern ActorResource g_actor_resources[];
extern signed char g_actor_link_map[16];
extern ActorResourceLinkRow g_actor_link_rows[];
extern ActorTokenReference g_actor_token_references[];
extern void release_actor_resource_state(Actor *p);
extern void release_actor_token(Actor *p);
extern void clear_actor(Actor *p);

#ifndef CLEANUP_HELPERS_SEMANTIC_FIXTURE
void release_actor_resource_state(Actor *p)
{
    ActorResource *resource;
    ActorReference *references;
    ActorResourceLinkRow *row;
    unsigned short key;
    unsigned short group;
    unsigned short item;
    signed char slot;
    int remove_links;

    if (p->resource_index == 0xffff) return;

    resource = g_actor_resources + p->resource_index;
    references = resource->references;

    key = (unsigned short)p->link_key_a;
    if (key != 0xffff) {
        remove_links = 0;
        if (references) {
            --references[p->reference_index_a].count;
            if (references[p->reference_index_a].count == 0) remove_links = 1;
        }
        if (resource->release_mode == 0) remove_links = 1;
        if (remove_links) {
            while (key != 0xffff) {
                group = (unsigned short)(key >> 4);
                item = (unsigned short)(key & 0x0f);
                row = g_actor_link_rows + group;
                row->live_mask &= (unsigned short)(0x0f - item);
                slot = g_actor_link_map[item];
                key = row->next[slot];
            }
        }
        p->link_key_a = (short)-1;
    }

    key = (unsigned short)p->link_key_b;
    if (key != 0xffff) {
        remove_links = 0;
        if (references) {
            --references[p->reference_index_b].count;
            if (references[p->reference_index_b].count == 0) remove_links = 1;
        }
        if (resource->release_mode == 0) remove_links = 1;
        if (remove_links) {
            while (key != 0xffff) {
                group = (unsigned short)(key >> 4);
                item = (unsigned short)(key & 0x0f);
                row = g_actor_link_rows + group;
                row->live_mask &= (unsigned short)(0x0f - item);
                slot = g_actor_link_map[item];
                key = row->next[slot];
            }
        }
        p->link_key_b = (short)-1;
    }

    p->reference_index_a = p->initial_resource_index;
    p->resource_index = p->category;
}

void release_actor_token(Actor *p)
{
    unsigned int token;

    token = p->token_low;
    if (token != 0xff) {
        --g_actor_token_references[token].count;
        p->token_low = 0xff;
        p->token_high = 0xff;
    }
}
#endif

void clear_actor(Actor *p)
{
    Actor *child;
    unsigned long render_flags;

    render_flags = p->render_flags;
    if (!(render_flags & 0x10000800)) {
        release_actor_resource_state(p);
        release_actor_token(p);
        if (render_flags & 0x00800000) {
            child = p->child;
            if (child) {
                clear_actor(child);
                release_actor_token(child);
            }
        }
    }
}
