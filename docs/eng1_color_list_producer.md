# Level-6 color-list producer

`1ebf0` now has bounded ordinary C source in the genuine ENG1 graph. It uses
the existing transition state, copied host RNG bridge, sound implementation and
shared actor-pipeline queue. Ten observed six-byte rows at `58a20` join the
existing transition storage provider; no second queue, audio service or renderer
implementation was introduced.

The callback preserves the original service order. Selection `-69` becomes 9
before RNG; an ordinary RNG zero sets selection 4. The late signed actor WORD
at `+0x32` selects the row, which is captured before the count/pointer stores.
An odd context tick decrements the live WORD after append. RNG and sound can
change the actor, current RNG limit, queue count or context; those values are
reloaded at their original consumption points. Previous selection `-1` takes
the sound-only branch without reading the actor, including a null actor.

REA's existing seeded `1ebf0` query helped identify the state and row structure.
Independent decoding of the original instructions and fresh VC5 comparisons
provide the implementation evidence. No REA pseudocode was promoted as an exact
match, and `recovery.json` is unchanged.

## Verification and limits

```powershell
python scripts/eng1_bounded_differential.py color-list-producer --native-eng1 <pinned-ENG1.DLL>
```

The existing color fixture passes 74 new original/source cases and 2,262 checks,
plus four source-only guards/16 checks. Cases include all ten rows, both tick
parities, signed/unsigned boundaries, the last supported queue slot, service
mutations and a real actor-WORD/count alias. Composed cases call actual
`26730 -> 5a60 -> 5e00`; comparisons cover the whole pipeline, actor guards,
contexts, RNG/audio transcripts, all 512 LUT entries, guarded frames, ESP and
native module writes. The 2,484 prior color states also pass: 2,558 total states
and 38,279 checks. The known fade callback upper-DWORD residue stays diagnostic.

Caching the context before callbacks fails two cases. Reloading the row after
the count store also disagrees on the actor/count alias. These controls distinguish
the live state and store order from superficially equivalent source.

Rows beyond the ten-row prefix and append counts outside `0..134` fail explicitly
after preceding service effects, before append/decrement. These are measured
support boundaries, not assertions of original capacities. Sound composition
covers the real silent-bank path; it establishes no general audio/device claim.
Arbitrary pointer aliases and malformed objects remain unverified.

All 134 strict matches, 38 source fixture groups, 85 Python tests and the
60-provider/930-field data audit pass. The genuine graph has 370 mapped entries,
401 function symbols, 528 unresolved symbols and zero duplicate providers. All
76 strict ENG1 functions remain exact; no runnable DLL is emitted. The new
function's fresh raw comparison is unequal, so no `FUNCTION_MATCH` is claimed.

The next dependency is pending-target producer `1ed30` and its existing
attribute/scale services. The equal-color/empty-list ambient-stack dependency
in `26730` remains separate and explicitly unsupported. No original game process
or DLL initializer ran.

[Behavior evidence](../evidence/eng1_color_list_producer.json) and
[data evidence](../evidence/eng1_color_list_storage.json) record the pinned inputs.
