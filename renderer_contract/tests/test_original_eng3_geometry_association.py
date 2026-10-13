"""Replay the bounded original ENG3 producer fixture into a fresh work directory."""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--eng3", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path,
                        help="fresh directory below --repo-root/work")
    parser.add_argument("--deps", type=Path, default=Path(r"C:\tools\hercules\renderer-oracle-python"))
    parser.add_argument("--analysis-deps", type=Path, default=Path(r"C:\tools\hercules\python"))
    args = parser.parse_args()
    repo = args.repo_root.resolve()
    output = args.output.resolve()
    try:
        output.relative_to((repo / "work").resolve())
    except ValueError:
        parser.error("--output must stay below --repo-root/work")
    if output.exists() or not output.parent.is_dir():
        parser.error("--output must be a fresh directory with an existing parent")
    fixture = Path(__file__).resolve().parents[1] / "fixtures" / "eng3_geometry_association" / "run_fixture.py"
    command = [sys.executable, str(fixture), "--repo-root", str(repo),
               "--exe", str(args.exe.resolve()), "--eng3", str(args.eng3.resolve()),
               "--output", str(output), "--deps", str(args.deps.resolve()),
               "--analysis-deps", str(args.analysis_deps.resolve())]
    subprocess.run(command, check=True)
    report_path = output / "eng3_geometry_association.json"
    report = json.loads(report_path.read_text(encoding="utf-8"))
    association = report["association"]
    if report["status"] != "ORIGINAL_ENG3_FT4_PRODUCER_EXECUTED_SYNTHETIC_CONTEXT":
        raise AssertionError("unexpected fixture status")
    if len(association["source_triplets_signed_word_xyz"]) != 4 or not association["third_point_scratch_reuse"]:
        raise AssertionError("four-point source association or scratch reuse was not observed")
    if association["projected_xy_words_by_point"] != association["packet_xy_words_by_corner"]:
        raise AssertionError("projected XY values do not match the emitted packet corner writes")
    if report["producer_call_boundary"]["handler_gate"].startswith("UNRESOLVED") is False:
        raise AssertionError("fixture improperly promoted the unresolved upstream gate")
    if not report["no_original_code_writes"]:
        raise AssertionError("fixture observed an original code write")
    print(json.dumps({"status": report["status"], "report": str(report_path),
                      "association_points": len(association["source_triplets_signed_word_xyz"]),
                      "packet_and_projection_match": association["projection_to_packet_equal"],
                      "handler_gate": report["producer_call_boundary"]["handler_gate"]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
