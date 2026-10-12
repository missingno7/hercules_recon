#include "worklist_factory_types.h"
/* Semantic ordinary-C recovery of PC26040/26480. The actual shipped-map
 * producer has one root group; unknown table indices fail explicitly rather
 * than extending the measured g_kind_info[1] owner with fabricated rows. */
#include "forest_actor_init.h"
#include "resource_runtime_data.h"
#include "initial_spatial_roots.h"
#include "spatial_read_guard.h"
#include "incomplete.h"
#include "../../../calibration/engine_interface.h"

extern unsigned long g_configuration_dword_71398;
extern unsigned short g_pipeline_word_713ae;
extern long g_camera_x_70ca0, g_camera_center_x_70cdc;
extern unsigned long g_frame_global_734f4;
extern void *g_pipeline_actor_7200c;
extern void *__cdecl engine_pool_allocate(void **,unsigned long,unsigned long);
extern void __cdecl eng1_native_code_target_028290(unsigned long);

static ResourceKindInfo *kind_row(unsigned int index)
{
    if(index>=1) {
        eng1_incomplete_behavior(0x6fee8UL,"26040/26480 kind-info index exceeds the one shipped-map root-group prefix");
        return 0;
    }
    return &g_kind_info[index];
}
static const Eng1ForestPlacement8 *placements(const unsigned char *block)
{
    return (const Eng1ForestPlacement8 *)(block+10+*(const short *)(block+10));
}
static const Eng1SpatialType10 *spatial_row(const Eng1SpatialType10 *base,unsigned int index)
{
    eng1_guard_spatial_read(base,index,10,10,"26040/spatial-row");
    return base+index;
}
static void populate(Eng1ForestRecord20 *out,const Eng1ForestPlacement8 *p,
                     const Eng1SpatialType10 *type)
{
    /* Native zero-extends placement coordinates and sign-extends offsets.
     * Unsigned shift preserves x86 low32-bit wrapping without signed UB. */
    out->x=(long)((unsigned long)((long)p->x+type->x)<<16);
    out->y=(long)((unsigned long)((long)p->y+type->y)<<16);
    out->z=(long)((unsigned long)((long)p->z+type->z)<<16);
    out->flags=type->flags;
    out->state=0;
    out->kind=type->kind;
    /* Native leaves the20-byte header and final WORD+0x12 unchanged. */
}

void __cdecl eng1_call_26040(void)
{
    const Eng1SpatialType10 *types=(const Eng1SpatialType10 *)g_configuration_dword_71398;
    unsigned int group,index;
    unsigned long special_2018=0,special_2019=0;
    unsigned short groups;
    ResourceKindInfo *row;
    const unsigned char *block;
    const Eng1ForestPlacement8 *input;
    Eng1ForestRecord20 *out;
    const Eng1SpatialType10 *type;
    void *allocated;
    if(types==0)return;

    groups=g_pipeline_word_713ae;
    if(groups==0) {
        groups=(unsigned short)(short)g_root_count_70ed0;
        g_pipeline_word_713ae=groups;
        /* Guard the actual table before any root-group/kind slot read. */
        if(groups>1) {kind_row(groups-1);return;}
        for(group=0;group<groups;++group) {
            row=kind_row(group);if(!row)return;
            block=g_root_group_rows_70d5c[group];
            row->owner=0;
            row->count=*(const unsigned short *)(block+8);
            if(row->count==0)continue;
            allocated=engine_pool_allocate(&row->owner,(row->count+1)*20UL,0);
            if(allocated==0) {
                groups=g_pipeline_word_713ae;
                if(groups>1) {kind_row(groups-1);return;}
                continue;
            }
            /* Both block pointer and count are live after allocation. */
            block=g_root_group_rows_70d5c[group];input=placements(block);
            out=(Eng1ForestRecord20 *)((unsigned char *)allocated+20);
            for(index=0;(long)index<(long)row->count;++index) {
                type=spatial_row(types,input[index].spatial_index);
                populate(out+index,input+index,type);
                if(out[index].kind==0x2018U)++special_2018;
                if(out[index].kind==0x2019U)++special_2019;
            }
            /* Native26169 reloads cached groups after the host callback and
             * population, including the failed-allocation path. */
            groups=g_pipeline_word_713ae;
            if(groups>1) {kind_row(groups-1);return;}
        }
        *(unsigned short *)&g_engine_interface.context_004->unknown_000[0x16]=(unsigned short)special_2019;
        *(unsigned short *)&g_engine_interface.context_004->unknown_000[0x1a]=(unsigned short)special_2018;
        return;
    }

    if(groups>1) {kind_row(groups-1);return;}
    for(group=0;group<groups;++group) {
        row=kind_row(group);if(!row)return;
        if(row->owner==0)continue;
        block=g_root_group_rows_70d5c[group];input=placements(block);
        out=(Eng1ForestRecord20 *)((unsigned char *)row->owner+20);
        for(index=0;(long)index<(long)row->count;++index) {
            type=spatial_row(types,input[index].spatial_index);
            if(type->flags&0x0200U)populate(out+index,input+index,type);
        }
    }
}

static long add32(long a,long b){return (long)((unsigned long)a+(unsigned long)b);}
static long sub32(long a,long b){return (long)((unsigned long)a-(unsigned long)b);}
static void submit(Eng1ForestRecord20 *record)
{
    unsigned short type=record->kind&0x7000U;
    /* Type is captured before a callback can mutate the record. */
    if(type==0)eng1_call_270c0(record,0);
    else if(type==0x4000U)eng1_call_27720(record,0);
    else if(type==0x2000U)eng1_call_27c40(record,0);
}

void __cdecl eng1_call_26480(void)
{
    unsigned int group=(unsigned short)(short)g_active_category_70cfe;
    ResourceKindInfo *row=kind_row(group);
    Eng1ForestRecord20 *record;
    unsigned int index;
    unsigned short flags;
    long camera_delta,x;
    if(!row || row->owner==0)return;
    camera_delta=sub32(g_camera_x_70ca0,g_camera_center_x_70cdc);
    record=(Eng1ForestRecord20 *)((unsigned char *)row->owner+20);
    for(index=0;(long)index<(long)row->count;++index,++record) {
        if(g_frame_global_734f4==(unsigned long)g_pipeline_actor_7200c &&
           (record->flags&0x0400U))continue;
        x=sub32(record->x>>16,camera_delta);
        flags=record->flags;
        if(flags&0x0040U) {
            if(add32(x,500)<0 || sub32(x,500)>320)continue;
            if(add32(x,32)>0 && sub32(x,32)<320)continue;
            if(flags&0xc000U)continue;
            submit(record);
        } else {
            if(add32(x,256)<0 || sub32(x,256)>320)continue;
            if(add32(x,32)>0 && sub32(x,32)<320) {
                if(sub32(x,120)<0 || sub32(x,180)>0 || (flags&0xc000U))continue;
                if((record->kind&0x7000U)==0x1000U)
                    eng1_native_code_target_028290((unsigned long)record->kind);
            } else {
                if(flags&0xc000U)continue;
                submit(record);
            }
        }
        /* Owner is cached; count is deliberately reloaded after each call. */
    }
}
