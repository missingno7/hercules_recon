#include <string.h>
/* HERCULES.EXE GTE (geometry) emulation 0xb2f0..0xbcd0 (lane h2 candidate).
 * PSX libgte-style entries reached through the host interface table.
 * Names provisional; one aggregate is required (pointer-relative cross-field access in 0xb320). */

typedef struct HostMatrix {
    short m[3][3];
    long t[3];
} HostMatrix;

typedef struct HostVector { long vx, vy, vz, pad; } HostVector;
typedef struct HostSVector { short vx, vy, vz, pad; } HostSVector;

/* 0xa8e708: vertex registers, MAC results, screen XY, offsets, projection, rotation */
typedef struct HostGte {
    short v[3][3];      /* 0x708: [axis][vertex] */
    long mac[3][3];     /* 0x71c: [axis][vertex] */
    short sxy[2][3];    /* 0x740: [axis][vertex] */
    long ofx;           /* 0x74c */
    long ofy;           /* 0x750 */
    long h;             /* 0x754 */
    long mac0;          /* 0x758 */
    short rot[3][3];    /* 0x75c */
    HostVector trans;   /* 0x770; pad doubles as a mode flag (0xbd60/0xbd70) */
} HostGte;

extern HostGte g_host_gte;
extern short *g_exe_4361cc;

void __cdecl host_0b2f0(HostVector *out)
{
    out->vx = g_host_gte.mac[0][0];
    out->vy = g_host_gte.mac[1][0];
    out->vz = g_host_gte.mac[2][0];
}

void __cdecl host_0b310(long *out)
{
    *out = g_host_gte.mac[2][0];
}

void __cdecl host_0b320(void)
{
    int i;
    for (i = 0; i < 3; i++) {
        long x, y, z, h;
        x = (short)(((g_host_gte.rot[0][0] * g_host_gte.v[0][i] + g_host_gte.rot[0][1] * g_host_gte.v[1][i]
              + g_host_gte.rot[0][2] * g_host_gte.v[2][i]) >> 12) + g_host_gte.trans.vx);
        g_host_gte.mac[0][i] = x;
        y = (short)(((g_host_gte.rot[1][0] * g_host_gte.v[0][i] + g_host_gte.rot[1][1] * g_host_gte.v[1][i]
              + g_host_gte.rot[1][2] * g_host_gte.v[2][i]) >> 12) + g_host_gte.trans.vy);
        g_host_gte.mac[1][i] = y;
        z = (short)(((g_host_gte.rot[2][0] * g_host_gte.v[0][i] + g_host_gte.rot[2][1] * g_host_gte.v[1][i]
              + g_host_gte.rot[2][2] * g_host_gte.v[2][i]) >> 12) + g_host_gte.trans.vz);
        g_host_gte.mac[2][i] = z;
        h = g_host_gte.h << 16;
        if (z != 0)
            h /= z;
        g_host_gte.sxy[0][i] = (short)((h * g_host_gte.mac[0][i] >> 16) + g_host_gte.ofx);
        g_host_gte.sxy[1][i] = (short)((h * g_host_gte.mac[1][i] >> 16) + g_host_gte.ofy);
    }
}

void __cdecl host_0b430(HostSVector *v0, HostSVector *v1, HostSVector *v2)
{
    g_host_gte.v[0][0] = v0->vx;
    g_host_gte.v[1][0] = v0->vy;
    g_host_gte.v[2][0] = v0->vz;
    g_host_gte.v[0][1] = v1->vx;
    g_host_gte.v[1][1] = v1->vy;
    g_host_gte.v[2][1] = v1->vz;
    g_host_gte.v[0][2] = v2->vx;
    g_host_gte.v[1][2] = v2->vy;
    g_host_gte.v[2][2] = v2->vz;
}

void __cdecl host_0b4a0(long *out)
{
    *out = g_host_gte.mac0;
}

void __cdecl host_0b4b0(void)
{
    g_host_gte.mac0 = g_host_gte.sxy[0][0] * g_host_gte.sxy[1][1]
        + g_host_gte.sxy[0][1] * g_host_gte.sxy[1][2]
        + g_host_gte.sxy[0][2] * g_host_gte.sxy[1][0]
        - g_host_gte.sxy[0][0] * g_host_gte.sxy[1][2]
        - g_host_gte.sxy[0][1] * g_host_gte.sxy[1][0]
        - g_host_gte.sxy[0][2] * g_host_gte.sxy[1][1];
}

void __cdecl host_0b520(unsigned char *p)
{
    *(short *)(p + 0x08) = g_host_gte.sxy[0][0];
    *(short *)(p + 0x0a) = g_host_gte.sxy[1][0];
    *(short *)(p + 0x0c) = g_host_gte.sxy[0][1];
    *(short *)(p + 0x0e) = g_host_gte.sxy[1][1];
    *(short *)(p + 0x10) = g_host_gte.sxy[0][2];
    *(short *)(p + 0x12) = g_host_gte.sxy[1][2];
}

