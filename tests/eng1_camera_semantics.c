/* Source-only fixture storage and controlled observers; never linked into DLL. */
#include <stdio.h>
#include <string.h>
#include "../src/eng1/camera.c"

EngineInterface g_engine_interface;
u32 g_frame_global_734f4, g_frame_global_70ce8, g_frame_global_70cec;
u8 g_mode_table_value_70d00, g_group_count_70ece;
short g_element_width_70d14;
u8 *g_header_pointer_70d54, *g_header_pointer_70d58;
u8 *g_rebuilt_row_pointers_70dcc[16];
u16 g_rebuilt_row_offsets_70e0c[32];
long g_render_transform_70c60[8];
long g_camera_x_70ca0, g_camera_y_70ca4, g_camera_z_70ca8;
long g_camera_grid_x_70cac, g_camera_grid_y_70cb0, g_camera_grid_z_70cb4;
long g_camera_offset_x_70cb8, g_camera_offset_y_70cbc, g_camera_offset_z_70cc0;
long g_camera_prior_x_70cc4, g_camera_prior_y_70cc8;
long g_camera_angle_x_70cd0, g_camera_angle_y_70cd4;
long g_camera_center_x_70cdc, g_camera_center_y_70ce0, g_camera_focal_70ce4;
u8 g_camera_iterations_70cfd, g_camera_bounds_id_70cff;
short g_camera_adjust_x_70d02, g_camera_adjust_y_70d04, g_camera_adjust_z_70d06;
short g_camera_depth_low_70d0c, g_camera_depth_high_70d0e;
u16 g_camera_flags_70d10;
long g_camera_z_offset_70d18, g_camera_z_base_70d1c;
u16 g_camera_threshold_x_5e058[8], g_camera_threshold_y_5e080[8], g_camera_threshold_z_5e0a8[8];

static u8 actor_a[0x100], actor_b[0x100];
static short trig[4096];
static long tile_words[16];
static u16 header_words[8];
static long descriptor[8];
static int checks, failures, bounds_calls, observer_calls, bounds_mutates;
static int callback_mutates_focal;
static long observed_x, observed_y, observed_focal;
#define CHECK(c) do { ++checks; if (!(c)) { ++failures; printf("FAIL %d\n",__LINE__); } } while(0)

