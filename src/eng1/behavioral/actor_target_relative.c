/* ENG1 2ad70. Reuse the measured shared/relative.c estimator with the second
 * position at actor+d8. DWORD subtraction wraps before signed high-WORD
 * extraction; the original signed-short -32768/absolute-value behavior stays. */
typedef struct Position { long x,y,z; } Position;
typedef struct ActorTarget {
    unsigned char pad000[0xd8];
    Position target;
} ActorTarget;
typedef struct Measure {
    short x,y,z,planar_distance,distance;
    unsigned short direction;
} Measure;
typedef char relative_long_is_32[sizeof(long)==4?1:-1];
typedef char measure_size_is_12[sizeof(Measure)==12?1:-1];
typedef char target_prefix_size_is_e4[sizeof(ActorTarget)==0xe4?1:-1];

void __cdecl eng1_native_code_target_002ad70(Position *a,ActorTarget *b,Measure *out)
{
    short x,y,z;
    int temp;
    unsigned short direction=4;
    x=(long)((unsigned long)a->x-(unsigned long)b->target.x)>>16;
    y=(long)((unsigned long)a->y-(unsigned long)b->target.y)>>16;
    z=(long)((unsigned long)a->z-(unsigned long)b->target.z)>>16;
    if(x<0){x=~x+1;direction=6;}
    if(y<0){y=~y+1;direction-=4;}
    if(z<0){z=~z+1;direction|=8;}
    out->x=x;out->y=y;out->z=z;
    if(x<y){temp=x;x=y;y=temp;++direction;}
    out->planar_distance=x+(y>>3)+(y>>2);
    if(x<z){temp=x;x=z;z=temp;}
    out->distance=x+11*(y>>5)+(z>>2);
    out->direction=direction;
}
