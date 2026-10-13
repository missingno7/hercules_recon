/* Bounded VC5 source comparison against actual archived-resource calls.
 * ENG1 reads the real MAP bytes; the candidate independently projects each
 * native B6F0 input through the shared C reference before packet emission. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "renderer_contract/hercules_projection.h"
#include "renderer_contract/hercules_projection.c"
#include "src/eng1/behavioral/mesh_emit.c"
#include "src/eng1/behavioral/primitive_emitters_a.c"

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
#include "native_source_case_data.h"

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
static u8 packet_pool[0x1000];
static u32 ot_words[4096];
static Eng1MeshVisit visit;
static short face_xy[3][2];
static struct hercules_projection_point_view projection_input;
static struct hercules_projection_result_view projection_result;
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

int __cdecl dispatch_local_2e3b0(Eng1MeshVisit *visit)
{
    const NativeCase *c=&g_native_cases[active_case];
    /* Explicit data-only prep boundary: the native real-resource traversal
     * selected 2E600, proving result 0; its post-prep depth is captured from
     * the same native mesh call. Matrix/translation setup is not reconstructed
     * by this fixture and is not silently represented as an empty helper. */
    CHECK(visit->depth_010==(short)c->entry_depth,
          "2E3B0 entry depth differs from original visit input");
    visit->depth_010=(short)c->depth;
    CHECK(visit->cell_02a==0,"captured 2E3B0 cell identity changed");
    CHECK(visit->variant_025==0,"captured 2E3B0 variant changed");
    CHECK(c->prep_result==0,"captured 2E3B0 branch result is not the 2E600 path");
    return c->prep_result;
}
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
    projection_input.xyz[0]=(renderer_i16)p->input_xyz[0];
    projection_input.xyz[1]=(renderer_i16)p->input_xyz[1];
    projection_input.xyz[2]=(renderer_i16)p->input_xyz[2];
    hercules_projection_eval(&p->config,&projection_input,&projection_result);
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

static void reset_case(int index)
{
    int i;
    const NativeCase *c=&g_native_cases[index];
    Eng1MeshVisit *v;
    active_case=index;
    /* The complete measured transport is copied first. Only the controlled
     * context and the host scratch pointer's fixture backing are substituted. */
    memcpy(&g_engine_interface,original_transport,sizeof g_engine_interface);
    memset(&native_context,0,sizeof native_context);
    memset(point_scratch,0,sizeof point_scratch);
    /* Native Unicorn backing starts zeroed; mirror that bounded initial state
     * for packet bytes which the original writer leaves untouched. */
    memset(packet_pool,0,sizeof packet_pool);
    memset(ot_words,0,sizeof ot_words);
    memset(g_mesh_colors_6f4e8,0,sizeof g_mesh_colors_6f4e8);
    g_engine_interface.context_004=&native_context;
    /* Original word4 is BA25C0, the observed matrix/point scratch base. Map
     * that address to this bounded candidate-owned scratch buffer. */
    g_engine_interface.opaque_010[0]=(u32)point_scratch;
    g_header_pointer_70d30=map_bytes+0xB1B4;
    g_header_pointer_70d34=map_bytes+0x44;
    g_header_pointer_70d54=map_bytes+0x26D80;
    g_dispatch_frame_current_71ec4=(u8 *)ot_words;
    g_frame_global_73484=(u32)packet_pool;
    v=&visit;
    memset(v,0,sizeof *v);
    v->header_004=map_bytes+0x26D30;
    v->packed_020=(u32)c->packed_cell;
    v->depth_010=(short)c->entry_depth;
    v->winding_024=c->entry_winding;
    v->shift_026=(u8)c->shift;
    v->limit_027=(u8)c->limit;
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
    if(argc!=3 || !read_map(argv[1]) || !read_transport(argv[2])) {
        printf("FAIL archived map or original host transport input\n");return 2;
    }
    for(i=0;i<NATIVE_CASE_COUNT;++i) compare_case(i);
    printf("source_mesh_cases=%d checks=%d failures=%d\n",NATIVE_CASE_COUNT,checks,failures);
    return failures?1:0;
}
