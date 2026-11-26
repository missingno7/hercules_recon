/* ENG1 effect handlers reached through the data tables at 0x58108..0x58750.
 * Uses the provisional shared Effect layout of src/shared/effect_state.c,
 * extended by the countdown at 0x108. Not a recovered original translation unit. */
typedef struct Effect Effect;
struct Effect {
    long x, y, z;
    unsigned char unknown_c[0x22 - 0xc];
    unsigned char category, mode;
    unsigned char unknown_24[6];
    short state;
    unsigned char unknown_2c[4];
    short ticks, phase;
    unsigned short sprite;
    unsigned char unknown_36[4];
    short scale, unknown_3c;
    unsigned char unknown_3e[0x4a - 0x3e];
    short unknown_4a;
    long parameter;
    unsigned long flags, render_flags;
    unsigned char unknown_58[4];
    Effect *attachment;
    unsigned char unknown_60[0x70 - 0x60];
    short speed, step, unknown_74;
    unsigned char unknown_76[0x108 - 0x76];
    long countdown;
};

void initialize_wide_effect(Effect *p)
{ p->ticks=1; p->sprite=0x50; p->x=0x8600000; p->y=0x3ee0000; p->z=0x2800000; }

void initialize_wider_effect(Effect *p)
{ p->ticks=1; p->sprite=0x60; p->x=0x8e00000; p->y=0x3ee0000; p->z=0x2800000; }

void initialize_low_backdrop(Effect *p)
{
    p->render_flags=0x90000000; p->flags|=0x20000;
    p->step=0x200; p->scale=0x3ff; p->sprite=3;
    p->x=0x4000000; p->z+=0x100000; p->mode=4;
}

void select_side_sprite(Effect *p)
{
    if(p->x>0x8000000) { p->sprite=10; p->render_flags|=0x12000000; }
    else { p->sprite=9; p->render_flags|=0x12000000; }
}

void start_cycle_effect(Effect *p)
{ p->mode=5; p->render_flags|=0x80001000; p->sprite=0x15; p->countdown=3; }

void advance_cycle_effect(Effect *p)
{
    if(--p->countdown>0) return;
    if(++p->sprite<0x1d) { p->countdown=3; return; }
    p->countdown=3; p->sprite=0x15;
}

void start_sequence_effect(Effect *p)
{
    p->sprite=0x1d; p->render_flags|=0x80000000; p->countdown=3; p->mode=5;
    p->speed=0x300; p->x=0xf000000; p->y=0x3d80000; p->z=0x26f0000; p->scale=0x3ff;
}

void advance_sequence_effect(Effect *p)
{
    if(--p->countdown>0) return;
    p->countdown=3;
    if(p->sprite<0x2d) p->sprite++;
}

void initialize_flagged_effect(Effect *p)
{ p->unknown_4a=0; p->flags|=0x10000; p->sprite=0x3e; p->render_flags|=0x684; }

void follow_attachment_scale(Effect *p)
{
    long scale;
    p->x=p->attachment->x;
    p->y=0x3e00000;
    p->z=p->attachment->z;
    scale=((0x3e00000-p->attachment->y)>>19)+((p->parameter<<8)/20);
    if(scale>1023) scale=1023;
    p->scale=(short)scale;
}

void clear_effect_animation(Effect *p)
{ p->render_flags|=5; p->ticks=0; p->sprite=0; }

void initialize_high_backdrop(Effect *p)
{
    p->render_flags=0x90000000; p->flags|=0x20000;
    p->step=0x3c0; p->speed=0; p->unknown_74=0; p->scale=0x3ff; p->sprite=0x48;
    p->x=0xf000000; p->y=0x500000; p->z=0x26f0000;
    p->ticks=0x3ae; p->mode=7; p->state=0; p->unknown_3c=0;
}

/* First native-backed pass, ENG1 1b230..1dc80. NStype member types are
 * from the original PsyQ symbols; PC offsets checked against native accesses.
 * 1d050 is cf50's switch table, not a callable body. */
