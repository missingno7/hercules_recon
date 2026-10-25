# Worklist C frontend and indexed-owner review

The pinned C frontend closes normal `270c0`'s last two nonreference differences
without changing the typed function logic. Middle `27720` drops from seventeen
to three differences. These are compiler diagnostics, with zero new accepted
matches. The ledger remains 479 accepted entries / 48,304 bytes.

| Reproducing profile | Normal, 272 bytes | Middle, 192 bytes | Alternate, 144 bytes |
| --- | ---: | ---: | ---: |
| Previous typed C++ recipe | 2 | 17 | 0 |
| Same typed logic through C | 0 | 3 | 3 |

The only frontend control edits remove the C++ linkage wrapper and select `.c`.
The normal flag ORs then use the original registers; middle's group-load/push
schedule also agrees. The remaining C differences are three scale-one SIB
operand-order bytes in each middle/alternate body. Each address is arithmetically
equivalent, but these bytes still fail strict instruction equality. Alternate
already has a zero-residual C++ recipe. This is two reproducing profiles, not a
single recovered historical translation unit or proof of its source language.

Four further C controls—DWORD address arithmetic, unsigned index masks,
index-first subscripting and byte/DWORD/function-pointer field strides—produce
the same complete function contributions and relocation inventories. Stop
repeating those controls without a new discriminator. Fresh ENG1/ENG3 twin
comparisons confirm all three complete native spans agree after HIGHLOW and
external REL32 operands are masked; this is a diagnostic, not a reference proof.

Replay the C profile with the existing probe:

```text
python scripts/worklist_typed_probe.py --recipe evidence/worklist_c_frontend_recipe_2026_10_25.json --output work/worklist_c_frontend
```

The original default C++ recipe remains compatible and reproduces its complete
prior contributions. The probe permits only `.c`/`.cpp` materialization under
ignored `work/`, with recipes under repository evidence. Four focused negative
checks reject other suffixes, changed baselines and out-of-scope output/recipe
paths. Neither profile replaces the guarded production C providers.

All 108 C-profile native/source cases pass. Their native results and case
definitions are identical to the previous C++ run. This retains the same group0,
synthetic-bank, record-alias and root/kind-intervention scope, with modeled
callbacks and no source guard invocations in the private candidate. The nine
actual production prefix-guard controls retain their previous passing receipt
and unchanged inputs. No original OS module is loaded or started.

The indexed-owner question remains open. An independent native HIGHLOW census
accounts for all references whose target lies in the selected eight-byte prefixes:
44 fields in ENG1 at `6fee8`, 73 in ENG3 at `3fbd0`, and three in TITLE at `2a0a0`.
Every selected field belongs to an entry-decoded diagnostic span. This does not
exhaust computed aliases or prove an allocation boundary. ENG1's initializer
uses the live WORD at `713ae`, initially derived from signed byte `70ed0`, and
reloads that count after allocation/population. Those loops supply no fixed
capacity. The first nearby referenced cells are at offsets `a0`, `a0` and `10`
respectively; spacing between named references does not establish array extents.

The existing scalar loader-zero verifier rejects indexed memory operands.
`g_kind_info[1]` remains an observed source prefix. No array support, new owner
extent, scalar-gate exception or callback-mode change was introduced. Factory
callbacks still fall outside `rooted-callback-v1`, even when masked instructions
agree. New acceptance requires independent indexed-owner and dispatch proofs.

The ledger, build plan, production sources, core verifiers and all prior strict
proof inputs remain unchanged. Their previous fresh 438-proof verification is
retained; this turn rechecks input identities rather than repeating unchanged
builds. Current measurements and the compact native reference census are in
`evidence/worklist_c_frontend_progress_2026_10_25.json`; the materializable recipe
is `evidence/worklist_c_frontend_recipe_2026_10_25.json`.
