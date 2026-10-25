/* Source-only traversal observer; this file is excluded from production. */
#include <stdio.h>
#include "../src/eng1/mesh_traversal.c"
EngineInterface g_engine_interface;
ResourceCallbackContext context;
long g_render_transform_70c60[8];
long g_camera_x_70ca0,g_camera_z_70ca8,g_camera_z_base_70d1c;
short g_element_width_70d14;
u8 g_group_count_70ece,g_mode_table_value_70d00;
u8 *g_header_pointer_70d54,*g_header_pointer_70d58;
u8 *g_rebuilt_row_pointers_70dcc[16];
u16 g_rebuilt_row_offsets_70e0c[32];
ResourceGroupState g_resource_group_state_70e4c;
u32 g_frame_global_70d24;
Eng1ProfileRow28 eng1_profile_rows_5df08[12];
long g_render_identity_5ce28[8]={0x2000,0,0x2000,0,0x2000,0,0,0};

static long matrix[18],tiles[16];
static int checks,failures,visits,finished,return_at,mark_row;
#define CHECK(c) do {++checks;if(!(c)){++failures;printf("FAIL %d\n",__LINE__);}}while(0)
static void completion(void) {++finished;}
int __cdecl dispatch_local_2df20(Eng1MeshVisit *visit,int layer)
{
    CHECK(layer==0); CHECK(visit->packed_020==2);
    CHECK(visit->header_000==g_header_pointer_70d54 && visit->header_004==g_header_pointer_70d58);
    CHECK(visit->mode_028==0 && visit->side_029==0);
    CHECK(visit->tick_034==(long)0xff876543UL);
    CHECK(visit->layer_y_038==63 && visit->layer_z_03c==257);
    CHECK(visit->limit_027==33 && visit->shift_026==4);
    CHECK(visit->row_018>=0 && visit->row_018<4);
    CHECK(visit->column_01c>=0 && visit->column_01c<4);
    ++visits;
    if(mark_row) visit->row_stop_017=1;
    return return_at && visits==return_at;
}
static void reset(void)
{
    int i;
    visits=finished=return_at=mark_row=0;
    for(i=0;i<18;++i) matrix[i]=0x11223344;
    for(i=0;i<16;++i) tiles[i]=2;
    memset(&context,0,sizeof context); g_engine_interface.context_004=&context;
    context.tick_038=0xff876543UL;
    g_engine_interface.opaque_010[0]=(u32)(matrix+1);
    memset(g_render_transform_70c60,0,sizeof g_render_transform_70c60);
    g_render_transform_70c60[7]=1;
    g_camera_x_70ca0=64;g_camera_z_70ca8=-256;g_camera_z_base_70d1c=1;
    g_element_width_70d14=4;g_group_count_70ece=1;g_mode_table_value_70d00=0;
    g_header_pointer_70d54=(u8 *)tiles;g_header_pointer_70d58=(u8 *)matrix;
    g_rebuilt_row_pointers_70dcc[0]=(u8 *)tiles;
    g_rebuilt_row_offsets_70e0c[0]=4;g_rebuilt_row_offsets_70e0c[16]=0;
    g_group_cumulative_70e4c[0]=64;
    g_frame_global_70d24=(u32)completion;
    g_frame_config_5df08[8]=33;g_frame_config_5df08[9]=4;
    g_render_visible_bounds_70e6c[0][0]=123;
    g_render_visible_bounds_70e6c[0][1]=456;
    g_render_visible_bounds_70e6c[0][2]=789;
}
int main(void)
{
    int i;
    reset();dispatch_local_2da90();
    CHECK(visits==16);CHECK(finished==1);
    CHECK(g_render_visible_bounds_70e6c[0][0]==0);
    CHECK(g_render_visible_bounds_70e6c[0][1]==5);
    CHECK(g_render_visible_bounds_70e6c[0][2]==789);
    CHECK(matrix[0]==0x11223344 && matrix[17]==0x11223344);
    for(i=0;i<16;++i) CHECK(matrix[i+1]==g_render_identity_5ce28[i%8]);
    reset();mark_row=1;dispatch_local_2da90();
    CHECK(visits==8);CHECK(finished==1);CHECK(g_render_visible_bounds_70e6c[0][1]==2);
    reset();return_at=3;dispatch_local_2da90();
    CHECK(visits==3);CHECK(finished==1);
    CHECK(g_render_visible_bounds_70e6c[0][0]==(short)0xff81);
    CHECK(g_render_visible_bounds_70e6c[0][1]==456);
    reset();g_group_count_70ece=0;dispatch_local_2da90();
    CHECK(visits==0);CHECK(finished==1);CHECK(g_render_visible_bounds_70e6c[0][0]==123);
    printf("checks=%d failures=%d\n",checks,failures);
    return failures?1:0;
}
