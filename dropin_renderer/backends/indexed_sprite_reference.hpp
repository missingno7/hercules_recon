#ifndef HERCULES_DROPIN_INDEXED_SPRITE_REFERENCE_HPP
#define HERCULES_DROPIN_INDEXED_SPRITE_REFERENCE_HPP

#include "indexed_reference.hpp"
#include "indexed_sprite_command.hpp"

namespace hercules::dropin {

/* A nonempty, source-backed one-page sprite domain. The shared resolver owns
 * clipping; source bytes and the selected blend table are captured at this
 * callback instant. Passive entry observations cannot establish consumption. */
IndexedReferenceStatus capture_indexed_sprite(
    const Event &event, const IndexedAddressMemory &memory,
    IndexedSpriteCommand *command);

IndexedReferenceStatus render_indexed_sprite(
    const IndexedSpriteCommand &command, IndexedFrame *frame,
    IndexedReferenceEffects *effects);

} // namespace hercules::dropin
#endif
