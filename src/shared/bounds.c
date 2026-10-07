/* Six signed bounds at offsets 0, 2, 4, 6, 8, 10. Touching is overlap.
 * Semantic names are provisional; no original type identity is claimed. */
typedef struct Bounds3D {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds3D;
int bounds_overlap(Bounds3D *a, Bounds3D *b)
{
    return !((a->max_y < b->min_y) | (b->max_y < a->min_y) |
             (a->max_z < b->min_z) | (b->max_z < a->min_z) |
             (a->max_x < b->min_x) | (b->max_x < a->min_x));
}
