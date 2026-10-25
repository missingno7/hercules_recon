extern unsigned long __cdecl host_sequence_next(int ignored);
/* Native BE30 is an actual shared empty routine called with varying arities. */
extern void __cdecl empty_host_callback();

void __cdecl host_callback_42e0(unsigned long value)
{
    host_sequence_next(0);
    empty_host_callback(value);
}
