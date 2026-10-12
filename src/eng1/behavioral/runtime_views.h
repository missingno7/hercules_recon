#ifndef HERCULES_ENG1_RUNTIME_VIEWS_H
#define HERCULES_ENG1_RUNTIME_VIEWS_H
/* Include after legacy declarations. Each expression addresses its actual
 * shared owner, rather than introducing another array or byte cell. */
extern unsigned char g_control_argument_prefix[12];
extern unsigned long g_clock;
#define g_control_argument_6eef8 (g_control_argument_prefix)
#define g_control_argument_6eefc (g_control_argument_prefix + 4)
#define g_control_argument_6ef00 (g_control_argument_prefix + 8)
#define g_spawn_gate (*(unsigned char *)&g_clock)
#endif
