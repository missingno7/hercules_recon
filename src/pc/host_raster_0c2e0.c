/* HERCULES.EXE software rasterizer 0xc2e0..0xef5f (lane h5 candidate).
 *
 * One /O2 C unit (VC5 RTM) with MSVC inline __asm for the hand-written parts:
 *   C:     all early-outs, vertex min/max, clipping, edge walking, span clipping and the
 *          per-line advance; this code caches/reloads the file statics exactly like
 *          compiled code and is reproduced by the compiler.
 *   __asm: the inner pixel loops (rep stos fill; texel loops with ebp as a data register
 *          under push/pop ebp; u/v packed into register halves with add/adc carries) and
 *          the 2.30 fixed-point slope multiplies (imul m32 / shrd 30) that C cannot express
 *          without __int64 helpers.
 * Functions containing __asm get a frame pointer and save ebx/esi/edi; that is the only
 * reason these /O2 functions have push ebp/mov ebp,esp prologues.
 * Scan-conversion temporaries are file statics: only this code references
 * 0xa9091c..0xa909ac and 0xa90a08, and `static` is required for the observed register
 * allocation (0xc2e0 is masked equal only with them static). */

extern unsigned char g_exe_ba01f0;      /* software rendering enabled */
extern unsigned char *g_exe_ba0210;     /* frame buffer */
extern unsigned long g_exe_ba0214;      /* frame buffer pitch */
extern unsigned char g_exe_43a590;      /* frame dirty */

/* Rasterizer input state: written by the order-table renderer (0xac60) and the polygon
 * dispatchers here.  Explicit-zero definitions in this order reproduce the contiguous
 * 0xa909b0..0xa90a13 block that follows this unit's statics (layout evidence, not proof). */
long g_raster_clip_x0 = 0;              /* 0xa909b0 */
long g_raster_clip_y0 = 0;              /* 0xa909b4 */
long g_raster_x[4] = {0};               /* 0xa909b8 */
long g_raster_y[4] = {0};               /* 0xa909c8 */
long g_raster_u[4] = {0};               /* 0xa909d8: 16.16 (polygons) or texels (sprites) */
long g_raster_v[4] = {0};               /* 0xa909e8 */
long g_raster_count = 0;                /* 0xa909f8: vertices */
unsigned char *g_raster_texture = 0;    /* 0xa909fc: 256x256 texture page */
long g_raster_color = 0;                /* 0xa90a00: 0x80 = unshaded */
long g_raster_clut = 0;                 /* 0xa90a04: CLUT word, then remap row */
unsigned char g_raster_fill = 0;        /* 0xa90a08: flat colour (rasterizer only) */
short g_raster_w = 0;                   /* 0xa90a0c: sprite width */
short g_raster_h = 0;                   /* 0xa90a10: sprite height */
long g_raster_clip_x1 = 0x140;          /* 0x43a488 */
long g_raster_clip_y1 = 0xf0;           /* 0x43a48c */

extern long g_tpage_abr;                /* 0xa8e6f8 */
extern int g_exe_96e3e0;                /* CLUT remap rows differ */
extern int g_host_graphics_reciprocal_68e918[2048];  /* 0xa8e918: 0x40000000 / n */
/* 0x92c490 g_host_graphics_tables_52c490.remap[31][256], followed at 0x92e3d0 by the
 * average/add/subtract/half-add blend tables; 0x91c488 g_host_graphics_shade_51c488 */
extern unsigned char g_host_graphics_remap[31][256];
extern unsigned char g_host_graphics_blend[4][256][256];
extern unsigned char g_host_graphics_shade[256][256];

/* scan-conversion state */
static long g_edge_dul;                        /* 0xa9091c */
static long g_edge_dvl;                        /* 0xa90920 */
static long g_edge_dxl;                        /* 0xa90924 */
static long g_edge_dur;                        /* 0xa90928 */
static unsigned char *g_shade_row;      /* 0xa9092c */
static long g_edge_dvr;                        /* 0xa90930 */
static long g_edge_dxr;                        /* 0xa90934 */
static long g_span_du;                         /* 0xa90938 */
static long g_span_dv;                         /* 0xa9093c */
static unsigned char *g_span_dest;             /* 0xa90940 */
static unsigned char *g_row_dest;              /* 0xa90944 */
static unsigned char *g_blend_table;           /* 0xa90948 */
static long g_left;                            /* 0xa90950 */
static unsigned char *g_clut_table;            /* 0xa90954 */
static long g_ul;                              /* 0xa90958 */
static long g_vl;                              /* 0xa9095c */
static long g_xl;                              /* 0xa90960 */
static long g_left_end;                        /* 0xa90964 */
static long g_y;                               /* 0xa90968 */
static long g_n;                               /* 0xa9096c */
static long g_right;                           /* 0xa90970 */
static long g_x_end;                           /* 0xa90974 */
static unsigned char g_span_dv_int;            /* 0xa90978 */
static long g_dy;                              /* 0xa9097c */
static long g_ur;                              /* 0xa90980 */
static long g_vr;                              /* 0xa90984 */
static long g_xr;                              /* 0xa90988 */
static long g_right_end;                       /* 0xa9098c */
static long g_lines;                           /* 0xa909a4 */
static long g_span_u;                          /* 0xa90990 */
static long g_span_v;                          /* 0xa90994 */
static long g_span_frac_step;                    /* 0xa90998 */
static long g_span_int_step;            /* 0xa9099c */
static unsigned char *g_span_shade;     /* 0xa909a0 */
static long g_y_end;                           /* 0xa909a8 */
static unsigned char *g_shade_table;           /* 0xa909ac */

