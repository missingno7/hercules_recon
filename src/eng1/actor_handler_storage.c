/* Typed loader-zero cells used by the high actor handlers. */
typedef struct Entry16 { long x,y,z,time; } Entry16;
typedef struct Cursor {
 Entry16 *base;
 long index,count,remaining,last,extra;
 long step,step2,step3;
 long ax,bx,cx,dx,ay,by,cy,dy,az,bz,cz,dz;
 long x,vx,ddx,dddx,y,vy,ddy,dddy,z,vz,ddz,dddz;
} Cursor;
typedef char cursor_size[(sizeof(Cursor)==132)?1:-1];
Cursor g_eng1_cursor_701b8;
long g_eng1_native_dword_7019c,g_eng1_native_state_7028c;
