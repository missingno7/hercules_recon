/* Bounded VC5 source comparison against actual archived-resource calls.
 * ENG1 reads the real MAP bytes; the candidate independently projects each
 * native B6F0 input through the shared C reference before packet emission. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "renderer_contract/hercules_projection.h"
#include "renderer_contract/hercules_projection.c"
#include "src/eng1/behavioral/mesh_emit.c"

typedef struct NativePoint {
    unsigned long source_offset;
    short source_xyz[3];
    short input_xyz[3];
    short camera_xyz[3];
    short projected_xy[2];
    struct hercules_projection_config_view config;
} NativePoint;
typedef struct NativePacket {
    unsigned long bucket_index;
    unsigned long original_previous_head;
    unsigned char raw[40];
} NativePacket;
typedef struct NativeCase {
    unsigned long packed_cell;
    int layer, entry_depth, depth, shift, limit, prep_result;
    int point_count, face_count, packet_count;
    unsigned char entry_winding, effective_winding;
    NativePoint points[16];
    NativePacket packets[8];
} NativeCase;
#include "phase20_native_source_case_data.h"

typedef struct NativePrepCase {
    int mesh_call_index;
    int mesh_index;
    int return_value;
    int return_depth;
    unsigned long header_000_map_offset;
    unsigned long header_004_map_offset;
    unsigned long child_map_offset;
    int camera_angle_x;
    long scale_longs[3];
    int expected_b6f0_xyz[3];
    int entry_matrix_words[9];
    int expected_matrix_words[9];
    int entry_translation[3];
    int expected_translation[3];
    int expected_camera[3];
    unsigned int expected_screen[2];
    unsigned int expected_callback_count;
    unsigned int expected_callback_ids[12];
    unsigned char entry_visit[0x74];
    unsigned char return_visit[0x74];
    unsigned char entry_matrix_bytes[0x40];
    unsigned char return_matrix_bytes[0x40];
    unsigned char entry_scratch[0x60];
    unsigned char return_scratch[0x60];
} NativePrepCase;
#include "prep_case_data.h"

/* Compile the existing ENG1 preparation source unchanged behind separate
 * bounded callback spies. Its public callee symbol remains 2E3B0. */
#define dispatch_local_2e3b0 eng1_prep_2e3b0_body
#define dispatch_local_2e7a0 eng1_prep_2e7a0_body
#define dispatch_local_2ef60 eng1_prep_2ef60_body
#define dispatch_engine_2c920 eng1_prep_2c920
#define dispatch_engine_2c940 eng1_prep_2c940
#define dispatch_engine_2c700 eng1_prep_2c700
#define dispatch_engine_2c710 eng1_prep_2c710
#define dispatch_engine_2c690 eng1_prep_2c690
#define dispatch_engine_2c6e0 eng1_prep_2c6e0
#define dispatch_engine_2c6c0 eng1_prep_2c6c0
#define dispatch_engine_2c750 eng1_prep_2c750
#define dispatch_engine_2c760 eng1_prep_2c760
#define dispatch_engine_2c7b0 eng1_prep_2c7b0
#include "src/eng1/behavioral/mesh_prepare.c"
#undef dispatch_local_2e3b0
#undef dispatch_local_2e7a0
#undef dispatch_local_2ef60
#undef dispatch_engine_2c920
#undef dispatch_engine_2c940
#undef dispatch_engine_2c700
#undef dispatch_engine_2c710
#undef dispatch_engine_2c690
#undef dispatch_engine_2c6e0
#undef dispatch_engine_2c6c0
#undef dispatch_engine_2c750
#undef dispatch_engine_2c760
#undef dispatch_engine_2c7b0
#include "src/eng1/behavioral/primitive_emitters_a.c"

EngineInterface g_engine_interface;
ResourceCallbackContext native_context;
u8 *g_header_pointer_70d30,*g_header_pointer_70d34,*g_header_pointer_70d54;
u32 g_frame_global_73484,g_mesh_colors_6f4e8[512];
u8 *g_dispatch_frame_current_71ec4;
long g_camera_angle_x_70cd0;
long g_mesh_scale_5ceb0[3];
long g_mesh_cached_column_6ffb8;

