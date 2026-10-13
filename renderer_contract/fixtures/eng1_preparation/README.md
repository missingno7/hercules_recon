# ENG1 preparation feeding archived-mesh emission

This fixture compiles the existing `src/eng1/behavioral/mesh_prepare.c`, its
engine wrappers, mesh/primitive emitters and the shared projection reference.
It compares three resource-backed original `2E3B0` calls, then feeds the
source-derived preparation state into the C emitter. Generated case headers
come from a fresh original/archive trace; no private historical artifact is
required to reproduce the test.

The finite cases cover cold and warm transform caches, camera/projection
state, callback order, return values, face culling, packet bytes and OT links.
Original B810 writes camera XYZ as three WORDs at visit `+0x0C`; its Z write
overlaps depth at `+0x10`. The subsequent signed shift produces `1280 >> 2 =
320` in the depth discriminator. The source must preserve that alias and order.

The fresh VC5 comparison passes 51 preparation checks and 914 preparation-fed
emitter checks. The original meshes produce three, one and zero packets.
Host-service models accept only the documented zero Euler angles and uniform
Q12 scale `0x2000`; unsupported service inputs fail visibly. Warm-cache matrices
and camera center/focal are controlled pre-call inputs. This is bounded source
behavior, with no new strict matching claim, live scene or whole-map proof.

The observed preparation probe reaches `2E56E`. Its `0x1BF` observed span stays
separate from the historical `0x1AD` first-return pin; neither changes
`recovery.json`. The existing FT4 and point branch-tail pins also remain separate.

Run from the repository root with pinned read-only originals and a fresh output
directory under `work`:

```powershell
$py = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py -B renderer_contract\fixtures\eng1_preparation\run_prep_source_compare.py `
  --repo-root . `
  --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE `
  --eng1 D:\Prog\hercules_recon\work\discs\pc_install\ENG1.DLL `
  --archive D:\Prog\hercules_recon\assets\pc\HERCULES\HERCULES.FS `
  --output work\eng1-preparation-replay
```

The runner pins the original images, archive/map member, callback prefix,
source inputs and VC5 toolchain. Headers, extracted bytes, traces and compiler
products stay in the output directory. It rejects redirected, existing and
out-of-work outputs. Negative/high-bit camera Z, other rotations/scales,
resource/traversal C equivalence and production allocation capacities remain
outside this fixture. See the compact
[preparation evidence](../../evidence/eng1_preparation_phase20.json).
