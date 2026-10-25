/* ENG1 271d0 / 27db0: allocate, initialize, then bind coordinates and assets.
 * Reuses the instruction-equal TITLE twin's field view and assignment groups.
 * The configuration owners are the existing DWORD address carriers. Repeated
 * root reads and actor stores are intentional; do not capture a descriptor.
 * Descriptor strides are native-backed; their allocation capacities and the
 * historical declarations/translation unit remain unknown. The alternate
 * path uses only the shared 0x94 prefix of the view below.
 */
extern "C" {
typedef struct NormalFactoryDescriptor {
    int a_00;
    int b_04;
    int c_08;
    unsigned char d_0c;
    unsigned char unknown_0d[3];
} NormalFactoryDescriptor;

typedef struct AlternateFactoryDescriptor {
    unsigned long d0;
    unsigned char b4, b5, b6, b7;
} AlternateFactoryDescriptor;

typedef struct FactoryActorView {
    long d00;
    long d04;
    unsigned long d08;
    unsigned char unknown_0c[0x16];
    unsigned char b22;
    unsigned char b23;
    unsigned char unknown_24[0x0a];
    unsigned short w2e;
    unsigned char unknown_30[4];
    unsigned short field_34;
    unsigned char unknown_36[8];
    unsigned short w3e;
    unsigned char unknown_40[0x0a];
    unsigned short w4a;
    unsigned char unknown_4c[4];
    unsigned long d50;
    unsigned long d54;
    unsigned char unknown_58[0x0c];
    unsigned long d64;
    unsigned char unknown_68[0x0c];
    unsigned short w74;
    unsigned char unknown_76[0x1e];
    unsigned char b94, b95, b96, b97;
    unsigned short w98, w9a;
    unsigned long d9c, da0, da4, da8, dac, db0, db4, db8, dbc, dc0;
    unsigned long dc4, dc8, dcc, dd0, dd4, dd8, ddc, de0, de4, de8;
    unsigned char b0ec, b0ed;
    unsigned short w0ee;
    unsigned long d0f0, d0f4, d0f8, d0fc, d100, d104, d108, d10c, d110;
    short w114;
    unsigned short w116;
    unsigned long d118;
    short w11c;
    unsigned short w11e;
    unsigned long d120;
    unsigned short w124, w126;
    unsigned long d128, d12c, d130;
} FactoryActorView;
extern unsigned long g_configuration_dword_71388,g_configuration_dword_713a4;
FactoryActorView *allocate_normal_actor(unsigned short);
void *allocate_alternate_actor(unsigned short);
void initialize_normal_actor(FactoryActorView*);
void initialize_alternate_actor(void*);
#define normal_factory_descriptors ((NormalFactoryDescriptor *)g_configuration_dword_71388)
#define alternate_factory_descriptors ((AlternateFactoryDescriptor *)g_configuration_dword_713a4)
typedef char factory_normal_row16[sizeof(NormalFactoryDescriptor)==16?1:-1];
typedef char factory_alternate_row8[sizeof(AlternateFactoryDescriptor)==8?1:-1];
typedef char factory_actor134[sizeof(FactoryActorView)==0x134?1:-1];
FactoryActorView *spawn_normal(int a1, int a2, int a3, int a4)
{
    FactoryActorView *rec;

    rec = allocate_normal_actor(a4);
    if (rec != 0) {
        initialize_normal_actor(rec);
        rec->w2e = (unsigned short)a4;
        rec->d00 = a1;
        rec->d9c = a1;
        rec->d04 = a2;
        rec->da0 = a2;
        rec->d08 = a3;
        rec->da4 = a3;
        rec->b22 = normal_factory_descriptors[a4 & 0xffff].d_0c;
        rec->d64 = normal_factory_descriptors[a4 & 0xffff].a_00;
        rec->d110 = normal_factory_descriptors[a4 & 0xffff].b_04;
        rec->d118 = normal_factory_descriptors[a4 & 0xffff].c_08;
    }
    return rec;
}

FactoryActorView *spawn_kind(unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4)
{
    FactoryActorView *p = (FactoryActorView *)allocate_alternate_actor(a4);
    if (p != 0) {
        initialize_alternate_actor(p);
        p->w2e = (unsigned short)(a4 | 0x2000);
        p->d00 = a1;
        p->d04 = a2;
        p->d08 = a3;
        p->b22 = alternate_factory_descriptors[a4 & 0xfff].b6;
        p->d64 = alternate_factory_descriptors[a4 & 0xfff].d0;
    }
    return p;
}
}