/* 0xc2e0: flat-shaded polygon (g_raster_count vertices, colour g_raster_fill) */
void __cdecl host_0c2e0(void)
{
    if (!g_exe_ba01f0 || !g_exe_ba0210)
        return;
    g_y_end = g_y = g_raster_y[0];
    g_xl = g_xr = g_raster_x[0];
    g_left = 0;
    for (g_n = 1; g_n < g_raster_count; g_n++) {
        if (g_raster_y[g_n] < g_y) {
            g_y = g_raster_y[g_n];
            g_left = g_n;
        } else if (g_raster_y[g_n] > g_y_end)
            g_y_end = g_raster_y[g_n];
        if (g_raster_x[g_n] < g_xl)
            g_xl = g_raster_x[g_n];
        else if (g_raster_x[g_n] > g_xr)
            g_xr = g_raster_x[g_n];
    }
    if (g_y_end - g_y >= 0x800 || g_xr - g_xl >= 0x800 || g_xr < 0 || g_xl >= 0x140 ||
        g_y >= g_raster_clip_y1 || g_y_end < g_raster_clip_y0)
        return;
    if (g_y < g_raster_clip_y0)
        g_y = g_raster_clip_y0;
    if (g_y_end > g_raster_clip_y1)
        g_y_end = g_raster_clip_y1;
    g_row_dest = g_exe_ba0210 + g_y * g_exe_ba0214;
    g_left_end = g_right_end = 0;
    g_right = g_left;
    g_raster_count--;
    while (g_y < g_y_end) {
        while (g_left_end <= g_y) {
            g_n = g_left - 1;
            if (g_n < 0)
                g_n = g_raster_count;
            g_dy = g_raster_y[g_n] - g_raster_y[g_left];
            if (g_dy != 0)
                g_edge_dxl = ((g_raster_x[g_n] - g_raster_x[g_left]) << 16) / g_dy;
            g_xl = (g_y - g_raster_y[g_left]) * g_edge_dxl + (g_raster_x[g_left] << 16);
            g_left = g_n;
            g_left_end = g_raster_y[g_n];
        }
        while (g_right_end <= g_y) {
            g_n = g_right + 1;
            if (g_n > g_raster_count)
                g_n = 0;
            g_dy = g_raster_y[g_n] - g_raster_y[g_right];
            if (g_dy != 0)
                g_edge_dxr = ((g_raster_x[g_n] - g_raster_x[g_right]) << 16) / g_dy;
            g_xr = (g_y - g_raster_y[g_right]) * g_edge_dxr + (g_raster_x[g_right] << 16);
            g_right = g_n;
            g_right_end = g_raster_y[g_n];
        }
        g_n = g_left_end < g_right_end ? g_left_end : g_right_end;
        g_lines = g_n - g_y;
        g_y = g_n;
        if (g_n > g_y_end)
            g_lines += g_y_end - g_n;
        while (g_lines > 0) {
            if (g_xr > g_xl) {
                g_n = g_xl >> 16;
                g_x_end = g_xr >> 16;
            } else {
                g_n = g_xr >> 16;
                g_x_end = g_xl >> 16;
            }
            if (g_n < 0)
                g_n = 0;
            g_span_dest = g_row_dest + g_n;
            if (g_x_end > 0x140)
                g_x_end = 0x140;
            g_n = g_x_end - g_n;
            if (g_n > 0) {
                __asm {
                    mov     ecx, g_n
                    mov     al, byte ptr g_raster_fill
                    cld
                    cmp     ecx, 4
                    mov     edi, g_span_dest
                    jle     tail
                    mov     ah, al              ; replicate the colour into all four bytes
                    mov     dx, ax
                    shl     eax, 16
                    mov     ax, dx
                    mov     edx, ecx
                    mov     esi, edi            ; bytes up to the next dword boundary
                    and     esi, 3
                    mov     ecx, 4
                    sub     ecx, esi
                    sub     edx, ecx
                    rep     stosb
                    mov     ecx, edx
                    shr     ecx, 2
                    rep     stosd
                    mov     ecx, edx
                    and     ecx, 3
                tail:
                    rep     stosb
                }
            }
            g_lines--;
            g_row_dest += g_exe_ba0214;
            g_xl += g_edge_dxl;
            g_xr += g_edge_dxr;
        }
    }
    g_exe_43a590 = 1;
}


/* 0xc700: shaded semi-transparent sprite (g_raster_w x g_raster_h texels at u[0], v[0]) */
void __cdecl host_0c700(void)
{
    long x1, y1, v;

    if (!g_exe_ba01f0 || !g_exe_ba0210)
        return;
    if (g_raster_x[0] >= 0x140 || g_raster_y[0] >= g_raster_clip_y1)
        return;
    x1 = g_raster_x[0] + g_raster_w;
    if (x1 < 0)
        return;
    y1 = g_raster_y[0] + g_raster_h;
    if (y1 < 0)
        return;
    if (g_raster_x[0] < 0) {
        g_raster_w += (short)g_raster_x[0];
        g_raster_u[0] -= g_raster_x[0];
        g_raster_x[0] = 0;
    }
    if (g_raster_y[0] < 0) {
        g_raster_h += (short)g_raster_y[0];
        g_raster_v[0] -= g_raster_y[0];
        g_raster_y[0] = 0;
    }
    if (x1 >= 0x140)
        g_raster_w += (short)(0x140 - x1);
    if (y1 >= g_raster_clip_y1)
        g_raster_h += (short)(g_raster_clip_y1 - y1);
    if (g_raster_w <= 0 || g_raster_h <= 0)
        return;
    g_row_dest = g_exe_ba0210 + g_raster_y[0] * g_exe_ba0214 + g_raster_x[0];
    g_blend_table = g_host_graphics_blend[g_tpage_abr & 3][0];
    g_shade_table = g_host_graphics_shade[g_raster_color & 0xff];
    g_raster_texture += g_raster_u[0];
    v = g_raster_v[0] << 8;
    do {
        __asm {
            push    ebp
            xor     eax, eax
            mov     ecx, g_blend_table
            xor     ebx, ebx
            mov     esi, g_raster_texture
            add     esi, v                  ; texture row
            mov     bx, g_raster_w
            mov     edi, g_row_dest
            add     esi, ebx                ; run both pointers to the row end
            add     edi, ebx
            mov     ebp, g_shade_table      ; ebp is a data register from here on
            neg     ebx
        texel:
            mov     al, [esi+ebx]
            test    al, al                  ; texel 0 is transparent
            je      skip
            and     eax, 0xff
            mov     ah, [ebp+eax]           ; shade the texel
            mov     al, [edi+ebx]
            mov     al, [ecx+eax]           ; blend with the destination
            mov     [edi+ebx], al
        skip:
            inc     ebx
            jl      texel
            pop     ebp
        }
        g_raster_h--;
        g_row_dest += g_exe_ba0214;
        v = (v + 0x100) & 0xffff;
    } while (g_raster_h > 0);
    g_exe_43a590 = 1;
}

