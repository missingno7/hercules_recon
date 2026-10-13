# Bounded producer-reachability audit

**Prediction:** TITLE mode 7's computed command word would resolve to the
existing FT4 family. The audited ENG1, ENG3, and TITLE producer routes would
not establish exact FT3 or fixed-sprite tag/link/stride writes.

**Falsifier:** A pinned first-party path would write AC60 tag `0x24/0x26`,
`0x74/0x76`, or `0x7c/0x7e` into a record, link that record into an order table,
and show a cursor advance or equivalent bounded extent.

## Result

TITLE mode 7 is conditionally reachable through the original child dispatcher:
the mode-2 list handler calls `0x2dc0`; that dispatcher reads a child mode byte
at `+0x23`, zero-extends it as a table index, and calls through the mode table
at `0x220d0`. Table entry 7 targets `0x186e0`. This proves a static dispatch
path when an eligible child has mode 7; it does not show that a particular
gameplay object takes that path.

The `0x186e0` writer calls `0x189a0` and stores the returned DWORD at packet
`+0x04`. The helper reads WORD object `+0x4a`, zero-extends it and clamps it to
255, then replicates that value into RGB. It ORs `0x2c` into AH before two
byte shifts, so the returned DWORD's byte `+3` is `0x2c`. If byte object
`+0x54` has bit `0x04`, it additionally ORs `0x02000000`, changing that byte
to `0x2e`. Thus the packet command byte at `+0x07` is `0x2c` or `0x2e`, both
already recognized by AC60 as FT4-compatible. The writer inserts the record
into its order-table bucket and advances its cursor by `0x28`; that stride is
not a backing-allocation capacity claim.

This closes the command-domain question for TITLE `0x186e0`, but does not add
an FT3 or fixed-sprite producer. The exact unresolved witnesses are:

| Consumer family | AC60 direct minimum | Missing producer witness |
|---|---:|---|
| FT3 `0x24/0x26` | `0x1e` | No exact first-party tag store linked to an order-table write and producer stride/extent in the cited routes. |
| Fixed 8x8 sprite `0x74/0x76` | `0x0e` | Same missing tag/link/stride witness. |
| Fixed 16x16 sprite `0x7c/0x7e` | `0x0e` | Same missing tag/link/stride witness. |

The already audited frontiers explain the limit without turning it into a
whole-module absence claim. ENG1's native mesh route `0x2df20` calls the GT4
callback and advances `0x34`; its existing tile/draw routes cover the known
`0x68`, `0x64`, and FT4 forms. ENG3's indexed routes cover FT4, delegation,
`0x68`, and the variable-sprite writer `0x72a0` (`0x64`, `0x14` stride). The
TITLE mode receipts cover known FT4/`0x68` output and delegated children;
mode 7 now has an exact computed FT4 command domain. None is an exhaustive
search of every DLL function or resource-driven path. A scalar passed to
TITLE helper `0x28c0` remains an input value until a packet write/link path
connects it to AC60.

## Original evidence

| Module/span | RVA | Size | SHA-256 | Use |
|---|---:|---:|---|---|
| TITLE mode table (data) | `0x220d0` | `0x20` | `415195e248ded9aefc3bba1701c680ed3f995fb163b29400793362439889d23c` | Entry 7 maps to `0x186e0`; entry 2 maps to `0x17b90`. |
| TITLE mode-2 list handler | `0x17b90` | `0x60` | `a2c89c494a260cee9c55c1dec26a7f1bc43be297492f37430ebfe11d347708fc` | Calls child dispatcher at `0x17bda`. |
| TITLE child dispatcher | `0x2dc0` | `0x290` | `4b5325e70cf3bac4c0fbc06d0c6c04c9daf4179e70c542338d45ba381be30a08` | Reads `child+0x23`; index-dispatches through the table. |
| TITLE mode-7 writer | `0x186e0` | `0x2b3` | `7353881ec5fa393a22c46d53ca59d492b41d24b6c4073180b0e939f3e37c8383` | Calls helper and emits/links 0x28-stride FT4 records. |
| TITLE command/color helper | `0x189a0` | `0x3c` | `fed5051e785d04f377193fd255c278671587fdeabe9ecc2a4dedb7ce41e76b59` | Computes color DWORD and exact `0x2c/0x2e` command byte. |
| HERCULES AC60 consumer | `0xac60` | `0x690` | `f7724c04bef582e378b183a12550ba841df1a6fa3660be51da71154aa5c525af` | Reused consumer evidence: reads tag `+7`, routes base `0x2c` as FT4. |

The audit records the checked instruction-site sizes, hashes, and decoded text
in [producer_reachability.json](evidence/producer_reachability.json); it does not include copied opcode arrays. It fully decodes
the bounded TITLE code spans and reads the mode table as data. Cross-module
frontier references and their current hashes are listed in the receipt.

## Reproduction and proof boundary

From the worktree root, with Python and Capstone 5.0.7 available:

```powershell
python renderer_contract/tests/audit_producer_reachability.py `
  --output work/replay/producer_reachability.json
```

If the original images are outside the checkout's ancestor search, pass both
`--title PATH` and `--exe PATH`. Output is rejected unless its normalized path
is below this checkout's `work/` directory. The audit is `STATIC_OBSERVED`;
it performs no original execution, gameplay, fixture, or pixel equivalence
test. AC60 consumer minima remain distinct from producer extents.
