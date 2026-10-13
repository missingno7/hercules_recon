# Dirty-latch evidence package

`tests/test_original_dirty_gate.py` combines a complete `.text` candidate inventory with bounded original-x86 Unicorn fixtures for D420/CC50/C2E0/CAB0, E680/C8F0/DD70, and 70C0. It records compact route receipts in JSON and checks the pinned EXE, helper/evidence inputs, all relevant original function-span hashes, each expected dirty-store site, and the absence of original-code writes. All output is guarded to remain under `<repo-root>/work/` and outside the original image directory.

## Reproduction

From PowerShell in the repository, with the pinned analysis/oracle tools already present:

```powershell
& $pythonExe -B renderer_contract\tests\test_original_dirty_gate.py `
  --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE `
  --repo-root D:\Prog\hercules_recon\work\renderer_interface_repo `
  --output-dir work\replay\dirty_gate `
  --deps C:\tools\hercules\renderer-oracle-python `
  --analysis-deps C:\tools\hercules\python
```

The runner writes `dirty_gate.json` and `dirty_refs.json` into the requested ignored `work/` directory. The source is pinned to the existing `test_original_polygon_dispatch.py`, `oracle_common.py`, `scripts/pe.py`, `state_ownership.json`, and `presentation.json`; hash mismatches abort before accepting results. Runtime checks require Unicorn 2.1.4 and the pinned `unicorn.dll` SHA-256.

## Findings supported by finite cases

`43A590` is read and cleared by presentation service `6620` at `0x6637` before downstream presentation is attempted. In reviewed raster routes it is a sticky request, not a byte-difference flag. Keep prior state, do not clear it in an adapter, and set it only where the route-specific original behavior is reproduced. `70C0` itself never sets or clears it in the bounded routine: its `0xff` color remap rewrites all 76,800 synthetic 320x240 surface bytes when the lock/view preconditions pass, yet leaves the dirty byte unchanged.

Concrete tested route conditions and their exact store RVAs are encoded per case in the receipt:

- D420 row-zero triangle with `BA01F0=1` reaches `0xDD5F` even for transparent input; a tested all-X-equal scan path also reaches that store with no texture reads or pixel stores. The corresponding fully clipped and `BA01F0=0` inputs do not store.
- With row code `0x78C0`, the tested `96E3E0=1` route reaches CC50 and stores at `0xD407`; `96E3E0=0` reaches C2E0 and stores at `0xC6EA`, including when its zero texels overwrite three nonzero synthetic pixels with zero. Tested clipped inputs do not store.
- The tested 2x2 CAB0/C8F0 rectangles store at `0xCC37`/`0xCA9A` with all source texels zero. Their tested all-X-equal and fully right-clipped inputs return without storing.
- E680 generic mode `0x80` stores at `0xEF52` for the tested in-bounds triangle, including equal-X/zero-pixel-work input. Its tested clipped and `BA01F0=0` inputs do not store. Mode `0x7f` delegates to DD70: the tested transparent in-bounds case stores at `0xE667`, while its clipped case does not.
- A suppressed D420 invocation leaves a seeded 1 unchanged. It does not clear the latch.

These facts reject both `dirty = pixels_changed` and a universal positive-area test. They do not define untested geometry, modes, caller sequences, or all owners of frame presentation.

## Static inventory limits

The complete skipdata sweep covers all 183,808 file-backed `.text` bytes from RVA `0x1000` through `0x2de00` (65,375 valid instructions and four one-byte pseudoops). The strict decoder stops at `0x1124a`, after 21,486 instructions / 66,122 bytes; any earlier strict-sweep inventory must be called prefix-only. The complete linear sweep finds 17 absolute-memory candidate references to `0x43A590`, 14 inside reviewed hash-pinned spans and three outside reviewed spans. It also produces complete candidate call lists for `6ff0`, `7140`, and `6620`; every listed immediate call is before the strict stop, but this is not proof of reachability. Embedded data and computed/indirect accesses remain limitations.

Direct non-raster paths also request presentation: `6bb0` transition paths and `6ee0` color setup can set the byte; outside the reviewed renderer closure, candidates are at `0x566b` (movie callback), `0x675d` (windowed palette handler), and `0x976b` (local non-DirectDraw path, enclosing owner unresolved). This package does not replace those host services and does not claim complete frame ownership or presentation serialization.

The public receipts are [native_dirty_gate.json](evidence/native_dirty_gate.json) and [dirty_refs.json](evidence/dirty_refs.json).