extern int __cdecl abs(int);
extern void * __cdecl memset(void *,int,unsigned int);
typedef struct Vec3s { short x,y,z,pad; } Vec3s;
typedef struct NStype NStype;
struct NStype {
 long Xpos,Ypos,Zpos;
 Vec3s Vec,XYZOffset;
 unsigned char Palette,SysPal,PALIndex,pFlag,Container,StepRate2,Dbase,SpecialFX,Ambience,Failure;
 short ClipOffset,ShadowYoffs;
 unsigned short NAStatus,Light,Type;
 short Aspeed,Aindex;
 unsigned short Frame,SubFrame,CurMap,Scale,ScaleY,ScaleZ,LastFrame,LastSubFrame,LastDbase,VRAMindex,VRAMindexSub,FadeLevel;
 long StepRate;
 unsigned long Class,pCtrl;
 void *Trigger,*Shadow,*Extra,*Atable,*Tag,*SFX;
 Vec3s Angle;
 long Lvtr[4];
 short DefCol[6];
 unsigned char Xdir,Ydir,Zdir,Boomshanka;
 short Zmove,Bumblesprout;
 long Old_Xpos,Old_Ypos,Old_Zpos,XvelActing,XmaxActing,XaccActing,XdecActing,YvelActing,YmaxActing,YaccActing,YdecActing,ZvelActing,ZmaxActing,ZaccActing,ZdecActing,Xaim,Yaim,Zaim;
 NStype *PlParent,*PlChild;
 unsigned char Immunity,WeaponLog;
 short Strength;
 long Ptable,Pindex,Pspeed,YvelReacting,YmaxReacting,YaccReacting,Special1,Special2;
 void *Aux1Ptr;
 short Aux1Index,Aux1Count;
 void *Aux2Ptr;
 short Aux2Index,Aux2Count;
 void *Aux3Ptr;
 short Aux3Index,Aux3Count;
 void *Extension;
 NStype *Parent,*Child;
};
typedef char NStype_size[sizeof(NStype)==0x134?1:-1];
typedef struct Extent { short x,y,z,w,h; } Extent;
typedef struct Slot {
 NStype *actor,*effect,*extra;
 long program,old_x,target_x,bend,shift,state,fall,timer,delay,tracking,sound,unknown_38,override,aim,sequence;
} Slot;
typedef struct Placement { long x,y,z,ax,ay,az,ox,oy,oz,bend,unused0,unused1,unused2,follow,min_x,max_x,ex,ey,ez,unused3,unused4; } Placement;
typedef struct Flash { NStype *actor; long x,y,z,ax,ay,az,index,state,timer; } Flash;
extern long g_6f3ec,g_6f44c,g_6efb8,g_6f3f8,g_6f444,g_6f448,g_6f454,g_6f468,g_scene_cell_6f3e4,g_6f480,g_5602c,g_56030,g_6f3e8,g_6f440,g_72110,g_56040,g_6f460,g_6f464,g_72184,g_71ecc,g_6f3d8,g_6ffbc,g_6f3d4,g_6f380;
extern unsigned long g_6f46c,g_6f470;
extern long *g_6f3c8,*g_6f3cc;
extern NStype *g_normal_actor_pool,*g_6f3f4,*g_6f474,*g_6f478,*g_734f4,*g_6f47c;
extern void *g_7217c;
extern Slot g_6f188[];
extern Placement g_574d8[];
extern Flash g_57b18[];
extern long g_57778[][10],g_56398[][3],g_562d8[][3],g_56338[][3],g_578b8[][16],g_57ab8[],g_57ae8[];
extern short g_562b8[],g_55c28[],g_5a848[];
extern short *g_57498[];
extern Vec3s g_70840[];
extern unsigned short g_72168,g_configuration_word_713b4,g_56400;
extern short g_pipeline_word_71740[],g_pipeline_word_71720[],g_pipeline_word_71ca0[];
extern long g_6f488,g_6f48c,g_6f484,g_70cd0,g_70cd4,g_70d18;
extern short g_6f490,g_camera_adjust_y_70d04;
extern long g_71580[];
extern void (**g_71370)(NStype *),(**g_7139c)(NStype *);
extern void (*g_562d0[])(NStype *);
extern char g_56240[],g_56680[],g_56208[],g_561f0[],g_56220[],g_562a0[],g_56270[],g_56288[],g_56258[];
extern unsigned int random_limit(int);
extern int create_command(NStype *),eng1_interpret_animation_triples(NStype *),remove_actor(NStype *),eng1_native_code_target_002b130(NStype *),eng1_19b40(int);
extern void advance_frame(NStype *),select_frame(NStype *,int),title_08cd0(NStype *),spawn_actor_with_owner(NStype *,long,long,long,int),eng1_1e110(void),play_sound(int,NStype *);
extern int extent_producer_30e40(NStype *,Extent *,int);
extern long eng1_native_code_target_00266a0(NStype *,int,int),rounded_sine_angle(long),wrap_delta_461a0(int,int);
extern NStype *title_17ad0(long,long,long,int,int),*spawn_normal(long,long,long,int),*spawn_kind(long,long,long,int);
extern int eng1_native_code_target_001dd50(NStype *);
extern void eng1_131c0(long,long,long,int);
void eng1_native_code_target_001b370(NStype *,NStype *);
int eng1_native_code_target_001b760(void);
void eng1_native_code_target_001c3f0(NStype *);
NStype *eng1_native_code_target_001c4d0(int);
void eng1_native_code_target_001c660(int),eng1_native_code_target_001c6c0(void),eng1_native_code_target_001c720(NStype *);
int eng1_native_code_target_001cf10(NStype *);
long eng1_native_code_target_001ce80(long,long,long,long);
void eng1_native_code_target_001d070(NStype *,int),eng1_native_code_target_001d170(NStype *),eng1_native_code_target_001d360(NStype *),eng1_native_code_target_001d5f0(NStype *),eng1_native_code_target_001d6c0(NStype *),eng1_native_code_target_001d780(NStype *),eng1_native_code_target_001d910(void),eng1_native_code_target_001da00(NStype *),eng1_native_code_target_001da20(NStype *);

