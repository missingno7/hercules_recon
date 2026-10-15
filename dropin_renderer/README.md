# Hercules drop-in renderer

This modern C++17 subproject applies the original PC reconstruction through the
shared `hercules_renderer_contract` CMake target. Historical source and strict
acceptance remain separate. The current implementation is a verified foundation
and offline diagnostic slice; original-game rendering and deployment remain open.
Its present role is research and bounded integration. It may eventually feed a
renderer directly integrated with reconstructed game source; the DLL deployment
is not a permanent architectural commitment. See [project direction](../docs/project_direction.md).

A [windowed resource checkpoint](docs/game_checkpoint.md) is now available for
local testing. It loads real archived pixels and colors, compares original CPU,
shared CPU and GPU output, and provides reload, resize and difference controls.
The original TITLE producer and image/palette timing remain to be recovered.

## Implemented boundaries

- `proxy/`: Windows x86 `ddraw.dll`, exporting the pinned game's sole static
  DirectDraw import, `DirectDrawCreate`. It resolves system DirectDraw once,
  outside DllMain, validates its system path including WOW64 redirection, and
  forwards the original arguments, HRESULT and COM object without wrapping it.
  DllMain records its own module; the optional MSVC combined build also publishes
  a content-checked callback clone. No game files or instructions are patched.
- `dropin_host_adapter` (test-only target): explicit, process-lifetime callback-table clone using
  the shared 250-DWORD ABI and clone helper. Slots 81/89 forward to the supplied
  callbacks and publish synchronous entry/return observations. Installation is a
  synthetic seam, with no automatic RVA attachment to Hercules. Slot 89 remains
  semantically void; its private diagnostic thunk preserves physical x86 EAX.
- `core/` and `capture/`: owned packet observations with decoded fields, state
  effects, references and visible unsupported status. Entry observations and
  controlled offline consumption have distinct provenance. The offline callback
  returns before the shared walker reloads links and reads subsequent packets.
  Texture/palette mutations, clears and presents are reserved event categories;
  packet capture does not manufacture those independently owned writer events.
- `backends/geometry_diagnostic.*`: the shared scalar compatibility reference
  checks captured camera and screen WORDs; the shared private reprojection uses
  camera XYZ to produce a separate 4:3 numerical view. Face corners retain their
  original point indices and OT association. It neither clips nor rasterizes.
- Optional `sdl_gpu_probe`: standalone, no-window x86 SDL_GPU/D3D12 diagnostic.
  Runtime HLSL draws an illustrative solid-color strip, followed by fence-waited
  texture readback. Its fill, color and triangle split are diagnostic policies;
  this is not a faithful textured Hercules backend or an original-host hook.
- `core/indexed_memory.*`: caller-bounded owned address storage with explicit
  byte validity, detached synchronous word uploads, and one backing for aliased
  raw texture/palette writes. Unknown bytes reject capture; VRAM moves are deferred.
- `backends/indexed_reference.*` and optional `indexed_gpu.*`: controlled FT4
  rectangle consumption captures only the sampled texels at each command. The
  reference calls the shared D420/CAB0 and E680/DD70 rectangle utilities; the GPU
  uses integer R8_UINT storage-texture loads and an indexed target. Raw and shaded
  blending use owned selected table snapshots and a separate prior-destination
  texture. Both preserve zero-source pixels.

The reference mode of the proxy continues through system DirectDraw and the
original renderer. The diagnostic GPU path uses a separate offscreen target and
does not present to the game's window or change simulation-visible state.

## Build and verify

Use an ignored directory under `work/`. The proxy and live adapter reject x64
builds; offline libraries can be built with `DROPIN_BUILD_PROXY=OFF`. CMake 3.24+
and a C++17 compiler are required. SDL3 is optional and remains outside Git.

On the inspected local toolchain:

```powershell
$py = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py dropin_renderer/tests/run_build.py --output work/dropin-build
# Optional hardware diagnostic; unavailable prerequisites are explicit skips.
& $py dropin_renderer/tests/run_build.py --output work/dropin-build-gpu --gpu
```

The runner verifies [local tool identities](docs/local_toolchain.json), compiles
fresh, runs CTest, and checks the production DLL is PE32/i386 with exactly
`DirectDrawCreate` exported and only Windows runtime imports. Build logs, source
hashes, JUnit output and receipts stay in the supplied fresh directory. Existing
MSYS2 compilers are reused; historical VC5 retains its separate pinned gate.
The installed SDL3 is an existing source build, independently compared with all
2,183 files in its source archive and its installed DLL; no tool is patched.

For other configured compilers, build directly with CMake:

```powershell
cmake -S dropin_renderer -B work/dropin-build -G "Visual Studio 17 2022" -A Win32
cmake --build work/dropin-build --config Release
ctest --test-dir work/dropin-build -C Release --output-on-failure
```

The optional **combined DLL** is freshly verified with MSVC x86: enable
`DROPIN_BUILD_HOST_SHIM=ON`. See [the inventory, ABI/lifetime boundaries and
reproducible synthetic acceptance run](docs/combined_dll.md). It retains
forwarding-only operation and rejects unsupported hosts. It is not a verified
original-process deployment or a complete native renderer.

Enable `DROPIN_BUILD_SDL_GPU_DIAGNOSTIC=ON` and point `SDL3_DIR` at a compatible
x86 SDL3 CMake package to build the optional probe and indexed backend.

## Uploaded indexed rectangles

The public comparison runner executes original A710 upload and AC60/D420/CAB0
in one bounded CPU-emulator image, then feeds the same synthetic inputs through
the modern owned-upload/ordered-capture path. It checks the complete 320x240
index buffer against the original; `--gpu` additionally compares fence-complete
GPU readback against the shared reference. The fresh build receipt and current
source hashes must match the supplied CLI.

```powershell
& $py dropin_renderer/tests/run_build.py --output work/dropin-indexed-build --gpu
& $py -B dropin_renderer/tests/run_indexed_rectangle.py `
  --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE `
  --cli work/dropin-indexed-build/indexed_rectangle_cli.exe `
  --output work/dropin-indexed-comparison --gpu
```

Three cases cover a base rectangle, shifted upload/UV/destination coordinates,
and an all-zero texture. The last produces no pixel stores but still requests
presentation, independently observed with the original dirty flag initially zero.
This supports a finite unblended FT4 subset, not general polygons or a complete
game frame. See [the indexed checkpoint](docs/indexed_slice.md) and the
[shared rectangle boundary](../renderer_contract/indexed_rectangle.md).

The bounded blended extension uses code 0x2e, page-zero selectors 0..3, and
observed indexed tables. The public comparison adds raw red 0x80 and shaded red
0x7f cases, transparent cases and a nonzero source shaded to zero:

```powershell
& $py -B dropin_renderer/tests/run_indexed_blend.py `
  --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE `
  --cli work/dropin-indexed-build/indexed_rectangle_cli.exe `
  --output work/dropin-blend-comparison --gpu
```

Use a fresh build from the current sources before either comparison. Table
inputs are explicit synthetic writers; no game palette or startup formula is
assumed. See [the blend checkpoint](docs/blend_slice.md).

## Real archived geometry

The public runner invokes the existing fresh native resource trace and historical
source preparation/emitter comparison before the modern CLI. It verifies input
identities, receipt hash links, archived records, callback-time point identity,
camera/XY outputs and all twelve FT4 packet corners. The TSV bridge, extracted
map and JSON output are generated only under ignored work.

```powershell
& $py -B dropin_renderer/tools/run_real_mesh_geometry.py --repo-root . `
  --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE `
  --eng1 D:\Prog\hercules_recon\work\discs\pc_install\ENG1.DLL `
  --archive D:\Prog\hercules_recon\assets\pc\HERCULES\HERCULES.FS `
  --geometry-cli work/dropin-build-gpu/geometry_diagnostic_cli.exe `
  --gpu-probe work/dropin-build-gpu/sdl_gpu_probe.exe `
  --output work/dropin-real-mesh
```

Omit `--gpu-probe` for deterministic command output alone. The selected controlled
mesh is outside the viewport; a successful optional GPU pass therefore proves
submission, clipping and readback, with zero visible mesh pixels. It is not a
captured gameplay image. Historical compiler error 623 needs the existing
documented launch fallback, without changing flags or compiler binaries.

## Feedback into reconstruction

The consumer exposed the difference between command-01's 16-byte direct envelope
and 24-byte transitive observation span. The authoritative contract now provides
[`renderer_packet_capture_size`](../renderer_contract/capture_extent.md), backed
by a new original-code guard-page discriminator and compiler regression suite.
Both sides use that one helper. No historical function was promoted.

The next integration boundary is still write/profile/registry isolation, proven
windowed startup and restoration before an authorized original post-entry probe.
The next rendering boundary is active CLUT/palette mapping and reachable
producer commands beyond the bounded rectangle subset. Movie, desktop-DC text,
palette and presentation ownership still need coordinated observations before
any game-window takeover. See [the measured checkpoint](docs/initial_slice.md)
and [shared blockers](../renderer_contract/blockers.md).
