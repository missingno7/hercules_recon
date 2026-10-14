"""Fresh strict gate with read-only originals in a separate checkout's root.

Uses the existing acceptance implementation and this checkout's recovery state.
Only target path resolution differs; no source, ledger or oracle is changed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import match  # noqa: E402


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-root", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    args = parser.parse_args()
    receipt = args.receipt.resolve()
    receipt.relative_to(ROOT / "work")
    state_path = ROOT / "recovery.json"
    before = state_path.read_bytes()
    state = json.loads(before)
    results = []
    for row in state["functions"]:
        if row["status"] != "FUNCTION_MATCH":
            continue
        bound = dict(row)
        original = (args.original_root.resolve() / row["target"]).resolve()
        original.relative_to(args.original_root.resolve())
        bound["target"] = str(original)
        result = match.check(bound)
        results.append(result)
        if not result["exact"]:
            print(row["id"] + ": MISMATCH", flush=True)
    assert state_path.read_bytes() == before, "recovery state changed during gate"
    report = {"schema_version": 1, "purpose": __doc__,
              "recovery_sha256": hashlib.sha256(before).hexdigest(),
              "count": len(results), "exact": sum(r["exact"] for r in results),
              "results": results}
    receipt.parent.mkdir(parents=True, exist_ok=True)
    receipt.write_bytes((json.dumps(report, indent=2) + "\n").encode())
    print(f"historical strict gate: {report['exact']}/{report['count']} exact")
    return 0 if results and all(r["exact"] for r in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
