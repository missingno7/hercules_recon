/* Reuse of existing behavioral candidate for native2a810. Historical
 * candidate codegen mismatch remains unchanged; finite native tests support semantics. */
#include "query_2a170.h"
int __cdecl point_in_box_2a810(unsigned long x, unsigned long y, unsigned long z, const Query2a170Bounds *b)
{
    short hx = (short)(x >> 16);
    short hy = (short)(y >> 16);
    short hz = (short)(z >> 16);
    int r;
    r = (hz >= b->mn[2]);
    r &= (hx <= b->mx[0]);
    r &= (hy >= b->mn[1]);
    r &= (hz <= b->mx[2]);
    r &= (hy <= b->mx[1]);
    r &= (hx >= b->mn[0]);
    return r;
}