/* 0xc8f0: semi-transparent sprite */
void __cdecl host_0c8f0(void)
{
    long x1, y1, v;

    if (!g_exe_ba01f0 || !g_exe_ba0210)
        return;
    if (g_raster_x[0] >= 0x140 || g_raster_y[0] >= g_raster_clip_y1)
        return;
    x1 = g_raster_x[0] + g_raster_w;
    if (x1 < 0)
        return;
    y1 = g_raster_y[0] + g_raster_h;
    if (y1 < 0)
        return;
    if (g_raster_x[0] < 0) {
        g_raster_w += (short)g_raster_x[0];
        g_raster_u[0] -= g_raster_x[0];
        g_raster_x[0] = 0;
    }
    if (g_raster_y[0] < 0) {
        g_raster_h += (short)g_raster_y[0];
        g_raster_v[0] -= g_raster_y[0];
        g_raster_y[0] = 0;
    }
    if (x1 >= 0x140)
        g_raster_w += (short)(0x140 - x1);
    if (y1 >= g_raster_clip_y1)
        g_raster_h += (short)(g_raster_clip_y1 - y1);
    if (g_raster_w <= 0 || g_raster_h <= 0)
        return;
    g_row_dest = g_exe_ba0210 + g_raster_y[0] * g_exe_ba0214 + g_raster_x[0];
    g_raster_texture += g_raster_u[0];
    g_blend_table = g_host_graphics_blend[g_tpage_abr & 3][0];
    v = g_raster_v[0] << 8;
    do {
        __asm {
            xor     eax, eax
            mov     ecx, g_blend_table
            xor     ebx, ebx
            mov     esi, g_raster_texture
            add     esi, v
            mov     bx, g_raster_w
            mov     edi, g_row_dest
            add     esi, ebx
            add     edi, ebx
            neg     ebx
        texel:
            mov     ah, [esi+ebx]
            test    ah, ah
            je      skip
            mov     al, [edi+ebx]
            mov     al, [ecx+eax]
            mov     [edi+ebx], al
        skip:
            inc     ebx
            jl      texel
        }
        g_raster_h--;
        g_row_dest += g_exe_ba0214;
        v = (v + 0x100) & 0xffff;
    } while (g_raster_h > 0);
    g_exe_43a590 = 1;
}

/* 0xcab0: opaque sprite with transparent texel 0 */
void __cdecl host_0cab0(void)
{
    long x1, y1, v;

    if (!g_exe_ba01f0 || !g_exe_ba0210)
        return;
    if (g_raster_x[0] >= 0x140 || g_raster_y[0] >= g_raster_clip_y1)
        return;
    x1 = g_raster_x[0] + g_raster_w;
    if (x1 < 0)
        return;
    y1 = g_raster_y[0] + g_raster_h;
    if (y1 < 0)
        return;
    if (g_raster_x[0] < 0) {
        g_raster_w += (short)g_raster_x[0];
        g_raster_u[0] -= g_raster_x[0];
        g_raster_x[0] = 0;
    }
    if (g_raster_y[0] < 0) {
        g_raster_h += (short)g_raster_y[0];
        g_raster_v[0] -= g_raster_y[0];
        g_raster_y[0] = 0;
    }
    if (x1 >= 0x140)
        g_raster_w += (short)(0x140 - x1);
    if (y1 >= g_raster_clip_y1)
        g_raster_h += (short)(g_raster_clip_y1 - y1);
    if (g_raster_w <= 0 || g_raster_h <= 0)
        return;
    g_row_dest = g_exe_ba0210 + g_raster_y[0] * g_exe_ba0214 + g_raster_x[0];
    g_raster_texture += g_raster_u[0];
    v = g_raster_v[0] << 8;
    do {
        __asm {
            xor     ebx, ebx
            mov     esi, g_raster_texture
            add     esi, v
            mov     bx, g_raster_w
            mov     edi, g_row_dest
            add     esi, ebx
            add     edi, ebx
            neg     ebx
        texel:
            mov     al, [esi+ebx]
            test    al, al
            je      skip
            mov     [edi+ebx], al
        skip:
            inc     ebx
            jl      texel
        }
        g_raster_h--;
        g_row_dest += g_exe_ba0214;
        v = (v + 0x100) & 0xffff;
    } while (g_raster_h > 0);
    g_exe_43a590 = 1;
}

