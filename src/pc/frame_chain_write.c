/* Native pointer-chain writer; original source name and count declaration unproved. */
int __cdecl frame_chain_write(void *first, unsigned long raw_count)
{
    void **cursor;

    cursor = first;
    --raw_count;
    if ((int)raw_count > 0) {
        do {
            void **next;
            next = cursor + 1;
            --raw_count;
            *cursor = next;
            cursor = next;
        } while (raw_count != 0);
    }
    *cursor = 0;
    return 0;
}