static u8 *map_bytes;
static u8 original_transport[1000];
static u8 point_scratch[0x200];
static short trig_table[4096];
static u8 packet_pool[0x1000];
static u32 ot_words[4096];
static Eng1MeshVisit visit;
static short face_xy[3][2];
static struct hercules_projection_point_view projection_input;
static struct hercules_projection_result_view projection_result;
static struct hercules_projection_config_view projection_config;
static const NativePrepCase *active_prep_case;
static int prep_active, prep_callback_count;
static unsigned int prep_callback_ids[12];
static long face_area;
static int active_case, input_count, output_count, emitted_count;
static int current_input_slot, projection_ready;
static int checks, failures;

#define CHECK(c, message) do { ++checks; if (!(c)) { ++failures; printf("FAIL case=%d %s\n",active_case,message); } } while (0)

static int unavailable_helper(const char *identity)
{
    ++failures;
    printf("UNEXPECTED original-family dependency case=%d helper=%s\n",active_case,identity);
    fflush(stdout);
    exit(90);
    return -2;
}

static void prep_require(int condition, const char *identity)
{
    if (!condition) {
        printf("UNSUPPORTED prep input case=%d helper=%s\n", active_case, identity);
        fflush(stdout);
        exit(90);
    }
}

static void prep_log(unsigned int id)
{
    if (prep_callback_count >= 12)
        prep_require(0, "callback count beyond measured trace");
    prep_callback_ids[prep_callback_count++] = id;
}

static int prep_callback_order_matches(void)
{
    unsigned int i;
    if (prep_callback_count != active_prep_case->expected_callback_count)
        return 0;
    for (i=0;i<prep_callback_count;++i)
        if (prep_callback_ids[i] != active_prep_case->expected_callback_ids[i])
            return 0;
    return 1;
}

static void rebase_prep_visit(unsigned char *raw, const NativePrepCase *p)
{
    unsigned long address;
    address=(unsigned long)(map_bytes+p->header_000_map_offset);
    memcpy(raw,&address,4);
    address=(unsigned long)(map_bytes+p->header_004_map_offset);
    memcpy(raw+4,&address,4);
    address=(unsigned long)(map_bytes+p->child_map_offset);
    memcpy(raw+8,&address,4);
}

static int prep_reported_return;

int __cdecl dispatch_local_2e3b0(Eng1MeshVisit *v)
{
    int result;
    unsigned char expected_visit[0x74];
    int i;
    active_prep_case=&g_native_prep_cases[active_case];
    prep_active=1;
    prep_callback_count=0;
    CHECK(v->depth_010==(short)g_native_cases[active_case].entry_depth,
          "2E3B0 input depth differs from native visit prefix");
    result=eng1_prep_2e3b0_body(v);
    prep_active=0;
    prep_reported_return=result;
    memcpy(expected_visit,active_prep_case->return_visit,sizeof expected_visit);
    rebase_prep_visit(expected_visit,active_prep_case);
    CHECK(result==active_prep_case->return_value,
          "source 2E3B0 return differs from native selected call");
    CHECK(v->depth_010==active_prep_case->return_depth,
          "source depth after B810 overlap and signed SAR2 differs from native");
    CHECK(memcmp((u8 *)v,expected_visit,0x0c)==0,
          "source 2E3B0 pointer prefix differs from native return");
    CHECK(memcmp((u8 *)v+0x0c,expected_visit+0x0c,0x74-0x0c)==0,
          "source 2E3B0 visit writes differ from native return");
    CHECK(memcmp(point_scratch,active_prep_case->return_matrix_bytes,0x40)==0,
          "source prep matrix/translation bytes differ from native");
    CHECK(memcmp(point_scratch+0x40,active_prep_case->return_scratch,0x60)==0,
          "source prep XY scratch differs from native");
    CHECK(prep_callback_order_matches(),
          "source callback sequence differs from pinned native 2E3B0 path");
    for(i=0;i<9;++i)
        CHECK(projection_config.matrix_row_major[i]==
              active_prep_case->expected_matrix_words[i],
              "source post-prep matrix configuration differs from native");
    for(i=0;i<3;++i)
        CHECK(projection_config.translation_xyz[i]==
              active_prep_case->expected_translation[i],
              "source post-B840 translation configuration differs from native");
    return result;
}

/* Test-only aliases bind 2E3B0's callback calls to the already verified
 * projection provider and bounded zero-angle/uniform-scale service spies. */
