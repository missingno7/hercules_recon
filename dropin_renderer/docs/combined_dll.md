# Combined DirectDraw proxy and host shim

The combined-DLL milestone is implemented for **synthetic integration**. This
does not establish original-process compatibility, presentation ownership or a
functional native renderer. No original executable or engine was launched.

## Independently established starting state

The main checkout was `codex/eng1-behavioral-reconstruction` at `73c6320`, with
unrelated edits in `calibration/title/title_proc.cpp` and `recovery.json` and
three untracked process-audit/storage evidence files. They were left untouched.
The existing `work/rea_repo` worktree was clean on
`claude/rea-renderer-experiment` at `c52dd91`; all work here uses that worktree.
The other worktree, `work/renderer_interface_repo`, was clean at `f571717`.

`a2b9079` supplied a working standalone shim and REA evaluation. `e2f964c`
changed documentation only: the incomplete combined attempt had already been
reverted. No combined implementation or failed combined integration result was
present in the tracked starting tree. Ignored `work/host_shim_run` held the
old standalone build; `work/rea_experiment` held analysis inputs and output.
Neither was treated as fresh verification. A fresh baseline independently
passed the original 346 harness and 9 AC60 checks before implementation.

| Component | Existing capability | Boundary after this change |
| --- | --- | --- |
| `prototypes/host_shim` | 13-slot native relay, versioned C backend ABI, guarded AC60 pre-walk, fault containment, standalone harness and Unicorn differential | The same source is compiled into the combined DLL; FPU firewall repaired |
| `prototypes/rea_eval` | Optional pinned Ghidra/REA exploration, independently audited findings | No runtime/build/acceptance dependency |
| `dropin_renderer/proxy` | Lazy, system-path-checked DirectDraw forwarding; separate two-slot synthetic adapter | Existing forwarder now hosts optional attach publication; old adapter remains a tested explicit synthetic API and is not linked into the combined DLL |
| `dropin_renderer/core` | Archive/resource loading, owned indexed memory and event records | Reused existing offline implementation; no new renderer architecture |
| `dropin_renderer/capture` | Owned packet capture and controlled offline consumption through shared utilities | Separate from borrowed live shim events; no claim of a complete live scene |
| `dropin_renderer/backends` | Indexed CPU reference, bounded GPU and geometry diagnostics | No backend takes over the game window |
| `renderer_contract` | Original-backed callback/packet/state/projection/resource/presentation evidence and portable references | Remains the shared authority for semantics; unchanged by this implementation |
| Existing `presentation_probe/src/postentry_engine.c` | Source-built engine that copies all 250 words and calls slots 81/89 | Compiled directly by the new integration runner, without another engine implementation |

The later `20a01fa`/`c52dd91` timing work was already available. See
[frame_pacing.md](../../renderer_contract/frame_pacing.md): `0x5F20` uses a
33 ms wall-clock step with render-skipping catch-up and clock-reset coupling.
No Sleep, timer, wait, message pump or simulation changes are introduced here.
The shim README's earlier statement that this policy was unknown is obsolete.

## Composition and use

```powershell
cmake -S dropin_renderer -B work/combined-build -G "Visual Studio 17 2022" -A Win32 -DDROPIN_BUILD_HOST_SHIM=ON
cmake --build work/combined-build --config Release
ctest --test-dir work/combined-build -C Release --output-on-failure
```

The default `DROPIN_BUILD_HOST_SHIM=OFF` retains the forwarding-only DLL and
existing MinGW build. Enabling it requires MSVC x86 and compiles the existing
`ddraw_proxy.cpp`, the existing `host_shim.c`, the shared contract target and
one small `host_bootstrap.c` publisher into `ddraw.dll`. The enabled DLL also
exports the shim C API and `hs_bootstrap_status`; DirectDraw's export and lazy
forwarding body are unchanged. No second DLL is needed by this composition.

At attach, the forwarder records its module and calls the publisher. The
publisher reads the opt-out and main-image identity, checks initialized table
contents, builds a static clone and compare-exchanges the pointer cell. It
does no file I/O, heap allocation, library loading, backend invocation, COM,
window or thread work. DirectDraw resolution remains deferred until the first
actual `DirectDrawCreate` call, outside attach.

Any nonempty `HERCULES_SHIM_DISABLE` value skips publication. Otherwise, a
recognized host starts with relays in `HS_MODE_FORWARD_ONLY`, without a backend.
After loading, `hs_select_backend` accepts the existing `HsBackendV1` and
`HS_MODE_OBSERVE`; selecting NULL/forward-only turns observation off without
changing any cold-copied table. Invalid mode/ABI values are rejected.
`hs_bootstrap_status` reports installed, disabled, unsupported host, table
mismatch or installation failure. Rejection retains system DirectDraw
forwarding and leaves the host table pointer unchanged.

