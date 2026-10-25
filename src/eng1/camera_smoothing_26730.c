/* ENG1 26730, bounded camera and level-6 color update.
 * Equal real colors with no list retain an explicit stack-dependent guard. */
#include "../shared/engine_interface.h"
#include "actor_pipeline_storage.h"
#include "initial_spatial_roots.h"
#include "incomplete.h"
extern EngineInterface g_engine_interface;
extern long g_camera_z_offset_70d18, g_camera_angle_x_70cd0;
extern long g_camera_angle_y_70cd4, g_camera_offset_y_70cbc;
extern short g_camera_adjust_y_70d04;
#define W(r) (g_actor_pipeline_storage_71440.word_cells[(0x##r-0x71440)/2])
#define D(r) (g_actor_pipeline_storage_71440.dword_cells[(0x##r-0x71440)/4])

extern unsigned char g_level6_color_target_5a7b0[6];
extern unsigned char g_level6_color_current_5a7b8[6];
extern void __cdecl eng1_color_grid_update_5a60(int,int,int,int,int,int);
static const unsigned char color_order[6]={0,2,1,3,5,4};

static void validate_level6(void)
{
    long count=(long)D(71ae0);
    unsigned i,changed=0;
    unsigned char *target=g_level6_color_pending_6fea0;
    if(count<0 || count>135)
        eng1_incomplete_behavior(0x26730UL,"level-6 color list exceeds measured storage prefix");
    if(target && target!=g_level6_color_target_5a7b0 &&
       (unsigned long)target < (unsigned long)g_level6_color_target_5a7b0+6UL &&
       (unsigned long)target+6UL > (unsigned long)g_level6_color_target_5a7b0)
        eng1_incomplete_behavior(0x26730UL,"level-6 pending target partially overlaps target storage");
    if(!target)target=g_level6_color_target_5a7b0;
    for(i=0;i<6;++i)changed|=target[i]!=g_level6_color_current_5a7b8[i];
    if(count==0 && !changed)
        eng1_incomplete_behavior(0x26730UL,"level-6 equal colors with empty list depend on native stack lanes");
}
static void update_level6(void)
{
    int values[6];unsigned i,j;long count=(long)D(71ae0);
    unsigned char *pending=g_level6_color_pending_6fea0;
    if(pending) {
        for(i=0;i<6;++i)g_level6_color_target_5a7b0[color_order[i]]=pending[color_order[i]];
        g_level6_color_pending_6fea0=0;
    }
    if(count!=0) {
        for(i=0;i<6;++i)values[i]=0;
        for(j=0;j<(unsigned)count;++j) {
            unsigned char *color=(unsigned char *)g_actor_pipeline_storage_71440.dword_cells[0x6a4/4+j];
            for(i=0;i<6;++i)values[i]+=color[color_order[i]];
        }
        for(i=0;i<6;++i)values[i]/=count;
    } else {
        for(i=0;i<6;++i) {
            int delta=(int)g_level6_color_current_5a7b8[i]-g_level6_color_target_5a7b0[i];
            int step=delta>>1;
            if(delta<0)step=(delta&1)-step;else step= -step-(delta&1);
            values[i]=(int)g_level6_color_current_5a7b8[i]+step;
        }
        /* Preserve the observed output order; all values were read first. */
        g_level6_color_current_5a7b8[0]=(unsigned char)values[0];
        g_level6_color_current_5a7b8[2]=(unsigned char)values[2];
        g_level6_color_current_5a7b8[4]=(unsigned char)values[4];
        g_level6_color_current_5a7b8[5]=(unsigned char)values[5];
        g_level6_color_current_5a7b8[1]=(unsigned char)values[1];
        g_level6_color_current_5a7b8[3]=(unsigned char)values[3];
        /* Native physical B/G order differs from the six cdecl arguments. */
        {int tmp;tmp=values[1];values[1]=values[2];values[2]=tmp;
         tmp=values[4];values[4]=values[5];values[5]=tmp;}
    }
    eng1_color_grid_update_5a60(values[0],values[1],values[2],values[3],values[4],values[5]);
}

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
    if(level==6)validate_level6();
    if((level!=6 && ((short)W(71ca0)>47 || (short)W(71740)>735 || (short)W(71720)>751)) || (long)D(71680)>415)
        eng1_incomplete_behavior(0x26730UL,"camera lists exceed measured pipeline storage prefix");
    if(level!=6) {
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
    if(context[0]==6)update_level6();
}
