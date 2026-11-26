typedef struct Entry16 {
    int w[4];
} Entry16;

typedef struct Cursor {
    Entry16 *base;
    int index;
    int count;
    int remaining;
    int last;
    int extra;
    long step,step2,step3;
    long ax,bx,cx,dx,ay,by,cy,dy,az,bz,cz,dz;
    long x,vx,ddx,dddx,y,vy,ddy,dddy,z,vz,ddz,dddz;
} Cursor;

void init_cursor_44740(Entry16 *base, int index, Cursor *c, int extra)
{
    c->base = base;
    c->index = index;
    c->remaining = 0;
    c->last = 0;
    c->extra = extra;
    c->count = 2;
    base[1] = base[2];
    c->base[c->index + 2] = c->base[c->index + 1];
}

/* Cubic path preparation and forward differences; 30-bit parameter. */
int eng1_45010(Cursor *c)
{
    Entry16 *p,*q,*r;
    long next,third,sum;
    __int64 a,b,t;
    if(!c->remaining){
        if(c->count>c->index)return 0;
        p=c->base;
        c->remaining=p[c->count].w[3];
        if(c->remaining!=c->last){
            c->last=c->remaining;c->step=0x40000000/(c->remaining+1);
            c->step2=((__int64)c->step*c->step)>>30;
            c->step3=((__int64)c->step2*c->step)>>30;
        }
        next=c->count+1;r=&p[c->count+2];q=&p[c->count];p=&p[next];
        c->ax=(long)(((__int64)q->w[0]*0x18000)>>16)+(long)(((__int64)r->w[0]*0x8000)>>16)+(long)(((__int64)p->w[0]*-0x18000)>>16)+(long)(((__int64)q[-1].w[0]*-0x8000)>>16);
        c->bx=(long)(((__int64)r->w[0]*-0x8000)>>16)+(long)(((__int64)q->w[0]*-0x28000)>>16)+(long)(((__int64)p->w[0]*0x20000)>>16)+q[-1].w[0];
        c->cx=(long)(((__int64)p->w[0]*0x8000)>>16)+(long)(((__int64)q[-1].w[0]*-0x8000)>>16);
        c->x=c->dx=q->w[0];
        a=c->ax;b=c->bx;t=c->step3;
        sum=(long)(((__int64)c->cx*c->step)>>30);sum+=(long)((b*c->step2)>>30);sum+=(long)((a*t)>>30);c->vx=sum;
        third=(long)((((a*0x60000)>>16)*t)>>30);
        c->ddx=third+(long)((((b*0x20000)>>16)*c->step2)>>30);c->dddx=third;
        c->ay=(long)(((__int64)q->w[1]*0x18000)>>16)+(long)(((__int64)q[2].w[1]*0x8000)>>16)+(long)(((__int64)q[-1].w[1]*-0x8000)>>16)+(long)(((__int64)q[1].w[1]*-0x18000)>>16);
        c->by=(long)(((__int64)q[2].w[1]*-0x8000)>>16)+(long)(((__int64)q->w[1]*-0x28000)>>16)+(long)(((__int64)q[1].w[1]*0x20000)>>16)+q[-1].w[1];
        c->cy=(long)(((__int64)q[1].w[1]*0x8000)>>16)+(long)(((__int64)q[-1].w[1]*-0x8000)>>16);
        c->y=c->dy=q->w[1];
        a=c->ay;b=c->by;
        sum=(long)((b*c->step2)>>30);sum+=(long)((a*t)>>30);sum+=(long)(((__int64)c->cy*c->step)>>30);c->vy=sum;
        third=(long)((((a*0x60000)>>16)*t)>>30);
        c->ddy=third+(long)((((b*0x20000)>>16)*c->step2)>>30);c->dddy=third;
        if(c->extra){
            c->az=(long)(((__int64)q->w[2]*0x18000)>>16)+(long)(((__int64)q[2].w[2]*0x8000)>>16)+(long)(((__int64)q[1].w[2]*-0x18000)>>16)+(long)(((__int64)q[-1].w[2]*-0x8000)>>16);
            c->bz=(long)(((__int64)q[2].w[2]*-0x8000)>>16)+(long)(((__int64)q->w[2]*-0x28000)>>16)+(long)(((__int64)q[1].w[2]*0x20000)>>16)+q[-1].w[2];
            c->cz=(long)(((__int64)q[1].w[2]*0x8000)>>16)+(long)(((__int64)q[-1].w[2]*-0x8000)>>16);
            c->z=c->dz=q->w[2];
            a=c->az;b=c->bz;
            sum=(long)((a*t)>>30);sum+=(long)(((__int64)c->cz*c->step)>>30);sum+=(long)((b*c->step2)>>30);c->vz=sum;
            third=(long)((((a*0x60000)>>16)*t)>>30);
            c->ddz=third+(long)((((b*0x20000)>>16)*c->step2)>>30);c->dddz=third;
        }
        c->count=next;return 1;
    }
    --c->remaining;
    c->x+=c->vx;c->vx+=c->ddx;c->ddx+=c->dddx;
    c->y+=c->vy;c->vy+=c->ddy;c->ddy+=c->dddy;
    c->z+=c->vz;c->vz+=c->ddz;c->ddz+=c->dddz;
    return 1;
}
