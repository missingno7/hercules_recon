#include "host_bootstrap_text.h"
/* Observed PC startup text/cell; original TU and complete zero-text extent unproved. */
char g_host_empty_text_3f74c[1] = {0};
char g_host_default_engine_name_35160[20] = "\\TITLE\\ENGINE\\TITLE";
const char *g_host_default_engine_name_cell_35100 = g_host_default_engine_name_35160;

typedef char default_engine_name_is_20[(sizeof(g_host_default_engine_name_35160)==20)?1:-1];