Backend callbacks are synchronous borrowed records on the host UI thread.
Registration, selection and stats access must be serialized there, outside
callbacks and active frames; this is not a concurrent backend-management API.
The backend must not pump messages or introduce blocking. Keep backend code and
context alive until deselected; copy records needed later. The published
clone and DLL must remain loaded for process lifetime. No detach/uninstall or
general recursive/concurrent rendering guarantee is provided.

Original callback exceptions still propagate through the relay's termination
handler. Backend exceptions disable observation. The firewall now saves and
restores the complete FXSAVE state, including x87 register stack, tags, status,
control and MXCSR, on normal return and caught faults. Two new discriminators
failed with the prior control-only restoration and pass with this repair.
The target is modern Windows x86 with FXSAVE/SSE support. The generic relay's
existing eight-DWORD cdecl stack-window scope is unchanged.

## Host identity

Only the pinned unpatched host is supported for automatic publication. Its
file SHA-256 is
`587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`.
The bootstrap checks PE32/i386, preferred and actual base `0x400000`, image
size `0x7ad000`, no base relocations, the `.text` section's identity and the
entire initialized 250-DWORD table before touching its pointer cell.

| Input | Extent | FNV-1a-64 | SHA-256 provenance |
| --- | --- | --- | --- |
| Mapped `.text` | RVA `0x1000`, VirtualSize `0x2cdbe` | `080fb4c55acc7402` | `976b9342ab3453ab2e9cae4fb64602eb4c5179dd6ff21e2f5d2af87f59bf6d33` |
| Initial table | RVA `0x361b8`, 1000 bytes | `97949ff63f2069e7` | `52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8` |

These pure in-memory hashes discriminate compatibility; they are not a
cryptographic authenticity guarantee. Timestamp and image size alone are
never accepted. The known user-assets variant has the same headers but its
`.text` FNV hash is `0262bcec41167cf2`; it is deliberately unsupported. See
[exe_variant.md](../../renderer_contract/exe_variant.md). Forwarding remains
available for it and all unknown hosts. No original data or code is embedded.

## Reproduce the acceptance run

Use the existing Python with the pinned Capstone/Unicorn packages available:

```powershell
$env:PYTHONPATH = 'C:/tools/gp-recon-python;C:/tools/hercules/python'
$py = 'C:/Users/jiriv/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $py dropin_renderer/tests/run_combined.py --exe D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE --output work/combined-check
```

The runner requires a fresh output directory under this worktree's `work/`.
It uses the existing VS2022 MSVC x86 compiler and CMake, records commands,
compiler/linker/tool hashes and source hashes, and checks sources remain stable.
It never starts the `--exe` input. Builds and verbose receipts remain ignored.
The compact durable result is [combined_dll_verification.json](combined_dll_verification.json).

The synthetic host statically imports the combined DLL and copies its table
in a CRT initializer before `main`. The reused engine copies that prefix again.
All 250 words, the 13 relay replacements and unmodified source words are
checked. Forward-only, observation and faulting observation produce the same
host call log and slot-81 result from the cold copy even after its source
pointer cell changes. The fixture's sentinel arguments deliberately skip the
packet pre-walk; packet/state/consumption proof comes from the reused complete
shim harness and original AC60 differential, also run against the combined DLL.

The runner builds a second DLL using the same sources and CMake target with a
generated **synthetic-only** content profile. Production has no synthetic
profile. It checks attach publication, opt-out (short and long values), changed
`.text` with otherwise equal profile metadata, changed table with unchanged
`.text`, and production rejection of the synthetic host. The system smoke
test also calls the real DirectDraw forwarder from the composed module.

On this Windows host the sandbox rejected the fixed-address synthetic EXE
before entry with WinError 623. The unchanged source-built fixture ran through
approved execution outside that sandbox. This was a launch limitation, not a
reason to alter the pinned originals or bypass the original-process gate.

The MSVC regression run additionally exposed an existing test ambiguity:
`vector<uint8_t>({0xEEU})` constructed 238 elements there. Direct list
initialization expresses the intended one-element expected value. Production
indexed-memory code is unchanged.

## Remaining dependency

The synthetic combined-DLL boundary does not grant a live deployment. Original
post-entry execution still needs explicit execution authorization, demonstrated
write/profile/registry isolation, windowed startup and display restoration.
No such launch was attempted here. `assets/`, originals, historical source and
`recovery.json` are unchanged.

For offline native-renderer work, the next concrete dependency is an owned
event/resource bridge from these borrowed shim callbacks into the established
indexed memory/capture/reference path, with original-backed upload, palette
and command ordering. A live integration then needs measured coexistence with
movie, GDI text, palette, clear, mode-change and presentation writers before
framebuffer ownership can be claimed. Native resolution separately requires
upstream vertex/primitive association; scaling 320x240 packets cannot supply it.
