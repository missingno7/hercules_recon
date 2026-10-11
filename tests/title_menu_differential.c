/* Isolated loaded-DLL differential; no game process, original dispatch in product,
 * copied machine code, or patching. Writable globals are fixture setup only. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "title_menu_bindings.h"

typedef char menu_fixture_pointer_width_must_be_32[(sizeof(void *) == 4) ? 1 : -1];

typedef struct Proc { DWORD fn, state; long delay; DWORD arg, next, word; } Proc;
typedef void (__cdecl *MenuFn)(Proc *);
typedef struct Event { DWORD kind, a, b, c, d, e; } Event;
typedef struct Result {
    Proc process;
    BYTE context[0x1b34];
    BYTE objects[2][0x134];
    BYTE buffers[0x28f0];
    DWORD scalars[SCALAR_COUNT];
    BYTE draw_data[DRAW_BYTES];
    Event events[128];
    DWORD event_count, unexpected;
} Result;
typedef struct View { BYTE *module; const DWORD *rvas; } View;
static View active;
static Result *out;
static DWORD *cell(int key) { return (DWORD *)(active.module + active.rvas[key]); }
static void event(DWORD k,DWORD a,DWORD b,DWORD c,DWORD d,DWORD e)
{
    Event *r;
    if (out->event_count >= 128) { ++out->unexpected; return; }
    r = &out->events[out->event_count++];
    r->kind=k; r->a=a; r->b=b; r->c=c; r->d=d; r->e=e;
}
static DWORD primitive(void *p)
{
    DWORD i;
    for(i=0;i<DRAW_COUNT;i++) {
        DWORD base=(DWORD)active.module+active.rvas[DRAW_FIRST+i];
        if((DWORD)p>=base && (DWORD)p<base+draw_sizes[i])
            return (i<<16)|((DWORD)p-base);
    }
    ++out->unexpected; return 0xffffffff;
}
static int __cdecl mode(int n) { event(1,n,0,0,0,0); return 0; }
static int __cdecl enter_sound(void) { event(2,0,0,0,0,0); return 0; }
static int __cdecl leave_sound(void) { event(3,0,0,0,0,0); return 0; }
static int __cdecl exit_spy(void) { event(4,0,0,0,0,0); return 0; }
static int __cdecl page(int tp,int abr,int x,int y)
{ event(5,tp,abr,x,y,0); return 0; }
static int __cdecl init_page(void *p,int dfe,int dtd,int value)
{ event(6,primitive(p),dfe,dtd,value,0); return 0; }
static int __cdecl add_prim(DWORD *bucket,void *p)
{
    DWORD buffer=*(cell(G_BUFFER_CURRENT));
    event(7,((DWORD)bucket-buffer-0x70)/4,primitive(p),((BYTE *)p)[7],0,0);
    return 0;
}
static int __cdecl unexpected(void) { ++out->unexpected; event(99,0,0,0,0,0); return 0; }

static void setup(View v,Result *r,int selection,DWORD held,DWORD edge,DWORD repeat,DWORD flags)
{
    DWORD *table;
    DWORD i;
    active=v; out=r;
    memset(r,0,sizeof(*r));
    memset(r->objects,0xa5,sizeof(r->objects));
    /* Sound 0x302 rejects bank/sample 0xff before dispatching playback. */
    memset(r->context+0x2f0,0xff,0x800*3);
    *(WORD *)(r->context+0x5c)=(WORD)held;
    *(WORD *)(r->context+0x5e)=(WORD)edge;
    *(DWORD *)(r->context+0x30)=flags;
    r->process.state=2; r->process.delay=1;
    for(i=SCALAR_FIRST;i<SCALAR_FIRST+SCALAR_COUNT;i++) *cell(i)=0;
    *cell(G_BUSY)=1; /* Native normal busy flag skips object animation/sparkles. */
    *cell(G_PHASE)=2; *cell(G_BACKGROUND)=2; *cell(G_MIX)=0x80;
    *cell(G_SELECTION)=selection; *cell(G_REPEAT)=repeat; *cell(G_ACTION)=0x12345678;
    *cell(G_OBJECT_A)=(DWORD)&r->objects[0];
    *cell(G_OBJECT_B)=(DWORD)&r->objects[1];
    *cell(G_BUFFER_CURRENT)=(DWORD)r->buffers;
    *cell(G_BUFFER_PAIR)=(DWORD)r->buffers;
    for(i=0;i<DRAW_COUNT;i++) memset(active.module+active.rvas[DRAW_FIRST+i],0,draw_sizes[i]);
    table=cell(G_INTERFACE);
    for(i=0;i<250;i++) table[i]=(DWORD)unexpected;
    table[0]=0x5ac00cac; table[1]=(DWORD)r->context;
    table[0x224/4]=(DWORD)mode;
    table[0x178/4]=(DWORD)enter_sound; table[0x180/4]=(DWORD)leave_sound;
    table[0x238/4]=(DWORD)exit_spy;
    table[0x1f4/4]=(DWORD)page;
    table[0x2b4/4]=(DWORD)init_page;
    table[0x108/4]=(DWORD)add_prim;
}
static void run(View v,Result *r,DWORD fn,int selection,DWORD held,DWORD edge,DWORD repeat,DWORD flags)
{
    DWORD i,pos=0;
    setup(v,r,selection,held,edge,repeat,flags);
    ((MenuFn)(v.module+fn))(&r->process);
    for(i=0;i<SCALAR_COUNT;i++) r->scalars[i]=*cell(SCALAR_FIRST+i);
    for(i=0;i<DRAW_COUNT;i++) {
        memcpy(r->draw_data+pos,v.module+v.rvas[DRAW_FIRST+i],draw_sizes[i]);
        pos+=draw_sizes[i];
    }
}
static int check(int ordinal,View native,View linked,int selection,DWORD held,DWORD edge,DWORD repeat,DWORD flags)
{
    Result a,b;
    DWORD i;
    run(native,&a,NATIVE_MENU_RVA,selection,held,edge,repeat,flags);
    run(linked,&b,LINKED_MENU_RVA,selection,held,edge,repeat,flags);
    if(a.unexpected || b.unexpected) {
        printf("FAIL dependency case=%d native=%lu linked=%lu\n",ordinal,a.unexpected,b.unexpected); return 0;
    }
    {
        DWORD ea=0,eb=0;
        DWORD expected_exit=selection==3 && (edge&0x4008)!=0;
        for(i=0;i<a.event_count;i++) if(a.events[i].kind==4) ++ea;
        for(i=0;i<b.event_count;i++) if(b.events[i].kind==4) ++eb;
        if(ea!=expected_exit || eb!=expected_exit) {
            printf("FAIL exit invocation case=%d expected=%lu native=%lu linked=%lu\n",
                   ordinal,expected_exit,ea,eb); return 0;
        }
    }
    if(memcmp(&a,&b,sizeof(a))) {
        const BYTE *x=(const BYTE *)&a,*y=(const BYTE *)&b;
        for(i=0;i<sizeof(a);i++) if(x[i]!=y[i]) {
            printf("DIVERGENCE case=%d selection=%d held=%lx edge=%lx repeat=%lu flags=%lx byte=%lu original=%02x linked=%02x\n",
                   ordinal,selection,held,edge,repeat,flags,i,x[i],y[i]); break;
        }
        return 0;
    }
    printf("PASS case=%d selection=%d held=%lx edge=%lx events=%lu result_selection=%lu action=%lx state=%lu\n",
           ordinal,selection,held,edge,a.event_count,a.scalars[G_SELECTION-SCALAR_FIRST],
           a.scalars[G_ACTION-SCALAR_FIRST],a.process.state);
    return 1;
}
int main(int argc,char **argv)
{
    HMODULE n,l;
    View native,linked;
    int pass=1,ordinal=0,i;
    int selections[3]={0,1,3};
    DWORD holds[4]={0,0x10,0x40,0x4000};
    if(argc!=3) return 2;
    n=LoadLibraryA(argv[1]); l=LoadLibraryA(argv[2]);
    if(!n || !l) { printf("FAIL LoadLibrary error=%lu\n",GetLastError()); return 3; }
    if(n==l) { printf("FAIL native and linked modules share a loader handle\n"); return 3; }
    native.module=(BYTE *)n; native.rvas=native_rvas;
    linked.module=(BYTE *)l; linked.rvas=linked_rvas;
    for(i=0;i<3;i++) {
        int k;
        for(k=0;k<4;k++) pass &= check(++ordinal,native,linked,selections[i],holds[k],holds[k],0,0);
    }
    pass &= check(++ordinal,native,linked,1,0x40,0,0,0); /* held directional input remains active */
    pass &= check(++ordinal,native,linked,3,0x4000,0,0,0); /* confirm requires rising edge */
    pass &= check(++ordinal,native,linked,1,0x40,0x40,2,0); /* repeat delay suppresses navigation */
    pass &= check(++ordinal,native,linked,3,0x40,0x40,0,0x80000000); /* extra selection enabled by high flag */
    for(i=0;i<3;i++) pass &= check(++ordinal,native,linked,selections[i],8,8,0,0);
    pass &= check(++ordinal,native,linked,4,0x4000,0x4000,0,0x80000000);
    printf("menu differential: %s cases=%d compared_bytes_each=%lu\n",pass?"PASS":"FAIL",ordinal,(DWORD)sizeof(Result));
    FreeLibrary(l); FreeLibrary(n);
    return pass?0:1;
}