/* 0xcc50: CLUT-textured polygon, affine u/v */
void __cdecl host_0cc50(void)
{
    if (!g_exe_ba01f0 || !g_exe_ba0210)
        return;
    g_clut_table = g_host_graphics_remap[g_raster_clut - 1];
    g_y_end = g_y = g_raster_y[0];
    g_xl = g_xr = g_raster_x[0];
    g_left = 0;
    for (g_n = 1; g_n < g_raster_count; g_n++) {
        if (g_raster_y[g_n] < g_y) {
            g_left = g_n;
            g_y = g_raster_y[g_n];
        } else if (g_raster_y[g_n] > g_y_end)
            g_y_end = g_raster_y[g_n];
        if (g_raster_x[g_n] < g_xl)
            g_xl = g_raster_x[g_n];
        else if (g_raster_x[g_n] > g_xr)
            g_xr = g_raster_x[g_n];
    }
    if (g_y_end - g_y >= 0x800 || g_xr - g_xl >= 0x800 || g_xr < 0 || g_xl >= 0x140 ||
        g_y >= g_raster_clip_y1 || g_y_end < g_raster_clip_y0)
        return;
    if (g_y < g_raster_clip_y0)
        g_y = g_raster_clip_y0;
    if (g_y_end > g_raster_clip_y1)
        g_y_end = g_raster_clip_y1;
    g_row_dest = g_exe_ba0210 + g_y * g_exe_ba0214;
    g_right = g_left;
    g_left_end = g_right_end = 0;
    g_raster_count--;
    while (g_y < g_y_end) {
        while (g_left_end <= g_y) {
            g_n = g_left - 1;
            if (g_n < 0)
                g_n = g_raster_count;
            g_dy = g_raster_y[g_n] - g_raster_y[g_left];
            if (g_dy != 0) {
                g_edge_dxl = (g_raster_x[g_n] - g_raster_x[g_left]) << 16;
                g_edge_dul = g_raster_u[g_n] - g_raster_u[g_left];
                g_edge_dvl = g_raster_v[g_n] - g_raster_v[g_left];
                g_dy = g_host_graphics_reciprocal_68e918[g_dy & 0x7ff];
                __asm {
                    mov     eax, g_edge_dxl         ; slope = delta * (1 << 30) / dy
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dxl, eax
                    mov     eax, g_edge_dul
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dul, eax
                    mov     eax, g_edge_dvl
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dvl, eax
                }
            }
            g_ul = g_raster_u[g_left];
            g_vl = g_raster_v[g_left];
            g_xl = g_raster_x[g_left] << 16;
            g_dy = g_y - g_raster_y[g_left];
            if (g_dy != 0) {
                g_xl += g_dy * g_edge_dxl;
                g_ul += g_dy * g_edge_dul;
                g_vl += g_dy * g_edge_dvl;
            }
            g_left = g_n;
            g_left_end = g_raster_y[g_n];
        }
        while (g_right_end <= g_y) {
            g_n = g_right + 1;
            if (g_n > g_raster_count)
                g_n = 0;
            g_dy = g_raster_y[g_n] - g_raster_y[g_right];
            if (g_dy != 0) {
                g_edge_dxr = (g_raster_x[g_n] - g_raster_x[g_right]) << 16;
                g_edge_dur = g_raster_u[g_n] - g_raster_u[g_right];
                g_edge_dvr = g_raster_v[g_n] - g_raster_v[g_right];
                g_dy = g_host_graphics_reciprocal_68e918[g_dy & 0x7ff];
                __asm {
                    mov     eax, g_edge_dxr
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dxr, eax
                    mov     eax, g_edge_dur
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dur, eax
                    mov     eax, g_edge_dvr
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dvr, eax
                }
            }
            g_ur = g_raster_u[g_right];
            g_vr = g_raster_v[g_right];
            g_xr = g_raster_x[g_right] << 16;
            g_dy = g_y - g_raster_y[g_right];
            if (g_dy != 0) {
                g_xr += g_dy * g_edge_dxr;
                g_ur += g_dy * g_edge_dur;
                g_vr += g_dy * g_edge_dvr;
            }
            g_right = g_n;
            g_right_end = g_raster_y[g_n];
        }
        g_n = g_left_end < g_right_end ? g_left_end : g_right_end;
        g_lines = g_n - g_y;
        g_y = g_n;
        if (g_n > g_y_end)
            g_lines += g_y_end - g_n;
        while (g_lines > 0) {
            if (g_xr > g_xl) {
                g_dy = g_xr >> 16;
                g_n = g_xl >> 16;
                g_span_du = g_ur - g_ul;
                g_span_u = g_ul;
                g_span_dv = g_vr - g_vl;
                g_span_v = g_vl;
            } else {
                g_dy = g_xl >> 16;
                g_n = g_xr >> 16;
                g_span_du = g_ul - g_ur;
                g_span_u = g_ur;
                g_span_dv = g_vl - g_vr;
                g_span_v = g_vr;
            }
            g_x_end = g_dy - g_n;
            if (g_x_end != 0 && g_dy >= 0 && g_n < 0x140) {
                g_dy = g_host_graphics_reciprocal_68e918[g_x_end & 0x7ff];
                __asm {
                    mov     eax, g_span_du
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_span_du, eax
                    mov     eax, g_span_dv
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_span_dv, eax
                }
                g_x_end += g_n;
                if (g_n < 0) {
                    g_span_u -= g_n * g_span_du;
                    g_span_v -= g_n * g_span_dv;
                    g_n = 0;
                }
                g_span_dest = g_row_dest + g_n;
                if (g_x_end > 0x140)
                    g_x_end = 0x140;
                g_n = g_x_end - g_n;
                if (g_n > 0) {
                    g_span_frac_step = g_span_du << 16;
                    g_span_dv_int = (unsigned char)(g_span_dv >> 16);
                    __asm {
                        push    ebp
                        mov     eax, g_span_u       ; eax: u fraction | pixel count
                        shl     eax, 16
                        mov     edx, g_n
                        mov     ax, dx
                        mov     ebx, g_span_v       ; ebx: v fraction | u integer
                        shl     ebx, 16
                        mov     edx, g_span_u
                        shr     edx, 16
                        mov     bl, dl
                        mov     ecx, g_span_dv      ; ecx: dv fraction | v integer | du integer
                        shl     ecx, 16
                        mov     edx, g_span_du
                        shr     edx, 16
                        mov     cl, dl
                        mov     edx, g_span_v
                        shr     edx, 16
                        mov     ch, dl
                        mov     edi, g_span_dest
                        mov     esi, g_raster_texture
                        mov     ebp, g_clut_table   ; ebp is a data register from here on
                        mov     edx, 0
                    texel:
                        mov     dl, bl
                        add     eax, g_span_frac_step
                        adc     ebx, ecx
                        mov     dh, ch
                        mov     dl, [esi+edx]       ; texture[v][u]
                        adc     ch, g_span_dv_int
                        mov     dh, 0
                        test    dl, dl              ; texel 0 is transparent
                        je      skip
                        mov     dl, [ebp+edx]       ; CLUT
                        mov     [edi], dl
                    skip:
                        inc     edi
                        dec     ax
                        jg      texel
                        pop     ebp
                    }
                }
            }
            g_lines--;
            g_row_dest += g_exe_ba0214;
            g_xl += g_edge_dxl;
            g_xr += g_edge_dxr;
            g_ul += g_edge_dul;
            g_vl += g_edge_dvl;
            g_ur += g_edge_dur;
            g_vr += g_edge_dvr;
        }
    }
    g_exe_43a590 = 1;
}

