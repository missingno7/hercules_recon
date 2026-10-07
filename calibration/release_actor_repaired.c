/* Private ordinary-C reconstruction hypothesis for ENG1 release_actor @ 0x27e20.
 * Not canonical source; external mappings are diagnostic only. */
typedef struct Actor Actor;
typedef struct Sidecar Sidecar;
typedef struct KindInfo KindInfo;

struct Actor {
    long x, y, z;
    unsigned char unknown_00c[0x22 - 0x0c];
    unsigned char category, mode;
    unsigned char unknown_024[6];
    short field_02a;
    unsigned char unknown_02c[2];
    unsigned short kind;
    unsigned char unknown_030[8];
    unsigned short profile;
    unsigned char unknown_03a[0x50 - 0x3a];
    unsigned long flags, render_flags;
    void *owner_context;
};

struct Sidecar {
    long x, y, z;
    unsigned short flags;
    short field_00e;
};

struct KindInfo {
    unsigned long sidecar_offset;
    unsigned long unknown;
};

typedef char actor_profile_offset[(unsigned long)&(((Actor *)0)->profile) == 0x38 ? 1 : -1];
typedef char actor_kind_offset[(unsigned long)&(((Actor *)0)->kind) == 0x2e ? 1 : -1];
typedef char actor_flags_offset[(unsigned long)&(((Actor *)0)->flags) == 0x50 ? 1 : -1];
typedef char actor_context_offset[(unsigned long)&(((Actor *)0)->owner_context) == 0x58 ? 1 : -1];
typedef char sidecar_flags_offset[(unsigned long)&(((Sidecar *)0)->flags) == 0x0c ? 1 : -1];
typedef char sidecar_word_offset[(unsigned long)&(((Sidecar *)0)->field_00e) == 0x0e ? 1 : -1];

extern KindInfo g_kind_info[];
extern signed char g_special_kind;
extern void detach_child(Actor *);
extern void detach_buffer(Actor *);
extern void detach_attachment(Actor *);
extern void clear_actor(Actor *);
extern int remove_actor(Actor *);

int release_actor(Actor *p)
{
    unsigned short kind = p->kind;
    unsigned short profile;
    unsigned short owner_flags;
    Sidecar *owner;
    unsigned long sidecar_offset;

    if (kind == 0) return 1;
    if (kind & 0x2000) {
    if (p->flags & 0x10000000) {
        p->kind = 0;
        clear_actor(p);
        return 1;
    }
    owner = (Sidecar *)p->owner_context;
    if (!owner) {
        detach_child(p);
        detach_buffer(p);
        detach_attachment(p);
        p->kind = 0;
        clear_actor(p);
        return 1;
    }
    profile = p->profile;
    sidecar_offset = g_kind_info[profile].sidecar_offset;
    owner = (Sidecar *)((unsigned char *)owner + sidecar_offset);
    owner_flags = owner->flags;
    if (owner_flags & 0x2000) return 0;
    if ((owner_flags & 0x0100) && profile == (unsigned short)g_special_kind) {
        owner->x = p->x;
        owner->y = p->y;
        owner->z = p->z;
        owner->field_00e = p->field_02a;
    }
    if ((owner_flags & 0x0080) && p->profile == (unsigned short)g_special_kind) {
        owner->field_00e = p->field_02a;
    }
    detach_child(p);
    detach_buffer(p);
    detach_attachment(p);
    p->kind = 0;
    owner->flags &= 0x7fff;
    clear_actor(p);
    return 1;
    }
    return remove_actor(p);
}
