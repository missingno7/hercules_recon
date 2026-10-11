typedef struct Entry16 {
    int w[4];
} Entry16;

typedef struct Cursor {
    Entry16 *base;
    int index;
    int count;
    int f0c;
    int f10;
    int f14;
} Cursor;

void init_cursor_44740(Entry16 *base, int index, Cursor *c, int extra)
{
    c->base = base;
    c->index = index;
    c->f0c = 0;
    c->f10 = 0;
    c->f14 = extra;
    c->count = 2;
    base[1] = base[2];
    c->base[c->index + 2] = c->base[c->index + 1];
}
