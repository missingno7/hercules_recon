/* PC ENG12DF20: native pointer-linked FT4/GT4 emission from decoded mesh.
 * Genuine prep/emitter helpers remain external, supplied by disjoint workers. */
#include "mesh_visit.h"
extern u8 *g_header_pointer_70d30,*g_header_pointer_70d34,*g_header_pointer_70d54;
extern u32 g_frame_global_73484;
extern u32 g_mesh_colors_6f4e8[];
extern int __cdecl dispatch_local_2e3b0(Eng1MeshVisit *);
extern int __cdecl dispatch_local_2e7a0(Eng1MeshVisit *);
extern void __cdecl dispatch_local_2ef60(Eng1MeshVisit *);
extern void __cdecl dispatch_local_2e600(const u32 *,Eng1MeshVisit *,int);
extern void __cdecl dispatch_local_2e6c0(const u32 *,Eng1MeshVisit *,int);
extern void __cdecl dispatch_local_2ea50(const u32 *,Eng1MeshVisit *,int);
extern void __cdecl dispatch_local_2ed10(const u32 *,Eng1MeshVisit *,int,int);
extern void __cdecl dispatch_local_2f120(const u32 *,Eng1MeshVisit *,int);
extern void __cdecl dispatch_local_2f340(const u32 *,Eng1MeshVisit *,int);
extern void __cdecl dispatch_engine_2c670(u32,u32,u32);
extern void __cdecl dispatch_engine_2c6d0(void);
extern void __cdecl dispatch_engine_2c740(void *);
extern void __cdecl dispatch_engine_2c770(void *);
extern void __cdecl dispatch_engine_2c790(void *);

