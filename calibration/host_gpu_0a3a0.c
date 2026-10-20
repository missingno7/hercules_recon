/* HERCULES.EXE GPU emulation 0xa3a0..0xac60 (lane h2 candidate): PSX libgpu-style
 * primitive setters, texture-page words, VRAM upload/move and the order-table renderer.
 * Names provisional; the recorded packet setters 0xa4a0..0xa610 sit between these entries. */
#include <string.h>

typedef struct HostRect { short x, y, w, h; } HostRect;

extern unsigned char g_host_vram[];
extern int __cdecl host_zero_callback();
extern void __cdecl host_invoke_callback_zero(void);

#define TPAGE_HI(tp, abr, x, y)     ((((tp) & 0x3) << 9) | (((abr) & 0x3) << 7) | (((y) & 0x300) >> 3) | (((x) & 0x3ff) >> 6))
#define TPAGE_LO(tp, abr, x, y)     ((((tp) & 0x3) << 7) | (((abr) & 0x3) << 5) | (((y) & 0x100) >> 4) | (((x) & 0x3ff) >> 6) | (((y) & 0x200) << 2))
#define VRAM_PAGE(n) (((unsigned char (*)[0x10000])g_host_vram)[n])
#define VRAM_HIGH_ROW(y) ((((y) & 0x1ff) + 0x1000) << 8)
#define VRAM_LOW_ROW(y) ((((y) >= 0x100 ? 0x800 : 0) + ((y) & 0xff)) << 8)
#define VRAM_COLUMN(xb) ((((xb) & 0x700) << 8) + ((xb) & 0xff))

unsigned short __cdecl host_0a3a0(int x, int y)
{
    return (unsigned short)((y << 6) | ((x >> 4) & 0x3f));
}

unsigned short __cdecl host_0a3c0(void *ignored, int x, int y)
{
    return (unsigned short)((y << 6) | ((x >> 4) & 0x3f));
}

void __cdecl host_0a3e0(unsigned char *p)
{
    p[7] = 0x7c;
}

unsigned short __cdecl host_0a3f0(int tp, int abr, int x, int y)
{
    if (host_zero_callback() == 1 || host_zero_callback() == 2)
        return (unsigned short)TPAGE_HI(tp, abr, x, y);
    return (unsigned short)TPAGE_LO(tp, abr, x, y);
}

void __cdecl host_0a480(unsigned char *p, int abe)
{
    if (abe)
        p[7] |= 2;
    else
        p[7] &= ~2;
}

void __cdecl host_0a4c0(unsigned char *p, int dfe, int dtd, int tpage)
{
    unsigned long mode;
    if (host_zero_callback() == 1 || host_zero_callback() == 2)
        mode = (dfe ? 0x1000 : 0) | (dtd ? 0x800 : 0) | (tpage & 0x27ff);
    else
        mode = (dfe ? 0x400 : 0) | (dtd ? 0x200 : 0) | (tpage & 0x9ff);
    *(unsigned long *)(p + 4) = 0xe1000000 | mode;
}

void __cdecl host_0a710(HostRect *r, unsigned long *p);

unsigned short __cdecl host_0a640(unsigned char *pix, int tp, int abr, int x, int y, int w, int h)
{
    HostRect rect;
    rect.x = (short)x;
    rect.y = (short)y;
    rect.w = (short)w;
    rect.h = (short)h;
    host_0a710(&rect, (unsigned long *)pix);
    if (host_zero_callback() == 1 || host_zero_callback() == 2)
        return (unsigned short)TPAGE_HI(tp, abr, x, y);
    return (unsigned short)TPAGE_LO(tp, abr, x, y);
}

