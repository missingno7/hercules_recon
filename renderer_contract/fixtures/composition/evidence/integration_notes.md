# Integration notes

Install `composition_ordering.md` at `renderer_contract/composition_ordering.md`
and copy the complete `fixtures/composition/{src,tests,evidence}` subtree.
The public audit reuses these existing contract inputs rather than copying
them into the fixture:

- `renderer_contract/hercules_frame.{c,h}`
- `renderer_contract/hercules_packets.{c,h}`
- `renderer_contract/hercules_state.{c,h}` and `hercules_state_wire.h`
- `renderer_contract/hercules_staging.{c,h}`
- `renderer_contract/closure.md`
- `renderer_contract/tests/original_ac60_mixed_runner.py`
- `renderer_contract/tests/oracle_common.py`

Preserve the pre-family prediction bytes. Their pinned sizes and SHA-256 values
are 1,968 / `3892ff88452c5dd021184c79b85d5a9bbb2544f99951af588374cd0b2969465f`
for `fixtures/composition/tests/prediction.json`, and 1,808 /
`761156b5ce9a410a045cbc288f9d2e3a0687f7e2c6c236157f92cfbf594dd896` for
`fixtures/composition/evidence/c_orchestration_prediction.json`. The current
repository-wide `*.json text=eol=lf` rule means both exact paths need `-text`
attributes:

```gitattributes
renderer_contract/fixtures/composition/tests/prediction.json -text
renderer_contract/fixtures/composition/evidence/c_orchestration_prediction.json -text
```

Do not substitute the earlier ignored native receipt for the public audit's
fresh run. `audit_composition.py` creates a receipt beneath its new
`repo/work` output, then checks the original image hash, fixture provenance,
guard trace, and source event projection. C tests compile as a bounded source
model and are not the original host's source/build.

The requested Visual C++ 5 check was attempted with the installed SP3
compiler, `C:\tools\msvc5_sp3\VC\BIN\CL.EXE` (product 5.00.7022, SHA-256
`BF9F9C74F756FED96E13F7F9A4273495C7DDE0A1FB968E3EF6D760AD6D73DFEB`). Windows
refused to start it due to legacy system-DLL relocation. No VC5 compile or
execution result is claimed. GCC 12.2.0 strict C89 compile/run is covered by
the public audit.

Root verification subsequently passed the pinned VC5 bundle; see the current
composition_ordering.md and the separate VC5 receipt. The earlier SP3 startup
failure above is retained as an experiment outcome, not a current blocker.
