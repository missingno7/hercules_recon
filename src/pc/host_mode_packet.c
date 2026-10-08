/* Actual PC compatibility packet writes; original SDK/prototype identity unproved. */
void __cdecl host_mode_packet_adapter(void *packet, unsigned long ignored2,
                                     unsigned long ignored3,
                                     unsigned long value4,
                                     unsigned long value5)
{
    unsigned char *bytes = (unsigned char *)packet;

    *(unsigned long *)(bytes + 4) = value4;
    *(unsigned long *)(bytes + 8) = value5;
    bytes[7] = 0xe1;
}
