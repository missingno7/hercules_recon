#include <string.h>

/* One shared-scratch view; these external bounds are intentionally unknown. */
extern char g_archive_scratch[];
extern const char g_archive_prefix_maps[15];
extern const char g_archive_prefix_chop[15];
extern const char g_archive_prefix_sonix[18];

char * __cdecl archive_normalize_name(const char *input)
{
    const unsigned char *source;
    char *destination;
    unsigned char value;
    int start;
    int position;

    start = 100;
    if (input == NULL)
        return NULL;
    if (*input == '\0')
        return NULL;

    source = (const unsigned char *)input;
    destination = &g_archive_scratch[100];
    do {
        value = *source;
        if (value >= (unsigned char)'a' && value <= (unsigned char)'z')
            value = (unsigned char)(value - (unsigned char)('a' - 'A'));
        *destination = (char)value;
        ++destination;
        ++source;
    } while (*source != 0);
    *destination = '\0';

    if (g_archive_scratch[101] != ':') {
        if (g_archive_scratch[100] != '\\') {
            g_archive_scratch[99] = '\\';
            start = 99;
        }
        --start;
        g_archive_scratch[start] = ':';
        --start;
        g_archive_scratch[start] = 'M';
    }

    switch ((signed char)g_archive_scratch[start]) {
    case 'O':
        start -= 12;
        memcpy(&g_archive_scratch[start], g_archive_prefix_maps, 15);
        g_archive_scratch[start + 14] = '\\';
        return &g_archive_scratch[start];
    case 'P':
        start -= 12;
        memcpy(&g_archive_scratch[start], g_archive_prefix_chop, 15);
        g_archive_scratch[start + 14] = '\\';
        return &g_archive_scratch[start];
    case 'Q':
    case 'R':
        start -= 15;
        memcpy(&g_archive_scratch[start], g_archive_prefix_sonix, 18);
        g_archive_scratch[start + 17] = '\\';
        return &g_archive_scratch[start];
    case 'N':
    default:
        position = start + 3;
        while (g_archive_scratch[position] != '\\' &&
               g_archive_scratch[position] != '\0')
            ++position;
        --position;
        g_archive_scratch[position] = ':';
        g_archive_scratch[position - 1] = 'M';
        return &g_archive_scratch[position - 1];
    case 'M':
        return &g_archive_scratch[start];
    }
}

unsigned long __cdecl archive_name_hash(const char *path)
{
    const signed char *cursor;
    unsigned long sum;
    unsigned long length;
    unsigned long value;
    int shift;

    cursor = (const signed char *)path;
    sum = 0UL;
    length = 0UL;
    shift = 0;
    while (*cursor != 0) {
        value = (unsigned long)(long)*cursor;
        sum += value << shift;
        shift += 8;
        if (shift > 24)
            shift = 0;
        ++length;
        ++cursor;
    }
    return sum + length;
}
