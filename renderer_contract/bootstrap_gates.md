# Static bootstrap gate contract

## Conclusion

The pinned host starts with all inspected dispatch cells closed: `BA0238`, `BA0228`, `91C45C`, `91C460`, and the auxiliary resolver-in-progress cell `91C464` lie in the zero-filled virtual tail of EXE `.data`. Thus the independent `79A0 -> 79FC -> 5F20 -> 5EC0` route cannot make a cold export call from that initial state: `BA0238 == 0` bypasses its tick, and the later export pointer guard sees `91C45C == 0`.

The normal direct initialization route publishes the export pointer after `DirectDrawCreate` returns. Direct call edges are `7937 -> 5E10 -> 72A0` (`5E2C`); `72A0` calls the import thunk at `7426` and returns; `5E10` later calls `1FF0` (`5E40`), `1FF0` calls `4040` (`20A6`), and `4040` calls `5CD0` (`40C4`). On the ordinary `91C464 == 0` branch, `5CD0` calls `5C20` at `5DFE`; `5C20` performs `LoadLibraryA` / `GetProcAddress`, and the direct pointer store is at `5C9D`. This establishes post-DirectDraw publication for that direct bootstrap path.

This does not prove a process-wide “no cold export call before DirectDraw” guarantee. The enable writers at `9820` and `9840`, and suppression writer at `9870`, have no direct `.text` callsites in the four pinned originals, but their code addresses appear in EXE `.rdata` at `2E5CC`, `2E62C`, and `2E644`. Their possible indirect invocation order is unknown. The resolver gate can also be set at `5DE3` only when `91C464 != 0`; the dispatcher sets `91C464` around its opaque call at `5ED4`. A callback or reentrant route into `5CD0` while that cell is set is not excluded by this audit. No direct caller of `5CD0` besides `40C4` was found, but linear-sweep call xrefs and absent direct references do not rule out aliases or indirect calls.

The code address `0x4079A0` appears at `.rdata` RVA `0x2E240` within a short run of dwords that mostly point into `.text` (the final dword at `0x2E27C` is zero). No instruction directly references the `0x42E240` address. The nearby immediate references at `79A5` and `80F0` target `0x42E280` and `0x42E290`; they do not identify the preceding run’s consumer. Treat the `2E240` value as function-pointer data, not proof of a registration mechanism or schedule.

## Direct writes

- `BA0238`: stores at `5685` and `7A0A` clear the event-enable byte. `9820` stores `1` or `2` based on its argument; `9840` stores `1`. The event callback checks this byte at `79EA`.
- `BA0228`: `9870` sets the byte to `1` when either measured window extent is less than one, otherwise `0`; the event callback skips the tick when nonzero at `79F3`.
- `91C45C`: `5C9D` stores the `GetProcAddress` result. `5EEA` reads it and the nonnull branch at `5EEF` guards the indirect call at `5F03`.
- `91C460`: `5C28` clears the lazy resolver gate. `5DE3` sets it only on the `91C464 != 0` branch. `5ED9` reads it; a nonzero value selects the `5EE5 -> 5C20` refresh path.
- Auxiliary `91C464`: `5ECA` sets it before the `5ED4` call; `5F08` clears it on the dispatcher exit path. `5DD3` in `5CD0` reads it to choose the `5DE3` branch versus the direct `5DFE -> 5C20` loader call.

The `.text` scan covered each complete raw section with Capstone skipdata enabled: EXE 183,808 bytes, ENG1 307,200, ENG3 200,192, and TITLE 130,560. It found direct target references only in HERCULES.EXE (13 to `BA0238`, 9 to `BA0228`, 2 to `91C45C`, 3 to `91C460`, and 3 to auxiliary `91C464`); no matching absolute references occur in the three DLL text sections. This is candidate direct-reference evidence, not instruction-alignment proof. Register-computed aliases, external writes, callback dispatch, CRT/runtime behavior, and API reentrancy remain outside scope.

## Reproduction and provenance

From the renderer checkout root, with the pinned analysis dependencies available:

```powershell
$env:HERCULES_PYTHON_DEPS = 'C:\tools\hercules\python'
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' `
  renderer_contract/tests/audit_bootstrap_gates.py `
  --original-root . `
  --image-dir D:\Prog\hercules_recon\work\discs\pc_install `
  --output work/renderer_phase8/bootstrap_gates/evidence.json
```

The runner validates SHA-256 identities for HERCULES.EXE, ENG1.DLL, ENG3.DLL, and TITLE.DLL, then writes only beneath `work/`. It used Capstone 5.0.7 and `scripts/pe.py` SHA-256 `8da71eb87b5a88dd1d80a3fe180e2b94557e2c3017fb4e5c37c4a8c630af62ca`.

The fresh public receipt is [bootstrap_gates.json](evidence/bootstrap_gates.json); it pins the runner, original images and exact bounded spans.

The JSON receipt contains the exact per-site instruction hashes, direct callers, full raw `.text` coverage counts, zero-fill basis for initial values, callback-literal neighborhood, and hashes for the bounded code/data spans (`5E10`, `5EC0`, `5F20`, `79A0`, `1FF0`, `4040`, `5C20`, `5CD0`, `9820`, `9840`, `9870`, and `2E220`).