void __cdecl host_0a710(HostRect *r, unsigned long *p)
{
    unsigned char *src = (unsigned char *)p;
    int y, xb, row;
    if (r->y >= 0x200) {
        for (y = r->y; y < r->y + r->h; y++) {
            row = VRAM_HIGH_ROW(y);
            if ((((r->x + r->w - 1) ^ r->x) & ~0x7f) == 0) {
                xb = r->x * 2;
                memcpy(&g_host_vram[VRAM_COLUMN(xb) + row], src, r->w * 2);
                src += r->w * 2;
            } else {
                for (xb = r->x * 2; xb < (r->x + r->w) * 2; xb++)
                    g_host_vram[VRAM_COLUMN(xb) + row] = *src++;
            }
        }
    } else {
        for (y = r->y; y < r->y + r->h; y++) {
            row = VRAM_LOW_ROW(y);
            if ((((r->x + r->w - 1) ^ r->x) & ~0x7f) == 0) {
                xb = r->x * 2;
                memcpy(&g_host_vram[VRAM_COLUMN(xb) + row], src, r->w * 2);
                src += r->w * 2;
            } else {
                for (xb = r->x * 2; xb < (r->x + r->w) * 2; xb++)
                    g_host_vram[VRAM_COLUMN(xb) + row] = *src++;
            }
        }
    }
    host_invoke_callback_zero();
}

int __cdecl host_0a8f0(HostRect *r, int x, int y)
{
    int sy, sxb, dxb, srow, drow;
    if (r->y >= 0x200) {
        for (sy = r->y; sy < r->y + r->h; sy++, y++) {
            srow = VRAM_HIGH_ROW(sy);
            drow = VRAM_HIGH_ROW(y);
            if ((((r->x + r->w - 1) ^ r->x) & ~0x7f) == 0 && (((x + r->w - 1) ^ x) & ~0x7f) == 0) {
                sxb = r->x * 2;
                dxb = x * 2;
                memcpy(&g_host_vram[VRAM_COLUMN(dxb) + drow], &g_host_vram[VRAM_COLUMN(sxb) + srow], r->w * 2);
            } else {
                for (sxb = r->x * 2, dxb = x * 2; sxb < (r->x + r->w) * 2; sxb++, dxb++)
                    g_host_vram[VRAM_COLUMN(dxb) + drow] = g_host_vram[VRAM_COLUMN(sxb) + srow];
            }
        }
    } else {
        for (sy = r->y; sy < r->y + r->h; sy++, y++) {
            srow = VRAM_LOW_ROW(sy);
            drow = VRAM_LOW_ROW(y);
            if ((((r->x + r->w - 1) ^ r->x) & ~0x7f) == 0 && (((x + r->w - 1) ^ x) & ~0x7f) == 0) {
                sxb = r->x * 2;
                dxb = x * 2;
                memcpy(&g_host_vram[VRAM_COLUMN(dxb) + drow], &g_host_vram[VRAM_COLUMN(sxb) + srow], r->w * 2);
            } else {
                for (sxb = r->x * 2, dxb = x * 2; sxb < (r->x + r->w) * 2; sxb++, dxb++)
                    g_host_vram[VRAM_COLUMN(dxb) + drow] = g_host_vram[VRAM_COLUMN(sxb) + srow];
            }
        }
    }
    host_invoke_callback_zero();
    return 0;
}

void __cdecl host_0ac40(void)
{
    host_invoke_callback_zero();
}

/* Software rasterizer state (0xa909b0..0xa90a10) and texture-page state (0xa8e6f4..0xa8e6fc). */
extern long g_raster_clip_x0;       /* 0xa909b0 */
extern long g_raster_clip_y0;       /* 0xa909b4 */
extern long g_raster_x[4];          /* 0xa909b8 */
extern long g_raster_y[4];          /* 0xa909c8 */
extern long g_raster_u[4];          /* 0xa909d8 */
extern long g_raster_v[4];          /* 0xa909e8 */
extern long g_raster_count;         /* 0xa909f8 */
extern unsigned char *g_raster_texture; /* 0xa909fc */
extern long g_raster_color;         /* 0xa90a00 */
extern long g_raster_clut;          /* 0xa90a04 */
extern short g_raster_w;            /* 0xa90a0c */
extern short g_raster_h;            /* 0xa90a10 */
extern long g_raster_clip_x1;       /* 0x43a488 */
extern long g_raster_clip_y1;       /* 0x43a48c */
extern unsigned long g_tpage_page;           /* 0xa8e6f4 */
extern long g_tpage_abr;            /* 0xa8e6f8 */
extern long g_tpage_direct;         /* 0xa8e6fc */
extern unsigned char g_exe_ba01fc;  /* render suppression */
extern unsigned char g_exe_96e698, g_exe_96e699, g_exe_96e69a, g_exe_96e69b;

