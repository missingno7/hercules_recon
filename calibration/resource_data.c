/* Private ordinary-C data hypotheses from writer-backed PC spans.
 * Record/path labels and source declaration ownership remain unproved. */
#include <stddef.h>

typedef short ActorReference[2];
typedef short ActorTokenReference[2];

typedef struct ActorResource ActorResource;
struct ActorResource {
    void *path;
    unsigned short entry_count;
    unsigned short unknown_006;
    unsigned long file_length;
    ActorReference *references;
    unsigned short residency_state;
    unsigned short unknown_012;
};

typedef struct ActorResourceLinkRow ActorResourceLinkRow;
struct ActorResourceLinkRow {
    unsigned short live_mask;
    unsigned short unknown_002;
    unsigned short next[4];
};

typedef char actor_reference_size_is_4[(sizeof(ActorReference) == 4) ? 1 : -1];
typedef char actor_resource_size_is_20[(sizeof(ActorResource) == 20) ? 1 : -1];
typedef char actor_resource_fields_match_offsets[
    (offsetof(ActorResource, path) == 0 &&
     offsetof(ActorResource, entry_count) == 4 &&
     offsetof(ActorResource, unknown_006) == 6 &&
     offsetof(ActorResource, file_length) == 8 &&
     offsetof(ActorResource, references) == 12 &&
     offsetof(ActorResource, residency_state) == 16 &&
     offsetof(ActorResource, unknown_012) == 18) ? 1 : -1];
typedef char actor_resource_link_row_size_is_12[(sizeof(ActorResourceLinkRow) == 12) ? 1 : -1];

/* Observed current image extent: 81 rows at stride 20. */
ActorResource g_actor_resources[81] = {
    { "", 0x0000U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\HERO\\HERCULES", 0x0251U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\HERO\\XTRA_DB1", 0x011aU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\HERO\\HERCULES\\HC_THROW", 0x0057U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\HERO\\HERCULES\\HC_DIE", 0x0025U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\HERO\\HERCULES\\HC_DUNIT", 0x0040U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\HERO\\XTRA_DB1\\LETTERH", 0x0018U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\HERO\\XTRA_DB1\\LETTERE", 0x0018U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\HERO\\XTRA_DB1\\LETTERR", 0x0018U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\HERO\\XTRA_DB1\\LETTERC", 0x0018U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\HERO\\XTRA_DB1\\LETTERU", 0x0018U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\HERO\\XTRA_DB1\\LETTERL", 0x0018U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\HERO\\XTRA_DB1\\LETTERS", 0x0018U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\PHIL", 0x0073U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\PHIL\\CLIMB", 0x006fU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\PHIL\\REACT", 0x003cU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\PHIL\\MACHINE", 0x00d4U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\PHIL\\DANCE", 0x0040U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\PAIN", 0x00b3U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\PANIC", 0x00b5U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\THEBIANS\\THSNOW", 0x003bU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\THEBIANS\\THFLOOD", 0x001fU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\THEBIANS\\THEQ", 0x0067U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\THEBIANS\\THFAT", 0x0018U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\THEBIANS\\THOLD", 0x0019U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\THEBIANS\\THFLASH", 0x002bU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\THEBIANS\\THBURNT", 0x0037U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\THEBIANS\\THUGS", 0x00a3U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\THEBIANS\\THEWGUY", 0x0040U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\THEBIANS\\THCHAR", 0x0074U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\THEBIANS\\THHAFOOD", 0x0082U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\TRAINING", 0x00a0U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\TRAINING\\TOPS", 0x0101U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\TRAINING\\SHARK", 0x002eU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\TRAINING\\PEG", 0x0026U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\TRAINING\\TRAINFX", 0x000bU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\FANGIRLS", 0x00c2U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\SKELETON", 0x00c0U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\GENERIC1", 0x0078U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\ALIEN1", 0x002aU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\MINOTAUR", 0x007eU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN1\\HARPIES", 0x0071U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\ROCKC", 0x0061U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\WRESTLEC", 0x006dU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\ARCHERC", 0x0083U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\ALEXFISH", 0x0076U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\STYBIRD", 0x00c6U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\FPHIL", 0x0069U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\PEGASUS", 0x0085U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\RockT", 0x0042U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\IceT", 0x0033U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\LavaT", 0x0054U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\TornadoT", 0x0028U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Aphrod", 0x0010U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Apollo", 0x000fU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Ares", 0x000eU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Athena", 0x000fU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Bacchus", 0x0011U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Water", 0x0012U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\RcRock", 0x000fU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Griffon", 0x008dU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Zeus", 0x000eU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\RtRock", 0x0009U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Harp", 0x0045U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Spy", 0x0022U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Pig", 0x00b2U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Grif", 0x01f2U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Merv", 0x010fU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\NESSUS\\Basic", 0x002aU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\NESSUS\\Punch", 0x000cU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\NESSUS\\Kick", 0x0026U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\NESSUS\\Look", 0x000eU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\NESSUS\\Ride", 0x0035U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\NESSUS\\Reject", 0x000eU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\NESSUS\\Finish", 0x0006U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\NESSUS\\Defeat", 0x00c7U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\ALIEN2\\Meg", 0x002eU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\BOSS\\MEDUSA", 0x0094U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\BOSS\\SKELWAR", 0x00a1U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\BOSS\\HYDRA", 0x0069U, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
    { "P:\\SRC1\\BOSS\\HADES", 0x02baU, 0x0000U, 0x00000000UL, 0, 0x0000U, 0x0000U },
};

/* Signed byte lookup values from the 16-byte file-backed map. */
signed char g_actor_link_map[16] = { 0, 0, 1, 0, 2, 0, 0, 0, 3, 1, 0, 0, 2, 0, 0, 0 };

/* The runtime initializer/allocator bound the 32 token rows. */
ActorTokenReference g_actor_token_references[32];

/* Only the leading 256 initialized rows are supported; total capacity is unknown. */
ActorResourceLinkRow g_actor_link_rows[256];
typedef char actor_token_reference_extent_is_0x80[(sizeof(g_actor_token_references) == 0x80) ? 1 : -1];
typedef char actor_link_row_extent_is_0xc00[(sizeof(g_actor_link_rows) == 0xc00) ? 1 : -1];
