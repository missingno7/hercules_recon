/* ENG1 2a710 and 2aa30, reconstructed from the original PC bodies.
 * The six signed bounds and actor screen prefix are measured views, not
 * original source type or translation-unit identities. VC5 C++ lowering is
 * a measured reproducing context: ordinary C reorders the six comparisons. */
extern "C" {
typedef struct SpatialBounds {
    short mn[3], mx[3];
} SpatialBounds;
typedef struct SpatialActor {
    unsigned char pad_000[0x0c];
    short screen_x, screen_y;
    unsigned char pad_010[0x1f - 0x10];
    unsigned char screen_flags;
} SpatialActor;
typedef char bounds_size_is_12[sizeof(SpatialBounds)==12?1:-1];
typedef char actor_prefix_is_32[sizeof(SpatialActor)==32?1:-1];
typedef char spatial_long_is_32[sizeof(unsigned long)==4?1:-1];
extern int __cdecl actor_bounds_2a880(void *, SpatialBounds *, unsigned long);
extern unsigned short g_pipeline_word_71376;

/* Physical argument order is area, actor. Obtain the actor's bounds in mode0;
 * touching faces count as overlap, and a failed extent query returns zero. */
int __cdecl eng1_native_code_target_002a710(SpatialBounds *area, void *actor)
{
    SpatialBounds b;
    if (actor_bounds_2a880(actor, &b, 0))
        return !((b.mx[2] < area->mn[2]) | (b.mx[1] < area->mn[1]) |
                 (b.mx[0] < area->mn[0]) | (area->mx[1] < b.mn[1]) |
                 (area->mx[2] < b.mn[2]) | (area->mx[0] < b.mn[0]));
    return 0;
}

/* Margins use only their unsigned low WORDs. Coordinates are signed WORDs;
 * every comparison contributes a bit, without short-circuit evaluation. */
int __cdecl eng1_native_code_target_002aa30(SpatialActor *actor,
    unsigned short x_margin, unsigned short y_margin)
{
    if (g_pipeline_word_71376 == 0 && (actor->screen_flags & 3) == 1) {
        return ((actor->screen_y - y_margin) > 239) |
               ((actor->screen_y + y_margin) < 0) |
               ((actor->screen_x + x_margin) < 0) |
               ((actor->screen_x - x_margin) > 319);
    }
    return 0;
}
}