extern void __cdecl graphics_native_06ee0(unsigned char, unsigned char, unsigned char);
extern int __cdecl graphics_native_06ff0(void);
extern void __cdecl graphics_native_070c0(int);
extern int __cdecl graphics_native_07140(void);
extern void __cdecl graphics_native_06620(void);
extern void __cdecl host_0d420(void);
extern void __cdecl host_0e680(void);
extern void __cdecl host_0c8f0(void);
extern void __cdecl host_0cab0(void);

#define PB(o) (((unsigned char *)p)[o])
#define PS(o) (*(short *)((unsigned char *)p + (o)))
#define PW(o) (*(unsigned short *)((unsigned char *)p + (o)))

void __cdecl host_0ac60(unsigned long *ot)
{
    unsigned long *p, *next;
    unsigned int code;

    if (!g_exe_ba01fc) {
        if (g_exe_96e698)
            graphics_native_06ee0(g_exe_96e699, g_exe_96e69a, g_exe_96e69b);
        graphics_native_06ff0();
    }
    p = ot;
    next = ot + 1;
    for (;;) {
        p = (unsigned long *)*p;
        while (p == next) {
            p = (unsigned long *)*p;
            next++;
        }
        if (p == 0)
            break;
        code = PB(7);
        if ((code & 0xe0) == 0x20) {
            if (g_exe_ba01fc)
                continue;
            code &= ~2;
            if (code == 0x2c) {
                g_raster_x[0] = PS(0x08);
                g_raster_y[0] = PS(0x0a);
                g_raster_x[1] = PS(0x10);
                g_raster_y[1] = PS(0x12);
                g_raster_x[3] = PS(0x18);
                g_raster_y[3] = PS(0x1a);
                g_raster_x[2] = PS(0x20);
                g_raster_y[2] = PS(0x22);
                g_raster_v[0] = PB(0x0d) << 16;
                g_raster_u[0] = PB(0x0c) << 16;
                g_raster_v[1] = PB(0x15) << 16;
                g_raster_u[1] = PB(0x14) << 16;
                g_raster_v[3] = PB(0x1d) << 16;
                g_raster_u[3] = PB(0x1c) << 16;
                g_raster_v[2] = PB(0x25) << 16;
                g_raster_u[2] = PB(0x24) << 16;
                g_tpage_direct = (PW(0x16) & 0x180) == 0x180;
                g_tpage_page = PB(0x16) & 0x1f;
                g_raster_texture = VRAM_PAGE(g_tpage_page >> 1);
                if (g_tpage_direct)
                    g_raster_texture = VRAM_PAGE(16);
                g_tpage_abr = (PB(0x16) >> 5) & 3;
                g_raster_color = PB(4);
                g_raster_count = 4;
            } else if (code == 0x3c) {
                g_raster_x[0] = PS(0x08);
                g_raster_y[0] = PS(0x0a);
                g_raster_x[1] = PS(0x14);
                g_raster_y[1] = PS(0x16);
                g_raster_x[3] = PS(0x20);
                g_raster_y[3] = PS(0x22);
                g_raster_x[2] = PS(0x2c);
                g_raster_y[2] = PS(0x2e);
                g_raster_v[0] = PB(0x0d) << 16;
                g_raster_u[0] = PB(0x0c) << 16;
                g_raster_v[1] = PB(0x19) << 16;
                g_raster_u[1] = PB(0x18) << 16;
                g_raster_v[3] = PB(0x25) << 16;
                g_raster_u[3] = PB(0x24) << 16;
                g_raster_v[2] = PB(0x31) << 16;
                g_raster_u[2] = PB(0x30) << 16;
                g_tpage_direct = (PW(0x1a) & 0x180) == 0x180;
                g_tpage_page = PB(0x1a) & 0x1f;
                g_raster_texture = VRAM_PAGE(g_tpage_page >> 1);
                if (g_tpage_direct)
                    g_raster_texture = VRAM_PAGE(16);
                g_tpage_abr = (PB(0x1a) >> 5) & 3;
                g_raster_color = PB(4);
                g_raster_count = 4;
            } else if (code == 0x24) {
                g_raster_x[0] = PS(0x08);
                g_raster_y[0] = PS(0x0a);
                g_raster_x[1] = PS(0x10);
                g_raster_y[1] = PS(0x12);
                g_raster_x[3] = g_raster_x[2] = PS(0x18);
                g_raster_y[3] = g_raster_y[2] = PS(0x1a);
                g_raster_v[0] = PB(0x0d) << 16;
                g_raster_u[0] = PB(0x0c) << 16;
                g_raster_v[1] = PB(0x15) << 16;
                g_raster_u[1] = PB(0x14) << 16;
                g_raster_v[3] = g_raster_v[2] = PB(0x1d) << 16;
                g_raster_v[3] = g_raster_u[2] = PB(0x1c) << 16;
                g_tpage_direct = (PW(0x16) & 0x180) == 0x180;
                g_tpage_page = PB(0x16) & 0x1f;
                g_raster_texture = VRAM_PAGE(g_tpage_page >> 1);
                if (g_tpage_direct)
                    g_raster_texture = VRAM_PAGE(16);
                g_tpage_abr = (PB(0x16) >> 5) & 3;
                g_raster_color = PB(4);
                g_raster_count = 3;
            } else
                continue;
            g_raster_clut = PW(0x0e);
            if (PB(7) & 2)
                host_0e680();
            else
                host_0d420();
        } else if ((code & 0xe0) == 0x60) {
            if (g_exe_ba01fc)
                continue;
            g_raster_texture = VRAM_PAGE(g_tpage_page >> 1);
            if (g_tpage_direct)
                g_raster_texture = VRAM_PAGE(16);
            code &= ~2;
            if (code == 0x7c) {
                g_raster_x[0] = PS(0x08);
                g_raster_y[0] = PS(0x0a);
                g_raster_u[0] = PB(0x0c);
                g_raster_v[0] = PB(0x0d);
                g_raster_w = g_raster_h = 16;
            } else if (code == 0x74) {
                g_raster_x[0] = PS(0x08);
                g_raster_y[0] = PS(0x0a);
                g_raster_u[0] = PB(0x0c);
                g_raster_v[0] = PB(0x0d);
                g_raster_w = g_raster_h = 8;
            } else if (code == 0x64) {
                g_raster_x[0] = PS(0x08);
                g_raster_y[0] = PS(0x0a);
                g_raster_u[0] = PB(0x0c);
                g_raster_v[0] = PB(0x0d);
                g_raster_w = PW(0x10);
                g_raster_h = PW(0x12);
            } else
                continue;
            if (PB(7) & 2)
                host_0c8f0();
            else
                host_0cab0();
        } else if ((code & 0xe0) == 0x40) {
            if (g_exe_ba01fc)
                continue;
        } else if ((code & 0xe0) == 0xe0 || (code & 0xe0) == 0) {
            if (code == 0xe1) {
                g_tpage_direct = (p[1] & 0x180) == 0x180;
                g_tpage_page = p[1] & 0x1f;
                g_tpage_abr = (p[1] >> 5) & 3;
            } else if (code == 1) {
                host_0a8f0((HostRect *)((unsigned char *)p + 0x10), PS(0x0c), PS(0x0e));
            } else if (code == 0xf3) {
                g_raster_clip_x0 = PS(4);
                g_raster_clip_y0 = PS(6) & 0xff;
                if (g_raster_clip_y0 >= 0xf0)
                    g_raster_clip_y0 -= 0xf0;
                g_raster_clip_x1 = PS(8) + g_raster_clip_x0;
                g_raster_clip_y1 = PS(0x0a) + g_raster_clip_y0;
                if (g_raster_clip_x0 < 0)
                    g_raster_clip_x0 = 0;
                if (g_raster_clip_x0 > 0x140)
                    g_raster_clip_x0 = 0x140;
                if (g_raster_clip_x1 < 0)
                    g_raster_clip_x1 = 0;
                if (g_raster_clip_x1 > 0x140)
                    g_raster_clip_x1 = 0x140;
                if (g_raster_clip_y0 < 0)
                    g_raster_clip_y0 = 0;
                if (g_raster_clip_y0 > 0xf0)
                    g_raster_clip_y0 = 0xf0;
                if (g_raster_clip_y1 < 0)
                    g_raster_clip_y1 = 0;
                if (g_raster_clip_y1 > 0xf0)
                    g_raster_clip_y1 = 0xf0;
            }
        }
    }
    if (!g_exe_ba01fc) {
        graphics_native_070c0(0xff);
        graphics_native_07140();
        graphics_native_06620();
    }
    host_invoke_callback_zero();
}
