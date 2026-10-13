# Verification boundaries

The contract starts from original PC instructions, not a shared reconstructed
software-rasterizer model. Original-image hashes and bounded RVA span hashes
are checked by tests/validate_originals.py. This catches changed inputs, not
errors in semantic interpretation; the evidence also records producer/consumer
offsets and callback bindings.

The C suite compiles fresh with pinned VC5. Size/offset assertions and explicit
byte fixtures exercise packet fields, truncation/unknown statuses, clone
validation with unchanged output on rejection, and bounded OT traversal. Raw
addresses are resolved into synthetic mapped regions. Decoder range/cycle
guards are intentional tool safeguards absent from the original AC60.

The optional native CPU oracle maps original PE sections at their preferred
address inside Unicorn, with synthetic arguments, stack and notification
state. Target instructions remain unchanged. Unknown endpoints, OS calls,
unmapped memory and instruction-budget exhaustion fail. These tests execute
the original instruction region and compare its observed writes/call boundary
with explicit expected values. No original code is embedded in this package.

A710's finite native tests corrected the preliminary y-row bank interpretation.
That correction is reflected in the address helper. It demonstrates why
synthetic tests of a shared formula alone do not establish behavioral accuracy.

AC20's existing strict FUNCTION_MATCH evidence is reused from recovery.json;
this work does not promote functions or recompute recovery counts. Existing
canonical source and strict evidence stay outside the renderer changes.

Host loading, interception, GPU pixels and window coexistence are unobserved.
No successful game launch or rendering replacement is claimed. Native CPU
emulation is distinct from HOST_INTEGRATION_OBSERVED.

The integrated checkpoint passes seven VC5 C/C++ suites. Numeric fixtures
include 36 frame, 37 packet, 43 state and 47 indexed-pixel checks. The native
CPU checks cover three upload rectangles, one mixed AC60 chain with recorded
callee boundaries, and four C700/C882 selector cases. Original validation
covers four image hashes, 108 spans, 24 callback bindings, 22 wrappers and 57
dispatch sites. These counts describe this finite checkpoint.

Luna xhigh workers performed the bounded call/field audits, module comparison,
frame/texture work, native CPU harnesses and reference utilities. The integrator
selected the clone boundary, resolved conflicting LEA/field/address evidence,
kept snapshot capture from mutating geometry state, and ran the package gates.