void __cdecl eng1_prep_2c920(const void *angles_arg, void *matrix_arg)
{
    const short *angles=(const short *)angles_arg;
    short *matrix=(short *)matrix_arg;
    int i;
    prep_log(0xBB00);
    prep_require(matrix_arg==point_scratch,"BB00 matrix address");
    prep_require(angles[0]==0 && angles[1]==0 && angles[2]==0,
                 "BB00 nonzero angles");
    for(i=0;i<9;++i) matrix[i]=(short)((i%4)==0?0x1000:0);
}
void __cdecl eng1_prep_2c940(void *matrix_arg,const void *scale_arg)
{
    short *matrix=(short *)matrix_arg;
    const long *scale=(const long *)scale_arg;
    int i;
    prep_log(0xBC30);
    prep_require(matrix_arg==point_scratch,"BC30 matrix address");
    prep_require(scale==g_mesh_scale_5ceb0,"BC30 scale address");
    prep_require(scale[0]==0x2000&&scale[1]==0x2000&&scale[2]==0x2000,
                 "BC30 unmeasured scale");
    for(i=0;i<9;++i) {
        prep_require(matrix[i]==(short)((i%4)==0?0x1000:0),
                     "BC30 nonidentity input");
        if((i%4)==0) matrix[i]=(short)scale[i/4];
    }
}
void __cdecl eng1_prep_2c700(const void *matrix_arg)
{
    int i;
    const short *matrix=(const short *)matrix_arg;
    prep_log(0xB880);
    prep_require(matrix_arg==point_scratch,"B880 matrix source");
    for(i=0;i<9;++i) projection_config.matrix_row_major[i]=matrix[i];
}
void __cdecl eng1_prep_2c710(const void *matrix_arg)
{
    int i;
    const u8 *matrix=(const u8 *)matrix_arg;
    prep_log(0xB8F0);
    prep_require(matrix_arg==point_scratch,"B8F0 matrix source");
    for(i=0;i<3;++i)
        memcpy(&projection_config.translation_xyz[i],matrix+0x14+i*4,4);
}
void __cdecl eng1_prep_2c690(const void *vector)
{
    const short *xyz=(const short *)vector;
    int i;
    prep_log(0xB6F0);
    prep_require(prep_active && vector==point_scratch+0x40,
                 "prep B6F0 input address");
    for(i=0;i<3;++i) {
        projection_input.xyz[i]=(renderer_i16)xyz[i];
        CHECK(xyz[i]==active_prep_case->expected_b6f0_xyz[i],
              "prep B6F0 input differs from native reads");
    }
}
void __cdecl eng1_prep_2c6e0(void)
{
    prep_log(0xB920);
    prep_log(0xB740);
    hercules_projection_eval(&projection_config,&projection_input,&projection_result);
    CHECK(projection_result.camera_x==active_prep_case->expected_camera[0]&&
          projection_result.camera_y==active_prep_case->expected_camera[1]&&
          projection_result.camera_z==active_prep_case->expected_camera[2],
          "prep B740 result differs from native camera lanes");
    CHECK(projection_result.projected_x==active_prep_case->expected_screen[0]&&
          projection_result.projected_y==active_prep_case->expected_screen[1],
          "prep B920 result differs from native screen lanes");
    projection_ready=1;
}
void __cdecl eng1_prep_2c6c0(void)
{
    prep_log(0xB840);
    prep_require(projection_ready,"prep B840 without projection");
    projection_config.translation_xyz[0]=projection_result.camera_x;
    projection_config.translation_xyz[1]=projection_result.camera_y;
    projection_config.translation_xyz[2]=projection_result.camera_z;
}
void __cdecl eng1_prep_2c760(void *screen)
{
    short *xy=(short *)screen;
    prep_log(0xB720);
    prep_require(projection_ready && screen==point_scratch+0x40,
                 "prep B720 destination/result");
    xy[0]=(short)projection_result.projected_x;
    xy[1]=(short)projection_result.projected_y;
}
void __cdecl eng1_prep_2c750(void *visit_words)
{
    short *out=(short *)visit_words;
    prep_log(0xB810);
    prep_require(projection_ready && visit_words==(u8 *)&visit+0x0c,
                 "prep B810 destination/result");
    out[0]=(short)projection_result.camera_x;
    out[1]=(short)projection_result.camera_y;
    out[2]=(short)projection_result.camera_z;
    projection_ready=0;
}
void __cdecl eng1_prep_2c7b0(void *unused)
{ (void)unused; (void)unavailable_helper("unexpected prep 2C7B0/B870"); }
int __cdecl dispatch_local_2e7a0(Eng1MeshVisit *visit)
{ (void)visit; return unavailable_helper("ENG1+0x2E7A0"); }
void __cdecl dispatch_local_2ef60(Eng1MeshVisit *visit)
{ (void)visit; (void)unavailable_helper("ENG1+0x2EF60"); }
void __cdecl dispatch_local_2ed10(const u32 *s,Eng1MeshVisit *v,int n,int p)
{ (void)s;(void)v;(void)n;(void)p;(void)unavailable_helper("ENG1+0x2ED10"); }
void __cdecl dispatch_local_2f120(const u32 *s,Eng1MeshVisit *v,int n)
{ (void)s;(void)v;(void)n;(void)unavailable_helper("ENG1+0x2F120"); }
void __cdecl dispatch_local_2f340(const u32 *s,Eng1MeshVisit *v,int n)
{ (void)s;(void)v;(void)n;(void)unavailable_helper("ENG1+0x2F340"); }

