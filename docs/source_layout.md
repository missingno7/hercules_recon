# Current source layout and cleanup

Current reconstructed game source lives in `src/`, grouped by original PC module:

```text
src/
  pc/       HERCULES.EXE host and services
  title/    TITLE.DLL
  eng1/     ENG1.DLL gameplay and data
  eng3/     ENG3.DLL
  shared/   reused logic, interfaces, layouts and owners
config/
  eng1/     source selection and incomplete dependency inventory
  title/    source selection, data units and historical link plan
  research/ bounded compiler experiment recipes
```

This follows the useful convention in
[Blupi](https://github.com/missingno7/blupi_recon) and
[Rayman](https://github.com/missingno7/rayman_recon): reconstructed source stays
together, while metadata records each function's verification status. Hercules
retains its existing `recovery.json` authority and its separate PC module owners.

`behavioral/` was an earlier discovery/build work stream, not a matching tier.
Strictly accepted bodies had accumulated there, while many other accepted units
still lived under `calibration/`. Both names obscured the single implementation.
The cleanup retires those source directories. It does not split mixed-status
compiler units, infer historical source names, merge headers, retune expressions
or change gameplay logic. Original addresses in unresolved names remain useful
evidence until their semantics support a better name.

The move inventory records 549 file moves, including two TITLE configuration
files. Current build configuration is under `config/`; the former build evidence
files remain historical snapshots. Five unselected superseded probes move to
`prototypes/reconstruction/` with an explicit non-provider description. Every
ledger source points to `src/`. Module build plans select the actual providers;
compiling all files by wildcard is not a supported build.

Run `python scripts/source_status.py` for a derived overview, or pass a filename,
module, function ID or exact historical source path. It reads current ledger rows
and translates old paths through the migration review for inspection. It grants
no acceptance and creates no parallel ledger. A source unit can contain RAW,
RELOCATED, retained LINKED, diagnostic and reconstructed-only functions together.
Headers and data-only owners may have no function rows.

The migration baseline is commit `73b611a1fbf8edf3666873ab4beb9c14c7d3b62d`.
Original reference manifests remain unchanged. Thirty-two reviewed descendants
under `evidence/source_layout_2026_10_25/` pin their immutable parents, the layout
review, relocated owners/headers and current tool inputs. Target identities,
function extents, binding destinations and native witness instructions remain
the same. Only source/include paths and the corresponding context/evidence hashes
are refreshed. No new functions gain acceptance.
The retained linked rows also record the new path-only link-plan identity;
their existing broader source/image provenance drifts are not relabeled as fixed.

Fresh before/after compilation checks 426 source/compiler profiles. Complete
section bytes, data, COMMON objects, relocations and symbol identities agree.
COFF FILE records must contain the exact old/new compiler input paths. Two MFC
units also reorder undefined symbols: weak fallback indices are resolved to the
same named target, preserving every other auxiliary byte. These are object
metadata checks, with no instruction, data or reference normalization and no
object patching. The three recorded whole-unit inventory hashes are refreshed
only after those checks; their accepted function bytes remain identical.

The ordinary strict verifier freshly protects all 446 strict proofs for the same
487 accepted ledger rows. Retained linked proofs, supported masked diagnostics,
the 321-test suite, source-graph comparison and bounded compositions provide the
remaining regression checks. Exact results and hashes are in the final layout
receipt. ENG1 still has missing dependencies; reorganizing source does not make
the game complete or prove runtime behavior.

The broader source-only fixture survey contains 59 groups: 56 pass. Three failures
also reproduce from the pre-cleanup commit using their bounded 23-file source
closure: `resource_pool_pipeline` crashes, while `host_archive_pipeline` and
`host_file_service` lack `g_host_engine_interface` in their old fixture setup.
These existing fixture issues are separate from the passing matching and layout
gates. The notification fixture's stale owner/prototype is repaired: it uses the
current four-argument interface and an ignored source projection whose complete
16-byte contribution must equal a fresh compile of the real `host_setup.c` unit.
No replacement helper enters game source and no test assertions are removed.

The review's `moves` map is the old-to-current path index. Dated reports keep their
historical paths and input hashes; replay their original state at the baseline
commit when required. Use the current scripts/configuration for new work. The
legacy `eng1_behavioral.py` and `title_behavioral.py` command names remain supported
source-graph tools; their names do not classify the source or its matching status.
