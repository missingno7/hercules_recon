# Matching velocity pilot — 25 October 2026

Canonical baseline: `190015ef39a7338f8c531768d98e54daad6df603`.
Reconstruction and acceptance commit: `fc8bba0234c01e8725d9ecbfa21038061f173f47`.
This pilot adds **2 RELOCATED functions / 144 bytes** and four genuine ENG1
providers. It does not establish a sustained throughput speedup.

## Gaps checked against the implementation

| Capability | Finding and action |
| --- | --- |
| Worker lifecycle | Native spawn/continuation already exists. Reused completed Luna workers with bounded follow-ups; no new scheduler. |
| Ownership / recovery | Shared filesystem requires one integrator, disjoint scopes and source/context hashes. Native task status and compact handoffs provide recovery; no extra lease database. |
| External monitor | Read-only audit of `D:/Prog/codex_agents_monitor`: `cx run` dispatches one session, resume/status exist, and the dashboard exposes session tokens/timing. It has no task dependency scheduler; its `codex_exec` filter excludes these native workers. No external changes or service startup. |
| Shared proof work | Added on-demand `region.py matching-frontier`, derived from ledger, recorded census and pinned typed handoffs. It is planning data, never acceptance. |
| Early RAW rejection | Extracted existing unchanged-unit metadata validation and moved it before fresh full protection. `match.py preflight --raw --select` exposes compiler-free readiness using the same checks. |
| Integration batching | Existing repeatable `--bindings` and journaled promotion already suffice. One fresh protected batch accepted both ready callers. |
| Compiler capacity | Retained previous 8-versus-24 measurement; no reason to repeat it or equate compiler jobs with reasoning workers. |
| Invocation accounting | Added opt-in per-attempt events to the existing compiler wrapper, including compiler duration and calls from temporary fixture roots. No shared cache or scheduler. |
| Wait / token metrics | Native runtime does not expose complete task timing, integration wait, tokens or monetary cost here. These are unavailable, not zero. |

The frontier deliberately does not infer an owner from a name/RVA or turn masked
equality into readiness. Real handoffs for `title.0cce0` and `title.0cd00` produced
one shared same-TU BSS investigation rather than two caller-body tasks. All source,
target and eight evidence pins were current; the older census remained
`recorded_only`. After integration, a repeat automatically filtered the accepted
metadata caller and retained two TITLE candidates behind that one owner blocker.
Indexed negatives require the exact region context; omitted candidates are explicit.

The previous 122-compile rejected integration concerned mixed-status unit metadata.
That guard now runs before `verify_rows`; regression tests require zero compiler
calls on that rejection. This is an early-rejection mechanism, not a measured claim
that this pilot saved 122 calls. The reference handoff also rejected an incorrect
draft hash and an incomplete loader-zero owner include context before compiling.

## Published reconstruction

| Function | Result |
| --- | --- |
| ENG1 `resource_metadata_pointer`, 0x31080 | 96 B RELOCATED; four independently grounded references; current source unchanged. |
| ENG1 `spatial_query_1a9b0`, 0x1a9b0 | 48 B RELOCATED; two independently inspected callees and the existing verified engine-interface owner, with exact COFF uses. |
| ENG1 0x1a9e0 / 0x1ab00 / 0x26de0 | Reconstructed in the same canonical family; NEAR, without STRICT claims. |

The family preserves the nonzero query-selector return, signed six-WORD bounds,
touching overlap, global reloads and call order. A finite Unicorn comparison passed
18/18 cases. It models `actor_bounds_2a880` identically on both sides and populates
at most two synthetic query rows; it proves neither that service nor the owner or
capacity of `g_71ee0`. No original OS module or game was executed.

The necessary fresh ENG1 source-graph build added one unit (251→252) and four
mapped providers (969→973). Unresolved names fell **622→620**, function names
327→325; 295 untyped externals remain. Six consumer edges now resolve through two
names. All 251 existing units' structural contributions were preserved after two
reviewed symbol-alias normalizations; all 79 RAW functions remain exact and there
are zero duplicate providers. Link still exits 25: this is not a standalone DLL.

Accepted totals are **545 functions / 55,872 bytes**: 144 RAW, 358 RELOCATED,
43 legacy LINKED. Per module: EXE 61 / 2,368 B; TITLE 56 / 6,688 B;
ENG1 333 / 35,248 B; ENG3 95 / 11,568 B. These are accepted ledger extents,
not a denominator for all game-source or vendor-library coverage.

All 543 prior accepted row objects remain unchanged. The baseline gate used
123 compiler calls for 502 obligations in 47.409 seconds. The one successful
promotion used 125 calls for 504 obligations in 48.157 seconds, followed by all
43 legacy LINKED checks (86 unchanged provenance drifts). The full suite passed
416 tests. After adding optional compiler accounting, the suite passed 422 tests
and a freshly compiled RAW control remained exact. There was no rejected full
integration. The new metric tests use simulated subprocesses, not game binaries.

## Cost and limits

Focused source/evidence work used 25 historical compiler calls: source recovery 9,
resource harvest including its stopped source-context trial 7, cleanup identity 3,
canonical finite fixture 2, diagnostic staging 2 and final focused references 2.
Yield was 8 accepted functions / 576 accepted bytes per 100 focused calls, plus
two unresolved names closed. The cleanup identity work remains unaccepted.

Before instrumentation validation, 590 completed compiler invocation logs were
retained in the normal wrapper directory. The new event collector then measured
30 additional real calls: one RAW control and 29 regression-fixture calls, including
six intentional compiler rejections. All events identify `msvc5_rtm`; total measured
compiler execution was 1.109 seconds for those tiny fixtures/control. No integration
was rejected. Earlier directory accounting is not guaranteed exhaustive, so total
end-to-end cost is **at least 620**, not just 25. This includes 252 necessary
source-graph compiles and protection overhead. Yield is at most 0.32 functions /
23.2 bytes per 100 calls. The prior campaign's 585 calls
for 6 / 272 B had a different task mix and no fresh source-closure build. Neither
comparison demonstrates improved end-to-end throughput.

The observed task span includes a multi-hour interruption before continuation.
No uninterrupted worker-time, idle-time or hourly speedup is inferred from it.
Ready results were committed and pushed before the optional remaining audit.
The actionable benefit demonstrated here is one shared proof task, earlier rejection,
reusable owner evidence and a completed protected integration with source closure.
Scratch receipts are under `work/velocity/`; they grant no matching acceptance.

## Next assignments

1. Fix the ENG1 cleanup declaration conflict before promoting its two 80-byte
   callers: the selected writer provider returns void, while a retained sprite
   unit's omitted-body prototype returns a pointer. All observed callers discard
   EAX. Preserve the existing native identity review; compare peers after correcting
   source declarations instead of repeating the helper audit.
2. Review the narrow same-TU BSS representation for TITLE 0x0cce0/0x0cd00 and
   0x05d90 (112 candidate bytes). Existing extent and negative controls are reusable;
   indexed-only anchors and unproved capacities remain blocked.
3. Recover `g_71ee0` ownership and writers once for its render/query consumers.
   The new source does not assign it a guessed capacity.
4. Reconcile active ENG1 readiness source at 0x3750/0x37a0. A matching TITLE-source
   twin does not prove the configured ENG1 provider; the bounded two-call source
   trial stopped with a real scan residue.

Keep the 97 interface wrappers, ambiguous focus/kind-info owners, stripped-EXE
anchors and exhausted declaration/algebraic/permuter searches closed without new
evidence. Expand reasoning concurrency only as independent proof-ready work and
integration capacity become available.