void __cdecl dispatch_engine_2c690(const void *vector)
{
    const NativeCase *c=&g_native_cases[active_case];
    const short *xyz=(const short *)vector;
    u8 *expected=(u8 *)point_scratch+0x40+input_count*8;
    CHECK(vector==(const void *)expected,"B6F0 source vector address does not match point scratch slot");
    CHECK(input_count<c->point_count,"too many point input callbacks");
    if(input_count<c->point_count) {
        CHECK(xyz[0]==c->points[input_count].input_xyz[0] &&
              xyz[1]==c->points[input_count].input_xyz[1] &&
              xyz[2]==c->points[input_count].input_xyz[2],
              "source 2E600 packed point to B6F0 XYZ differs from original native capture");
        projection_input.xyz[0]=(renderer_i16)xyz[0];
        projection_input.xyz[1]=(renderer_i16)xyz[1];
        projection_input.xyz[2]=(renderer_i16)xyz[2];
    }
    current_input_slot=input_count;
    projection_ready=0;
    ++input_count;
}
void __cdecl dispatch_engine_2c6e0(void)
{
    const NativeCase *c=&g_native_cases[active_case];
    const NativePoint *p;
    int slot=current_input_slot;
    if(slot<0 || slot>=c->point_count) {
        (void)unavailable_helper("expected B6F0-before-B920 point slot");
        return;
    }
    p=&c->points[slot];
    {
        int i;
        for(i=0;i<9;++i)
            CHECK(projection_config.matrix_row_major[i]==p->config.matrix_row_major[i],
                  "prep-derived matrix differs from native point input config");
        for(i=0;i<3;++i)
            CHECK(projection_config.translation_xyz[i]==p->config.translation_xyz[i],
                  "prep-derived translation differs from native point input config");
        CHECK(projection_config.center_x==p->config.center_x &&
              projection_config.center_y==p->config.center_y &&
              projection_config.focal==p->config.focal,
              "controlled center/focal input differs from native point config");
    }
    hercules_projection_eval(&projection_config,&projection_input,&projection_result);
    CHECK(projection_result.camera_x==p->camera_xyz[0] &&
          projection_result.camera_y==p->camera_xyz[1] &&
          projection_result.camera_z==p->camera_xyz[2],
          "C-derived camera XYZ differs from original B740 output");
    CHECK(projection_result.projected_x== (renderer_u16)p->projected_xy[0] &&
          projection_result.projected_y== (renderer_u16)p->projected_xy[1],
          "C-derived projected XY differs from original B920 output");
    projection_ready=1;
}
void __cdecl dispatch_engine_2c760(void *projected_xy)
{
    const NativeCase *c=&g_native_cases[active_case];
    ptrdiff_t offset=(u8 *)projected_xy-((u8 *)point_scratch+0x40);
    int slot=(int)(offset/8);
    short *xy=(short *)projected_xy;
    CHECK(offset>=0 && (offset&7)==0 && slot<c->point_count,
          "B720 destination does not identify a measured scratch slot");
    CHECK(slot==output_count,"B720 source/projection callback order changed");
    CHECK(projection_ready,"B720 ran without an independently computed B920 result");
    if(slot>=0 && slot<c->point_count) {
        CHECK(xy[0]==c->points[slot].input_xyz[0] &&
              xy[1]==c->points[slot].input_xyz[1],
              "B720 input scratch no longer contains the B6F0 source words");
        xy[0]=(short)projection_result.projected_x;
        xy[1]=(short)projection_result.projected_y;
    }
    projection_ready=0;
    ++output_count;
}
void __cdecl dispatch_engine_2c670(u32 a,u32 b,u32 c)
{
    memcpy(face_xy[0],&a,4);memcpy(face_xy[1],&b,4);memcpy(face_xy[2],&c,4);
}
void __cdecl dispatch_engine_2c6d0(void)
{
    long x1=(long)face_xy[1][0]-(long)face_xy[0][0];
    long y1=(long)face_xy[1][1]-(long)face_xy[0][1];
    long x2=(long)face_xy[2][0]-(long)face_xy[0][0];
    long y2=(long)face_xy[2][1]-(long)face_xy[0][1];
    face_area=x1*y2-x2*y1;
}
void __cdecl dispatch_engine_2c740(void *out)
{
    *(long *)out=face_area;
}
void __cdecl dispatch_engine_2c770(void *out)
{
    u8 *p=(u8 *)out;
    memcpy(p+8,face_xy[0],4);memcpy(p+16,face_xy[1],4);memcpy(p+24,face_xy[2],4);
    ++emitted_count;
}
void __cdecl dispatch_engine_2c790(void *out)
{ (void)out;(void)unavailable_helper("ENG1+0x2C790 GT4 packet callback"); }

