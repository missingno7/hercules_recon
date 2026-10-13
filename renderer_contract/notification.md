# Notification ownership and live command storage

BD80 synchronously tail-jumps through the primary callback at VA A8E780 when
that pointer and the DWORD gate at A8E788 are nonzero. It passes no explicit
stack arguments and retains the caller's return address. It has no reentry
guard. A replacement must retain this timing and reload the live callback on
each call. The actual primary target and its write set remain UNRESOLVED.

| Slot | Offset | EXE RVA | Operation |
|---|---|---|---|
| 92 | +0x170 | BDA0 | Exchange primary callback, return old DWORD |
| 249 | +0x3e4 | BDD0 | Exchange secondary callback A8E784, return old DWORD |
| 161 | +0x284 | BDF0 | Enable gate and clear both pointers |
| 240 | +0x3c0 | BDE0 | Disable gate |

The observed host initialization path calls BDF0. TITLE temporarily installs
RVA 5D60 through slot 249, then restores the prior secondary callback. That
callback belongs to BDB0's secondary dispatch, not BD80. No direct registration
slot reference was found in ENG1/ENG3's file-backed text. This static absence
does not exclude indirect aliases or external registrations.

Command 01 in AC60 calls A8F0, which reaches BD80 after the VRAM write path.
AC60 subsequently reloads the current primitive's first DWORD at RVA ACA2.
Preserve that live-link read after notification; prefetching the entire frame
can change behavior if a callback mutates linked storage or shared state.
No particular shipped callback mutation is proven by this audit.

The bounded tool API `frame_walk_ac60_order_table_visit` calls its visitor after
the address guard and before resolving that primitive's live next link. Its
synthetic fixture rewrites a link during the visit and checks the new node is
followed. A zero visitor result fails visibly. The old walker retains a null
visitor. Bounds/cycle guards are tool safeguards absent from native AC60.

Static identities, spans and registration references are in
[notification_ownership.json](evidence/notification_ownership.json). The C
fixture has 44 checks. No original callback or Windows service was executed;
no full initialization, ownership extent or host integration is claimed.

The [ownership audit](ownership.md) additionally pins AC00's eight-byte wrapper
and the BD80 gate body. AC00 calls BD80 and then clears EAX; its callback returns
to AC05, while AC60's direct call returns to B2E7. A verified-host adapter can
resolve BD80 by image base plus its RVA without receiving a DLL's copied table.
Resolving the address statically does not prove live invocation or opaque
callback behavior.