void __cdecl host_0b570(unsigned char *p)
{
    *(short *)(p + 0x08) = g_host_gte.sxy[0][0];
    *(short *)(p + 0x0a) = g_host_gte.sxy[1][0];
    *(short *)(p + 0x10) = g_host_gte.sxy[0][1];
    *(short *)(p + 0x12) = g_host_gte.sxy[1][1];
    *(short *)(p + 0x18) = g_host_gte.sxy[0][2];
    *(short *)(p + 0x1a) = g_host_gte.sxy[1][2];
}

void __cdecl host_0b5c0(unsigned char *p)
{
    *(short *)(p + 0x08) = g_host_gte.sxy[0][0];
    *(short *)(p + 0x0a) = g_host_gte.sxy[1][0];
    *(short *)(p + 0x10) = g_host_gte.sxy[0][1];
    *(short *)(p + 0x12) = g_host_gte.sxy[1][1];
    *(short *)(p + 0x18) = g_host_gte.sxy[0][2];
    *(short *)(p + 0x1a) = g_host_gte.sxy[1][2];
}

void __cdecl host_0b610(unsigned char *p)
{
    *(short *)(p + 0x08) = g_host_gte.sxy[0][0];
    *(short *)(p + 0x0a) = g_host_gte.sxy[1][0];
    *(short *)(p + 0x0c) = g_host_gte.sxy[0][1];
    *(short *)(p + 0x0e) = g_host_gte.sxy[1][1];
    *(short *)(p + 0x10) = g_host_gte.sxy[0][2];
    *(short *)(p + 0x12) = g_host_gte.sxy[1][2];
}

void __cdecl host_0b660(unsigned char *p)
{
    *(short *)(p + 0x08) = g_host_gte.sxy[0][0];
    *(short *)(p + 0x0a) = g_host_gte.sxy[1][0];
    *(short *)(p + 0x14) = g_host_gte.sxy[0][1];
    *(short *)(p + 0x16) = g_host_gte.sxy[1][1];
    *(short *)(p + 0x20) = g_host_gte.sxy[0][2];
    *(short *)(p + 0x22) = g_host_gte.sxy[1][2];
}

void __cdecl host_0b6b0(long sxy0, long sxy1, long sxy2)
{
    g_host_gte.sxy[0][0] = (short)sxy0;
    g_host_gte.sxy[1][0] = (short)(sxy0 >> 16);
    g_host_gte.sxy[0][1] = (short)sxy1;
    g_host_gte.sxy[1][1] = (short)(sxy1 >> 16);
    g_host_gte.sxy[0][2] = (short)sxy2;
    g_host_gte.sxy[1][2] = (short)(sxy2 >> 16);
}

void __cdecl host_0b6f0(HostSVector *v0)
{
    g_host_gte.v[0][0] = v0->vx;
    g_host_gte.v[1][0] = v0->vy;
    g_host_gte.v[2][0] = v0->vz;
}

void __cdecl host_0b720(short *out)
{
    out[0] = g_host_gte.sxy[0][0];
    out[1] = g_host_gte.sxy[1][0];
}

void __cdecl host_0b740(void)
{
    long vx, vz, vy;
    vx = g_host_gte.v[0][0];
    vy = g_host_gte.v[1][0];
    vz = g_host_gte.v[2][0];
    g_host_gte.mac[0][0] = (short)(((g_host_gte.rot[0][0] * vx + g_host_gte.rot[0][1] * vy
        + g_host_gte.rot[0][2] * vz) >> 12) + g_host_gte.trans.vx);
    g_host_gte.mac[1][0] = (short)(((g_host_gte.rot[1][0] * vx + g_host_gte.rot[1][1] * vy
        + g_host_gte.rot[1][2] * vz) >> 12) + g_host_gte.trans.vy);
    g_host_gte.mac[2][0] = (short)(((g_host_gte.rot[2][0] * vx + g_host_gte.rot[2][1] * vy
        + g_host_gte.rot[2][2] * vz) >> 12) + g_host_gte.trans.vz);
    if (g_host_gte.mac[2][0] < 1)
        g_host_gte.mac[2][0] = 1;
}

void __cdecl host_0b810(HostSVector *out)
{
    out->vx = (short)g_host_gte.mac[0][0];
    out->vy = (short)g_host_gte.mac[1][0];
    out->vz = (short)g_host_gte.mac[2][0];
}

void __cdecl host_0b840(void)
{
    g_host_gte.trans.vx = g_host_gte.mac[0][0];
    g_host_gte.trans.vy = g_host_gte.mac[1][0];
    g_host_gte.trans.vz = g_host_gte.mac[2][0];
}

void __cdecl host_0b870(short *out)
{
    *out = (short)(g_host_gte.mac[2][0] >> 2);
}

void __cdecl host_0b880(HostMatrix *m)
{
    memcpy(g_host_gte.rot, m->m, sizeof(g_host_gte.rot));
}

void __cdecl host_0b8c0(HostVector *t)
{
    memcpy(&g_host_gte.trans, t, sizeof(g_host_gte.trans));
}