static int read_map(const char *path)
{
    FILE *f=fopen(path,"rb");
    size_t n;
    if(!f) return 0;
    map_bytes=(u8 *)malloc(1304576);
    if(!map_bytes) { fclose(f);return 0; }
    n=fread(map_bytes,1,1304576,f);fclose(f);
    return n==1304576;
}

static int read_transport(const char *path)
{
    FILE *f=fopen(path,"rb");
    size_t n;
    if(!f) return 0;
    n=fread(original_transport,1,sizeof original_transport,f);
    fclose(f);
    return n==sizeof original_transport;
}

static int read_trig(const char *path)
{
    FILE *f=fopen(path,"rb");
    size_t n;
    if(!f) return 0;
    n=fread(trig_table,1,sizeof trig_table,f);
    fclose(f);
    return n==sizeof trig_table;
}

static void reset_case(int index)
{
    int i;
    const NativeCase *c=&g_native_cases[index];
    const NativePrepCase *p=&g_native_prep_cases[index];
    Eng1MeshVisit *v;
    active_case=index;
    /* The complete measured transport is copied first. Only the controlled
     * context and the host scratch pointer's fixture backing are substituted. */
    memcpy(&g_engine_interface,original_transport,sizeof g_engine_interface);
    memset(&native_context,0,sizeof native_context);
    memset(point_scratch,0,sizeof point_scratch);
    memcpy(point_scratch,p->entry_matrix_bytes,sizeof p->entry_matrix_bytes);
    memcpy(point_scratch+0x40,p->entry_scratch,sizeof p->entry_scratch);
    /* Native Unicorn backing starts zeroed; mirror that bounded initial state
     * for packet bytes which the original writer leaves untouched. */
    memset(packet_pool,0,sizeof packet_pool);
    memset(ot_words,0,sizeof ot_words);
    memset(g_mesh_colors_6f4e8,0,sizeof g_mesh_colors_6f4e8);
    g_engine_interface.context_004=&native_context;
    /* Original word4 is BA25C0, the observed matrix/point scratch base. Map
     * that address to this bounded candidate-owned scratch buffer. */
    g_engine_interface.opaque_010[0]=(u32)point_scratch;
    g_engine_interface.opaque_010[1]=(u32)trig_table;
    g_camera_angle_x_70cd0=p->camera_angle_x;
    for(i=0;i<3;++i) g_mesh_scale_5ceb0[i]=p->scale_longs[i];
    memset(&projection_config,0,sizeof projection_config);
    for(i=0;i<9;++i)
        projection_config.matrix_row_major[i]=(renderer_i16)p->entry_matrix_words[i];
    for(i=0;i<3;++i)
        projection_config.translation_xyz[i]=p->entry_translation[i];
    projection_config.center_x=NATIVE_CAMERA_CENTER_X;
    projection_config.center_y=NATIVE_CAMERA_CENTER_Y;
    projection_config.focal=NATIVE_CAMERA_FOCAL;
    g_header_pointer_70d30=map_bytes+0xB1B4;
    g_header_pointer_70d34=map_bytes+0x44;
    g_header_pointer_70d54=map_bytes+0x26D80;
    g_dispatch_frame_current_71ec4=(u8 *)ot_words;
    g_frame_global_73484=(u32)packet_pool;
    v=&visit;
    memcpy(v,p->entry_visit,sizeof *v);
    rebase_prep_visit((u8 *)v,p);
    CHECK(v->packed_020==(u32)c->packed_cell,
          "prep prefix packed cell differs from emitted source case");
    CHECK(v->depth_010==(short)c->entry_depth,
          "prep prefix depth differs from emitted source case");
    for(i=0;i<c->point_count;++i) {
        const NativePoint *p=&c->points[i];
        signed char source_x, source_z;
        short source_y;
        if(p->source_offset+6>1304576UL) {
            ++failures;
            printf("UNEXPECTED source point outside archived map case=%d index=%d offset=%lu\n",
                   index,i,p->source_offset);
            continue;
        }
        memcpy(&source_x,map_bytes+p->source_offset,1);
        memcpy(&source_y,map_bytes+p->source_offset+1,2);
        memcpy(&source_z,map_bytes+p->source_offset+3,1);
        CHECK(source_x==p->source_xyz[0] && source_y==p->source_xyz[1] &&
              source_z==p->source_xyz[2],
              "recorded source XYZ differs from actual archived point bytes");
    }
    for(i=0;i<c->packet_count;++i) {
        unsigned long bucket=c->packets[i].bucket_index;
        if(bucket>=4096) { ++failures; continue; }
        ot_words[bucket]=(u32)c->packets[i].original_previous_head;
    }
    input_count=output_count=emitted_count=0;
    current_input_slot=-1;
    projection_ready=0;
    prep_active=0;
    prep_callback_count=0;
    prep_reported_return=0;
}

