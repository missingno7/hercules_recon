/* ENG1 27fe0: initialize the shared 0x94-byte actor prefix.
 * Reuses the recovered TITLE initializer's field view and assignment groups,
 * with the native ENG1 unconditional entry. w32 is the signed setup index.
 * Unlisted bytes retain their previous values; do not replace with memset.
 */
typedef struct ActorCoreState {
    unsigned long d00;
    unsigned long d04;
    unsigned long d08;
    unsigned short w0c;
    unsigned short w0e;
    unsigned short w10;
    unsigned short w12;
    unsigned short w14;
    unsigned short w16;
    unsigned short w18;
    unsigned char unknown_1a[2];
    unsigned char b1c;
    unsigned char b1d;
    unsigned char b1e;
    unsigned char b1f;
    unsigned char b20;
    unsigned char b21;
    unsigned char b22;
    unsigned char b23;
    unsigned char b24;
    unsigned char b25;
    unsigned short w26;
    unsigned short w28;
    unsigned short w2a;
    unsigned short w2c;
    unsigned short w2e;
    unsigned short w30;
    short w32;
    unsigned short w34;
    unsigned short w36;
    unsigned short w38;
    unsigned short w3a;
    unsigned short w3c;
    unsigned short w3e;
    unsigned short w40;
    unsigned short w42;
    unsigned short w44;
    unsigned short w46;
    unsigned short w48;
    unsigned short w4a;
    unsigned long d4c;
    unsigned long d50;
    unsigned long d54;
    unsigned long d58;
    unsigned long d5c;
    unsigned long d60;
    unsigned long d64;
    unsigned long d68;
    unsigned long d6c;
    unsigned short w70;
    unsigned short w72;
    unsigned short w74;
    unsigned short w76;
    unsigned char unknown_78[0x10];
    unsigned short w88;
    unsigned short w8a;
    unsigned short w8c;
    unsigned short w8e;
    unsigned short w90;
    unsigned short w92;
} ActorCoreState;
typedef char base_size_is_94[sizeof(ActorCoreState)==0x94?1:-1];
void initialize_alternate_actor(void *slot)
{
    ActorCoreState *p;

    p = (ActorCoreState *)slot;

    p->b1d = 0xff;
    p->b1e = 0xff;
    p->b1c = 0xff;
    p->d54 = 0x80000000;
    p->b1f = 0;
    p->b22 = 1;
    p->w2e = p->w30 = 0;
    p->d50 = 2;
    p->w32 = -1;
    p->w34 = p->w36 = p->w38 = 0;
    p->w0c = p->w0e = p->w10 = p->w12 = 0;
    p->w70 = p->w72 = p->w74 = p->w76 = 0;
    p->w88 = p->w8a = p->w8c = 0;
    p->w8e = p->w90 = p->w92 = 0;
    p->w3a = p->w3c = p->w3e = 0x100;
    p->w48 = 0xffff;
    p->w42 = 0xffff;
    p->w46 = 0xffff;
    p->w44 = 0xffff;
    p->w40 = 0xffff;
    p->b20 = 0xff;
    p->b23 = 0;
    p->w2a = 0;
    p->d64 = p->d68 = p->d6c = 0;
    p->d58 = 0;
    p->d5c = 0;
    p->d60 = 0;
    p->d00 = p->d04 = p->d08 = 0;
    p->w4a = 0x80;
    p->b24 = 0x80;
    p->w2c = 0;
    p->w26 = 0;
    p->b21 = 0;
    p->d4c = 0;
    p->b25 = 0;
    p->w14 = p->w16 = p->w18 = 0;
    p->w28 = 0;

}
