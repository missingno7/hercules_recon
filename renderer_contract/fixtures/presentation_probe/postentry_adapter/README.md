# Synthetic post-entry adapter package

This nested fixture contains `build_phase20.py` and `src/`. It reuses the
existing `presentation_probe` fixture and the shared host ABI constants.
The builder reads the existing fixture scripts and sources plus the three
`renderer_contract` host ABI files and toolchain manifest. It writes only to a
new output directory below the checkout's `work/` directory, rejecting
redirected or out-of-tree paths, reuse, and the `assets/` tree.

## Replay

From the checkout root, use a fresh output directory:

```powershell
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' -B renderer_contract\fixtures\presentation_probe\postentry_adapter\build_phase20.py --repo-root . --output-dir work\renderer_phase20\postentry_adapter_replay_01
```

The default toolchain is the pinned `C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953`; override only with the same manifest-pinned toolchain. The run refuses an existing output directory. Its receipt is `phase20_receipt.json` under that output path.

## Synthetic evidence and limits

Initial mirror verification used an intended-layout checkout at
`work/renderer_phase20/public_candidate/`. The receipt is at
`work/renderer_phase20/public_candidate/work/phase20/postentry_adapter_public_layout_run2/phase20_receipt.json`.
It records repository-relative source SHA-256 identities, VC5/toolchain
provenance, commands, and source-built PE hashes. The run passed 109 adapter
assertions and seven synthetic cases: positive forwarding, cold/warm copy,
invalid signature, source mutation rejection, wrong module identity, hash
mismatch, and timeout/reap. `--original` returned `BLOCKED` before process
creation. Additional guard probes rejected both `assets/blocked_probe_output`
and `work/../assets/blocked_probe_output` before creating either path.

The positive case snapshots the source table before publication, validates
all 250 copied/local words, observes slot-81/89 callee returns separately from
the fixture's caller-return completion markers, and preserves raw slot-89
EAX. These caller-return markers are source-built fixture addresses, not
original-engine PCs. This proves synthetic controller behavior only. The
original-host path remains blocked pending registry/profile isolation,
writable-path confinement, and windowed startup/display-mode restoration; no
original host, DLL, DirectDraw API, HWND, surface, GPU, assets, or gameplay was
used.