/* 0xd420: textured polygon: sprite, CLUT, average-colour or direct texels */
void __cdecl host_0d420(void)
{
    if (!g_exe_ba01f0 || !g_exe_ba0210)
        return;
    if (g_raster_count == 4 && g_raster_x[0] <= g_raster_x[2] && g_raster_y[0] <= g_raster_y[2] &&
        g_raster_y[0] == g_raster_y[1] && g_raster_y[2] == g_raster_y[3] &&
        g_raster_x[0] == g_raster_x[3] && g_raster_x[1] == g_raster_x[2] &&
        g_raster_u[0] <= g_raster_u[2] && g_raster_v[0] <= g_raster_v[2] &&
        g_raster_v[0] == g_raster_v[1] && g_raster_v[2] == g_raster_v[3] &&
        g_raster_u[0] == g_raster_u[3] && g_raster_u[1] == g_raster_u[2]) {
        /* an axis-aligned quad with unscaled texture is drawn as a sprite */
        g_raster_w = (short)(g_raster_x[1] - g_raster_x[0]);
        g_raster_h = (short)(g_raster_y[3] - g_raster_y[0]);
        if ((g_raster_u[2] - g_raster_u[0]) >> 16 == g_raster_w &&
            (g_raster_v[2] - g_raster_v[0]) >> 16 == g_raster_h) {
            g_raster_u[0] >>= 16;
            g_raster_v[0] >>= 16;
            host_0cab0();
            return;
        }
    }
    g_raster_clut = (g_raster_clut >> 6) - 0x1e0;
    if ((unsigned long)g_raster_clut > 0x1e)
        g_raster_clut = 0;
    if (g_exe_96e3e0 && g_raster_clut) {
        host_0cc50();
        return;
    }
    if (g_raster_clut) {
        /* without distinct CLUTs the polygon is filled with its average texel */
        g_raster_fill = g_raster_texture[((g_raster_v[0] + g_raster_v[1] + g_raster_v[2] + g_raster_v[3]) >> 18 << 8) +
                                         ((g_raster_u[0] + g_raster_u[1] + g_raster_u[2] + g_raster_u[3]) >> 18)];
        if (g_exe_96e3e0)
            g_raster_fill = g_host_graphics_remap[g_raster_clut][g_raster_fill];
        host_0c2e0();
        return;
    }
    g_y_end = g_y = g_raster_y[0];
    g_xl = g_xr = g_raster_x[0];
    g_left = 0;
    for (g_n = 1; g_n < g_raster_count; g_n++) {
        if (g_raster_y[g_n] < g_y) {
            g_left = g_n;
            g_y = g_raster_y[g_n];
        } else if (g_raster_y[g_n] > g_y_end)
            g_y_end = g_raster_y[g_n];
        if (g_raster_x[g_n] < g_xl)
            g_xl = g_raster_x[g_n];
        else if (g_raster_x[g_n] > g_xr)
            g_xr = g_raster_x[g_n];
    }
    if (g_y_end - g_y >= 0x800 || g_xr - g_xl >= 0x800 || g_xr < 0 || g_xl >= 0x140 ||
        g_y >= g_raster_clip_y1 || g_y_end < g_raster_clip_y0)
        return;
    if (g_y < g_raster_clip_y0)
        g_y = g_raster_clip_y0;
    if (g_y_end > g_raster_clip_y1)
        g_y_end = g_raster_clip_y1;
    g_row_dest = g_exe_ba0210 + g_y * g_exe_ba0214;
    g_right = g_left;
    g_left_end = g_right_end = 0;
    g_raster_count--;
    while (g_y < g_y_end) {
        while (g_left_end <= g_y) {
            g_n = g_left - 1;
            if (g_n < 0)
                g_n = g_raster_count;
            g_dy = g_raster_y[g_n] - g_raster_y[g_left];
            if (g_dy != 0) {
                g_edge_dxl = (g_raster_x[g_n] - g_raster_x[g_left]) << 16;
                g_edge_dul = g_raster_u[g_n] - g_raster_u[g_left];
                g_edge_dvl = g_raster_v[g_n] - g_raster_v[g_left];
                g_dy = g_host_graphics_reciprocal_68e918[g_dy & 0x7ff];
                __asm {
                    mov     eax, g_edge_dxl         ; slope = delta * (1 << 30) / dy
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dxl, eax
                    mov     eax, g_edge_dul
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dul, eax
                    mov     eax, g_edge_dvl
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dvl, eax
                }
            }
            g_ul = g_raster_u[g_left];
            g_vl = g_raster_v[g_left];
            g_xl = g_raster_x[g_left] << 16;
            g_dy = g_y - g_raster_y[g_left];
            if (g_dy != 0) {
                g_xl += g_dy * g_edge_dxl;
                g_ul += g_dy * g_edge_dul;
                g_vl += g_dy * g_edge_dvl;
            }
            g_left = g_n;
            g_left_end = g_raster_y[g_n];
        }
        while (g_right_end <= g_y) {
            g_n = g_right + 1;
            if (g_n > g_raster_count)
                g_n = 0;
            g_dy = g_raster_y[g_n] - g_raster_y[g_right];
            if (g_dy != 0) {
                g_edge_dxr = (g_raster_x[g_n] - g_raster_x[g_right]) << 16;
                g_edge_dur = g_raster_u[g_n] - g_raster_u[g_right];
                g_edge_dvr = g_raster_v[g_n] - g_raster_v[g_right];
                g_dy = g_host_graphics_reciprocal_68e918[g_dy & 0x7ff];
                __asm {
                    mov     eax, g_edge_dxr
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dxr, eax
                    mov     eax, g_edge_dur
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dur, eax
                    mov     eax, g_edge_dvr
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dvr, eax
                }
            }
            g_ur = g_raster_u[g_right];
            g_vr = g_raster_v[g_right];
            g_xr = g_raster_x[g_right] << 16;
            g_dy = g_y - g_raster_y[g_right];
            if (g_dy != 0) {
                g_xr += g_dy * g_edge_dxr;
                g_ur += g_dy * g_edge_dur;
                g_vr += g_dy * g_edge_dvr;
            }
            g_right = g_n;
            g_right_end = g_raster_y[g_n];
        }
        g_n = g_left_end < g_right_end ? g_left_end : g_right_end;
        g_lines = g_n - g_y;
        g_y = g_n;
        if (g_n > g_y_end)
            g_lines += g_y_end - g_n;
        while (g_lines > 0) {
            if (g_xr > g_xl) {
                g_dy = g_xr >> 16;
                g_n = g_xl >> 16;
                g_span_du = g_ur - g_ul;
                g_span_u = g_ul;
                g_span_dv = g_vr - g_vl;
                g_span_v = g_vl;
            } else {
                g_dy = g_xl >> 16;
                g_n = g_xr >> 16;
                g_span_du = g_ul - g_ur;
                g_span_u = g_ur;
                g_span_dv = g_vl - g_vr;
                g_span_v = g_vr;
            }
            g_x_end = g_dy - g_n;
            if (g_x_end != 0 && g_dy >= 0 && g_n < 0x140) {
                g_dy = g_host_graphics_reciprocal_68e918[g_x_end & 0x7ff];
                __asm {
                    mov     eax, g_span_du
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_span_du, eax
                    mov     eax, g_span_dv
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_span_dv, eax
                }
                g_x_end += g_n;
                if (g_n < 0) {
                    g_span_u -= g_n * g_span_du;
                    g_span_v -= g_n * g_span_dv;
                    g_n = 0;
                }
                g_span_dest = g_row_dest + g_n;
                if (g_x_end > 0x140)
                    g_x_end = 0x140;
                g_n = g_x_end - g_n;
                if (g_n > 0) {
                    __asm {
                        push    ebp
                        mov     ebx, g_span_u       ; ebx: u fraction
                        shl     ebx, 16
                        mov     ecx, g_span_u       ; cl: u integer, ch: v integer
                        shr     ecx, 16
                        mov     eax, g_span_v
                        shr     eax, 16
                        mov     ch, al
                        mov     edx, g_span_du      ; edx: du fraction | du integer, dh: dv integer
                        rol     edx, 16
                        mov     eax, g_span_dv
                        shr     eax, 16
                        mov     dh, al
                        mov     esi, g_raster_texture
                        mov     edi, g_span_dest
                        dec     edi
                        mov     ebp, g_span_dv      ; ebp: dv fraction | pixel count
                        shl     ebp, 16
                        mov     eax, g_n
                        mov     bp, ax
                        mov     eax, g_span_v       ; eax: v fraction | texel
                        shl     eax, 16
                    texel:
                        mov     al, [esi+ecx]       ; texture[v][u]
                        add     ebx, edx
                        adc     cl, dl
                        inc     edi
                        test    al, al              ; texel 0 is transparent
                        je      skip
                        mov     [edi], al
                    skip:
                        add     eax, ebp
                        adc     ch, dh
                        dec     bp
                        jg      texel
                        pop     ebp
                    }
                }
            }
            g_lines--;
            g_row_dest += g_exe_ba0214;
            g_xl += g_edge_dxl;
            g_xr += g_edge_dxr;
            g_ul += g_edge_dul;
            g_vl += g_edge_dvl;
            g_ur += g_edge_dur;
            g_vr += g_edge_dvr;
        }
    }
    g_exe_43a590 = 1;
}

