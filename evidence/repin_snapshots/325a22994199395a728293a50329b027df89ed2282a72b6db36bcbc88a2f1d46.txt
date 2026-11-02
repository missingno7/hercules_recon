/* ENG1 2a950/2a9e0/2aab0/2ae50, recovered from their original PC bodies.
 * Private views name only measured fields; no historical type/TU identity.
 * The setter's shared failure path and per-axis copies, and the screen
 * helper's Y-before-X zero assignments, reproduce measured VC5 scheduling. */
extern "C" {
typedef struct BoundsWords { short x0,y0,z0,x1,y1,z1; } BoundsWords;
typedef struct ScreenPrefix {
    unsigned char pad000[12];
    short x,y;
    unsigned char pad010[15];
    unsigned char flags;
} ScreenPrefix;
typedef struct ActorBoundsPrefix {
    unsigned char pad000[0x88];
    BoundsWords bounds;
} ActorBoundsPrefix;
typedef char bounds_is_12[sizeof(BoundsWords)==12?1:-1];
typedef char screen_is_32[sizeof(ScreenPrefix)==32?1:-1];
typedef char actor_bounds_is_94[sizeof(ActorBoundsPrefix)==0x94?1:-1];
extern unsigned short g_pipeline_word_71376;
extern int __cdecl extent_producer_30e40(void *,void *,unsigned long);
extern int __cdecl eng1_320d0(void *);

int __cdecl eng1_native_code_target_002a950(ActorBoundsPrefix *actor,
    unsigned short mode)
{
    BoundsWords b;
    if (extent_producer_30e40(actor,&b,mode) && (b.x1 | b.y1)) {
        actor->bounds.x0=b.x0;
        actor->bounds.x1=b.x1;
        actor->bounds.y0=b.y0;
        actor->bounds.y1=b.y1;
        actor->bounds.z0=b.z0;
        actor->bounds.z1=b.z1;
        return 1;
    }
    return 0;
}

int __cdecl eng1_native_code_target_002a9e0(ScreenPrefix *actor,
    unsigned short margin)
{
    if (g_pipeline_word_71376==0 && (actor->flags & 3)==1)
        return ((actor->x+margin)<0) | ((actor->x-margin)>319);
    return 0;
}

int __cdecl eng1_native_code_target_002aab0(ScreenPrefix *actor,
    unsigned short x_margin,unsigned short y_margin)
{
    if (g_pipeline_word_71376==0 && (actor->flags & 8))
        return ((actor->y-y_margin)>239) | ((actor->y+y_margin)<0) |
               ((actor->x+x_margin)<0) | ((actor->x-x_margin)>319);
    return 0;
}

void __cdecl eng1_native_code_target_002ae50(ScreenPrefix *actor)
{
    int x,y;
    if (eng1_320d0(actor)==-1) {
        actor->flags |= 8;
        return;
    }
    if (g_pipeline_word_71376) { y=0; x=0; }
    else { x=actor->x; y=actor->y; }
    if (x>=-32 && y>=-32 && x<=352 && y<=272)
        actor->flags |= 2;
}
}
