/* ENG1 0x2ab30; ENG3 0x1f280 remains a distinct code-generation case.
 * ~n + 1 preserves the observed NOT/INC absolute-value spelling. A 32-bit
 * swap temporary preserves the observed signed extension of 16-bit deltas.
 * Distances are approximations with historical signed-short narrowing. */
typedef struct FixedPosition { long x, y, z; } FixedPosition;
typedef struct RelativeMeasure {
    short x, y, z;
    short planar_distance;
    short distance;
    unsigned short direction;
} RelativeMeasure;

void measure_relative_vector(FixedPosition *a, FixedPosition *b, RelativeMeasure *out)
{
    short x, y, z;
    int temp;
    unsigned short direction = 4;
    x = out->x = (a->x - b->x) >> 16;
    y = out->y = (a->y - b->y) >> 16;
    z = out->z = (a->z - b->z) >> 16;
    if (x < 0) { x = ~x + 1; direction = 6; }
    if (y < 0) { y = ~y + 1; direction -= 4; }
    if (z < 0) { z = ~z + 1; direction |= 8; }
    if (x < y) { temp = x; x = y; y = temp; ++direction; }
    out->planar_distance = x + (y >> 3) + (y >> 2);
    if (x < z) { temp = x; x = z; z = temp; }
    out->distance = x + 11 * (y >> 5) + (z >> 2);
    out->direction = direction;
}
