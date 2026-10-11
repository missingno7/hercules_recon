#include <windows.h>
#include <stdio.h>
#include <errno.h>
#include "title_trace.h"

void title_trace(const char *phase, unsigned long a, unsigned long b, unsigned long c)
{
    char path[MAX_PATH];
    DWORD length;
    FILE *stream;
    DWORD saved_error = GetLastError();
    int saved_errno = errno;
    length = GetEnvironmentVariableA("HERCULES_TITLE_TRACE", path, sizeof(path));
    if (!length || length >= sizeof(path))
        goto done;
    stream = fopen(path, "a");
    if (!stream)
        goto done;
    fprintf(stream, "%s %08lx %08lx %08lx\n", phase, a, b, c);
    fclose(stream);
done:
    errno = saved_errno;
    SetLastError(saved_error);
}
