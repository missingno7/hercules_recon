/* ENG1 26730, ordinary-C recovery of non-level-6 camera smoothing.
 * The level-6 color branch remains explicitly unsupported. No new storage. */
#include "../../../calibration/engine_interface.h"
#include "actor_pipeline_storage.h"
#include "incomplete.h"
extern EngineInterface g_engine_interface;
extern long g_camera_z_offset_70d18, g_camera_angle_x_70cd0;
extern long g_camera_angle_y_70cd4, g_camera_offset_y_70cbc;
extern short g_camera_adjust_y_70d04;
#define W(r) (g_actor_pipeline_storage_71440.word_cells[(0x##r-0x71440)/2])
#define D(r) (g_actor_pipeline_storage_71440.dword_cells[(0x##r-0x71440)/4])

static long average_words(unsigned count_offset,unsigned seed_offset,int first)
{
    short sum=(short)g_actor_pipeline_storage_71440.word_cells[seed_offset/2];
    short count=(short)g_actor_pipeline_storage_71440.word_cells[count_offset/2];
    int index=first;
    while(index<count) {
        sum=(short)(sum+g_actor_pipeline_storage_71440.word_cells[count_offset/2+index+1]);
        ++index;
    }
    return (long)sum/(index+1);
}
static long approach(long current,long target,int shift,int angular)
{
    short delta=(short)((unsigned long)current-(unsigned long)target);
    long step;
    if(delta==0)return current;
    if(angular) {
        if(delta>0x7ff)delta=(short)(0x1000-delta);
        if(delta< -0x7ff)delta=(short)(delta+0x1000);
    }
    if(delta>= -16 && delta<=16) {
        step=((long)delta>>1);
        if(delta<0)step=(delta&1)-step;
        else step= -step-(delta&1);
    } else step= -((long)delta>>shift);
    return (long)((unsigned long)current+(unsigned long)step);
}
void __cdecl eng1_call_26730(void)
{
    int level,index,count;
    unsigned long sum,magnitude;
    long delta,step;
    unsigned char *context=(unsigned char *)g_engine_interface.context_004;
    level=context[0];
    if(level==6)
        eng1_incomplete_behavior(0x26730UL,"level-6 color path not recovered; no camera state changed");
    if((short)W(71ca0)>47 || (short)W(71740)>735 || (short)W(71720)>751 || (long)D(71680)>415)
        eng1_incomplete_behavior(0x26730UL,"camera lists exceed measured pipeline storage prefix");
    g_camera_z_offset_70d18=approach(g_camera_z_offset_70d18,
        average_words(0x860,0x408,0),3,0);
    g_camera_angle_x_70cd0=approach(g_camera_angle_x_70cd0,
        average_words(0x300,0x5d4,0),4,1);
    /* Native yaw skips list element one and still divides by two for count<=1. */
    g_camera_angle_y_70cd4=approach(g_camera_angle_y_70cd4,
        average_words(0x2e0,0x5d2,1),4,1);
    if(context[0]==1) {
        short dy=(short)(g_camera_adjust_y_70d04-(short)W(7183c));
        if(dy<0)g_camera_adjust_y_70d04=(short)W(7183c);
        else if(dy>0)g_camera_adjust_y_70d04=(short)(g_camera_adjust_y_70d04-(dy<=8?1:4));
    }
    sum=D(717b4);count=(long)D(71680);index=0;
    while(index<count)sum+=g_actor_pipeline_storage_71440.dword_cells[0x240/4+(++index)];
    delta=(long)((unsigned long)g_camera_offset_y_70cbc-(unsigned long)((long)sum/(index+1)));
    magnitude=delta<0?0UL-(unsigned long)delta:(unsigned long)delta;
    /* The original signed comparison leaves abs(INT_MIN) unchanged. */
    if((long)magnitude>0x4000) {
        if(delta>= -0x560000L && delta<=0x80000L) {
            step=delta>>1;
            if(delta<0)step=(long)((unsigned long)delta&0x8000UL)-step;
            else step= -step-(long)((unsigned long)delta&0x8000UL);
        } else step= -(delta>>3);
        g_camera_offset_y_70cbc=(long)((unsigned long)g_camera_offset_y_70cbc+(unsigned long)step);
    }
}
