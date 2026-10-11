#include <stdio.h>

extern FILE *g_archive_stream;

/* All reviewed paths reach this cleanup through a tail jump and discard EAX.
   Historical formal return type remains unproved. */
void __cdecl archive_shutdown(void)
{
    if (g_archive_stream != NULL)
        fclose(g_archive_stream);
    g_archive_stream = NULL;
}
