/* Independent PC boundary-entry reconstruction. This reference unit is not a
 * production provider: root-header and row-table ownership/capacity must still
 * be reviewed before integration. Views and function names are provisional.
 * Preserve the original X upper-bound comparison/store asymmetry. */
typedef unsigned long BoundaryDword;
typedef struct BoundaryActor { long x, y, z; } BoundaryActor;
typedef struct BoundaryHeader {
    unsigned char prefix[4];
    unsigned short width, height;
} BoundaryHeader;
extern void *g_normal_actor_pool;
extern short g_camera_flags_70d10;
extern signed char g_active_category_70cfe;
extern unsigned char *g_root_group_rows_70d5c[];
extern short g_rebuilt_row_offsets_70e0c[];
#define BOUNDARY_ACTOR() ((BoundaryActor *)(*(void * volatile *)(void *)&g_normal_actor_pool))
#define BOUNDARY_HEADER() ((BoundaryHeader *)(void *)g_root_group_rows_70d5c[(int)g_active_category_70cfe])

void eng1_19c30(void)
{
    BoundaryActor *actor;
    BoundaryDword width;
    long bound;
    if (g_camera_flags_70d10 & 1)
        return;
    actor = BOUNDARY_ACTOR();
    if (actor->x < 0x400000) {
        actor->x = 0x400000;
        actor = BOUNDARY_ACTOR();
    }
    width = (BoundaryDword)BOUNDARY_HEADER()->width << 22;
    bound = (long)(width - 0x400000UL);
    if (actor->x > bound) {
        actor->x = (long)(width - 0x40UL);
        actor = BOUNDARY_ACTOR();
    }
    if (actor->x > 0xc00000) {
        width = ((BoundaryDword)BOUNDARY_HEADER()->width - 3UL) << 22;
        if (actor->x < (long)width) {
            bound = (long)((BoundaryDword)((long)g_rebuilt_row_offsets_70e0c[16] + 3L) << 22);
            if (actor->x < bound) {
                actor->x = bound;
                actor = BOUNDARY_ACTOR();
            }
            bound = (long)((BoundaryDword)((long)g_rebuilt_row_offsets_70e0c[0] +
                (long)g_rebuilt_row_offsets_70e0c[16] - 3L) << 22);
            if (actor->x > bound)
                actor->x = bound;
        }
    }
}

void eng1_19d30(void)
{
    BoundaryActor *actor = BOUNDARY_ACTOR();
    long bound;
    if (actor->y < 0x400000) {
        actor->y = 0x400000;
        actor = BOUNDARY_ACTOR();
    }
    bound = (long)((BoundaryDword)BOUNDARY_HEADER()->height << 22);
    if (actor->y > bound)
        actor->y = bound;
}
