/* Private ENG1 storage-provider candidate; no original source/TU identity claim.
 * The actor source's g_spawn_gate is the low-byte view of g_clock at 0x6fe30;
 * compile that consumer with /Dg_spawn_gate=g_clock to preserve one storage cell.
 * g_control_argument_prefix is only the observed 12-byte start prefix; total
 * capacity and host-reader bounds remain unknown. */
int g_context_sequence_phase;
int g_context_sequence_index;
unsigned short *g_context_sequence_words;

char *g_control_forwarded_value = "\\TITLE\\ENGINE\\TITLE";
unsigned char g_control_argument_prefix[12];

long g_busy;
long g_effect_phase;
long g_effect_speed;
long g_countdown;
unsigned long g_clock;
unsigned char *g_collision_context;
