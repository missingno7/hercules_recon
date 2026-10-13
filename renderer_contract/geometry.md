# Geometry information at projection and packet boundaries

**Result.** The reviewed ENG1 mesh path starts from three 16-bit point coordinates, transforms them to camera-space XYZ, then projects to screen XY. The reviewed final packet consumers use projected XY plus color/texture fields; they do not consume camera Z, reciprocal depth, or normals. No depth should be inferred from XY. Camera XYZ and projected XY remain in host globals after the existing projection callback, but the safe behavior of adding extra callbacks to standard point emitters is unproved.

## Availability matrix

| Information | Earlier ENG1 callbacks | Final packet / AC60 consumer | Finding |
|---|---|---|---|
| Local/model XYZ | `2C690` / HERC `B6F0` accepts three WORDs. ENG1 point emitters write three WORDs before calling it. | Not among the reviewed FT3/FT4/GT4 fields. | Present before transform. |
| Camera-space XYZ | HERC `B740` writes transformed X/Y/Z to `A8E71C/28/34`; `B840` snapshots the full DWORDs and `B810` copies their low WORDs to an output buffer. | Not consumed. | The values are observable after the existing transform/project callback and before the next point. The probe's extra `B840`/`B810` calls establish exposure and narrowing only, not safe insertion into standard emitters. |
| Screen XY | HERC `B920` projects to `A8E740/46`; `B720` returns two WORDs. `B5C0`/`B660` copy projected XY to packet vertex slots. | Consumed as vertex coordinates. GT4 offsets are `+08,+14,+20,+2C`. | Survives projection and packet construction. |
| Camera Z / depth | `B740` clamps transformed Z to at least 1 and leaves full DWORD camera XYZ at `A8E71C/28/34`. `B810` exposes low WORDs only when called. | No Z member is read by the reviewed packet routes. | A wrapper around the existing projection callback can read the still-live camera globals, but this is a candidate seam whose side effects and packet association remain unproved. |
| Reciprocal/perspective factor | `B920` computes signed `(focal << 16) / Z` transiently, then uses it to scale camera X/Y. | Not emitted by `B720`, `B5C0`, `B660`, or consumed from the reviewed packet fields. | The exact quotient has no observed post-return packet or wrapper output; a safe capture path is not established. |
| Surface normal | No 3D normal input or output identified in the reviewed point callbacks. `B6B0` unpacks three screen-coordinate pairs; `B4B0` calculates their signed 2D area, returned by `B4A0`. | No normal field is consumed. | The private `mesh_emit.c` local named `normal` represents a winding/culling area result, not a vertex normal. |
| Color / lighting | Native GT4 producer copies four per-vertex DWORD payloads into the packet color positions. The origin and lighting meaning of those values are not established here. | GT4 consumer route reads texture/coordinate fields and ignores the per-vertex color members, per AC60 closure. | Color bytes may be present without participating in this GT4 route; no normal-based lighting data survives. |
| Material / texture | Face/texture lookup feeds packet UV/page/palette bytes. | UV and texture/page state are consumed. | Texture coordinates and material state survive; this does not encode geometric depth. |

The shared AC60 closure reports minimum consumed prefixes of `0x1E` for FT3-compatible, `0x26` for FT4-compatible, and `0x32` for GT4-compatible routes. Those are consumer access minima, not producer allocation proofs. The native ENG1 `2DF20` span directly calls the GT4 packet callback `2C790` and advances by `0x34`; it does not directly call the FT4 callback `2C770`. The two trailing bytes `+0x32..+0x33` of that producer stride fall outside the reviewed GT4 consumer prefix and remain unclassified; no claim is made about their contents.

## Capture boundary

The original ENG1 point emitters at `2E600`, `2E6C0`, and `2ED10` call `2C690` then `2C6E0` then `2C760`; they do not call the camera capture/output pair. The original probe helper at `2E3B0` calls `2C690 Ă˘â€ â€™ 2C6E0 Ă˘â€ â€™ 2C6C0 Ă˘â€ â€™ 2C760 Ă˘â€ â€™ 2C750`. Copied-table and host spans establish these bindings:

| ENG1 wrapper | Copied slot / host target | Verified effect |
|---|---|---|
| `2C690` | slot 101 / HERC `B6F0` | Read three WORDs from caller point record into transform input cells. |
| `2C6E0` | slot 106 / HERC `B920` | Transform and perspective-project current point. |
| `2C6C0` | slot 103 / HERC `B840` | Snapshot full transformed XYZ DWORDs into host work cells. |
| `2C760` | slot 114 / HERC `B720` | Copy projected XY to caller output. |
| `2C750` | slot 113 / HERC `B810` | Copy three low WORDs of snapshotted XYZ to caller output. |
| `2C9D0`, `2C9F0` | slots 174/175 / HERC `B970/B990` | Set projection center and focal value. |

For a future per-point capture, the narrowest candidate seam is a wrapper around the existing slot 106 / `2C6E0` / HERC `B920` projection callback: call the original callback unchanged, then copy camera XYZ from `A8E71C`, `A8E728`, and `A8E734`, plus projected XY from `A8E740` and `A8E746`, into private storage before the next point. Do not add slot 103 / `B840` snapshot or slot 113 / `B810` output calls to the standard emitters. The probe at `2E3B0` shows those services expose full XYZ and narrow it to WORDs, but does not prove that extra calls preserve host snapshot state or other behavior in the regular emitter paths. Even the wrapper-and-copy seam is a proposal: behavior-preserving dispatch, storage lifetime, and one-to-one association between captured points and emitted 3D packets remain unresolved. The exact reciprocal factor is transient inside `B920` and has no observed post-return output.

## Evidence and limits

Prediction recorded before inspecting the callback family in [prediction.md](prediction.md): the final packet would omit per-vertex Z/reciprocal depth, while earlier callbacks would expose coordinates. The packet prediction is supported. Earlier host globals contain camera and projected coordinates, but the probe's snapshot/output call sequence does not establish that adding those callbacks to standard emitters is behavior-preserving. The `normal` prediction is falsified: the reviewed Ă˘â‚¬Ĺ›normalĂ˘â‚¬ĹĄ result is projected 2D area.

The PC facts are STATIC observations over hash-checked original binaries. [evidence.json](evidence/geometry.json) contains original DLL/EXE hashes, copied-table target RVAs, wrapper spans and hashes, host spans and hashes, plus original ENG1 producer spans and direct-call sites. Key original target spans are HERC `B740` size `0xCA`, `B810` size `0x25`, `B840` size `0x23`, `B920` size `0x4E`, `B5C0`/`B660` size `0x47`, and ENG1 `2DF20` size `0x334`, `2E3B0` size `0x1AD`, `2E600` size `0x69`, `2E6C0` size `0x85`, `2ED10` size `0x165`.

The private behavioral sources are not asserted to match native functions. In particular, current `mesh_emit.c` contains an FT4/GT4 source branch; original ENG1 `2DF20` direct-call evidence supports GT4 only. FT4 producer paths elsewhere, ENG3/TITLE mesh production and consumers, other geometry kinds, lighting-value provenance, complete packet allocations, and the two unused GT4 stride bytes remain open. ENG3 and TITLE are audited here only for selected copied wrappers (TITLE GT4 slot 119 was not found). `calibration/frame_lifecycle_bridges.c` supplies lifecycle bridges rather than these geometry wrappers; geometry forwarding is in `host_wrappers.c` and `host_wrappers_extra.c`.

Proof is STATIC/LAYOUT only: no original execution, source identity, transform reconstruction, software rasterizer, whole-module equality, or runtime rendering behavior is claimed.

## Reproduction

From the worktree root, with the bundled Python runtime and existing Capstone dependency:

See [README.md](README.md) for the shipped verification commands. Private audit commands are retained only in receipt provenance.

The audit passed for 13 HERC host function spans, selected copied wrappers in all three DLLs, five original ENG1 producer spans, and the hash-checked AC60 consumer closure. The current packaged receipt and its private provenance hash are in evidence/geometry.json.

## Integrated reproduction

The private investigation paths above identify provenance. In this package, run tests/validate_originals.py with the original image paths to recheck recorded bytes and bindings, and tests/run_tests.py for the compiled synthetic suite. The byte validator does not independently redo the semantic analysis. The compact receipt is evidence/geometry.json.
