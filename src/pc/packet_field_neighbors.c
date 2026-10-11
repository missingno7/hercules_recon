/* Actual PC packet writes only. Names, full original formal/return declarations,
 * SDK ownership and complete packet types remain unproved. */
void __cdecl host_packet_fields_0a5f0(void *packet,
                                     const unsigned long *source)
{
    unsigned char *bytes = (unsigned char *)packet;
    *(unsigned long *)(bytes + 4) = source[0];
    *(unsigned long *)(bytes + 8) = source[1];
    bytes[7] = 0xf3;
}

void __cdecl host_packet_fields_0a610(void *packet,
                                     const unsigned long *source,
                                     unsigned short value3,
                                     unsigned short value4)
{
    unsigned char *bytes = (unsigned char *)packet;
    *(unsigned short *)(bytes + 12) = value3;
    *(unsigned short *)(bytes + 14) = value4;
    *(unsigned long *)(bytes + 16) = source[0];
    *(unsigned long *)(bytes + 20) = source[1];
    bytes[7] = 1;
}
