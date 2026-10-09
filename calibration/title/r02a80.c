/* TITLE 0x2a80: initializes both display buffers' drawing and display environments. Its unit is
   open: it lies between the 0x1de0..0x28c0 unit and the 0x2b70..0x4b70 unit (u02b70.c). */
#include "title_screen.h"
#include "title_gpu.h"

void title_02a80(void)
{
    title_0cab0(&g_2cc08[0].draw, 0, 0, 0x140, 0x100);
    title_0cab0(&g_2cc08[1].draw, 0, 0x100, 0x140, 0x100);
    title_0ca80(&g_2cc08[0].disp, 0, 0x100, 0x140, 0xf0);
    title_0ca80(&g_2cc08[1].disp, 0, 0, 0x140, 0xf0);
    g_2cc08[1].disp.screen.x = 0;
    g_2cc08[0].disp.screen.x = 0;
    g_2cc08[1].disp.screen.y = 0;
    g_2cc08[0].disp.screen.y = 0;
    g_2cc08[1].disp.screen.h = 0xf0;
    g_2cc08[0].disp.screen.h = 0xf0;
    g_2cc08[1].disp.screen.w = 0;
    g_2cc08[0].disp.screen.w = 0;
}