void eng1_native_code_target_001b230(NStype *p)
{
 p->Shadow=g_56240; p->pCtrl=0x10001000; p->Class|=0x20000;
 p->YvelActing=0; p->SpecialFX=2;
 p->Scale=(unsigned short)g_57778[g_6f3ec][p->NAStatus];
 p->ScaleZ=(unsigned short)g_57778[g_6f3ec][p->NAStatus];
}
void eng1_native_code_target_001b2a0(NStype *p)
{
 p->Class|=0x20000; p->pCtrl|=0x10001040; p->Shadow=g_56240;
 p->ScaleZ=0xd4;
 if(!g_6f44c)p->Scale=0xd4;else p->Scale=0x90;
 p->Xaim=p->Scale; p->SpecialFX=3;
 p->Angle.x=0x300;p->Angle.y=0;p->Angle.z=0x200;
 p->XvelActing=random_limit(5)+5;
 if(!random_limit(2))p->XvelActing=-p->XvelActing;
 p->YvelActing=-4;p->ZvelActing=-5;p->NAStatus=0;
 if(create_command(p)){advance_frame(p);eng1_interpret_animation_triples(p);}
}
void eng1_native_code_target_001b370(NStype *p,NStype *q)
{
 Extent box;
 int x0,x1,y0,y1,a,b,c,d,e,f,t,code,kind;
 if(q->NAStatus!=4 || q->Aux3Index<38 || q->Aux3Index>56)return;
 if(extent_producer_30e40(g_normal_actor_pool,&box,1) && (box.w|box.h)){
  if(g_normal_actor_pool->pCtrl&16){x1=g_normal_actor_pool->Vec.x-box.x;x0=x1-box.w;}
  else{x0=g_normal_actor_pool->Vec.x+box.x;x1=x0+box.w;}
  y0=g_normal_actor_pool->Vec.y+box.y;y1=y0+box.h;
  a=g_70840[p->Container].x;b=g_70840[p->Container+1].x;c=g_70840[p->Container+2].x;
  d=g_70840[p->Container].y;e=g_70840[p->Container+1].y;f=g_70840[p->Container+2].y;
  if(a>b){t=a;a=b;b=t;}if(a>c){t=a;a=c;c=t;}if(b>c)c=b;
  if(d>e){t=d;d=e;e=t;}if(d>f){t=d;d=f;f=t;}if(e>f)f=e;
  if(((((x0>=a&&x0<=c)||(x1>=a&&x1<=c)) && ((y0>=d&&y0<=f)||(y1>=d&&y1<=f)))) ||
     ((((a>=x0&&a<=x1)||(c>=x0&&c<=x1)) && ((d>=y0&&d<=y1)||(f>=y0&&f<=y1)))))
   eng1_native_code_target_00266a0(p,34,0);
 }
 if(!extent_producer_30e40(g_normal_actor_pool,&box,2) || !(box.w|box.h))return;
 if(g_normal_actor_pool->pCtrl&16){x1=g_normal_actor_pool->Vec.x-box.x;x0=x1-box.w;}
 else{x0=g_normal_actor_pool->Vec.x+box.x;x1=x0+box.w;}
 y0=g_normal_actor_pool->Vec.y+box.y;y1=y0+box.h;
 a=g_70840[p->Container].x-12;c=g_70840[p->Container].x+12;
 d=g_70840[p->Container].y-12;f=g_70840[p->Container].y+12;
 if(!((((x0>=a&&x0<=c)||(x1>=a&&x1<=c)) && ((y0>=d&&y0<=f)||(y1>=d&&y1<=f))) ||
      (((a>=x0&&a<=x1)||(c>=x0&&c<=x1)) && ((d>=y0&&d<=y1)||(f>=y0&&f<=y1)))))return;
 if(!eng1_19b40((~g_72168)&24))return;
 code=0;
 switch(q->Aux2Index){
 case 0:if(!(g_6f3d4&1)){g_6f3d4|=1;code=9;}break;
 case 2:if(!(g_6f3d4&2)){g_6f3d4|=2;code=9;}break;
 case 4:if(!(g_6f3d4&4)){g_6f3d4|=4;code=9;}break;
 case 6:if(!(g_6f3d4&8)){g_6f3d4|=8;code=9;}break;
 }
 if(code || (code=eng1_native_code_target_001b760())!=0){
  kind=0x280;if(q->Aux2Index<=1)kind=0x220;if(q->Aux2Index>=5)kind=0x240;
  eng1_131c0(p->Xpos,p->Ypos,g_normal_actor_pool->Zpos,kind|code);
 }
 switch(random_limit(3)){case 0:play_sound(0x5985,p);break;case 1:play_sound(0x5986,p);break;case 2:play_sound(0x5987,p);break;}
 eng1_native_code_target_001d6c0(q);
}
int eng1_native_code_target_001b760(void)
{
 int value=*g_6f3c8++;
 while(value<=0){
  if(value==0){g_6f46c=*g_6f3c8++;g_6f470=*g_6f3c8++;}
  if(value<0)g_6f3c8=g_6f3cc;
  value=*g_6f3c8++;
 }
 if(g_6f46c>=random_limit(g_6f470)+1 && g_6f46c!=g_6f470){--g_6f3c8;return 0;}
 return value;
}
void eng1_native_code_target_001b7f0(NStype *p)
{
 Slot *s=&g_6f188[p->Special1];NStype *q=s->actor;int done=0;
 if(eng1_interpret_animation_triples(p))switch(s->state){
 case 1:p->StepRate=0;p->Aspeed=10;break;
 case 2:s->state=3;select_frame(p,1);play_sound(0x5984,p);break;
 case 3:s->state=0;s->timer=random_limit(30)+30;break;
 }
 switch(s->fall){
 case 0:eng1_native_code_target_001b370(p,q);break;
 case 1:
  p->Xpos+=p->XvelActing<<16;p->Ypos+=p->YvelActing<<16;p->Zpos+=p->ZvelActing<<16;
  if(p->YvelActing<32)++p->YvelActing;
  p->Angle.x+=16;p->Angle.y-=p->YvelActing>>1;p->Angle.z+=p->YvelActing>>1;
  if(p->Ypos>=0x3d80000){
   eng1_1e110();if(g_6f468){g_6f468=0;play_sound(0x188,0);}
   p->XvelActing/=2;p->YvelActing=(-p->YvelActing*2)/3;
   p->Ypos=(p->YvelActing+0x3d8)<<16;
   if(p->NAStatus++>=2 || (p->pFlag&8) || !(p->pFlag&2))done=1;
  }
  if(p->Zpos<0x80000)done=1;
  if(done){remove_actor(p);s->effect=0;}
  break;
 }
}
void eng1_native_code_target_001b9a0(void)
{
 g_6f3f4=g_normal_actor_pool+2;title_08cd0(g_6f3f4);
 g_6f3f4->Type=1;g_6f3f4->Xpos=0x8800000;g_6f3f4->Ypos=0x34d0000;g_6f3f4->Zpos=0x3000000;
 g_6f3f4->Dbase=0;g_6f3f4->Class=0x20000;g_6f3f4->pCtrl=0x90000040;g_6f3f4->Scale=0x200;g_6f3f4->Angle.x=-16;g_6f3f4->SpecialFX=3;
 spawn_actor_with_owner(g_6f3f4,g_6f3f4->Xpos,g_6f3f4->Ypos,g_6f3f4->Zpos,0x2b);
 spawn_actor_with_owner(g_6f3f4,g_6f3f4->Xpos,g_6f3f4->Ypos,g_6f3f4->Zpos,0x2e);
 g_6f3f4->Aux1Ptr=g_56680;
 if(create_command(g_6f3f4)){advance_frame(g_6f3f4);eng1_interpret_animation_triples(g_6f3f4);}
}
void eng1_native_code_target_001bab0(NStype *p)
{
 int a,b,i;long rate;NStype *q;
 if(g_6efb8>0)eng1_interpret_animation_triples(p);
 a=g_6f3f4->Frame-34;b=g_6f3f4->SubFrame-34;rate=g_6f3f4->StepRate;
 g_6f3f8=g_56398[a][0]+((g_56398[b][0]-g_56398[a][0])*rate)/255;
 g_6f444=g_56398[a][1]+((g_56398[b][1]-g_56398[a][1])*rate)/255;
 g_6f448=g_56398[a][2]+((g_56398[b][2]-g_56398[a][2])*rate)/255;
 for(i=0;i<g_71580[0];i++){q=(NStype *)g_71580[i+1];if(q->Type==0x29)g_71370[0x1f4/4](q);}
 for(q=p->Child;q;q=q->Child){if(q->Type==0x2b)g_71370[0x20c/4](q);if(q->Type==0x2e)g_71370[0x230/4](q);}
 if(g_6f454>=2)remove_actor(p);
}
void eng1_native_code_target_001bc20(NStype *p)
{
 if(eng1_native_code_target_002b130(p)){
  p->Class=0x20000;p->pCtrl=0xb0001000;p->Frame=5;p->Scale=0x158;p->Ypos+=0x6b0000;p->Zpos+=0xbf0000;
  p->Angle.x=0x1c8;p->Zmove=0xb40;p->SpecialFX=5;p->Shadow=g_56208;g_6f474=p;
 }else remove_actor(p);
}
void eng1_native_code_target_001bca0(NStype *p)
{
 p->Shadow=g_561f0;p->pCtrl=0x10001040;p->Class|=0x20000;p->Scale=p->NAStatus;p->SpecialFX=2;p->Angle.x=0x200;
}
void eng1_native_code_target_001bce0(NStype *p)
{
 int a=g_6f3f4->Frame-34,b=g_6f3f4->SubFrame-34,i;long rate=g_6f3f4->StepRate;
 long x=g_562d8[a][0]+((g_562d8[b][0]-g_562d8[a][0])*rate)/255;
 long y=g_562d8[a][1]+((g_562d8[b][1]-g_562d8[a][1])*rate)/255;
 long z=g_562d8[a][2]+((g_562d8[b][2]-g_562d8[a][2])*rate)/255;
 NStype **list=(NStype **)p->Aux3Ptr,*q,*prev=p;long yoff=0x40000,zoff=0x100000;
 p->XYZOffset.x=(short)x;p->XYZOffset.y=(short)y;p->Zmove-=81;p->XYZOffset.z=(short)z;
 if(p->Zmove<0)p->Zmove+=11520;
 for(i=0;i<(long)list[0];i++){
  yoff+=0x40000;zoff+=0x100000;q=list[i+1];q->XYZOffset.x=(short)z;x>>=1;y>>=1;
  q->Xpos=p->Xpos+(rounded_sine_angle(((i*320+p->Zmove)/16)<<16)<<4)*(i+1);z>>=1;
  q->Ypos=p->Ypos+yoff;if(q->Ypos>0x3e00000)q->Ypos=0x3e00000;
  q->Zpos=p->Zpos+zoff;q->Angle.x=0x1e2;
  if(yoff>0x80000)prev->Angle.x=(prev->Angle.x+0x1e2)>>1;
  if(yoff>0x280000){if(q->Angle.y>0x200)q->Angle.y-=0x400;q->Angle.y=(q->Angle.y>>2)*(10-(i+1));}
  prev=q;
 }
 if(--p->Aindex<0){p->Aindex=0;p->NAStatus=1;p->Aspeed=random_limit(16)+8;}
 if(g_6f454>=2)remove_actor(p);
}
void eng1_native_code_target_001bf40(NStype *p)
{
 int a=g_6f3f4->Frame-34,b=g_6f3f4->SubFrame-34,i;long rate=g_6f3f4->StepRate;
 long x=g_562d8[a][0]+((g_562d8[b][0]-g_562d8[a][0])*rate)/255;
 long y=g_562d8[a][1]+((g_562d8[b][1]-g_562d8[a][1])*rate)/255;
 long z=g_562d8[a][2]+((g_562d8[b][2]-g_562d8[a][2])*rate)/255;
 NStype **list=(NStype **)p->Aux3Ptr,*q,*prev=p;long yoff=0x40000,zoff=0x100000,w;
 p->XYZOffset.x=(short)x;p->XYZOffset.y=(short)y;p->Zmove-=81;p->XYZOffset.z=(short)z;
 if(p->Zmove<0)p->Zmove+=11520;p->Aindex-=150;
 for(i=0;i<(long)list[0];i++){
  yoff+=0x40000;zoff+=0x100000;q=list[i+1];q->XYZOffset.x=(short)z;y>>=1;z>>=1;x>>=1;
  w=rounded_sine_angle(((i*320+p->Zmove)/16)<<16)<<4;
  a=rounded_sine_angle(((i*320+p->Aindex)/16)<<16)*p->Aspeed;if(a>0)a=0;
  q->Xpos=p->Xpos+w*(i+1);
  q->Ypos=p->Ypos+yoff+((a*(unsigned short)g_562b8[i+1])>>2);if(q->Ypos>0x3e00000)q->Ypos=0x3e00000;
  q->Zpos=p->Zpos+zoff;q->Angle.x=0x200;
  if(yoff>0x80000)prev->Angle.x=(prev->Angle.x+0x200)>>1;
  if(yoff>0x280000){if(q->Angle.y>0x200)q->Angle.y-=0x400;q->Angle.y=(q->Angle.y>>2)*(10-(i+1));}
  prev=q;
 }
 if(p->Aindex< -5760){p->Aindex=random_limit(100);p->NAStatus=0;}
 if(g_6f454>=2)remove_actor(p);
}
void eng1_native_code_target_001c200(NStype *p){g_562d0[p->NAStatus](p);}
void eng1_native_code_target_001c220(NStype *p)
{
 p->Class=0x20000;p->pCtrl=0x90001040;p->SpecialFX=3;p->Frame=0x2f;p->SubFrame=0x2e;p->StepRate=0;
 p->Xpos=g_6f3f4->Xpos;p->Ypos=g_6f3f4->Ypos-0x500000;p->Zpos=g_6f3f4->Zpos-0x3c0000;
 p->Angle.x=0x3f1;p->Scale=0x200;p->SpecialFX=6;p->Shadow=g_56220;g_6f478=p;
}
void eng1_native_code_target_001c2a0(NStype *p)
{
 int a,b;long rate;long x,y,z;
 x=g_6f480;p->Xpos=g_6f3f4->Xpos+x;p->Ypos=g_6f3f4->Ypos+g_5602c;p->Zpos=g_6f3f4->Zpos+g_56030;
 rate=p->StepRate;if(g_scene_cell_6f3e4==1)rate+=16;p->StepRate=rate;
 if(p->StepRate>255){p->StepRate=0;p->Frame=p->SubFrame;++g_scene_cell_6f3e4;}
 a=g_6f3f4->Frame-34;b=g_6f3f4->SubFrame-34;rate=g_6f3f4->StepRate;
 x=g_56338[a][0]+((g_56338[b][0]-g_56338[a][0])*rate)/255;p->XYZOffset.x=(short)(x-31);
 y=g_56338[a][1]+((g_56338[b][1]-g_56338[a][1])*rate)/255;p->XYZOffset.y=(short)(y+8);
 z=g_56338[a][2]+((g_56338[b][2]-g_56338[a][2])*rate)/255;p->XYZOffset.z=(short)(z-5);
 if(g_6f454>=2)remove_actor(p);
}
void eng1_native_code_target_001c3f0(NStype *p)
{
 int next=0,index;
 switch(g_6f44c){
 case 0:eng1_native_code_target_001c4d0(7);++g_6f44c;return;
 case 1:remove_actor(p);g_6f380=0;
  eng1_native_code_target_001c4d0(3);eng1_native_code_target_001c4d0(1);eng1_native_code_target_001c4d0(5);g_6f44c+=3;return;
 case 4:next=2;break;case 5:next=4;break;case 6:next=0;break;case 7:next=6;break;case 8:next=-1;break;
 }
 index=p->Aux2Index;remove_actor(p);eng1_native_code_target_001c4d0(index);
 if(next>=0){eng1_native_code_target_001c4d0(next);++g_6f44c;}
}
NStype *eng1_native_code_target_001c4d0(int index)
{
 NStype *p,*q;
 g_6f3ec=index;eng1_native_code_target_001c6c0();g_56400=4;
 if(g_6f3ec==1||g_6f3ec==5)g_56400=0x4e;
 p=title_17ad0(g_574d8[index].x,g_574d8[index].y,g_574d8[index].z,0x29,0);
 if(p){
  g_6f188[p->Aux2Index].actor=p;
  p->Angle.x=(short)g_574d8[g_6f3ec].ax+0x200;p->Angle.y=(short)g_574d8[g_6f3ec].ay;p->Angle.z=(short)g_574d8[g_6f3ec].az;
  p->Scale=(short)g_57778[g_6f3ec][0];p->ScaleZ=(short)g_57778[g_6f3ec][0];
  if(eng1_native_code_target_002b130(p)){
   eng1_native_code_target_001c660(0x2f);
   q=spawn_normal(p->Xpos,p->Ypos,p->Zpos,0x2d);g_6f188[g_6f3ec].effect=q;
   if(q)g_71370[0x21c/4](q);
   g_6f188[g_6f3ec].effect->Special1=g_6f3ec;
   eng1_native_code_target_001dd50(p);eng1_native_code_target_001c720(p);return p;
  }
  remove_actor(p);p=0;
 }
 return p;
}
void eng1_native_code_target_001c660(int kind)
{
 int count=g_configuration_word_713b4,i;NStype *pool=g_normal_actor_pool,*p=pool;
 for(i=0;i<count;i++,p++)if(p->Type==0)return;
 for(i=0,p=pool;i<count;i++,p++)if(p->Type==kind)goto found;
 return;
found:remove_actor(p);
}
void eng1_native_code_target_001c6c0(void)
{
 memset(&g_6f188[g_6f3ec],0,sizeof(Slot));
 g_6f188[g_6f3ec].timer=random_limit(30)+30;
 g_6f188[g_6f3ec].delay=random_limit(0);
}
void eng1_native_code_target_001c720(NStype *p)
{
 int index=p->Aux2Index,i,changed=0,lo,hi,t,limit,delta;
 Slot *s=&g_6f188[index];NStype **list=(NStype **)p->Aux3Ptr,*q=0,*effect;
 long ox=g_6f3f8,oy=g_6f444,oz=g_6f448,x=p->Xpos,y=p->Ypos,z=p->Zpos;
 long base=p->Angle.x,angle=base,a=-512,b=-512,radius=0;
 short *program;
 if(s->override){p->Xaim=s->aim;p->Yaim=s->aim;s->program=index+s->sequence*8;p->Zaim=0;p->Zmove=0;}
 program=g_57498[s->program];
 p->XYZOffset.x=(short)(ox+g_574d8[index].ox);p->XYZOffset.y=(short)(oy+g_574d8[index].oy);p->XYZOffset.z=(short)(oz+g_574d8[index].oz);
 for(i=1;i<=(long)list[0];i++){
  q=list[i];ox>>=1;oy>>=1;oz>>=1;
  q->XYZOffset.x=(short)(ox+g_574d8[index].ox);q->XYZOffset.y=(short)(oy+g_574d8[index].oy);q->XYZOffset.z=(short)(oz+g_574d8[index].oz);
  x=p->Xpos;
  switch(p->NAStatus){
  case 0:
   t=p->Aux3Index;lo=g_57ab8[i];hi=g_57ae8[i];
   if(t<lo)radius=0;else if(t>hi)radius=g_578b8[index][i];else radius=g_578b8[index][i]*(t-lo)/(hi-lo);
   if(i==1&&t<24){if(index==1)radius+=(24-t)*110/24;if(index==3)radius+=(24-t)*40/24;if(index==5)radius+=(24-t)*114/24;}
   x+=g_5a848[(p->Zmove+i*102)&1023]*i*6;break;
  case 1:
   t=p->Aux3Index;lo=g_57ab8[i];hi=g_57ae8[i];
   if(t<lo)radius=0;else if(t>hi)radius=g_578b8[index][i];else{changed=1;radius=g_578b8[index][i]*(t-lo)/(hi-lo);}
   x+=(g_5a848[(p->Zmove+i*102)&1023]*i)<<6;break;
  case 2:radius=0;break;
  case 3:radius=g_578b8[index][i];x+=g_5a848[(p->Zmove+i*102)&1023]*i*6;break;
  case 4:
   radius=g_578b8[index][i]+s->bend;x+=s->shift*i/10;
   t=p->Aux3Index;
   if(t<=16)x+=g_5a848[(p->Zmove+i*102)&1023]*(16-t)*i*6/16;
   else if(t>=56)x+=g_5a848[(p->Zmove+i*102)&1023]*(t-56)*i*6/8;
   break;
  }
  y+=(g_5a848[angle&1023]*2)*radius;z-=(g_5a848[(angle+256)&1023]*2)*radius;
  q->Xpos=x;q->Ypos=y;q->Zpos=z;
  a+=program[p->Xaim*11+i];b+=program[p->Yaim*11+i];
  angle=base-(((b-a)*p->Zaim)>>16)-a;q->Angle.x=(short)(angle+512);
 }
 if(changed){
  if(s->extra){s->extra->Xpos=x+g_574d8[index].ex;s->extra->Ypos=y+g_574d8[index].ey;s->extra->Zpos=z-g_574d8[index].ez;}
  effect=spawn_kind(x,y,z,0x202f);if(effect)g_7139c[0x178/4](effect);
 }
 s->tracking=0;effect=s->effect;
 if(!effect || p->NAStatus==1 || p->NAStatus==2)return;
 effect->Xpos=q->Xpos;effect->Ypos=q->Ypos;effect->Zpos=q->Zpos;
 if(g_6efb8<=0){
  delta=wrap_delta_461a0(0x180,effect->Angle.x);if(delta>8)delta=8;if(delta< -8)delta=-8;effect->Angle.x-=delta;
  if(delta<0x1c8 && !s->sound){s->sound=1;select_frame(effect,3);if(g_6f460){g_6f460=0;play_sound(0x19e,0);}}
  if(!random_limit(120))play_sound(0x184,0);
 }else effect->Angle.x=q->Angle.x+(short)g_574d8[index].follow;
 if(eng1_native_code_target_001cf10(p)){
  delta=wrap_delta_461a0(eng1_native_code_target_001ce80(effect->Zpos,effect->Xpos,g_normal_actor_pool->Zpos,g_normal_actor_pool->Xpos)+512,effect->Angle.y);
  limit=12;if(p->NAStatus==4&&p->Aux3Index>=38)limit=0;
  if(delta>limit)delta=limit;if(delta< -limit)delta=-limit;s->tracking=1;
 }else{delta=wrap_delta_461a0((g_5a848[(p->Zmove*2+256)&1023]<<7)>>16,effect->Angle.y);if(delta>4)delta=4;if(delta< -4)delta=-4;}
 effect->Angle.y-=delta;
 effect->XYZOffset.x=q->XYZOffset.x;effect->XYZOffset.y=q->XYZOffset.y;effect->XYZOffset.z=q->XYZOffset.z;
 q->Angle.x=effect->Angle.x-96;q->Angle.y=effect->Angle.y;q->Angle.z=effect->Angle.z+512;
}
long eng1_native_code_target_001ce80(long x,long y,long x2,long y2)
{
 int dx,dy,ax,ay,angle;
 dx=(x2-x)>>16;ax=abs(dx);dy=(y2-y)>>16;ay=abs(dy);
 if(ax==ay)angle=ax?128:0;
 else if(ay<ax)angle=g_55c28[(ay<<9)/ax];else angle=256-g_55c28[(ax<<9)/ay];
 if(dx<0)angle=512-angle;if(dy<0)angle=1024-angle;return angle;
}
int eng1_native_code_target_001cf10(NStype *p)
{
 int index=p->Aux2Index;
 if((g_normal_actor_pool->Xpos>>16)>=g_574d8[index].min_x&&(g_normal_actor_pool->Xpos>>16)<=g_574d8[index].max_x)return 1;return 0;
}
void eng1_native_code_target_001cf50(NStype *p)
{
 p->Aux2Index=(short)g_6f3ec;p->pCtrl|=0xb0001000;p->Class=0x20a00;p->Dbase=0;p->Frame=4;
 p->YvelReacting=p->Xpos;p->YmaxReacting=p->Ypos;p->YaccReacting=p->Zpos;
 p->Zmove=random_limit(1024);p->SpecialFX=5;
 switch(p->Aux2Index){case 1:p->Shadow=g_562a0;break;case 3:p->Shadow=g_56270;break;case 5:p->Shadow=g_56288;break;case 7:p->Shadow=g_56258;break;default:p->SpecialFX=0;}
 advance_frame(p);
 if(!g_6f44c){eng1_native_code_target_001d070(p,0);p->NAStatus=3;p->Aux3Index=60;}
 else{eng1_native_code_target_001d070(p,0);p->NAStatus=0;p->Aux3Index=0;if(g_6f460){g_6f460=0;play_sound(0x183,0);}}
}
void eng1_native_code_target_001d070(NStype *p,int program)
{
 g_6f188[p->Aux2Index].program=p->Aux2Index+program;select_frame(p,p->Aux2Index+program);
}
void eng1_native_code_target_001d0b0(NStype *p)
{
 if(!g_6f188[p->Aux2Index].override){g_6f3e8=eng1_native_code_target_001dd50(p);if(g_6f3e8)g_6f188[p->Aux2Index].program=g_6f440;}
 eng1_native_code_target_001c720(p);
 switch(p->NAStatus){case 3:eng1_native_code_target_001d170(p);break;case 4:eng1_native_code_target_001d360(p);break;case 1:eng1_native_code_target_001d780(p);break;case 2:eng1_native_code_target_001da00(p);break;case 0:eng1_native_code_target_001da20(p);break;}
 if(g_6f454>=1){eng1_native_code_target_001d5f0(p);remove_actor(p);}
}
void eng1_native_code_target_001d170(NStype *p)
{
 int index=p->Aux2Index,i,clear;Slot *s;long target;
 p->Zmove+=6;
 if(g_6f3e8 && g_6f188[index].delay--<=0 && !g_72110 && g_56040 && eng1_native_code_target_001cf10(p) && !g_6f188[index].state && g_6efb8>0){
  target=g_normal_actor_pool->Xpos+((random_limit(32)-16)<<16)+(g_574d8[index].ox<<7);
  if(target<0x4800000)target=0x4800000;if(target>0xcf00000)target=0xcf00000;
  clear=1;
  for(i=0;i<7;i++){s=&g_6f188[i];if(s->actor&&s->actor->NAStatus==4){
   if(((s->target_x+0x3e0000>=target-0x3e0000)&&(s->target_x-0x3e0000<=target-0x3e0000)) ||
      ((s->target_x+0x3e0000>=target+0x3e0000)&&(s->target_x-0x3e0000<=target+0x3e0000))){clear=0;break;}
  }}
  if(clear){g_6f188[index].old_x=g_574d8[index].x;g_6f188[index].target_x=target;p->NAStatus=4;p->Aux3Index=0;eng1_native_code_target_001d070(p,8);g_6f188[index].state=1;}
 }
 if(g_6f188[index].timer--<=0 && g_56040 && eng1_native_code_target_001cf10(p) && g_6f188[index].delay>=0 && !g_6f188[index].state && g_6efb8>0)g_6f188[index].state=2;
}
void eng1_native_code_target_001d360(NStype *p)
{
 int index=p->Aux2Index,t,v;long bend;
 p->Zmove+=6;bend=g_574d8[index].bend;t=p->Aux3Index++;
 if(t==28){select_frame(g_6f188[index].effect,2);g_6f188[index].state=0;switch(random_limit(4)){case 0:play_sound(0x598c,g_6f188[index].effect);break;case 1:play_sound(0x598d,g_6f188[index].effect);break;case 2:play_sound(0x598e,g_6f188[index].effect);break;case 3:play_sound(0x598f,g_6f188[index].effect);break;}}
 if(t==40)switch(random_limit(4)){case 0:play_sound(0x5196,g_6f188[index].effect);break;case 1:play_sound(0x5197,g_6f188[index].effect);break;case 2:play_sound(0x5198,g_6f188[index].effect);break;case 3:play_sound(0x5199,g_6f188[index].effect);break;}
 if(t==45&&!random_limit(2))switch(random_limit(2)){case 0:play_sound(0x51a0,g_6f188[index].effect);break;case 1:play_sound(0x51a1,g_6f188[index].effect);break;}
 if(t>=28&&t<=38){v=t-28;g_6f188[index].shift=(g_6f188[index].target_x-g_6f188[index].old_x)*v/10;g_6f188[index].bend=bend*v/10;}
 if(t>=53&&t<=63){v=63-t;g_6f188[index].shift=(g_6f188[index].target_x-g_6f188[index].old_x)*v/10;g_6f188[index].bend=bend*v/10;}
 if(g_6f3e8){p->NAStatus=3;eng1_native_code_target_001d070(p,0);g_6f188[index].bend=0;g_6f188[index].delay=random_limit(0);}
}
void eng1_native_code_target_001d5f0(NStype *p)
{
 NStype *q=g_6f188[p->Aux2Index].effect;
 if(q){select_frame(q,4);g_6f188[p->Aux2Index].fall=1;
  if(g_6f464){g_6f464=0;switch(random_limit(2)){case 0:play_sound(0x5994,g_6f188[p->Aux2Index].effect);break;case 1:play_sound(0x5995,g_6f188[p->Aux2Index].effect);break;}
   play_sound(0x5993,g_6f188[p->Aux2Index].effect);play_sound(0x1a2,0);
  }
 }
}
extern unsigned long g_eng1_setup_dword_72014;

