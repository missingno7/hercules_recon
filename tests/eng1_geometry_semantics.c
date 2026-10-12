/* Actual reconstructed geometry pipeline, controlled copied-host boundaries.
 * Observer callbacks below are fixture-only and never DLL providers. */
#include <stdio.h>
#include <string.h>
#include "../src/eng1/behavioral/mesh_emit.c"
#include "../src/eng1/behavioral/mesh_prepare.c"
#include "../src/eng1/behavioral/primitive_emitters_a.c"
#include "../src/eng1/behavioral/primitive_emitters_b.c"
#include "../src/eng1/behavioral/mesh_storage.c"

EngineInterface g_engine_interface;
ResourceCallbackContext context;
u8 *g_header_pointer_70d30,*g_header_pointer_70d34,*g_header_pointer_70d54,*g_header_pointer_70d50;
u32 g_frame_global_73484,g_mesh_colors_6f4e8[512];
u8 *g_dispatch_frame_current_71ec4;
long g_camera_angle_x_70cd0;
static u32 scratch[160],frame[0x1488/4],pool[40],streams[64],descriptors[8],uvs[6],header[2],lighting[2];
static short trig[4096],input[3],screen[2],screen_history[3][2];
static Eng1MeshVisit visit;
static long determinant;
static short projected_depth;
static int checks,failures,project_calls,matrix_calls,normal_calls;
#define CHECK(c) do {++checks;if(!(c)){++failures;printf("FAIL %d\n",__LINE__);}}while(0)

void __cdecl dispatch_engine_2c920(const void *angles,void *matrix)
{
    CHECK(matrix==(void *)scratch || matrix==(void *)((u8 *)scratch+0x20));
    CHECK(((const short *)angles)[2]==0);++matrix_calls;
}
void __cdecl dispatch_engine_2c940(void *matrix,const void *scale)
{
    CHECK(((const long *)scale)[0]==0x2000 && ((const long *)scale)[1]==0x2000 && ((const long *)scale)[2]==0x2000);
}
void __cdecl dispatch_engine_2c700(const void *matrix) {(void)matrix;}
void __cdecl dispatch_engine_2c710(const void *matrix) {(void)matrix;}
void __cdecl dispatch_engine_2c690(const void *vector)
{memcpy(input,vector,6);}
void __cdecl dispatch_engine_2c6e0(void)
{
    screen[0]=(short)(input[0]+10);screen[1]=(short)(input[1]+20);++project_calls;
}
void __cdecl dispatch_engine_2c6c0(void) {}
void __cdecl dispatch_engine_2c750(void *result)
{
    short *p=(short *)result;p[0]=input[0];p[1]=input[1];p[2]=projected_depth;
}
void __cdecl dispatch_engine_2c760(void *result) {memcpy(result,screen,4);}
void __cdecl dispatch_engine_2c7b0(void *result) {*(short *)result=(short)(projected_depth>>2);}
void __cdecl dispatch_engine_2c670(u32 a,u32 b,u32 c)
{
    memcpy(screen_history[0],&a,4);memcpy(screen_history[1],&b,4);memcpy(screen_history[2],&c,4);
}
void __cdecl dispatch_engine_2c6d0(void) {++normal_calls;}
void __cdecl dispatch_engine_2c740(void *out) {*(long *)out=determinant;}
void __cdecl dispatch_engine_2c770(void *out)
{
    u8 *p=(u8 *)out;memcpy(p+8,screen_history[0],4);memcpy(p+16,screen_history[1],4);memcpy(p+24,screen_history[2],4);
}
void __cdecl dispatch_engine_2c790(void *out)
{
    u8 *p=(u8 *)out;memcpy(p+8,screen_history[0],4);memcpy(p+20,screen_history[1],4);memcpy(p+32,screen_history[2],4);
}

