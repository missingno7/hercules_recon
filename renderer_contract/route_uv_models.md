# Bounded raw-register UV transitions

The C89 package exposes three separate single-iteration transitions for the
original CC50 palette loop and two E680 routes. Each call starts at the
original loop-entry register boundary and receives only the original global
step words plus the source and observed color bytes for that pixel. It does
not calculate the setup from polygon geometry, create a generic interpolator,
or read lookup tables. The shared fixed-width definitions come from
`renderer_contract/hercules_types.h`; every register and address stays a raw
32-bit value.

The instruction boundaries and byte lanes remain route-specific:

- **CC50 `D322..D348`:** `DL` first captures the incoming `BL`. The loop adds
  the captured `A90998` word to full `EAX`, then performs full-width `ADC
  EBX,ECX`. The source address is full-width `ESI+EDX`, where the offset uses
  the incoming `BL` and `CH`. The sampled byte replaces `DL`; `ADC CH,[A90978]`
  uses the carry from the preceding `ADC EBX,ECX`. The model reports the state
  at `D337`, after that U/V step. A nonzero source then uses `EBP+source` for
  the palette result; zero skips the lookup and store. `EDI` increments and
  only `AX` decrements, preserving EAX's upper word.
- **E680 mode `0x80`, `EE6D..EE93`:** `DH` takes incoming `BL`, `DL` is cleared,
  and the sampled-byte address is the full 32-bit sum `ESI+EDX`. Full-width
  `EAX` addition feeds `ADC EBX,ECX`; its carry feeds
  `ADC ESI,[A9099C]`. The post-UV result is captured after the pointer
  correction. A nonzero source selects a destination byte and a blend result
  through `EBP+EDX`; zero skips destination and blend accesses. The transition
  then increments `EDI` and decrements only `AX`.
- **E680 mode `0x7F` through DD70, `E56C..E5A8`:** The sample and ESI correction
  sequence matches the generic mode-`0x80` route, but the color path remains
  distinct. A nonzero sample indexes the shade table at `A909A0+source`; the
  observed shade result and destination byte form the blend index under `EBP`.
  Zero skips shade, destination, blend, and output accesses.

The native capture runner reuses the existing 17-case route fixture and records
all nine 32-bit values at each loop entry, the post-UV landmark, and the final
loop exit. It pins the original executable, the original instruction spans,
Unicorn 2.1.4 and its native DLL, and the public route test runner. The C driver
receives each native loop-entry state directly, then compares both its
post-UV state and its next loop/exit state against original execution. It also
checks full sampled and lookup addresses, destination addresses, source-zero
stores, and the AX branch decision.

The established fixture covers 5,423 transitions: 319 iterations in each of
17 native cases. Strict C89 GCC and VC5 match all post-UV and next-boundary registers,
sample and lookup addresses, and store/count decisions. A separate public
skewed-top/bottom-clipped quad contributes 27 native E680 mode-0x80 transitions
with nonconstant U and V. Its original loop reads `A9099C` values 5, 6, and 7;
the EAX/EBX carry lanes and ESI ADC carry-in each include both 0 and 1. Strict
C89 matches all 27 post-UV and post-count register states, sample and blend
addresses, carry decisions, and store/count decisions. The geometry fixture's
ESI ADC carry-out remains zero.

A separate one-step original E680 loop run is classified
`RAW_BOUNDARY_ONLY`. Its corrected prediction uses `A9099C=0xF0010000` with a controlled loop state to
exercise EAX carry, EBX carry into ESI, and ESI carry-out together. Native
execution and the C route model agree on all registers, source/destination/
blend addresses, output byte, and the exit boundary. This proves the raw
instruction transition only; that correction magnitude is not claimed to be
geometry-reachable. The first preregistered raw input used `A9099C=0xF0000000`
and predicted ESI carry-out, but original execution produced `ESI=0xFFFF0101`
with carry-out zero. The corrected input changes only that global to
`0xF0010000`; the rerunnable pointer test records both predictions and results.
If `--vc5-toolchain` is requested, the route-state runner returns failure when
the VC5 build, execution, or native-state comparison fails.

The API accepts positive `AX` values from 1 through 320. The recorded fixture
exercises countdown values 319 through 1; it does not capture an `AX=320`
input or infer separate native setup for that value. The models preserve full EFLAGS at the captured
post-UV and next boundaries, including carry propagation and partial-register
effects. Source bytes and color results are supplied as route-specific
observations; the C code does not emulate memory reads, callbacks, table
generation, geometry, or clipping.

Use the repository's primary Python executable held in `$pythonExe` and run
from the repository root. Supply a new build directory each time; the runners
write compiler products and receipts only under `work/`. The checked-in
sources are in `renderer_contract`.

```powershell
& $pythonExe -B `
  'renderer_contract\tests\test_original_route_uv_state.py' `
  --package 'renderer_contract' `
  --fixture-helper 'renderer_contract\tests\test_original_route_uv_feedback.py' `
  --trace-helper 'renderer_contract\tests\capture_original_route_states.py' `
  --exe 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE' `
  --original-root '.' `
  --deps 'C:\tools\hercules\renderer-oracle-python' `
  --analysis-deps 'C:\tools\hercules\python' `
  --build-dir 'work\replay\route_uv_state_build' `
  --receipt 'work\replay\route_uv_state_receipt.json'
```

The standalone C unit suite uses captured native state vectors and rejects
invalid arguments and AX counts outside 1 through 320. The C++ link exercise in
`tests/test_cpp_headers.cpp` calls the C implementation through the public
header's C linkage declarations. The streaming trace replayer lives separately
under `fixtures/route_uv/` and rejects empty input.

Reproduce the 27 geometry-reachable transitions and one controlled raw-only
step with the public skewed-quad fixture helper. This runner embeds the
pre-run prediction in the package fixture, checks the original EXE and code
spans, and refuses outputs that overlap original inputs:

```powershell
& $pythonExe -B `
  'renderer_contract\tests\test_original_pointer_correction.py' `
  --package 'renderer_contract' `
  --fixture-helper 'renderer_contract\tests\test_original_skewed_quads.py' `
  --exe 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE' `
  --original-root '.' `
  --deps 'C:\tools\hercules\renderer-oracle-python' `
  --analysis-deps 'C:\tools\hercules\python' `
  --gcc 'C:\msys64\mingw64\bin\gcc.exe' `
  --build-dir 'work\replay\pointer_uv_build' `
  --receipt 'work\replay\pointer_uv_receipt.json'
```

The receipt supports finite CPU state-transition equivalence for the listed
loop paths and fixture. It does not establish full raster or pixel coverage,
host integration, a GPU implementation, other mode/selector inputs, packet or
geometry setup, gameplay, or whole-renderer equivalence.

The public receipts are [native_route_uv_state.json](evidence/native_route_uv_state.json) and [native_pointer_correction.json](evidence/native_pointer_correction.json). The pointer discriminator is rerunnable with tests/test_original_pointer_correction.py using a new work build directory. Its C differential is GCC-only.