/* 0xdd70: shaded semi-transparent textured polygon */
void __cdecl host_0dd70(void)
{
    if (!g_exe_ba01f0 || !g_exe_ba0210)
        return;
    if (g_raster_count == 4 && g_raster_x[0] <= g_raster_x[2] && g_raster_y[0] <= g_raster_y[2] &&
        g_raster_y[0] == g_raster_y[1] && g_raster_y[2] == g_raster_y[3] &&
        g_raster_x[0] == g_raster_x[3] && g_raster_x[1] == g_raster_x[2] &&
        g_raster_u[0] <= g_raster_u[2] && g_raster_v[0] <= g_raster_v[2] &&
        g_raster_v[0] == g_raster_v[1] && g_raster_v[2] == g_raster_v[3] &&
        g_raster_u[0] == g_raster_u[3] && g_raster_u[1] == g_raster_u[2]) {
        /* an axis-aligned quad with unscaled texture is drawn as a sprite */
        g_raster_w = (short)(g_raster_x[1] - g_raster_x[0]);
        g_raster_h = (short)(g_raster_y[3] - g_raster_y[0]);
        if ((g_raster_u[2] - g_raster_u[0]) >> 16 == g_raster_w &&
            (g_raster_v[2] - g_raster_v[0]) >> 16 == g_raster_h) {
            g_raster_u[0] >>= 16;
            g_raster_v[0] >>= 16;
            host_0c700();
            return;
        }
    }
    g_blend_table = g_host_graphics_blend[g_tpage_abr & 3][0];
    g_shade_row = g_host_graphics_shade[g_raster_color & 0xff];
    g_y_end = g_y = g_raster_y[0];
    g_xl = g_xr = g_raster_x[0];
    g_left = 0;
    for (g_n = 1; g_n < g_raster_count; g_n++) {
        if (g_raster_y[g_n] < g_y) {
            g_left = g_n;
            g_y = g_raster_y[g_n];
        } else if (g_raster_y[g_n] > g_y_end)
            g_y_end = g_raster_y[g_n];
        if (g_raster_x[g_n] < g_xl)
            g_xl = g_raster_x[g_n];
        else if (g_raster_x[g_n] > g_xr)
            g_xr = g_raster_x[g_n];
    }
    if (g_y_end - g_y >= 0x800 || g_xr - g_xl >= 0x800 || g_xr < 0 || g_xl >= 0x140 ||
        g_y >= g_raster_clip_y1 || g_y_end < g_raster_clip_y0)
        return;
    if (g_y < g_raster_clip_y0)
        g_y = g_raster_clip_y0;
    if (g_y_end > g_raster_clip_y1)
        g_y_end = g_raster_clip_y1;
    g_row_dest = g_exe_ba0210 + g_y * g_exe_ba0214;
    g_right = g_left;
    g_left_end = g_right_end = 0;
    g_raster_count--;
    while (g_y < g_y_end) {
        while (g_left_end <= g_y) {
            g_n = g_left - 1;
            if (g_n < 0)
                g_n = g_raster_count;
            g_dy = g_raster_y[g_n] - g_raster_y[g_left];
            if (g_dy != 0) {
                g_edge_dxl = (g_raster_x[g_n] - g_raster_x[g_left]) << 16;
                g_edge_dul = g_raster_u[g_n] - g_raster_u[g_left];
                g_edge_dvl = g_raster_v[g_n] - g_raster_v[g_left];
                g_dy = g_host_graphics_reciprocal_68e918[g_dy & 0x7ff];
                __asm {
                    mov     eax, g_edge_dxl         ; slope = delta * (1 << 30) / dy
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dxl, eax
                    mov     eax, g_edge_dul
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dul, eax
                    mov     eax, g_edge_dvl
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dvl, eax
                }
            }
            g_ul = g_raster_u[g_left];
            g_vl = g_raster_v[g_left];
            g_xl = g_raster_x[g_left] << 16;
            g_dy = g_y - g_raster_y[g_left];
            if (g_dy != 0) {
                g_xl += g_dy * g_edge_dxl;
                g_ul += g_dy * g_edge_dul;
                g_vl += g_dy * g_edge_dvl;
            }
            g_left = g_n;
            g_left_end = g_raster_y[g_n];
        }
        while (g_right_end <= g_y) {
            g_n = g_right + 1;
            if (g_n > g_raster_count)
                g_n = 0;
            g_dy = g_raster_y[g_n] - g_raster_y[g_right];
            if (g_dy != 0) {
                g_edge_dxr = (g_raster_x[g_n] - g_raster_x[g_right]) << 16;
                g_edge_dur = g_raster_u[g_n] - g_raster_u[g_right];
                g_edge_dvr = g_raster_v[g_n] - g_raster_v[g_right];
                g_dy = g_host_graphics_reciprocal_68e918[g_dy & 0x7ff];
                __asm {
                    mov     eax, g_edge_dxr
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dxr, eax
                    mov     eax, g_edge_dur
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dur, eax
                    mov     eax, g_edge_dvr
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dvr, eax
                }
            }
            g_ur = g_raster_u[g_right];
            g_vr = g_raster_v[g_right];
            g_xr = g_raster_x[g_right] << 16;
            g_dy = g_y - g_raster_y[g_right];
            if (g_dy != 0) {
                g_xr += g_dy * g_edge_dxr;
                g_ur += g_dy * g_edge_dur;
                g_vr += g_dy * g_edge_dvr;
            }
            g_right = g_n;
            g_right_end = g_raster_y[g_n];
        }
        g_n = g_left_end < g_right_end ? g_left_end : g_right_end;
        g_lines = g_n - g_y;
        g_y = g_n;
        if (g_n > g_y_end)
            g_lines += g_y_end - g_n;
        while (g_lines > 0) {
            if (g_xr > g_xl) {
                g_dy = g_xr >> 16;
                g_n = g_xl >> 16;
                g_span_du = g_ur - g_ul;
                g_span_u = g_ul;
                g_span_dv = g_vr - g_vl;
                g_span_v = g_vl;
            } else {
                g_dy = g_xl >> 16;
                g_n = g_xr >> 16;
                g_span_du = g_ul - g_ur;
                g_span_u = g_ur;
                g_span_dv = g_vl - g_vr;
                g_span_v = g_vr;
            }
            g_x_end = g_dy - g_n;
            if (g_x_end != 0 && g_dy >= 0 && g_n < 0x140) {
                g_dy = g_host_graphics_reciprocal_68e918[g_x_end & 0x7ff];
                __asm {
                    mov     eax, g_span_du
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_span_du, eax
                    mov     eax, g_span_dv
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_span_dv, eax
                }
                g_x_end += g_n;
                if (g_n < 0) {
                    g_span_u -= g_n * g_span_du;
                    g_span_v -= g_n * g_span_dv;
                    g_n = 0;
                }
                g_span_dest = g_row_dest + g_n;
                if (g_x_end > 0x140)
                    g_x_end = 0x140;
                g_n = g_x_end - g_n;
                if (g_n > 0) {
                    g_span_frac_step = g_span_dv << 16;
                    g_span_int_step = g_span_du >> 16;
                    g_span_shade = g_shade_row;
                    __asm {
                        push    ebp
                        mov     eax, g_span_v       ; eax: v fraction | pixel count
                        shl     eax, 16
                        mov     edx, g_n
                        mov     ax, dx
                        mov     ebx, g_span_u       ; ebx: u fraction | v integer
                        shl     ebx, 16
                        mov     edx, g_span_v
                        shr     edx, 16
                        mov     bl, dl
                        mov     ecx, g_span_du      ; ecx: du fraction | dv integer
                        shl     ecx, 16
                        mov     edx, g_span_dv
                        shr     edx, 16
                        mov     cl, dl
                        mov     edi, g_span_dest
                        mov     esi, g_raster_texture   ; esi: texture column
                        mov     edx, g_span_u
                        shr     edx, 16
                        add     esi, edx
                        mov     ebp, g_blend_table  ; ebp is a data register from here on
                        mov     edx, 0
                    texel:
                        mov     dh, bl
                        add     eax, g_span_frac_step
                        adc     ebx, ecx
                        mov     dl, 0
                        mov     dh, [esi+edx]       ; texture[v][u]
                        adc     esi, g_span_int_step
                        test    dh, dh              ; texel 0 is transparent
                        je      skip
                        mov     dl, dh              ; shade the texel
                        and     edx, 0xff
                        add     edx, g_span_shade
                        mov     dh, [edx]
                        and     edx, 0xffff
                        mov     dl, [edi]
                        mov     dl, [ebp+edx]       ; blend[texel][destination]
                        mov     [edi], dl
                    skip:
                        inc     edi
                        dec     ax
                        jg      texel
                        pop     ebp
                    }
                }
            }
            g_lines--;
            g_row_dest += g_exe_ba0214;
            g_xl += g_edge_dxl;
            g_xr += g_edge_dxr;
            g_ul += g_edge_dul;
            g_vl += g_edge_dvl;
            g_ur += g_edge_dur;
            g_vr += g_edge_dvr;
        }
    }
    g_exe_43a590 = 1;
}

