/* Native 2aea0: count-prefixed live worklist; its capacity is not inferred. */
#include "actor_pipeline_storage_views.h"
typedef struct Position { long x,y,z; } Position;

void *__cdecl eng1_native_code_target_002aea0(Position *point,int depth_limit,
    int planar_limit)
{
    void *best=0;
    long x=point->x>>16;
    long y=point->y>>16;
    long z=point->z>>16;
    long count=(long)g_actor_worklist_71580[0];
    if(count>0) {
        int best_distance=planar_limit;
        unsigned long *cursor=g_actor_worklist_71580;
        do {
            Position *actor=(Position *)cursor[1];
            short dx,dy,dz;
            int temp,estimate;
            ++cursor;
            dz=z-(actor->z>>16);
            if(dz<0)dz=~dz+1;
            if(dz<depth_limit) {
                dx=x-(actor->x>>16);
                dy=y-(actor->y>>16);
                if(dx<0)dx=~dx+1;
                if(dy<0)dy=~dy+1;
                if(dx<dy){temp=dx;dx=dy;dy=temp;}
                estimate=dx+(dy>>3)+(dy>>2);
                if(estimate<best_distance) {
                    best_distance=estimate;
                    best=actor;
                }
            }
        } while(--count);
    }
    return best;
}
