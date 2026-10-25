/* Test-only objects for exact-address guard boundary probes. Production
 * aggregate bytes/offsets/relocations are checked against native separately. */
#include "../src/eng1/resource_overlap_owners.h"
#include "../src/eng1/descriptor_prefix_data.h"
Eng1ResourceOwner_04fec0 g_eng1_resource_owner_04fec0;
Eng1ResourceOwner_050d88 g_eng1_resource_owner_050d88;
struct Eng1DescriptorPayload_4f350 {
    unsigned char header[16];
    unsigned long remaining[12];
};
struct Eng1DescriptorPayload_4f350 g_eng1_descriptor_payload_4f350;