static void compare_case(int index)
{
    const NativeCase *c=&g_native_cases[index];
    u8 *start=packet_pool;
    Eng1MeshVisit *v=&visit;
    int i,j;
    reset_case(index);
    CHECK(dispatch_local_2df20(v,c->layer)==0,"2DF20 source candidate returned nonzero");
    CHECK(input_count==c->point_count,"B6F0 point input count differs from original");
    CHECK(output_count==c->point_count,"B720 point output count differs from original");
    CHECK(emitted_count==c->packet_count,"source face culling emitted a different number of packets");
    CHECK(v->winding_024==c->effective_winding,"source winding normalization differs from original 2DF33 write");
    CHECK(v->depth_010==c->depth,"source 2E3B0 result depth differs from original return boundary");
    CHECK(g_frame_global_73484==(u32)(start+c->packet_count*40),"packet allocation cursor/40-byte stride differs");
    for(i=0;i<c->packet_count;++i) {
        u8 *actual=start+i*40;
        const NativePacket *expected=&c->packets[i];
        unsigned long bucket=expected->bucket_index;
        CHECK(bucket<4096,"native OT bucket exceeds fixture backing");
        if(bucket<4096) CHECK(ot_words[bucket]==(u32)actual,"OT head does not link to emitted packet");
        for(j=0;j<40;++j) {
            ++checks;
            if(actual[j]!=expected->raw[j]) {
                ++failures;
                printf("FAIL case=%d packet=%d byte=%d source=%02x native=%02x\n",
                       index,i,j,(unsigned)actual[j],(unsigned)expected->raw[j]);
            }
        }
    }
    printf("case=%d points=%d faces=%d packets=%d entry_winding=%u effective_winding=%u\n",
           index,c->point_count,c->face_count,c->packet_count,
           (unsigned)c->entry_winding,(unsigned)c->effective_winding);
}

int main(int argc,char **argv)
{
    int i;
    if(argc!=4 || !read_map(argv[1]) || !read_transport(argv[2]) ||
       !read_trig(argv[3])) {
        printf("FAIL archived map, original host transport, or trig input\n");return 2;
    }
    for(i=0;i<NATIVE_CASE_COUNT;++i) compare_case(i);
    printf("source_mesh_cases=%d checks=%d failures=%d\n",NATIVE_CASE_COUNT,checks,failures);
    return failures?1:0;
}
