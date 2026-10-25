/* PC ENG1:2da90 traversal only. The actual2df20 mesh emitter remains external.
 * This record is an observed caller/callee view, not an original struct name. */
#include <string.h>
#include "mesh_visit.h"

extern long g_render_transform_70c60[8];
extern long g_camera_x_70ca0, g_camera_z_70ca8, g_camera_z_base_70d1c;
extern short g_element_width_70d14;
extern u8 g_group_count_70ece, g_mode_table_value_70d00;
extern u8 *g_header_pointer_70d54, *g_header_pointer_70d58;
extern u8 *g_rebuilt_row_pointers_70dcc[];
extern u16 g_rebuilt_row_offsets_70e0c[], g_group_cumulative_70e4c[];
extern u32 g_frame_global_70d24;
extern u8 g_frame_config_5df08[];
extern long g_render_identity_5ce28[8];
extern short g_render_visible_bounds_70e6c[][3];
extern int __cdecl dispatch_local_2df20(Eng1MeshVisit *, int);

static long mesh_add(long a,long b) {return (long)((u32)a+(u32)b);}
static long mesh_sub(long a,long b) {return (long)((u32)a-(u32)b);}
static long mesh_mul(long a,long b) {return (long)((u32)a*(u32)b);}
static long mesh_shift(long a,int n) {return (long)((u32)a<<n);}
/* The original CDQ + AND-bias + SAR implements signed division toward0. */
static long mesh_div_pow2(long a,int n)
{return mesh_add(a,a<0 ? (1L<<n)-1 : 0) >> n;}

#include "data_views.h"

void __cdecl dispatch_local_2da90(void)
{
    Eng1MeshVisit visit;
    long remaining, layer, step, distance, starts[2], ends[2];
    long width, layer_start, layer_count, n, end, center_column;
    long row, column, original_row_seen, side;
    u8 *matrix, *row_pointer;
    int i, level;
    u8 mode;

    matrix=(u8 *)g_engine_interface.opaque_010[0];
    memcpy(matrix,g_render_identity_5ce28,32);
    memcpy(matrix+0x20,g_render_identity_5ce28,32);
    memcpy(visit.transform_054,g_render_transform_70c60,32);
    i=0;
    while(i<(int)(s8)g_group_count_70ece) {
        g_render_visible_bounds_70e6c[i][0]=(short)0xff81;
        ++i;
    }
    visit.header_004=g_header_pointer_70d58;
    visit.header_000=g_header_pointer_70d54;
    visit.cached_044=visit.cached_040=-1;
    visit.tick_034=mesh_shift((long)g_engine_interface.context_004->tick_038,8)>>8;
    level=(int)(s8)g_engine_interface.context_004->unknown_000[0];
    visit.limit_027=g_frame_config_5df08[level*28+8];
    mode=g_mode_table_value_70d00;
    visit.mode_028=mode;
    level=(int)(s8)g_engine_interface.context_004->unknown_000[0];
    visit.shift_026=g_frame_config_5df08[level*28+9];
    width=(long)g_element_width_70d14;
    if(mode==0) {
        remaining=6;
        layer=mesh_add(mesh_div_pow2(mesh_add(g_camera_z_70ca8,0x180),7),5);
        if(layer<6) layer=6;
        step=0; distance=10;
        starts[0]=starts[1]=0; ends[0]=ends[1]=24;
    } else {
        remaining=2; layer=2; step=2; distance=16;
        starts[0]=starts[1]=0; ends[0]=ends[1]=32;
    }

    while(remaining-- != 0) {
        --layer;
        if(layer>=(int)(s8)g_group_count_70ece) continue;
        if(layer<0) break;
        if(mode==2) {
            if(layer==0) starts[0]=starts[1]=0;
        } else if(mode==1) {
            step=-2;
            starts[0]=mesh_shift(mesh_sub(8,layer),1);
            starts[1]=mesh_sub(17,mesh_shift(layer,1));
        }
        visit.cached_044=visit.cached_040=-1;
        visit.layer_y_038=mesh_sub((long)(short)g_group_cumulative_70e4c[layer],g_camera_z_base_70d1c);
        visit.layer_z_03c=mesh_sub(g_camera_z_base_70d1c,g_camera_z_70ca8);
        layer_count=(long)(short)g_rebuilt_row_offsets_70e0c[layer];
        layer_start=(long)(short)g_rebuilt_row_offsets_70e0c[layer+16];

        for(side=0;side<2;++side) {
            n=mesh_add(starts[side],side);
            end=mesh_add(ends[side],side);
            visit.row_stop_017=0;
            visit.side_029=(u8)side;
            while(n<end) {
                original_row_seen=(long)(s8)visit.row_stop_017;
                row=mesh_add(side ? mesh_sub(0,n) : n,visit.transform_054[6]);
                visit.row_018=row;
                if(visit.mode_028==0) {
                    if((u32)mesh_sub(row,layer_start)>=(u32)layer_count) break;
                } else {
                    row=mesh_add(row & 0x3f,2);
                    visit.row_018=row;
                }
                row_pointer=g_rebuilt_row_pointers_70dcc[layer]+mesh_mul(mesh_mul(mesh_sub(row,layer_start),width),4);
                visit.column_stop_016=0;
                visit.column_counter_015=0;
                center_column=visit.transform_054[7];
                while((long)visit.column_counter_015<distance) {
                    column=mesh_sub(center_column,(long)visit.column_counter_015);
                    visit.column_01c=column;
                    if((u32)mesh_add(column,step)>=(u32)width) break;
                    visit.packed_020=*(u32 *)(row_pointer+mesh_mul(column,4));
                    if(visit.packed_020 & 0x7feUL) {
                        if(dispatch_local_2df20(&visit,(int)layer)!=0) goto finished;
                        if(visit.column_stop_016 && visit.column_counter_015>2) break;
                    } else if(visit.packed_020 & 0x80000000UL) break;
                    ++visit.column_counter_015;
                }
                visit.column_stop_016=0;
                visit.column_counter_015=1;
                while((long)visit.column_counter_015<distance) {
                    column=mesh_add(center_column,(long)visit.column_counter_015);
                    visit.column_01c=column;
                    if((u32)mesh_add(column,step)>=(u32)width) break;
                    visit.packed_020=*(u32 *)(row_pointer+mesh_mul(column,4));
                    if(visit.packed_020 & 0x7feUL) {
                        if(dispatch_local_2df20(&visit,(int)layer)!=0) goto finished;
                        if(visit.column_stop_016 && visit.column_counter_015>1) break;
                    } else if(visit.packed_020 & 1UL) break;
                    ++visit.column_counter_015;
                }
                if(original_row_seen!=0 && visit.mode_028==0) break;
                ++n;
            }
            ends[side]=mesh_add(n,2);
            if(side)
                g_render_visible_bounds_70e6c[layer][0]=(short)mesh_sub(mesh_div_pow2(g_camera_x_70ca0,6),n);
            else
                g_render_visible_bounds_70e6c[layer][1]=(short)mesh_add(mesh_div_pow2(g_camera_x_70ca0,6),n);
        }
        mode=visit.mode_028;
    }
finished:
    if(g_frame_global_70d24)
        ((EngineNoArgCallback)g_frame_global_70d24)();
}
