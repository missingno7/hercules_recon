# Camera and actor smoothing recovery

This records the initial camera-chain recovery. The later
[level-6 follow-up](eng1_level6_color.md) implements the supported color paths
and narrows the original level-6 guard.

The real reconstructed `2fb70 -> 20ea0` chain now has an original/source
comparison. This exposed an error in the existing camera source: the native
threshold scan advances after its final read, while the old C scan did not.
The old source failed 72 field checks in the first 148 composed cases. The
instruction-supported post-increment fixes the X/Y/Z step selection, including
the step parity used to skip later iterations. Four source-fixture expectations
were corrected; the independent callback mutation checks remain.

`camera_smoothing_26730.c` supplies the actor pipeline's non-level-6 smoothing
call using the existing camera and pipeline storage providers. It preserves
signed WORD count/sum wrapping, the yaw list's skipped first entry and minimum
divisor of two, angular correction, level-1 adjustment and the offset smoothing
thresholds, including the native signed `abs(INT_MIN)` outcome. It adds no data
owner or runtime dependency. REA supplied structure; instructions and original
comparisons determine the implemented contract.

At this checkpoint the function rejected level 6 and lists extending beyond the existing
`0x8c0` pipeline storage prefix, before changing camera state. These limits are
recorded as an incomplete branch, rather than an all-entry placeholder. They
are source safety bounds, not claims about original list capacities.

Level 6 was the next dependency. Native `26b45..26b8c` visits eight DWORD
stack lanes, but only six lanes in each group are visibly initialized before
that loop. Its change flag can determine the call to the existing `5a60` color
routine; the last two lanes must not be invented as zero. The branch also uses
pending color storage and pointer lists that require owner/producer recovery.
The follow-up identifies those producers, proves the stack dependence and
implements the paths independent of the two extra stack lanes.

```powershell
python scripts/eng1_bounded_differential.py camera-chain --native-eng1 <pinned-ENG1.DLL>
```

Fresh VC5 verification passes **324 original/source cases and 6,614 checks**:
190 camera/bounds cases, 130 non-level-6 smoothing cases and four compositions
of smoothing followed by the camera/bounds path. Coverage includes threshold
equalities, signed and wrapped magnitudes, count/average edges, the last measured
WORD, complete native-module write auditing, input canaries, ESP and exceptions.
Five separate source-only guards pass 75 checks; they do not execute the native
level-6 path. The DLL is mapped without imports or initialization; no game runs.

Regression passes 134 strict rows, 38 source fixtures, 85 Python tests and the
59-provider/923-field data audit. The genuine graph has 369 mapped entries,
400 function symbols, 529 unresolved symbols and zero duplicate providers; all
76 strict ENG1 functions remain exact. It emits no DLL. Both changed functions
still differ from the original bytes, and `recovery.json` remains unchanged.

[Compact evidence](../evidence/eng1_camera_chain.json) pins the inputs,
prediction, instruction observations and private verification receipts.
