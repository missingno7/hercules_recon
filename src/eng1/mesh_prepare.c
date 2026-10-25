/* Actual PC transform/projection prep family2E3B0/2E7A0/2EF60. */
#include "mesh_visit.h"
extern long g_camera_angle_x_70cd0;
extern long g_mesh_scale_5ceb0[3];
extern long g_mesh_cached_column_6ffb8;
extern void __cdecl dispatch_engine_2c920(const void *,void *);
extern void __cdecl dispatch_engine_2c940(void *,const void *);
extern void __cdecl dispatch_engine_2c700(const void *);
extern void __cdecl dispatch_engine_2c710(const void *);
extern void __cdecl dispatch_engine_2c690(const void *);
extern void __cdecl dispatch_engine_2c6e0(void);
extern void __cdecl dispatch_engine_2c6c0(void);
extern void __cdecl dispatch_engine_2c750(void *);
extern void __cdecl dispatch_engine_2c760(void *);
extern void __cdecl dispatch_engine_2c7b0(void *);

static long prep_add(long a,long b){return (long)((u32)a+(u32)b);}
static long prep_sub(long a,long b){return (long)((u32)a-(u32)b);}
static long prep_mul(long a,long b){return (long)((u32)a*(u32)b);}
static long prep_shift(long a,int n){return (long)((u32)a<<n);}
static long prep_neg(long a){return (long)(0UL-(u32)a);}

int __cdecl dispatch_local_2e3b0(Eng1MeshVisit *visit)
{
    u8 *matrix,*scratch,*child;
    short *trig,angles[3];
    long theta,sine,cosine,x,z,turn,column,row;
    matrix=(u8 *)g_engine_interface.opaque_010[0];
    scratch=matrix+0x40;
    child=visit->child_008;
    if((long)visit->cell_02a!=visit->cached_040) {
        visit->cached_040=(long)visit->cell_02a;
        theta=visit->transform_054[4];
        x=prep_sub(*(long *)child,visit->transform_054[0])>>6;
        z=prep_sub(*(long *)(child+4),visit->transform_054[2])>>6;
        trig=(short *)g_engine_interface.opaque_010[1];
        cosine=(long)trig[prep_sub(theta,0x400)&0xfff];
        sine=(long)trig[theta&0xfff];
        *(long *)(matrix+0x14)=(prep_sub(prep_mul(x,sine),prep_mul(z,cosine))>>17)&~1L;
        *(long *)(matrix+0x1c)=prep_shift(prep_add(visit->layer_z_03c,
            prep_add(prep_mul(z,sine),prep_mul(x,cosine))>>18),1);
        turn=prep_neg(prep_add(*(long *)(child+8)&0xfff,visit->transform_054[4]));
        angles[0]=(short)visit->transform_054[3];
        angles[1]=(short)turn;
        angles[2]=0;
        if((turn|g_camera_angle_x_70cd0)!=0) angles[1]=(short)prep_sub(turn,1);
        dispatch_engine_2c920(angles,matrix);
        dispatch_engine_2c940(matrix,g_mesh_scale_5ceb0);
        dispatch_engine_2c700(matrix);
        visit->cached_044=turn;
    } else turn=visit->cached_044;
    dispatch_engine_2c710(matrix);
    column=prep_sub(prep_shift(visit->column_01c,6),visit->transform_054[1]);
    row=prep_shift(prep_sub(visit->row_018,*(u16 *)(child+0xe)&0x3fff),6);
    *(short *)(scratch+2)=(short)column;
    *(short *)(scratch+4)=(short)visit->layer_y_038;
    if((turn|g_camera_angle_x_70cd0)==0) {
        *(short *)scratch=(short)row;
        dispatch_engine_2c690(scratch);
        dispatch_engine_2c6e0();
        dispatch_engine_2c6c0();
        dispatch_engine_2c760(scratch);
        if(*(u16 *)(scratch+2)>240) visit->column_stop_016=1;
        if(*(u16 *)scratch>320) visit->row_stop_017=1;
        dispatch_engine_2c750(visit->projected_00c);
        if(visit->depth_010<0xc0) return -1;
        visit->depth_010=(short)(visit->depth_010>>2);
        return 0;
    }
    *(short *)scratch=(short)prep_add(row,0x20);
    dispatch_engine_2c690(scratch);
    dispatch_engine_2c6e0();
    dispatch_engine_2c760(scratch);
    if(visit->side_029==0) {
        if(*(short *)scratch>320) visit->row_stop_017=1;
    } else if(*(short *)scratch<0) visit->row_stop_017=1;
    dispatch_engine_2c750(visit->projected_00c);
    if(visit->depth_010<0x100) return -1;
    visit->depth_010=(short)(visit->depth_010>>2);
    visit->row_local_048=row;
    visit->column_local_04c=column;
    return 1;
}

