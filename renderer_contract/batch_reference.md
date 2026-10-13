# Three-point projection reference

`hercules_projection_batch_eval` in [hercules_projection.h](hercules_projection.h)
models the bounded arithmetic of host RVA `0xB320`. It is an offline tool API,
with explicit config, three points and a joined 48-byte result. It is not a
replacement for the zero-argument slot-107 callback. The live diagnostic still
calls the original callback once in its native position.

Each row uses wrapped 32-bit accumulation, arithmetic shift by 12, wrapped
translation addition, then signed low-WORD narrowing. Batch Z remains
unclamped. Nonzero Z uses signed division toward zero; zero Z skips division
and still writes screen XY using the wrapped `focal << 16` factor. Products,
center additions and screen WORD stores retain the original widths.

On division overflow the tool reports non-success. Earlier points remain
complete, the current camera triplet remains written, and current/later screen
fields plus later camera fields remain untouched. Raw EAX is exposed only on
success. This models observable partial host storage, not a completed primitive
or a substitute exception policy for the original process. Config, input and
result views require separate storage; arbitrary native pointer aliasing is
outside this joined-view API.

Five completed synthetic cases and one point-1 `INT_MIN / -1` fault were compared
directly with original x86 execution. The separately VC5-built model matched
camera/screen values, ordered result stores, successful raw EAX and the fault
canaries: 83 native output stores matched 83 model stores. Cases distinguish
sum-before-shift, negative arithmetic-shift rounding, wrapped three-product
sums, signed WORD wrap, negative Z and zero Z. The existing single-point
reference and diagnostic also pass a fresh 14-case, 632-assertion regression.
VC5 and GCC each pass the 19 C/C++ suites, including the extended projection tests.

Accepted compact evidence is [batch_reference.json](evidence/batch_reference.json).
The [prediction and falsifier](evidence/batch_reference_prediction.json) retain
their original byte identity. Reproduce with pinned tools and a new scratch
directory from the repository root:

```powershell
& $pythonExe -B renderer_contract/tests/test_original_batch_reference.py `
  --repo-root . --exe $originalExe --output work/replay_batch_reference/report.json
```

The runner builds an import-free fixed-base x86 PE from the current public
source, maps it separately from the read-only original image, and rejects
unknown execution/data endpoints or input hash changes. No original Windows
process, GPU, packet rasterization or gameplay is exercised. These finite
cases do not establish exhaustive equivalence or all-input aliasing behavior.
