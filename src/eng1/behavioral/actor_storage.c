/* Loader-zero pointer owners, PC ENG1 RVAs 72860/73494/734e0.
 * Allocation is performed by frame startup, not by this provider. The three
 * actor layouts remain the measured per-consumer views in recovered units.
 * See evidence/actor_pool_layout.json and actor_pool_reset_region.json.
 */
typedef char eng1_actor_pointer_is_four[(sizeof(void *) == 4) ? 1 : -1];
void *g_normal_actor_pool;
void *g_middle_actor_pool;
void *g_alternate_actor_pool;
/* Native scene-list anchor at6fed8, loader-zero pointer cell. */
void *g_scene_anchor;
