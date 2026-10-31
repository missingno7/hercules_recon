#include "../shared/engine_interface.h"

/* Observed slot-81 forwarding ABI; original source name/prototype unproved. */
int __cdecl title_0c690(void *first, unsigned long raw_count)
{
    return g_engine_interface.frame_chain_write_144(first, raw_count);
}