void eng1_native_code_target_001d6c0(NStype *p)
{
 NStype *q;
 if((p->NAStatus!=3&&p->NAStatus!=4)||g_72110)return;
 p->NAStatus=1;p->Aux3Index=63;
 q=spawn_normal(p->Xpos,p->Ypos,p->Zpos,0x30);g_6f188[p->Aux2Index].extra=q;
 if(g_6f188[p->Aux2Index].extra)g_71370[0x240/4](g_6f188[p->Aux2Index].extra);
 if(--g_6efb8<=0)g_eng1_setup_dword_72014=1;g_72184=32;eng1_native_code_target_001d5f0(p);g_71ecc=60;
}
void eng1_native_code_target_001d780(NStype *p)
{
 Flash *f;NStype *q;int i,index;
 if(p->Aux3Index>0){
  p->Zmove+=80;
  if(--p->Aux3Index==1){
   if(g_6f188[p->Aux2Index].effect){p->Aux3Index=2;return;}
   if(!g_scene_cell_6f3e4){g_scene_cell_6f3e4=1;g_6f3d8=5;g_6ffbc=1;
    for(f=g_57b18;f<g_57b18+3;f++){
     if(g_6f44c==1){index=f->index;q=spawn_normal(g_574d8[index].x,g_574d8[index].y,g_574d8[index].z,0x30);f->actor=q;if(q)g_71370[0x240/4](q);f->state=0;
      f->actor->Xpos=f->x;f->actor->Ypos=f->y;f->actor->Zpos=f->z;f->actor->Angle.x=(short)f->ax;f->actor->Angle.y=(short)f->ay;f->actor->Angle.z=(short)f->az;f->actor->Scale=64;
     }else f->actor=0;
    }
   }
  }
 }else{
  eng1_native_code_target_001d910();
  if(g_6f3d8<=0){p->NAStatus=2;q=g_6f188[p->Aux2Index].extra;if(q){remove_actor(q);g_6f188[p->Aux2Index].extra=0;}}
  else --g_6f3d8;
 }
}
void eng1_native_code_target_001d910(void)
{
 Flash *f;
 for(f=g_57b18;(long)f<(long)(g_57b18+3);f++)if(f->actor){
  f->actor->XYZOffset.x=(short)(g_574d8[f->index].ox+g_6f3f8);
  f->actor->XYZOffset.y=(short)(g_574d8[f->index].oy+g_6f444);
  f->actor->XYZOffset.z=(short)(g_574d8[f->index].oz+g_6f448);
  if(!f->state){f->actor->Scale+=90;if(f->actor->Scale>=0x240){f->state=1;f->timer=15;}}
  else if(--f->timer<0){f->actor->Scale-=8;if(f->actor->Scale<0x100){remove_actor(f->actor);f->actor=0;}}
 }
}
void eng1_native_code_target_001da00(NStype *p){eng1_native_code_target_001d910();if(g_6efb8>0)eng1_native_code_target_001c3f0(p);}
void eng1_native_code_target_001da20(NStype *p)
{
 NStype *q=g_6f188[p->Aux2Index].effect;unsigned short scale;
 ++p->Aux3Index;p->Zmove+=20;
 if(p->Aux3Index<64){
  if(q){scale=(unsigned short)(((q->ScaleZ-q->Xaim)*p->Aux3Index)/64+q->Xaim);q->Scale=scale;if(scale<2)q->Scale=2;}
  eng1_native_code_target_001d910();
 }else{if(q)q->Scale=q->ScaleZ;p->NAStatus=3;p->Aux3Index=random_limit(50)+50;}
}
void eng1_native_code_target_001daf0(NStype *p)
{
 if(--p->Aspeed<=0){p->Aspeed=1;if(++p->Frame>=0x5e)p->Frame=0x50;}
 p->Zpos-=0x70000;if(p->Zpos<0x400000)remove_actor(p);
}
void eng1_native_code_target_001db70(NStype *p)
{
 if(--p->Aspeed<=0){p->Aspeed=1;if(++p->Frame>=0x68)p->Frame=0x60;}
 p->Zpos-=0x70000;if(p->Zpos<0x400000)remove_actor(p);
}
void eng1_native_code_target_001dbc0(NStype *p)
{
 p->Frame=0;p->pCtrl=0;p->Class=0x20002;p->Shadow=g_7217c;g_734f4=p;p->Aindex=0x145;p->Pindex=0;g_6f47c=p;
}
void eng1_native_code_target_001dc00(NStype *p)
{
 g_70cd0=g_6f488;g_70cd4=g_6f48c;g_camera_adjust_y_70d04=g_6f490;g_70d18=g_6f484;
 g_pipeline_word_71740[++g_pipeline_word_71740[0]]=(short)g_70cd0;g_pipeline_word_71720[++g_pipeline_word_71720[0]]=(short)g_70cd4;g_pipeline_word_71ca0[++g_pipeline_word_71ca0[0]]=(short)g_70d18;
}