int __cdecl dispatch_local_2df20(Eng1MeshVisit *visit,int layer)
{
    u8 *stream,*lookup,*child,*faces,*vertices,*packet,*uv;
    u32 packed,face,uv_word,color,shade,xy[4],rgb[4];
    u32 *head,*ot;
    long index,segment,selector,normal,depth;
    int count,face_count,mode,prep,flags;
    u32 remaining;

    packed=visit->packed_020;
    visit->winding_024=(u8)packed & 1;
    index=(packed>>1)&0x3ff;
    lookup=g_header_pointer_70d34;
    segment=(long)*(short *)(lookup+(index-1)*4)+index;
    stream=lookup+(segment-1)*4;
    selector=(long)*(short *)stream;
    count=stream[2];
    stream+=2;
    mode=0;
    if(count==0) return 0;
    face_count=stream[1];
    stream+=2;
    if(g_engine_interface.context_004->unknown_040!=0) return 0;

    lookup=g_header_pointer_70d54+(((packed>>11)&0x1fff)*4);
    visit->cell_02a=*(u16 *)lookup;
    child=visit->header_004+((u32)visit->cell_02a<<4);
    visit->child_008=child;
    visit->variant_025=(u8)((*(u16 *)(child+0xe)>>14)&3);
    visit->flags_02c=*(u16 *)(lookup+2);
    if(visit->variant_025) {
        if(selector==1) {
            dispatch_local_2ef60(visit);
            dispatch_local_2f120((const u32 *)stream,visit,count);
            ++visit->column_counter_015;
        } else {
            if(dispatch_local_2e7a0(visit)==-1) return 0;
            if((u8)visit->flags_02c & 8)
                dispatch_local_2ea50((const u32 *)stream,visit,count);
            else {
                dispatch_local_2f340((const u32 *)stream,visit,count);
                mode=1;
            }
        }
    } else {
        prep=dispatch_local_2e3b0(visit);
        if(prep==-1) return 0;
        if((u8)visit->flags_02c & 8) {
            if(prep==0) dispatch_local_2e600((const u32 *)stream,visit,count);
            else dispatch_local_2e6c0((const u32 *)stream,visit,count);
        } else {
            dispatch_local_2ed10((const u32 *)stream,visit,count,prep);
            mode=1;
        }
    }

    depth=(long)visit->depth_010;
    ot=(u32 *)(g_dispatch_frame_current_71ec4+(0x521-depth)*4);
    faces=stream+count*4+4;
    packet=(u8 *)g_frame_global_73484;
    uv=g_header_pointer_70d30;
    vertices=(u8 *)g_engine_interface.opaque_010[0]+0x40;
    depth=(depth-0x40)>>(visit->shift_026&31);
    if(depth<0) depth=0;
    else if(depth>visit->limit_027) depth=visit->limit_027;
    shade=((u32)depth<<22)+0x78300000UL;
    if(mode!=0 && mode!=1) return 0;

    if(mode==0) {
        flags=(visit->flags_02c>>6)&0xff;
        flags+=(visit->flags_02c&7)<<6;
        color=g_mesh_colors_6f4e8[flags]|0x2c000000UL;
    }
    remaining=(u32)face_count;
    /* Native uses a post-body DEC/JNZ; a zero face count is not guarded. The
     * caller's encoded nonempty face precondition remains explicit. */
    do {
        face=*(u32 *)faces;
        xy[0]=*(u32 *)(vertices+(face&0x3f)*8);
        xy[1]=*(u32 *)(vertices+((face>>6)&0x3f)*8);
        xy[2]=*(u32 *)(vertices+((face>>12)&0x3f)*8);
        dispatch_engine_2c670(xy[0],xy[1],xy[2]);
        dispatch_engine_2c6d0();
        flags=(int)((long)face>>24);
        if(!mode) xy[3]=*(u32 *)(vertices+((face>>18)&0x3f)*8);
        dispatch_engine_2c740(&normal);
        if(visit->winding_024 ? normal>0 : normal<0) goto next_face;
        if(mode) {
            dispatch_engine_2c790(packet);
            xy[3]=*(u32 *)(vertices+((face>>18)&0x3f)*8);
            *(u32 *)(packet+0x2c)=xy[3];
            rgb[0]=*(u32 *)(vertices+(face&0x3f)*8+4);
            rgb[1]=*(u32 *)(vertices+((face>>6)&0x3f)*8+4);
            rgb[2]=*(u32 *)(vertices+((face>>12)&0x3f)*8+4);
            rgb[3]=*(u32 *)(vertices+((face>>18)&0x3f)*8+4);
            *(u32 *)(packet+4)=rgb[0]|((u32)(((flags&0x20)|0x3c0)>>4)<<24);
            *(u32 *)(packet+0x10)=rgb[1];
            *(u32 *)(packet+0x1c)=rgb[2];
            *(u32 *)(packet+0x28)=rgb[3];
        } else {
            dispatch_engine_2c770(packet);
            *(u32 *)(packet+0x20)=xy[3];
            *(u32 *)(packet+4)=color|((u32)(flags&0x20)<<20);
        }
        uv_word=*(u32 *)(faces+4);
        head=(u32 *)((u8 *)ot-((uv_word>>22)&0x3fc));
        *(u32 *)packet=*head;
        *head=(u32)packet;
        if(flags&0x80) {
            uv_word=(uv_word&0xffffUL)|0xff00UL;
            *(u16 *)(packet+(mode?0x24:0x1c))=(u16)uv_word;
            *(u16 *)(packet+(mode?0x30:0x24))=(u16)uv_word;
            *(u32 *)(packet+0xc)=shade|uv_word;
            *(u32 *)(packet+(mode?0x18:0x14))=0x9c0000UL|uv_word;
        } else {
            lookup=uv+(uv_word&0xffffUL);
            *(u32 *)(packet+0xc)=*(u32 *)lookup|shade;
            *(u32 *)(packet+(mode?0x18:0x14))=*(u32 *)(lookup+4);
            uv_word=*(u32 *)(lookup+8);
            *(u16 *)(packet+(mode?0x24:0x1c))=(u16)uv_word;
            *(u16 *)(packet+(mode?0x30:0x24))=(u16)(uv_word>>16);
        }
        packet+=mode?52:40;
next_face:
        faces+=8;
    } while(--remaining!=0);
    g_frame_global_73484=(u32)packet;
    return 0;
}