/* 0xe680: semi-transparent textured polygon */
void __cdecl host_0e680(void)
{
    if (!g_exe_ba01f0 || !g_exe_ba0210)
        return;
    if (g_raster_color != 0x80) {
        host_0dd70();
        return;
    }
    if (g_raster_count == 4 && g_raster_x[0] <= g_raster_x[2] && g_raster_y[0] <= g_raster_y[2] &&
        g_raster_y[0] == g_raster_y[1] && g_raster_y[2] == g_raster_y[3] &&
        g_raster_x[0] == g_raster_x[3] && g_raster_x[1] == g_raster_x[2] &&
        g_raster_u[0] <= g_raster_u[2] && g_raster_v[0] <= g_raster_v[2] &&
        g_raster_v[0] == g_raster_v[1] && g_raster_v[2] == g_raster_v[3] &&
        g_raster_u[0] == g_raster_u[3] && g_raster_u[1] == g_raster_u[2]) {
        /* an axis-aligned quad with unscaled texture is drawn as a sprite */
        g_raster_w = (short)(g_raster_x[1] - g_raster_x[0]);
        g_raster_h = (short)(g_raster_y[3] - g_raster_y[0]);
        if ((g_raster_u[2] - g_raster_u[0]) >> 16 == g_raster_w &&
            (g_raster_v[2] - g_raster_v[0]) >> 16 == g_raster_h) {
            g_raster_u[0] >>= 16;
            g_raster_v[0] >>= 16;
            host_0c8f0();
            return;
        }
    }
    g_blend_table = g_host_graphics_blend[g_tpage_abr & 3][0];
    g_y_end = g_y = g_raster_y[0];
    g_xl = g_xr = g_raster_x[0];
    g_left = 0;
    for (g_n = 1; g_n < g_raster_count; g_n++) {
        if (g_raster_y[g_n] < g_y) {
            g_left = g_n;
            g_y = g_raster_y[g_n];
        } else if (g_raster_y[g_n] > g_y_end)
            g_y_end = g_raster_y[g_n];
        if (g_raster_x[g_n] < g_xl)
            g_xl = g_raster_x[g_n];
        else if (g_raster_x[g_n] > g_xr)
            g_xr = g_raster_x[g_n];
    }
    if (g_y_end - g_y >= 0x800 || g_xr - g_xl >= 0x800 || g_xr < 0 || g_xl >= 0x140 ||
        g_y >= g_raster_clip_y1 || g_y_end < g_raster_clip_y0)
        return;
    if (g_y < g_raster_clip_y0)
        g_y = g_raster_clip_y0;
    if (g_y_end > g_raster_clip_y1)
        g_y_end = g_raster_clip_y1;
    g_row_dest = g_exe_ba0210 + g_y * g_exe_ba0214;
    g_right = g_left;
    g_left_end = g_right_end = 0;
    g_raster_count--;
    while (g_y < g_y_end) {
        while (g_left_end <= g_y) {
            g_n = g_left - 1;
            if (g_n < 0)
                g_n = g_raster_count;
            g_dy = g_raster_y[g_n] - g_raster_y[g_left];
            if (g_dy != 0) {
                g_edge_dxl = (g_raster_x[g_n] - g_raster_x[g_left]) << 16;
                g_edge_dul = g_raster_u[g_n] - g_raster_u[g_left];
                g_edge_dvl = g_raster_v[g_n] - g_raster_v[g_left];
                g_dy = g_host_graphics_reciprocal_68e918[g_dy & 0x7ff];
                __asm {
                    mov     eax, g_edge_dxl         ; slope = delta * (1 << 30) / dy
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dxl, eax
                    mov     eax, g_edge_dul
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dul, eax
                    mov     eax, g_edge_dvl
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dvl, eax
                }
            }
            g_ul = g_raster_u[g_left];
            g_vl = g_raster_v[g_left];
            g_xl = g_raster_x[g_left] << 16;
            g_dy = g_y - g_raster_y[g_left];
            if (g_dy != 0) {
                g_xl += g_dy * g_edge_dxl;
                g_ul += g_dy * g_edge_dul;
                g_vl += g_dy * g_edge_dvl;
            }
            g_left = g_n;
            g_left_end = g_raster_y[g_n];
        }
        while (g_right_end <= g_y) {
            g_n = g_right + 1;
            if (g_n > g_raster_count)
                g_n = 0;
            g_dy = g_raster_y[g_n] - g_raster_y[g_right];
            if (g_dy != 0) {
                g_edge_dxr = (g_raster_x[g_n] - g_raster_x[g_right]) << 16;
                g_edge_dur = g_raster_u[g_n] - g_raster_u[g_right];
                g_edge_dvr = g_raster_v[g_n] - g_raster_v[g_right];
                g_dy = g_host_graphics_reciprocal_68e918[g_dy & 0x7ff];
                __asm {
                    mov     eax, g_edge_dxr
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dxr, eax
                    mov     eax, g_edge_dur
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dur, eax
                    mov     eax, g_edge_dvr
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_edge_dvr, eax
                }
            }
            g_ur = g_raster_u[g_right];
            g_vr = g_raster_v[g_right];
            g_xr = g_raster_x[g_right] << 16;
            g_dy = g_y - g_raster_y[g_right];
            if (g_dy != 0) {
                g_xr += g_dy * g_edge_dxr;
                g_ur += g_dy * g_edge_dur;
                g_vr += g_dy * g_edge_dvr;
            }
            g_right = g_n;
            g_right_end = g_raster_y[g_n];
        }
        g_n = g_left_end < g_right_end ? g_left_end : g_right_end;
        g_lines = g_n - g_y;
        g_y = g_n;
        if (g_n > g_y_end)
            g_lines += g_y_end - g_n;
        while (g_lines > 0) {
            if (g_xr > g_xl) {
                g_dy = g_xr >> 16;
                g_n = g_xl >> 16;
                g_span_du = g_ur - g_ul;
                g_span_u = g_ul;
                g_span_dv = g_vr - g_vl;
                g_span_v = g_vl;
            } else {
                g_dy = g_xl >> 16;
                g_n = g_xr >> 16;
                g_span_du = g_ul - g_ur;
                g_span_u = g_ur;
                g_span_dv = g_vl - g_vr;
                g_span_v = g_vr;
            }
            g_x_end = g_dy - g_n;
            if (g_x_end != 0 && g_dy >= 0 && g_n < 0x140) {
                g_dy = g_host_graphics_reciprocal_68e918[g_x_end & 0x7ff];
                __asm {
                    mov     eax, g_span_du
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_span_du, eax
                    mov     eax, g_span_dv
                    imul    g_dy
                    shrd    eax, edx, 30
                    mov     g_span_dv, eax
                }
                g_x_end += g_n;
                if (g_n < 0) {
                    g_span_u -= g_n * g_span_du;
                    g_span_v -= g_n * g_span_dv;
                    g_n = 0;
                }
                g_span_dest = g_row_dest + g_n;
                if (g_x_end > 0x140)
                    g_x_end = 0x140;
                g_n = g_x_end - g_n;
                if (g_n > 0) {
                    g_span_frac_step = g_span_dv << 16;
                    g_span_int_step = g_span_du >> 16;
                    __asm {
                        push    ebp
                        mov     eax, g_span_v       ; eax: v fraction | pixel count
                        shl     eax, 16
                        mov     edx, g_n
                        mov     ax, dx
                        mov     ebx, g_span_u       ; ebx: u fraction | v integer
                        shl     ebx, 16
                        mov     edx, g_span_v
                        shr     edx, 16
                        mov     bl, dl
                        mov     ecx, g_span_du      ; ecx: du fraction | dv integer
                        shl     ecx, 16
                        mov     edx, g_span_dv
                        shr     edx, 16
                        mov     cl, dl
                        mov     edi, g_span_dest
                        mov     esi, g_raster_texture   ; esi: texture column
                        mov     edx, g_span_u
                        shr     edx, 16
                        add     esi, edx
                        mov     ebp, g_blend_table  ; ebp is a data register from here on
                        mov     edx, 0
                    texel:
                        mov     dh, bl
                        add     eax, g_span_frac_step
                        adc     ebx, ecx
                        mov     dl, 0
                        mov     dh, [esi+edx]       ; texture[v][u]
                        adc     esi, g_span_int_step
                        test    dh, dh              ; texel 0 is transparent
                        je      skip
                        mov     dl, [edi]
                        mov     dl, [ebp+edx]       ; blend[texel][destination]
                        mov     [edi], dl
                    skip:
                        inc     edi
                        dec     ax
                        jg      texel
                        pop     ebp
                    }
                }
            }
            g_lines--;
            g_row_dest += g_exe_ba0214;
            g_xl += g_edge_dxl;
            g_xr += g_edge_dxr;
            g_ul += g_edge_dul;
            g_vl += g_edge_dvl;
            g_ur += g_edge_dur;
            g_vr += g_edge_dvr;
        }
    }
    g_exe_43a590 = 1;
}