void __cdecl host_0b8f0(HostMatrix *m)
{
    memcpy(&g_host_gte.trans, m->t, sizeof(m->t));
}

void __cdecl host_0b920(void)
{
    long h;
    host_0b740();
    h = g_host_gte.h << 16;
    if (g_host_gte.mac[2][0] != 0)
        h /= g_host_gte.mac[2][0];
    g_host_gte.sxy[0][0] = (short)((h * g_host_gte.mac[0][0] >> 16) + g_host_gte.ofx);
    g_host_gte.sxy[1][0] = (short)((h * g_host_gte.mac[1][0] >> 16) + g_host_gte.ofy);
}

void __cdecl host_0b970(long ofx, long ofy)
{
    g_host_gte.ofx = ofx;
    g_host_gte.ofy = ofy;
}

void __cdecl host_0b990(long h)
{
    g_host_gte.h = h;
}

void __cdecl host_0b9a0(HostMatrix *m)
{
    host_0b880(m);
}

void __cdecl host_0b9b0(HostMatrix *m)
{
    host_0b8f0(m);
}

HostMatrix *__cdecl host_0b9c0(HostMatrix *m, HostVector *t)
{
    m->t[0] = t->vx;
    m->t[1] = t->vy;
    m->t[2] = t->vz;
    return m;
}

HostMatrix *__cdecl host_0b9e0(HostSVector *r, HostMatrix *m)
{
    long s0, c0, s1, c1, s2, c2;
    short t1, t2;
    s0 = g_exe_4361cc[r->vx & 0xfff];
    c0 = g_exe_4361cc[(r->vx - 0x400) & 0xfff];
    s1 = g_exe_4361cc[r->vy & 0xfff];
    c1 = g_exe_4361cc[(r->vy - 0x400) & 0xfff];
    s2 = g_exe_4361cc[r->vz & 0xfff];
    c2 = g_exe_4361cc[(r->vz - 0x400) & 0xfff];
    m->m[1][2] = (short)(-(c0 * s1) >> 12);
    m->m[2][2] = (short)((s1 * s0) >> 12);
    m->m[0][0] = (short)((s2 * s1) >> 12);
    m->m[0][2] = (short)c1;
    m->m[0][1] = (short)(-(c2 * s1) >> 12);
    t1 = (short)((c1 * c0) >> 12);
    m->m[1][1] = (short)((s2 * s0 - c2 * t1) >> 12);
    t2 = (short)(-(c1 * s0) >> 12);
    m->m[1][0] = (short)((s2 * t1 + c2 * s0) >> 12);
    m->m[2][1] = (short)((c0 * s2 - c2 * t2) >> 12);
    m->m[2][0] = (short)((s2 * t2 + c2 * c0) >> 12);
    return m;
}

HostMatrix *__cdecl host_0bb00(HostSVector *r, HostMatrix *m)
{
    long s0, c0, s1, c1, s2, c2;
    short t1, t2;
    s0 = g_exe_4361cc[r->vx & 0xfff];
    c0 = g_exe_4361cc[(r->vx - 0x400) & 0xfff];
    s1 = g_exe_4361cc[r->vy & 0xfff];
    c1 = g_exe_4361cc[(r->vy - 0x400) & 0xfff];
    s2 = g_exe_4361cc[r->vz & 0xfff];
    c2 = g_exe_4361cc[(r->vz - 0x400) & 0xfff];
    m->m[0][2] = (short)((c1 * s0) >> 12);
    m->m[1][2] = (short)-c0;
    m->m[2][2] = (short)((s1 * s0) >> 12);
    t1 = (short)((c1 * c0) >> 12);
    m->m[0][0] = (short)((c2 * t1 + s2 * s1) >> 12);
    m->m[0][1] = (short)((s2 * t1 - c2 * s1) >> 12);
    m->m[1][0] = (short)((c2 * s0) >> 12);
    m->m[1][1] = (short)((s2 * s0) >> 12);
    t2 = (short)((c0 * s1) >> 12);
    m->m[2][0] = (short)((c2 * t2 - c1 * s2) >> 12);
    m->m[2][1] = (short)((s2 * t2 + c2 * c1) >> 12);
    return m;
}

HostMatrix *__cdecl host_0bc30(HostMatrix *m, HostVector *v)
{
    m->m[0][0] = (short)((m->m[0][0] * v->vx) >> 12);
    m->m[0][1] = (short)((m->m[0][1] * v->vy) >> 12);
    m->m[0][2] = (short)((m->m[0][2] * v->vz) >> 12);
    m->m[1][0] = (short)((m->m[1][0] * v->vx) >> 12);
    m->m[1][1] = (short)((m->m[1][1] * v->vy) >> 12);
    m->m[1][2] = (short)((m->m[1][2] * v->vz) >> 12);
    m->m[2][0] = (short)((m->m[2][0] * v->vx) >> 12);
    m->m[2][1] = (short)((m->m[2][1] * v->vy) >> 12);
    m->m[2][2] = (short)((m->m[2][2] * v->vz) >> 12);
    return m;
}

long __cdecl host_0bcd0(long ignored, long value)
{
    return value;
}