/* Shared actual cylindrical setup.2EF60 leaves cached translation untouched
 * on a hit;2E7A0 updates only its Y translation and calls the real setter. */
static long prepare_cylinder(Eng1MeshVisit *visit,int update_cached_y)
{
    u8 *matrix,*child,*reference;
    short *trig,angles[3];
    long theta,orbit,key,x,z,sine,cosine,tx,tz,curve;
    matrix=(u8 *)g_engine_interface.opaque_010[0]+0x20;
    child=visit->child_008;
    theta=visit->transform_054[4];
    orbit=prep_add((*(u32 *)(child+8)>>12)&0xfff,theta);
    key=orbit|0x4000;
    if(key==visit->cached_044) {
        if(update_cached_y) {
            *(long *)(matrix+0x18)=prep_shift(prep_sub(prep_shift(visit->column_01c,6),visit->transform_054[1]),1);
            dispatch_engine_2c710(matrix);
            orbit=visit->cached_044;
        }
        return orbit;
    }
    reference=visit->header_004+((u32)*(u16 *)(child+0xc)<<4);
    x=prep_sub(*(long *)reference,visit->transform_054[0])>>6;
    z=prep_sub(*(long *)(reference+4),visit->transform_054[2])>>6;
    trig=(short *)g_engine_interface.opaque_010[1];
    cosine=(long)trig[prep_sub(theta,0x400)&0xfff];
    sine=(long)trig[theta&0xfff];
    tx=prep_sub(prep_mul(x,sine),prep_mul(z,cosine))>>6;
    tz=prep_add(prep_mul(x,cosine),prep_mul(z,sine))>>6;
    trig=(short *)g_engine_interface.opaque_010[1];
    if(visit->variant_025==1) {
        tx=prep_add(tx,prep_mul((long)trig[prep_add(orbit,0x400)&0xfff],652));
        curve=orbit;
    } else {
        tx=prep_add(tx,prep_mul((long)trig[prep_sub(orbit,0x400)&0xfff],652));
        curve=prep_sub(orbit,0x800);
    }
    tz=prep_add(tz,prep_mul((long)trig[curve&0xfff],652));
    *(long *)(matrix+0x14)=(tx>>11)&~1L;
    *(long *)(matrix+0x1c)=prep_shift(prep_add(visit->layer_z_03c,tz>>12),1);
    *(long *)(matrix+0x18)=prep_shift(prep_sub(prep_shift(visit->column_01c,6),visit->transform_054[1]),1);
    angles[0]=(short)visit->transform_054[3];angles[1]=angles[2]=0;
    dispatch_engine_2c920(angles,matrix);
    dispatch_engine_2c940(matrix,g_mesh_scale_5ceb0);
    dispatch_engine_2c710(matrix);
    dispatch_engine_2c700(matrix);
    visit->cached_044=key;
    visit->cached_040=(long)visit->cell_02a;
    visit->anchor_row_050=*(u16 *)(child+0xe)&0x3fff;
    g_mesh_cached_column_6ffb8=visit->column_01c;
    return orbit;
}

void __cdecl dispatch_local_2ef60(Eng1MeshVisit *visit)
{ prepare_cylinder(visit,0); }

int __cdecl dispatch_local_2e7a0(Eng1MeshVisit *visit)
{
    u8 *child,*scratch;
    short *trig,depth;
    long orbit,angle,radius;
    child=visit->child_008;
    orbit=prepare_cylinder(visit,1);
    scratch=(u8 *)g_engine_interface.opaque_010[0]+0x40;
    if(visit->variant_025==1) {
        angle=prep_add(prep_shift(prep_add(prep_sub(visit->row_018,*(u16 *)(child+0xe)&0x3fff),0x10),6),orbit);
        radius=prep_sub(visit->layer_y_038,652);
    } else {
        angle=prep_add(prep_shift(prep_sub(prep_sub(*(u16 *)(child+0xe)&0x3fff,visit->row_018),0x11),6),orbit);
        radius=prep_sub(-652,visit->layer_y_038);
    }
    trig=(short *)g_engine_interface.opaque_010[1];
    *(long *)scratch=prep_mul((long)trig[prep_add(angle,0x40)&0xfff],radius)>>12;
    trig=(short *)g_engine_interface.opaque_010[1];
    *(long *)(scratch+4)=prep_mul((long)trig[prep_sub(angle,0x3c0)&0xfff],radius)>>12;
    dispatch_engine_2c690(scratch);
    dispatch_engine_2c6e0();
    dispatch_engine_2c760(scratch);
    if(*(u16 *)(scratch+2)>240) visit->column_stop_016=1;
    if(*(u16 *)scratch>320) visit->row_stop_017=1;
    dispatch_engine_2c7b0(&depth);
    if(depth<0x40) return -1;
    visit->depth_010=depth;
    return 0;
}
