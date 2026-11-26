/* Measured VC5 unit-context hypothesis: this definition precedes the planar
 * helper, with local declaration order hy/hz/hx and initialization order x/y/z.
 * The folded comparison keys and register ties reproduce ENG1 0x2a810 while
 * preserving both accepted peers. This does not prove original source order. */
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

/* Reuse the measured ENG1 0x2a810 signed upper-WORD inclusive box test. */
int point_in_box_2a810(unsigned long x, unsigned long y, unsigned long z,
                       const Bounds3D *b)
{
    short hy, hz, hx;
    int inside;
    hx = (short)(x >> 16);
    hy = (short)(y >> 16);
    hz = (short)(z >> 16);

    inside = (hz >= b->min_z);
    inside &= (hx <= b->max_x);
    inside &= (hy >= b->min_y);
    inside &= (hz <= b->max_z);
    inside &= (hy <= b->max_y);
    inside &= (hx >= b->min_x);
    return inside;
}

int planar_bounds_overlap(Bounds3D *a, Bounds3D *b)
{
    return !((a->max_y < b->min_y) | (b->max_y < a->min_y) |
             (a->max_x < b->min_x) | (b->max_x < a->min_x));
}