static void reset(int gt4,int faces)
{
    int i;
    memset(&context,0,sizeof context);memset(&visit,0,sizeof visit);
    memset(scratch,0,sizeof scratch);memset(frame,0,sizeof frame);
    memset(pool,0xcd,sizeof pool);memset(streams,0,sizeof streams);
    memset(descriptors,0,sizeof descriptors);memset(header,0,sizeof header);
    memset(trig,0,sizeof trig);
    g_engine_interface.context_004=&context;
    g_engine_interface.opaque_010[0]=(u32)scratch;
    g_engine_interface.opaque_010[1]=(u32)trig;
    g_dispatch_frame_current_71ec4=(u8 *)frame;
    g_header_pointer_70d30=(u8 *)uvs;g_header_pointer_70d34=(u8 *)streams;
    g_header_pointer_70d54=(u8 *)header;g_header_pointer_70d50=(u8 *)lighting;
    lighting[0]=0x04030201;
    g_frame_global_73484=(u32)pool;g_camera_angle_x_70cd0=0;
    for(i=0;i<512;++i)g_mesh_colors_6f4e8[i]=0x00102030;
    visit.header_004=(u8 *)descriptors;visit.packed_020=2;
    visit.cached_040=visit.cached_044=-1;
    visit.limit_027=10;visit.shift_026=0;
    header[0]=gt4?0:0x00080000UL;
    *(short *)streams=2;
    *(short *)((u8 *)streams+8)=0;
    ((u8 *)streams)[10]=4;((u8 *)streams)[11]=(u8)faces;
    streams[3]=0x05000604;streams[4]=0x06000705;streams[5]=0x07000806;streams[6]=0x08000907;
    streams[8]=(1UL<<6)|(2UL<<12)|(3UL<<18);streams[9]=0;
    streams[10]=streams[8];streams[11]=12;
    uvs[0]=0x11112222;uvs[1]=0x33334444;uvs[2]=0x55667788;
    uvs[3]=0xaaaabbbb;uvs[4]=0xccccdddd;uvs[5]=0xeeefffff;
    determinant=1;projected_depth=512;
    project_calls=matrix_calls=normal_calls=0;
    frame[0x521-128]=0x12345678;
}
static void packet_contract(int gt4,int faces)
{
    u8 *p=(u8 *)pool;
    int bytes=gt4?52:40;
    CHECK(g_frame_global_73484==(u32)(p+bytes*faces));
    CHECK(frame[0x521-128]==(u32)(p+bytes*(faces-1)));
    CHECK(*(u32 *)p==0x12345678);
    CHECK(p[7]==(gt4?0x3c:0x2c));
    CHECK(*(u32 *)(p+0xc)==(0x11112222UL|0x78300000UL|((u32)10<<22)));
    CHECK(*(u32 *)(p+(gt4?0x18:0x14))==0x33334444);
    CHECK(*(u16 *)(p+(gt4?0x24:0x1c))==0x7788);
    CHECK(*(u16 *)(p+(gt4?0x30:0x24))==0x5566);
    CHECK(*(u16 *)(p+(gt4?0x26:0x1e))==0xcdcd);
    CHECK(*(u16 *)(p+(gt4?0x32:0x26))==0xcdcd);
    CHECK(p[bytes*faces]==0xcd);
    if(faces==2)CHECK(*(u32 *)(p+bytes)==(u32)p);
    CHECK(project_calls==5);CHECK(normal_calls==faces);CHECK(matrix_calls==1);
}
int main(void)
{
    reset(0,1);CHECK(dispatch_local_2df20(&visit,0)==0);packet_contract(0,1);
    CHECK(pool[1]==0x2c102030);CHECK(((short *)((u8 *)pool+8))[0]==70);
    reset(0,2);dispatch_local_2df20(&visit,0);packet_contract(0,2);
    reset(1,1);dispatch_local_2df20(&visit,0);packet_contract(1,1);
    reset(1,1);*(u16 *)((u8 *)descriptors+14)=0x4000;
    dispatch_local_2df20(&visit,0);packet_contract(1,1);
    CHECK(visit.cached_044==0x4000);CHECK(visit.depth_010==128);
    matrix_calls=project_calls=0;
    dispatch_local_2df20(&visit,0);
    CHECK(matrix_calls==0);CHECK(project_calls==5);
    CHECK(g_frame_global_73484==(u32)((u8 *)pool+104));
    reset(1,1);*(u16 *)((u8 *)descriptors+14)=0x8000;
    dispatch_local_2df20(&visit,0);packet_contract(1,1);
    reset(0,1);*(u16 *)((u8 *)descriptors+14)=0x4000;
    *(short *)((u8 *)streams+8)=1;
    dispatch_local_2df20(&visit,0);packet_contract(0,1);
    CHECK(visit.column_counter_015==1);

    reset(0,1);streams[8]|=0xa0000000UL;streams[9]=0x00000034;
    dispatch_local_2df20(&visit,0);
    CHECK(((u8 *)pool)[7]==0x2e);
    CHECK(*(u16 *)((u8 *)pool+0x1c)==0xff34);
    CHECK(*(u16 *)((u8 *)pool+0x24)==0xff34);
    CHECK(pool[3]==(0x78300000UL|((u32)10<<22)|0xff34UL));
    CHECK(pool[5]==0x009cff34);

    reset(0,1);visit.packed_020|=1;
    dispatch_local_2df20(&visit,0);CHECK(g_frame_global_73484==(u32)pool);CHECK(pool[0]==0xcdcdcdcd);
    reset(0,1);visit.packed_020|=1;determinant=-1;
    dispatch_local_2df20(&visit,0);CHECK(g_frame_global_73484==(u32)((u8 *)pool+40));
    CHECK(((short *)((u8 *)pool+8))[0]==14);
    reset(0,1);projected_depth=128;
    dispatch_local_2df20(&visit,0);CHECK(g_frame_global_73484==(u32)pool);CHECK(project_calls==1 && normal_calls==0);
    reset(0,1);context.unknown_040=1;
    dispatch_local_2df20(&visit,0);CHECK(g_frame_global_73484==(u32)pool);CHECK(project_calls==0 && matrix_calls==0);
    printf("checks=%d failures=%d\n",checks,failures);
    return failures?1:0;
}
