/* Private ordinary-C view of HERCULES.EXE's logical archive positions.
 * The observed initialized extent does not establish total capacity. */
extern int g_archive_logical_positions[];

int __cdecl archive_close(int handle)
{
    g_archive_logical_positions[handle - 1] = -1;
    return 0;
}

long __cdecl archive_position(int handle)
{
    return g_archive_logical_positions[handle - 1];
}
