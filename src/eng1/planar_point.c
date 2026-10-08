/* Point-in-planar-bounds test (ENG1 0x2a520). Kept in its own unit: it reproduces
 * standalone, but not after planar_bounds_overlap or other bounds-dereferencing
 * bodies (evidence/leaf_harvest.json, context_sensitivity). Not a recovered TU. */
typedef struct Bounds3D {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds3D;

int point_in_planar_bounds(unsigned int x, unsigned int y, Bounds3D *b)
{
    int out;
    out = (b->min_y > (short)(y >> 16));
    out |= (b->max_x < (short)(x >> 16));
    out |= (b->min_x > (short)(x >> 16));
    out |= (b->max_y < (short)(y >> 16));
    return !out;
}
