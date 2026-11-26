/* Element zero is the count. The shared scan/shift index makes this remove
 * only the first match; the vacated final slot is deliberately not cleared.
 * The post-decrement comparison is preserved from the measured PC code. */
void remove_counted_value(long *list, long value)
{
    int count, i;
    if (list) {
        count = list[0];
        for (i = 1; i <= count; ++i) {
            if (list[i] == value) {
                for (; i < count; ++i) list[i] = list[i+1];
                if (count-- < 0) count = 0;
                list[0] = count;
            }
        }
    }
}
