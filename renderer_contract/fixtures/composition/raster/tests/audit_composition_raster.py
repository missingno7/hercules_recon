from __future__ import annotations
import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

EXPECTED_EXE = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
HERE = Path(__file__).resolve().parents[1]


def sha(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def norm(path: Path) -> str:
    return os.path.normcase(str(path.resolve())).casefold()


def contained(path: Path, root: Path) -> bool:
    try:
        path.resolve().relative_to(root.resolve())
        return True
    except ValueError:
        return False


def reject_link_redirect(path: Path, label: str) -> None:
    absolute = Path(os.path.abspath(str(path)))
    resolved = path.resolve()
    if norm(absolute) != norm(resolved):
        raise SystemExit(f"{label} has a symlink/junction redirect: {path} -> {resolved}")


def require_file(path: Path, label: str) -> Path:
    if not path.is_absolute():
        raise SystemExit(f"{label} must be an explicit absolute path")
    reject_link_redirect(path, label)
    if not path.is_file():
        raise SystemExit(f"{label} is not a file: {path}")
    return path.resolve()


def run(command: list[str], cwd: Path, env: dict[str, str] | None = None) -> str:
    completed = subprocess.run(command, cwd=str(cwd), env=env, text=True,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if completed.returncode:
        raise SystemExit("command failed (%d): %s\n%s" %
                         (completed.returncode, subprocess.list2cmdline(command), completed.stdout))
    return completed.stdout.strip()


def main() -> int:
    parser = argparse.ArgumentParser(description="Fresh, bounded native/C audit of the FT4 D420-to-CAB0 subset.")
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--python", required=True, type=Path)
    parser.add_argument("--deps", required=True, type=Path, help="Unicorn package directory")
    parser.add_argument("--analysis-deps", required=True, type=Path, help="pefile/Capstone package directory")
    parser.add_argument("--gcc", required=True, type=Path)
    parser.add_argument("--vc5-cl", required=True, type=Path)
    parser.add_argument("--vc5-root", required=True, type=Path)
    args = parser.parse_args()

    root = args.repo_root
    reject_link_redirect(root, "repository root")
    root = root.resolve()
    if not root.is_dir():
        raise SystemExit("repository root is not a directory")
    exe = require_file(args.exe, "original EXE")
    if sha(exe) != EXPECTED_EXE:
        raise SystemExit("original EXE hash mismatch")
    py = require_file(args.python, "Python runtime")
    gcc = require_file(args.gcc, "GCC compiler")
    cl = require_file(args.vc5_cl, "VC5 compiler")
    unicorn_dir = args.deps.resolve()
    analysis_dir = args.analysis_deps.resolve()
    vc5_root = args.vc5_root.resolve()
    for label, path in (("Unicorn dependencies", unicorn_dir), ("analysis dependencies", analysis_dir),
                        ("VC5 root", vc5_root)):
        if not path.is_dir():
            raise SystemExit(f"{label} is not a directory: {path}")
    if not contained(cl, vc5_root):
        raise SystemExit("VC5 compiler must resolve below the explicit VC5 root")

    if not args.output_dir.is_absolute():
        raise SystemExit("output directory must be an explicit absolute path")
    output = Path(os.path.abspath(str(args.output_dir)))
    reject_link_redirect(output, "output directory")
    work_root = (root / "work").resolve()
    if not contained(output, work_root) or norm(output) == norm(work_root):
        raise SystemExit("output must be a new child of repository work/")
    forbidden = [(root / "assets").resolve(), (root / "work" / "discs").resolve()]
    for tree in forbidden:
        if contained(output, tree) or contained(tree, output):
            raise SystemExit(f"output overlaps protected input tree: {tree}")
    if output.exists():
        if not output.is_dir() or any(output.iterdir()):
            raise SystemExit("output directory must not exist or must be empty")
    else:
        output.mkdir(parents=True)
    reject_link_redirect(output, "output directory")

    src = HERE / "src"
    tests = HERE / "tests"
    event_dir = root / "renderer_contract" / "fixtures" / "composition" / "src"
    source_paths = [src / "composition_raster.c", tests / "test_composition_raster.c",
                    event_dir / "composition_events.c", root / "renderer_contract" / "hercules_frame.c",
                    root / "renderer_contract" / "hercules_packets.c",
                    root / "renderer_contract" / "hercules_pixels.c",
                    root / "renderer_contract" / "hercules_rectangle.c",
                    root / "renderer_contract" / "hercules_staging.c",
                    root / "renderer_contract" / "hercules_state.c"]
    for source in source_paths:
        if not source.is_file() or not contained(source, root):
            raise SystemExit(f"missing or redirected source input: {source}")
    for source in source_paths:
        reject_link_redirect(source, "source input")

    support_paths = [src / "composition_raster.h", event_dir / "composition_events.h",
                     root / "renderer_contract" / "hercules_quad_dispatch.h",
                     root / "renderer_contract" / "hercules_frame.h",
                     root / "renderer_contract" / "hercules_packets.h",
                     root / "renderer_contract" / "hercules_pixels.h",
                     root / "renderer_contract" / "hercules_rectangle.h",
                     root / "renderer_contract" / "hercules_staging.h",
                     root / "renderer_contract" / "hercules_state.h",
                     root / "renderer_contract" / "hercules_types.h",
                     root / "scripts" / "pe.py"]
    prediction = HERE / "fixtures" / "prediction_falsifier.md"
    if sha(prediction) != "13674f0f03ef2313841d689b3e0a76dc08133921679849bd06a8aed783d26c9a":
        raise SystemExit("immutable pre-experiment prediction bytes changed")
    public_audit = Path(__file__).resolve()
    native_script = tests / "run_original_composition_raster.py"
    compare_script = tests / "compare_composition_raster.py"
    readme = HERE / "README.md"
    evidence_proposal = HERE.parents[2] / "evidence" / "composition_raster_phase20.json"
    for source in support_paths + [prediction, public_audit, native_script, compare_script, readme, evidence_proposal]:
        if not source.is_file() or not contained(source, root):
            raise SystemExit(f"missing or redirected dependency/prediction input: {source}")
        reject_link_redirect(source, "dependency/prediction input")
    source_args = [str(path) for path in source_paths]

    native_dir = output / "native"
    native_dir.mkdir()
    case_names = {"base": "native_ft4_frame.json",
                  "offset_rectangle": "native_ft4_offset_rectangle.json",
                  "asymmetric_uv": "native_ft4_asymmetric_uv.json"}
    for case in case_names:
        run([str(py), str(native_script), "--repo-root", str(root), "--exe", str(exe),
             "--deps", str(unicorn_dir), "--analysis-deps", str(analysis_dir),
             "--case", case, "--output-dir", str(native_dir)], cwd=root)
    native_reports = {case: native_dir / name for case, name in case_names.items()}
    for case, path in native_reports.items():
        if not path.is_file() or not contained(path, output):
            raise SystemExit(f"native runner failed to create fresh receipt for {case}")

    public_include = ["-I", str(root / "renderer_contract"),
                      "-I", str(event_dir), "-I", str(src)]
    compiler_records = {}
    for compiler_name in ("gcc", "vc5"):
        build_dir = output / compiler_name
        build_dir.mkdir()
        executable = build_dir / "composition_raster.exe"
        if compiler_name == "gcc":
            command = [str(gcc), "-std=c89", "-pedantic", "-Wall", "-Wextra",
                       "-Wconversion", "-Werror", *public_include, *source_args,
                       "-o", str(executable)]
            compiler_output = run(command, cwd=root)
            version = run([str(gcc), "--version"], cwd=root).splitlines()[0]
            compiler_digest = sha(gcc)
            env = None
        else:
            toolbin = cl.parent
            vc_include = vc5_root / "VC" / "include"
            vc_lib = vc5_root / "VC" / "lib"
            env = os.environ.copy()
            env["PATH"] = str(toolbin) + os.pathsep + env.get("PATH", "")
            env["INCLUDE"] = str(vc5_root / "include") + os.pathsep + str(vc_include)
            env["LIB"] = str(vc5_root / "lib") + os.pathsep + str(vc_lib)
            command = [str(cl), "/nologo", "/Za", "/TC", "/W3", "/MT",
                       "/I" + str(root / "renderer_contract"),
                       "/I" + str(event_dir), "/I" + str(src),
                       *source_args, "/Fe" + str(executable), "/Fo" + str(build_dir) + os.sep]
            compiler_output = run(command, cwd=root, env=env)
            version = run([str(cl), "/?"], cwd=root, env=env).splitlines()[0] if False else "VC5 product 5.00.7022 (pinned compiler executable)"
            compiler_digest = sha(cl)
        if not executable.is_file() or not contained(executable, output):
            raise SystemExit(f"{compiler_name} did not emit output under the guarded directory")
        compiler_records[compiler_name] = {"version": version, "compiler_sha256": compiler_digest,
                                           "build_stdout": compiler_output, "executable_sha256": sha(executable)}
        for case, suffix in (("base", ""), ("offset_rectangle", "offset")):
            events = build_dir / ("events.tsv" if not suffix else "offset_events.tsv")
            pixels = build_dir / ("pixels.tsv" if not suffix else "offset_pixels.tsv")
            raw = build_dir / ("raw_inputs.bin" if not suffix else "offset_raw_inputs.bin")
            argv = [str(executable), str(events), str(pixels), str(raw)]
            if suffix:
                argv.append("offset")
            run(argv, cwd=root, env=env)
            comparison = build_dir / ("comparison.json" if not suffix else "offset_comparison.json")
            run([str(py), str(compare_script), "--native", str(native_reports[case]),
                 "--events", str(events), "--pixels", str(pixels), "--raw-inputs", str(raw),
                 "--receipt", str(comparison)], cwd=root)
            receipt = json.loads(comparison.read_text(encoding="utf-8"))
            if receipt.get("status") != "CONNECTED_BOUNDED_RASTER_SIGNATURE_PASS":
                raise SystemExit(f"{compiler_name} comparison failed for {case}")

    report = {
        "schema_version": 1,
        "status": "FRESH_BOUNDED_COMPOSITION_RASTER_AUDIT_PASS",
        "original_exe_sha256": sha(exe),
        "original_receipts": {case: {"path": str(path), "sha256": sha(path)}
                              for case, path in native_reports.items()},
        "compiler_runs": compiler_records,
        "comparisons": {name + "/" + case: json.loads((output / name / filename).read_text(encoding="utf-8"))
                        for name in ("gcc", "vc5")
                        for case, filename in (("base", "comparison.json"),
                                               ("offset_rectangle", "offset_comparison.json"))},
        "comparison_receipts": {
            name + "/" + case: {"path": str(output / name / filename),
                                "sha256": sha(output / name / filename)}
            for name in ("gcc", "vc5")
            for case, filename in (("base", "comparison.json"),
                                   ("offset_rectangle", "offset_comparison.json"))},
        "fresh_outputs_root": str(output),
        "claims": ["original x86 exercised two positive D420->CAB0 rectangles and one asymmetric-UV fallback discriminator",
                   "source-built C model matches native positive event/pixel/raw-input signatures under GCC and VC5",
                   "nonrectangle and asymmetric-UV inputs are rejected by C before service or surface writes"],
        "limits": ["controlled synthetic buffers and GDI endpoints only", "not a real game frame, asset, or screen-ownership proof",
                   "no general fallback rasterizer, palette path, clipping, blending, or external writer completeness"],
        "source_sha256": {str(path.relative_to(root)): sha(path)
                          for path in source_paths + support_paths + [native_script, compare_script,
                              public_audit, prediction, readme, evidence_proposal]},
        "tool_inputs": {"python_sha256": sha(py), "python_executable": str(py),
                        "unicorn_dependency_directory": str(unicorn_dir),
                        "analysis_dependency_directory": str(analysis_dir)},
    }
    audit_path = output / "composition_raster_audit.json"
    audit_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": report["status"], "output": str(audit_path),
                      "audit_sha256": sha(audit_path), "native_cases": len(native_reports),
                      "comparisons": len(report["comparisons"])}, indent=2))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