int __cdecl dispatch_local_20ea0(void *out, int id, int mode, long x)
{
    short *bounds=(short *)out;
    ++bounds_calls;
    CHECK(id == -2); CHECK(mode == 24); CHECK(x == 12);
    bounds[0]=120; bounds[1]=0; bounds[2]=240; bounds[3]=0;
    /* The original helper writes all seven WORDs for mode 24 as well. */
    bounds[4]=0x4444; bounds[5]=0x5555; bounds[6]=0x6666;
    if (bounds_mutates) {
        g_camera_x_70ca0=777;
        g_frame_global_734f4=(u32)actor_b;
    }
    return 1;
}
void __cdecl dispatch_engine_2c9d0(u32 x,u32 y)
{
    CHECK(observer_calls==0); ++observer_calls;
    observed_x=(long)x; observed_y=(long)y;
    if (callback_mutates_focal) g_camera_focal_70ce4=999;
}
void __cdecl dispatch_engine_2c9f0(u32 focal)
{
    CHECK(observer_calls==1); ++observer_calls; observed_focal=(long)focal;
}
static void coords(u8 *actor,long x,long y,long z)
{
    *(long *)actor=shift32(x,16); *(long *)(actor+4)=shift32(y,16);
    *(long *)(actor+8)=shift32(z,16); actor[0x94]=1;
}
static void reset(void)
{
    memset(actor_a,0,sizeof actor_a); memset(actor_b,0,sizeof actor_b);
    memset(trig,0,sizeof trig); memset(descriptor,0,sizeof descriptor);
    memset(tile_words,0,sizeof tile_words); memset(header_words,0,sizeof header_words);
    memset(g_rebuilt_row_offsets_70e0c,0,sizeof g_rebuilt_row_offsets_70e0c);
    memset(g_render_transform_70c60,0,sizeof g_render_transform_70c60);
    g_frame_global_734f4=(u32)actor_a;
    g_frame_global_70ce8=0; g_frame_global_70cec=600;
    g_camera_x_70ca0=g_camera_y_70ca4=g_camera_z_70ca8=0;
    g_camera_prior_x_70cc4=g_camera_prior_y_70cc8=0;
    g_camera_offset_x_70cb8=g_camera_offset_y_70cbc=g_camera_offset_z_70cc0=0;
    g_camera_angle_x_70cd0=g_camera_angle_y_70cd4=0;
    g_camera_center_x_70cdc=g_camera_center_y_70ce0=g_camera_focal_70ce4=0;
    g_camera_z_offset_70d18=g_camera_z_base_70d1c=0;
    g_camera_flags_70d10=0; g_camera_iterations_70cfd=1;
    g_camera_bounds_id_70cff=0xfe;
    g_camera_adjust_x_70d02=g_camera_adjust_y_70d04=g_camera_adjust_z_70d06=0;
    g_camera_depth_low_70d0c=-100; g_camera_depth_high_70d0e=100;
    g_element_width_70d14=4; g_group_count_70ece=1; g_mode_table_value_70d00=0;
    g_camera_threshold_x_5e058[0]=g_camera_threshold_y_5e080[0]=g_camera_threshold_z_5e0a8[0]=0;
    g_camera_threshold_x_5e058[1]=g_camera_threshold_y_5e080[1]=g_camera_threshold_z_5e0a8[1]=1;
    g_camera_threshold_x_5e058[2]=g_camera_threshold_y_5e080[2]=g_camera_threshold_z_5e0a8[2]=0xffff;
    g_engine_interface.opaque_010[1]=(u32)trig;
    g_rebuilt_row_pointers_70dcc[0]=(u8 *)tile_words;
    g_rebuilt_row_offsets_70e0c[0]=4;
    g_header_pointer_70d54=(u8 *)header_words;
    g_header_pointer_70d58=(u8 *)descriptor;
    bounds_calls=observer_calls=bounds_mutates=callback_mutates_focal=0;
}
int main(void)
{
    reset(); coords(actor_a,1000,200,300); g_camera_x_70ca0=12;
    g_camera_y_70ca4=24; g_camera_z_offset_70d18=10; g_camera_focal_70ce4=100;
    dispatch_local_2fb70();
    CHECK(g_camera_prior_x_70cc4==12); CHECK(g_camera_prior_y_70cc8==24);
    CHECK(g_camera_x_70ca0==600); CHECK(g_camera_y_70ca4==192);
    CHECK(g_camera_z_70ca8==210); CHECK(g_camera_grid_x_70cac==9);
    CHECK(g_camera_grid_y_70cb0==3); CHECK(g_camera_grid_z_70cb4==3);

    reset(); coords(actor_a,-32768,-32768,-32768); g_camera_iterations_70cfd=0x80;
    g_camera_x_70ca0=-65; g_camera_y_70ca4=-1; g_camera_z_70ca8=-64;
    dispatch_local_2fb70();
    CHECK(g_camera_x_70ca0==-65); CHECK(g_camera_y_70ca4==-1); CHECK(g_camera_z_70ca8==-64);
    CHECK(g_camera_grid_x_70cac==-2); CHECK(g_camera_grid_y_70cb0==-1); CHECK(g_camera_grid_z_70cb4==-1);

    reset(); coords(actor_a,100,200,300); g_camera_flags_70d10=0x300;
    g_camera_x_70ca0=3; g_camera_y_70ca4=4;
    dispatch_local_2fb70(); CHECK(g_camera_x_70ca0==3); CHECK(g_camera_y_70ca4==4); CHECK(g_camera_z_70ca8==300);

    reset(); coords(actor_a,4094,128,0); g_camera_flags_70d10=3; g_camera_x_70ca0=128;
    dispatch_local_2fb70(); CHECK(g_camera_x_70ca0==4222);

    reset(); coords(actor_a,0,50,0); coords(actor_b,0,250,0);
    g_camera_flags_70d10=0x180; g_camera_x_70ca0=12; bounds_mutates=1;
    dispatch_local_2fb70(); CHECK(bounds_calls==1); CHECK(g_camera_prior_x_70cc4==12);
    CHECK(g_camera_x_70ca0==777); CHECK(g_camera_grid_x_70cac==12); CHECK(g_camera_y_70ca4==240);
    reset(); coords(actor_a,0,50,0); g_camera_flags_70d10=0x80; g_camera_x_70ca0=12;
    dispatch_local_2fb70(); CHECK(g_camera_y_70ca4==120);

    reset(); coords(actor_a,100,200,100); g_camera_flags_70d10=0x10c;
    g_camera_y_70ca4=150; g_camera_center_y_70ce0=180; g_camera_iterations_70cfd=2;
    /* Native post-read advancement selects step two, so both Y passes run. */
    dispatch_local_2fb70(); CHECK(g_camera_z_70ca8==4); CHECK(g_camera_y_70ca4==154);
    reset(); coords(actor_a,100,200,100); g_camera_flags_70d10=0x10c;
    g_camera_y_70ca4=150; g_camera_center_y_70ce0=180; g_camera_iterations_70cfd=2;
    g_camera_threshold_z_5e0a8[2]=2; g_camera_threshold_z_5e0a8[3]=0xffff;
    dispatch_local_2fb70(); CHECK(g_camera_z_70ca8==6); CHECK(g_camera_y_70ca4==152);

    reset(); coords(actor_a,0,64,0); descriptor[0]=1000; descriptor[1]=2000;
    *(short *)((u8 *)descriptor+14)=(short)0xc000;
    g_camera_x_70ca0=64; g_camera_y_70ca4=300; g_camera_offset_y_70cbc=shift32(-7,16);
    g_camera_angle_x_70cd0=50; g_camera_angle_y_70cd4=123;
    g_camera_center_x_70cdc=160; g_camera_center_y_70ce0=120; g_camera_focal_70ce4=256;
    g_render_transform_70c60[5]=0x12345678; callback_mutates_focal=1;
    dispatch_local_2f840();
    CHECK(g_render_transform_70c60[0]==1000); CHECK(g_render_transform_70c60[1]==293);
    CHECK(g_render_transform_70c60[2]==2000); CHECK(g_render_transform_70c60[3]==50);
    CHECK(g_render_transform_70c60[4]==123); CHECK(g_render_transform_70c60[5]==0x12345678);
    CHECK(g_render_transform_70c60[6]==0); CHECK(g_render_transform_70c60[7]==6);
    CHECK(observed_x==160 && observed_y==120); CHECK(observed_focal==999); CHECK(observer_calls==2);

    reset(); coords(actor_a,0,64,0); descriptor[0]=1000; descriptor[1]=2000;
    *(short *)((u8 *)descriptor+14)=(short)0xc000;
    { int i; for(i=0;i<4096;++i) trig[i]=64; }
    g_camera_x_70ca0=65; g_camera_y_70ca4=300;
    g_camera_offset_x_70cb8=64L*1024L; g_camera_offset_z_70cc0=32L*1024L;
    g_camera_angle_y_70cd4=123; g_mode_table_value_70d00=1;
    dispatch_local_2f840();
    CHECK(g_render_transform_70c60[0]==1096); CHECK(g_render_transform_70c60[2]==2160);
    CHECK(g_render_transform_70c60[4]==91); CHECK(g_render_transform_70c60[1]==300);

    reset(); coords(actor_a,0,64,0); tile_words[5]=1L<<11; header_words[2]=1;
    descriptor[0]=1000; descriptor[1]=2000; descriptor[4]=111; descriptor[5]=222;
    *(short *)((u8 *)descriptor+30)=(short)0xc000; g_camera_x_70ca0=64;
    dispatch_local_2f840();
    CHECK(g_render_transform_70c60[0]==111); CHECK(g_render_transform_70c60[2]==222);
    printf("checks=%d failures=%d\n",checks,failures);
    return failures ? 1 : 0;
}
