/* PC-derived actor factories; descriptive names and field views only. */
typedef struct Actor Actor;
typedef unsigned short ActorKind;

struct Actor {
    long x, y, z;
    unsigned char unknown_00c[0x22 - 0x0c];
    unsigned char category;
    unsigned char unknown_023[0x2e - 0x23];
    ActorKind kind;
    unsigned char unknown_030[0x64 - 0x30];
    void *image;
    unsigned char unknown_068[0x9c - 0x68];
    long unknown_09c, unknown_0a0, unknown_0a4;
    unsigned char unknown_0a8[0x110 - 0x0a8];
    void **frames;
    unsigned char unknown_114[4];
    void *initial_steps;
};

typedef struct NormalFactoryAsset NormalFactoryAsset;
struct NormalFactoryAsset {
    void *image;
    void *field_004;
    void *field_008;
    unsigned char category;
    unsigned char unknown_00d[3];
};

typedef struct AlternateFactoryAsset AlternateFactoryAsset;
struct AlternateFactoryAsset {
    void *image;
    unsigned short unknown_004;
    unsigned char category;
    unsigned char unknown_007;
};

extern Actor *allocate_normal_actor(ActorKind kind);
extern void initialize_normal_actor(Actor *actor);
extern Actor *allocate_alternate_actor(ActorKind kind);
extern void initialize_alternate_actor(Actor *actor);
extern NormalFactoryAsset *g_normal_factory_assets;
extern AlternateFactoryAsset *g_alternate_factory_assets;

Actor *spawn_normal(long x, long y, long z, ActorKind kind)
{
    Actor *actor = allocate_normal_actor(kind);

    if (actor) {
        initialize_normal_actor(actor);
        actor->kind = kind;
        actor->x = x;
        actor->unknown_09c = x;
        actor->y = y;
        actor->unknown_0a0 = y;
        actor->z = z;
        actor->unknown_0a4 = z;
        actor->category = g_normal_factory_assets[kind].category;
        actor->image = g_normal_factory_assets[kind].image;
        actor->frames = (void **)g_normal_factory_assets[kind].field_004;
        actor->initial_steps = g_normal_factory_assets[kind].field_008;
    }

    return actor;
}

Actor *spawn_kind(long x, long y, long z, ActorKind kind)
{
    Actor *actor = allocate_alternate_actor(kind);

    if (actor) {
        initialize_alternate_actor(actor);
        actor->kind = kind | 0x2000;
        actor->x = x;
        actor->y = y;
        actor->z = z;
        actor->category = g_alternate_factory_assets[kind & 0xfff].category;
        actor->image = g_alternate_factory_assets[kind & 0xfff].image;
    }

    return actor;
}
