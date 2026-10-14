# Uploaded indexed rectangle checkpoint

The renderer now has a connected detached-upload -> ordered FT4 capture -> shared
indexed reference -> GPU readback slice. Owned storage tracks byte validity in
one caller-bounded address interval. Capture copies only each accepted command's
sampled texels and addresses; later uploads cannot alter that command. Missing
bytes and unsupported commands fail explicitly. Raw aliased palette writes use
the same backing, but palette interpretation and VRAM moves remain deferred.

The shared rectangle utility was extracted from the existing composition fixture.
Both that fixture and the modern CPU reference use its original-backed D420/CAB0
behavior. The SDL_GPU backend loads a tightly packed R8_UINT tile through fragment
storage-texture integer reads, renders to an R8_UINT 320x240 target, discards zero
sources and returns owned indices after a fence completes. Shaders compile at
runtime through the existing system x86 D3DCompiler47. No color conversion or
general polygon interpolation is inferred.

Three fresh original CPU-emulator cases join A710 upload to AC60/D420/CAB0.
For each, the modern CPU output matches the original hash of all 76,800 indices;
GPU readback then matches every CPU index. The base and offset cases store 3 and
6 pixels. The transparent case stores zero and preserves the seeded destination,
while the original still sets its presentation-request flag from an initial zero.
The contract and modern path preserve that separate request.

Validation completed on the frozen source snapshot:

- 10/10 modern CTest cases, including actual x86 D3D12 indexed readback, no skips.
- 23/23 shared tests with pinned VC5 and 23/23 with GCC x86.
- Prior connected composition native event/pixel comparisons under both compilers.
- 132/132 existing historical functions freshly compiled and raw-byte exact.
- Three joined original/CPU/GPU indexed comparisons.

[Machine-readable checkpoint](indexed_slice.json) records receipt hashes, exact
commands and source linkage. Generated binaries, readbacks, inputs and full traces
remain under ignored work. Historical source and recovery state are unchanged.

These observations use synthetic detached inputs and bounded original CPU code.
No original game process was launched in this stretch, and no game-window output
or visible original asset rendering is claimed. The existing transparent proxy
remains separate from the offline backend. The next useful rendering boundary is
CLUT/shade/blend ownership or a verified reachable producer feeding this path;
post-entry host execution still requires its existing isolation and authorization
gate. Movie/GDI/palette writers and exclusive presentation remain unresolved.
