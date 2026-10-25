/* Behavioral reconstruction of ENG1 RVA 0x39b0.
 * The row storage remains owned by calibration/resource_data.c. */
typedef struct ActorResourceLinkRow {
    unsigned short live_mask;
    unsigned short unknown_002;
    unsigned short next[4];
} ActorResourceLinkRow;

typedef char actor_resource_link_row_size_is_12[
    sizeof(ActorResourceLinkRow) == 12 ? 1 : -1];

extern ActorResourceLinkRow g_actor_link_rows[256];

void __cdecl eng1_target_039b0(void)
{
    unsigned long index;

    for (index = 0; index < 256UL; ++index) {
        g_actor_link_rows[index].live_mask = 0;
        g_actor_link_rows[index].unknown_002 =
            (unsigned short)(0x008cUL |
                             ((index & 0x08UL) << 1) |
                             ((index & 0x80UL) >> 6));
    }

    g_actor_link_rows[0].live_mask = 0x000fU;
    g_actor_link_rows[16].live_mask = 0x000fU;
    g_actor_link_rows[4].live_mask = 0x000fU;
    g_actor_link_rows[1].live_mask = 0x000fU;
}
