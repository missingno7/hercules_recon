"""Execute the pinned TITLE mode-5 FT4 producer and original HERC callbacks."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import stat
import struct
import subprocess
import sys
from collections import Counter


SCRIPT = Path(__file__).resolve()
PUBLIC_PACKAGE = SCRIPT.parents[1]
PREDICTION = PUBLIC_PACKAGE / "fixtures" / "title_geometry_association" / "prediction.md"
EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
TITLE_SHA256 = "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b"

EXE_BASE = 0x00400000
TITLE_BASE = 0x10000000
REFERENCE_BASE = 0x11000000
GAME_STATE = 0x00A8E700
GAME_STATE_SIZE = 0x88
STACK_BASE = 0x0D000000
STACK_SIZE = 0x10000
STACK_TOP = STACK_BASE + STACK_SIZE - 0x100
RETURN_SENTINEL = 0x0E000000
SYNTH_BASE = 0x0C000000
SYNTH_SIZE = 0x100000
OBJECT = SYNTH_BASE + 0x1000
RESOURCE = SYNTH_BASE + 0x2000
PACKET_POOL = SYNTH_BASE + 0x3000
ORDER_TABLE = SYNTH_BASE + 0x4000
REFERENCE_CONFIG = SYNTH_BASE + 0x6000
REFERENCE_POINT = SYNTH_BASE + 0x6100
REFERENCE_RESULT = SYNTH_BASE + 0x6200
INSTRUCTION_BUDGET = 300000

ORIGINAL_SPANS = {
    "title_producer": ("TITLE", 0x181E0, 0x2DC,
                       "dbfacf27b0299c14ddbf9782db11e22d3df9ed5a93eedda41a22fac6243556ca"),
    "title_matrix_setup": ("TITLE", 0x196F0, 0xF1,
                           "12f9af177b988e6c36876e47acf56e2943e9e6165cafafe5626921a59c48ee3b"),
    "title_color_word": ("TITLE", 0x189A0, 0x3C,
                         "fed5051e785d04f377193fd255c278671587fdeabe9ecc2a4dedb7ce41e76b59"),
    "title_slot101": ("TITLE", 0xC760, 0x0F,
                      "9f6c01b24c77c7a68bec34c1e425876906891b381f746ae27cf672650098f6be"),
    "title_slot102": ("TITLE", 0xC770, 0x19,
                      "b0a9549b5b845284ef45d755734549d22d175969332d71d58108f38e3b72db1e"),
    "title_slot103": ("TITLE", 0xC790, 0x06,
                      "5676d8b528153c04bb8608e47329dde37e857dd16ccb6905ce0d95638ccb40ce"),
    "title_slot106": ("TITLE", 0xC7B0, 0x06,
                      "0e5f1be91b03f941ddb414b6a9b8f77ee4d485b91a936ee026b0e2074c951dbf"),
    "title_slot107": ("TITLE", 0xC7C0, 0x06,
                      "c8145068ee02f0f8660cea50c1015e9531db0b2eece33f7acae75cce54cbea09"),
    "title_slot108": ("TITLE", 0xC7D0, 0x0F,
                      "cba845a33d966176a5a0b1007c0a84fd33d83cc01e5148019b08e0a29bae8c59"),
    "title_slot114": ("TITLE", 0xC810, 0x0F,
                      "50c9e23aff25f0798006c4c1cb8ef43f7bcd008952b91e99bf52f6a5651d6a27"),
    "title_slot117": ("TITLE", 0xC820, 0x0F,
                      "85463fae628ad6a5c62dd4ce84832500d2b1bdda0314125964e58505a30112ed"),
    "title_slot164": ("TITLE", 0xCA40, 0x14,
                      "260bddc2c08a348e284bc688aa4523f152921d735a79bbf9093417cc0d1c8bc7"),
    "title_slot165": ("TITLE", 0xCA60, 0x14,
                      "a755c1ada10725a5bc6ff96e767098c770076d3d895dff987bcb1b9df06bae0d"),
    "title_slot174": ("TITLE", 0xCB10, 0x14,
                      "e94e76a957d710546a39396291f980e047c38d5999edb94ea2ae9c42bbc9864f"),
    "title_slot175": ("TITLE", 0xCB30, 0x0F,
                      "146b139b7080f56f46a44b739fe76799751faa015ca396d14c08060e601c4256"),
    "herc_slot102": ("EXE", 0xB430, 0x6A,
                     "25367b48038d67ca7a3181586be659fd7a7e15abd9d5a97488eca8fd249c9484"),
    "herc_slot107": ("EXE", 0xB320, 0x10F,
                     "11368c1fa7fd8cd91b1d7b7de522ad06aacc724991ff4f8d46d7eecf1e7cff97"),
    "herc_slot117": ("EXE", 0xB5C0, 0x47,
                     "3bcc224aaa6a9b2fa5772613b2921c7d6341d90d781fdf136f338f3004b382aa"),
    "herc_slot101": ("EXE", 0xB6F0, 0x24,
                     "6a6848a640a84133abcc29bf3b0c01e2fb1638f36e916ed8e4f68890210aff29"),
    "herc_slot114": ("EXE", 0xB720, 0x1A,
                     "51f3989776f1410d8ffa246269a37fac2b44590d0f6bcf1cdca4937d0b7b0843"),
    "herc_transform": ("EXE", 0xB740, 0xCA,
                       "f745b08e23d8f718c7e269b2642a32a2ea852a1aa86c88dd774df9f555158f2c"),
    "herc_slot106": ("EXE", 0xB920, 0x4E,
                     "055adff43e62ef00b06e5313ee1701ac404cfe6b3286d75648e5c3265ae7ba86"),
    "herc_snapshot": ("EXE", 0xB840, 0x23,
                      "07912507e5fbc4ebaaa61ed9fb4ec06221601164ea8092b9740313f8afd8cfbc"),
    "herc_matrix_loader": ("EXE", 0xB880, 0x32,
                           "b890d113710e54184d9b505fe9c2a80214ec58f7980ebc2700e16f86bc7a8063"),
    "herc_matrix_helper": ("EXE", 0xBB00, 0x12C,
                           "eb77c95173af56f9ae8bb0646959e66ef24bed75a7122545b18169d30ab79217"),
    "herc_matrix_multiply": ("EXE", 0xBC30, 0x9F,
                             "8cf8663405ada692c0a993543b4dc1af1681774963575c3cd50c0364b161d040"),
    "herc_center_setter": ("EXE", 0xB970, 0x14,
                           "2526cc71c8d43572f1aabfacc70678e699908b02a13cf0b72f832b1e4062d252"),
    "herc_focal_setter": ("EXE", 0xB990, 0x0A,
                          "c44e898f7d175fbc52e1761ebbf409c20d9579c27174ca44a631d31909e8c68a"),
}

TITLE_CALLBACK_CELLS = {
    0x2BCD4: 0xB6F0, 0x2BCD8: 0xB430, 0x2BCDC: 0xB840,
    0x2BCE8: 0xB920, 0x2BCEC: 0xB320, 0x2BCF0: 0xB880,
    0x2BD08: 0xB720, 0x2BD14: 0xB5C0, 0x2BDD0: 0xBB00,
    0x2BDD4: 0xBC30, 0x2BDF8: 0xB970, 0x2BDFC: 0xB990,
}

TITLE_WRAPPERS = {
    0xC760: (0x2BCD4,), 0xC770: (0x2BCD8,), 0xC790: (0x2BCDC,),
    0xC7B0: (0x2BCE8,), 0xC7C0: (0x2BCEC,), 0xC7D0: (0x2BCF0,),
    0xC810: (0x2BD08,), 0xC820: (0x2BD14,), 0xCA40: (0x2BDD0,),
    0xCA60: (0x2BDD4,), 0xCB10: (0x2BDF8,), 0xCB30: (0x2BDFC,),
}

ALLOWED_RANGES = [
    ("TITLE", 0x181E0, 0x184BC), ("TITLE", 0x189A0, 0x189DC),
    ("TITLE", 0x196F0, 0x197E1),
    ("TITLE", 0xC760, 0xC76F), ("TITLE", 0xC770, 0xC789),
    ("TITLE", 0xC790, 0xC796), ("TITLE", 0xC7B0, 0xC7B6),
    ("TITLE", 0xC7C0, 0xC7C6), ("TITLE", 0xC7D0, 0xC7DF),
    ("TITLE", 0xC810, 0xC81F), ("TITLE", 0xC820, 0xC82F),
    ("TITLE", 0xCA40, 0xCA54), ("TITLE", 0xCA60, 0xCA74),
    ("TITLE", 0xCB10, 0xCB24), ("TITLE", 0xCB30, 0xCB3F),
    ("EXE", 0xB320, 0xB42F), ("EXE", 0xB430, 0xB49A),
    ("EXE", 0xB5C0, 0xB607), ("EXE", 0xB6F0, 0xB714),
    ("EXE", 0xB720, 0xB73A), ("EXE", 0xB740, 0xB80A),
    ("EXE", 0xB840, 0xB863), ("EXE", 0xB880, 0xB8B2),
    ("EXE", 0xB920, 0xB96E), ("EXE", 0xB970, 0xB984),
    ("EXE", 0xB990, 0xB99A), ("EXE", 0xBB00, 0xBC2C),
    ("EXE", 0xBC30, 0xBCCF),
]


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def is_relative_to(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def reject_reparse_components(path: Path) -> None:
    current = Path(path.anchor)
    reparse = getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400)
    for part in path.parts[1:]:
        current = current / part
        try:
            info = current.lstat()
        except FileNotFoundError:
            continue
        if current.is_symlink() or (getattr(info, "st_file_attributes", 0) & reparse):
            raise ValueError(f"symlink/junction path component is forbidden: {current}")


def validate_projection_report(args) -> None:
    try:
        report = json.loads(args.projection_report.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise SystemExit(f"cannot read --projection-report: {error}")
    if report.get("status") != "BOUNDED_X86_DIFFERENTIAL_AND_BRIDGE_OBSERVED":
        raise SystemExit("projection report does not record the current bounded x86 differential status")
    if report.get("oracle", {}).get("sha256") != EXE_SHA256 or \
            report.get("oracle", {}).get("image_base") != hex(EXE_BASE):
        raise SystemExit("projection report belongs to a different original executable or image base")
    summary = report.get("summary", {})
    if not summary.get("c_reference_matches_native_camera_and_screen_outputs") or \
            summary.get("package_vector_groups_passed", 0) < 12 or \
            summary.get("precision_discriminator_cases", 0) != 2:
        raise SystemExit("projection report does not establish the current public single-point model gates")
    harness = report.get("harness", {})
    current_inputs = {
        "test_original_projection.py": (args.repo_root / "renderer_contract" / "tests" /
                                        "test_original_projection.py", harness.get("sha256")),
        "oracle_common.py": (args.repo_root / "renderer_contract" / "tests" /
                             "oracle_common.py", harness.get("oracle_common_sha256")),
        "scripts/pe.py": (args.repo_root / "scripts" / "pe.py", harness.get("pe_helper_sha256")),
        "hercules_types.h": (args.repo_root / "renderer_contract" / "hercules_types.h",
                             harness.get("shared_width_header_sha256")),
        "hercules_host_abi.h": (args.repo_root / "renderer_contract" / "hercules_host_abi.h",
                               harness.get("host_abi_header_sha256")),
    }
    for name, (path, expected) in current_inputs.items():
        if not path.is_file() or not expected or sha256_file(path) != expected:
            raise SystemExit(f"projection report is stale for current {name}")
    toolchain = report.get("toolchain", {})
    for name, path, expected in (
            ("compiler", args.vc5 / "bin" / "cl.exe", toolchain.get("compiler_sha256")),
            ("linker", args.vc5 / "bin" / "link.exe", toolchain.get("linker_sha256"))):
        if not path.is_file() or not expected or sha256_file(path) != expected:
            raise SystemExit(f"supplied VC5 {name} does not match the current projection report")
    args.projection_metadata = report
    args.projection_report_sha256 = sha256_file(args.projection_report)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True,
                        help="explicit repository checkout root")
    parser.add_argument("--exe", type=Path, required=True,
                        help="read-only original HERCULES.EXE; expected SHA-256 is checked")
    parser.add_argument("--title", type=Path, required=True,
                        help="read-only original TITLE.DLL; expected SHA-256 is checked")
    parser.add_argument("--projection-report", type=Path, required=True,
                        help="fresh passing public single-point projection report under checkout work/")
    parser.add_argument("--output", type=Path, required=True,
                        help="new output directory, strictly under checkout work/ and not already present")
    parser.add_argument("--vc5", type=Path, required=True,
                        help="historical VC5 root matching the supplied projection report")
    parser.add_argument("--deps", type=Path, required=True,
                        help="Python dependency root containing Unicorn")
    parser.add_argument("--analysis-deps", type=Path, required=True,
                        help="analysis dependency root for repository helpers")
    args = parser.parse_args()
    args.repo_root = args.repo_root.resolve()
    if not (args.repo_root / "scripts" / "pe.py").is_file():
        parser.error("--repo-root must contain scripts/pe.py")
    args.exe = args.exe.resolve()
    args.title = args.title.resolve()
    args.projection_report = args.projection_report.resolve()
    if not args.output.is_absolute():
        args.output = args.repo_root / args.output
    args.output = Path(os.path.abspath(args.output))
    work_root = Path(os.path.abspath(args.repo_root / "work"))
    if not is_relative_to(args.output, work_root) or args.output == work_root:
        parser.error("--output must be strictly under checkout work/")
    try:
        reject_reparse_components(args.output)
    except ValueError as error:
        parser.error(str(error))
    resolved_work = work_root.resolve(strict=False)
    resolved_output = args.output.resolve(strict=False)
    if not is_relative_to(resolved_output, resolved_work) or resolved_output == resolved_work:
        parser.error("resolved output must remain strictly under checkout work/")
    if args.output.exists():
        parser.error("--output must name a fresh directory that does not already exist")
    protected = [args.exe.parent.resolve(strict=False), args.title.parent.resolve(strict=False),
                 (args.repo_root / "assets").resolve(strict=False)]
    if any(args.output == item or is_relative_to(args.output, item) or
           is_relative_to(item, args.output) for item in protected):
        parser.error("--output overlaps original-install or assets paths")
    if not args.exe.is_file() or sha256_file(args.exe) != EXE_SHA256:
        parser.error("HERCULES.EXE missing or hash differs from the pinned original")
    if not args.title.is_file() or sha256_file(args.title) != TITLE_SHA256:
        parser.error("TITLE.DLL missing or hash differs from the pinned original")
    if not args.projection_report.is_file():
        parser.error("--projection-report must name an existing public projection report")
    report_work = (args.repo_root / "work").resolve(strict=False)
    if not is_relative_to(args.projection_report.resolve(strict=False), report_work):
        parser.error("--projection-report must be under checkout work/")
    if args.output == args.projection_report or is_relative_to(args.output, args.projection_report) or \
            is_relative_to(args.projection_report, args.output):
        parser.error("--output must not overlap the projection report")
    args.vc5 = args.vc5.resolve()
    args.deps = args.deps.resolve()
    args.analysis_deps = args.analysis_deps.resolve()
    if not args.deps.is_dir() or not args.analysis_deps.is_dir():
        parser.error("pinned Unicorn and analysis dependency roots are required")
    validate_projection_report(args)
    return args


ARGS = parse_args()
ROOT = ARGS.repo_root
os.environ["HERCULES_PYTHON_DEPS"] = str(ARGS.analysis_deps)
sys.path[:0] = [str(ARGS.deps), str(ROOT / "scripts"), str(ROOT / "renderer_contract" / "tests")]

from pe import PE  # noqa: E402
from oracle_common import verify_runtime  # noqa: E402
import unicorn  # noqa: E402
from unicorn import (  # noqa: E402
    Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE,
    UC_MODE_32, UC_PROT_EXEC, UC_PROT_READ, UC_PROT_WRITE,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDI, UC_X86_REG_EDX, UC_X86_REG_EIP, UC_X86_REG_ESI,
    UC_X86_REG_ESP,
)


def read_u16(uc, address: int) -> int:
    return struct.unpack("<H", bytes(uc.mem_read(address, 2)))[0]


def read_i16(uc, address: int) -> int:
    return struct.unpack("<h", bytes(uc.mem_read(address, 2)))[0]


def read_u32(uc, address: int) -> int:
    return struct.unpack("<I", bytes(uc.mem_read(address, 4)))[0]


def write_u32(uc, address: int, value: int) -> None:
    uc.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))


def write_i16(uc, address: int, value: int) -> None:
    uc.mem_write(address, struct.pack("<h", value))


def map_pe(uc, image: PE, base: int) -> list[tuple[int, int]]:
    mapped = []
    executable = []
    for section in image.pe.sections:
        size = max(section.Misc_VirtualSize, section.SizeOfRawData)
        if not size:
            continue
        address = base + section.VirtualAddress
        start = address & ~0xFFF
        end = (address + size + 0xFFF) & ~0xFFF
        if any(start < old_end and old_start < end for old_start, old_end in mapped):
            raise RuntimeError(f"overlapping PE sections at {address:#x}")
        uc.mem_map(start, end - start, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC)
        data = image.pe.get_data(section.VirtualAddress, section.SizeOfRawData)
        if data:
            uc.mem_write(address, data)
        perms = 0
        if section.Characteristics & 0x40000000:
            perms |= UC_PROT_READ
        if section.Characteristics & 0x80000000:
            perms |= UC_PROT_WRITE
        if section.Characteristics & 0x20000000:
            perms |= UC_PROT_EXEC
            executable.append((address, address + size))
        uc.mem_protect(start, end - start, perms)
        mapped.append((start, end))
    return executable


def compile_projection_reference(output: Path) -> tuple[PE, dict]:
    output.mkdir(parents=True, exist_ok=True)
    temp = output / "tmp"
    temp.mkdir(parents=True, exist_ok=True)
    source = ROOT / "renderer_contract" / "hercules_projection.c"
    header = ROOT / "renderer_contract" / "hercules_projection.h"
    include_dirs = [ROOT / "renderer_contract"]
    cl = ARGS.vc5 / "bin" / "cl.exe"
    link = ARGS.vc5 / "bin" / "link.exe"
    env = os.environ.copy()
    for key in ("CL", "_CL_", "LINK", "_LINK_"):
        env.pop(key, None)
    env["PATH"] = str(ARGS.vc5 / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(ARGS.vc5 / "include")
    env["LIB"] = str(ARGS.vc5 / "lib")
    env["TEMP"] = str(temp)
    env["TMP"] = str(temp)
    obj = output / "hercules_projection.obj"
    dll = output / "hercules_projection_reference.dll"
    map_file = output / "hercules_projection_reference.map"
    commands = [
        [str(cl), "/nologo", "/c", "/O2", "/Za", "/W3",
         "/DHERCULES_PROJECTION_BUILD_EXPORTS=1",
         *["/I" + str(path) for path in include_dirs], "/Fo" + str(obj), str(source)],
        [str(link), "/nologo", "/DLL", "/NOENTRY", "/NODEFAULTLIB", "/INCREMENTAL:NO",
         "/OPT:NOREF", "/FIXED", f"/BASE:{REFERENCE_BASE:#x}", "/MACHINE:X86",
         f"/OUT:{dll}", f"/MAP:{map_file}", str(obj)],
    ]
    logs = []
    for command in commands:
        completed = subprocess.run(command, cwd=output, env=env, capture_output=True,
                                   text=True, errors="replace", timeout=120)
        logs.append({"command": command, "returncode": completed.returncode,
                     "stdout": completed.stdout, "stderr": completed.stderr})
        if completed.returncode:
            raise RuntimeError(json.dumps(logs[-1], indent=2))
    candidate = PE(dll)
    if candidate.image_base != REFERENCE_BASE:
        raise RuntimeError("projection reference fixture has unexpected base")
    if candidate.pe.OPTIONAL_HEADER.DllCharacteristics & 0x40:
        raise RuntimeError("projection reference fixture has relocations")
    if getattr(candidate.pe, "DIRECTORY_ENTRY_IMPORT", []):
        raise RuntimeError("projection reference fixture unexpectedly imports helpers")
    exports = {symbol.name.decode("ascii", "replace"): REFERENCE_BASE + symbol.address
               for symbol in candidate.pe.DIRECTORY_ENTRY_EXPORT.symbols
               if symbol.name}
    if "hercules_projection_eval" not in exports:
        raise RuntimeError(f"C projection API export missing: {sorted(exports)}")
    return candidate, {"compiler": str(cl), "compiler_sha256": sha256_file(cl),
                       "linker": str(link), "linker_sha256": sha256_file(link),
                       "flags": {"compile": ["/O2", "/Za", "/W3", "/DHERCULES_PROJECTION_BUILD_EXPORTS=1"],
                                 "link": ["/DLL", "/NOENTRY", "/NODEFAULTLIB", "/INCREMENTAL:NO",
                                          "/OPT:NOREF", "/FIXED", f"/BASE:{REFERENCE_BASE:#x}", "/MACHINE:X86"]},
                       "commands": logs, "dll_sha256": sha256_file(dll),
                       "map_sha256": sha256_file(map_file),
                       "source_sha256": sha256_file(source), "header_sha256": sha256_file(header),
                       "dll_path": str(dll)}


class NativeTrace:
    def __init__(self, uc, candidate_exec_ranges):
        self.uc = uc
        self.candidate_exec_ranges = candidate_exec_ranges
        self.allowed = []
        for module, start, end in ALLOWED_RANGES:
            base = TITLE_BASE if module == "TITLE" else EXE_BASE
            self.allowed.append((base + start, base + end, module, start))
        for start, end in candidate_exec_ranges:
            self.allowed.append((start, end, "PROJECTION_REFERENCE", start - REFERENCE_BASE))
        self.phase = "setup"
        self.unexpected_pc = None
        self.executed = Counter()
        self.events = []
        self.callback_trace = []
        self.point_reads = []
        self.native_writes = []
        self.batch_inputs = None
        self.batch_camera = None
        self.batch_xy = None
        self.single_input = None
        self.single_result = None
        self.snapshot_translation = None
        self.loaded_matrix = None
        self.source_stream_reads = []
        self.current_case = None
        uc.hook_add(UC_HOOK_CODE, self.on_code)
        uc.hook_add(UC_HOOK_MEM_READ, self.on_read)
        uc.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def is_allowed(self, pc):
        return next((row for row in self.allowed if row[0] <= pc < row[1]), None)

    def on_code(self, uc, pc, size, _):
        allowed = self.is_allowed(pc)
        if allowed is None:
            self.unexpected_pc = pc
            uc.emu_stop()
            return
        module = allowed[2]
        rva = pc - (TITLE_BASE if module == "TITLE" else EXE_BASE if module == "EXE" else REFERENCE_BASE)
        self.executed[(module, rva // 0x100)] += 1
        if module == "TITLE":
            self._title_event(pc)
        elif module == "EXE":
            self._exe_event(pc)

    def _stack_arg(self, offset):
        return read_u32(self.uc, self.uc.reg_read(UC_X86_REG_ESP) + offset)

    def _title_event(self, pc):
        if pc in (TITLE_BASE + 0x1827A, TITLE_BASE + 0x1827D,
                  TITLE_BASE + 0x18297, TITLE_BASE + 0x1832B,
                  TITLE_BASE + 0x18385, TITLE_BASE + 0x18399,
                  TITLE_BASE + 0x183A8):
            edi = self.uc.reg_read(UC_X86_REG_EDI)
            self.source_stream_reads.append({"pc_rva": hex(pc - TITLE_BASE),
                                             "address": hex(edi),
                                             "value": self.uc.mem_read(edi, 1)[0]})
        if pc == TITLE_BASE + 0x183F9:
            self.batch_camera = []
            for i in range(3):
                camera_x = struct.unpack("<i", bytes(self.uc.mem_read(GAME_STATE + 0x1C + i * 4, 4)))[0]
                camera_y = struct.unpack("<i", bytes(self.uc.mem_read(GAME_STATE + 0x28 + i * 4, 4)))[0]
                camera_z = struct.unpack("<i", bytes(self.uc.mem_read(GAME_STATE + 0x34 + i * 4, 4)))[0]
                self.batch_camera.append([camera_x, camera_y, camera_z])
            self.batch_xy = [[read_u16(self.uc, GAME_STATE + 0x40 + i * 2),
                              read_u16(self.uc, GAME_STATE + 0x46 + i * 2)]
                             for i in range(3)]
            self.events.append({"kind": "batch_107_return", "camera_xyz": self.batch_camera,
                                "screen_xy_words": self.batch_xy})
        if pc == TITLE_BASE + 0x18421:
            packet = self.uc.reg_read(UC_X86_REG_ESI)
            self.events.append({"kind": "batch_117_return", "packet": hex(packet),
                                "xy": [[read_u16(self.uc, packet + offset),
                                        read_u16(self.uc, packet + offset + 2)]
                                       for offset in (8, 0x10, 0x18)]})

    def _exe_event(self, pc):
        rva = pc - EXE_BASE
        if rva == 0xB430:
            pointers = [self._stack_arg(4), self._stack_arg(8), self._stack_arg(12)]
            inputs = [[read_i16(self.uc, ptr + 2 * i) for i in range(3)] for ptr in pointers]
            self.batch_inputs = [{"pointer": hex(ptr), "xyz_words": point}
                                 for ptr, point in zip(pointers, inputs)]
            self.callback_trace.append({"slot": 102, "host_rva": "0xb430", "points": self.batch_inputs})
        elif rva == 0xB320:
            self.callback_trace.append({"slot": 107, "host_rva": "0xb320"})
        elif rva == 0xB5C0:
            packet = self._stack_arg(4)
            self.callback_trace.append({"slot": 117, "host_rva": "0xb5c0", "packet": hex(packet)})
        elif rva == 0xB6F0:
            ptr = self._stack_arg(4)
            point = [read_i16(self.uc, ptr + 2 * i) for i in range(3)]
            self.single_input = {"pointer": hex(ptr), "xyz_words": point}
            self.callback_trace.append({"slot": 101, "host_rva": "0xb6f0", **self.single_input})
        elif rva == 0xB920:
            self.callback_trace.append({"slot": 106, "host_rva": "0xb920"})
        elif rva == 0xB720:
            ptr = self._stack_arg(4)
            self.single_result = {
                "camera_xyz": [struct.unpack("<i", bytes(self.uc.mem_read(GAME_STATE + off, 4)))[0]
                               for off in (0x1C, 0x28, 0x34)],
                "projected_xy_words": [read_u16(self.uc, GAME_STATE + 0x40),
                                        read_u16(self.uc, GAME_STATE + 0x46)],
                "packet_xy_output_pointer": hex(ptr),
            }
            self.callback_trace.append({"slot": 114, "host_rva": "0xb720", **self.single_result})
        elif rva == 0xB840:
            source = [struct.unpack("<i", bytes(self.uc.mem_read(GAME_STATE + off, 4)))[0]
                      for off in (0x1C, 0x28, 0x34)]
            self.snapshot_translation = source
            self.callback_trace.append({"slot": 103, "host_rva": "0xb840",
                                        "camera_xyz_copied_to_translation": source})
        elif rva == 0xB880:
            ptr = self._stack_arg(4)
            self.loaded_matrix = list(struct.unpack("<9h", bytes(self.uc.mem_read(ptr, 18))))
            self.callback_trace.append({"slot": 108, "host_rva": "0xb880",
                                        "matrix_source": hex(ptr),
                                        "matrix_words_row_major": self.loaded_matrix})
        elif rva == 0xBB00:
            self.callback_trace.append({"slot": 164, "host_rva": "0xbb00"})
        elif rva == 0xBC30:
            self.callback_trace.append({"slot": 165, "host_rva": "0xbc30"})
        elif rva == 0xB970:
            self.callback_trace.append({"slot": 174, "host_rva": "0xb970"})
        elif rva == 0xB990:
            self.callback_trace.append({"slot": 175, "host_rva": "0xb990"})

    def on_read(self, uc, access, address, size, value, _):
        if RESOURCE <= address < RESOURCE + 0x100:
            self.point_reads.append({"address": hex(address), "size": size, "value": hex(value)})

    def on_write(self, uc, access, address, size, value, _):
        if (GAME_STATE <= address < GAME_STATE + GAME_STATE_SIZE or
                PACKET_POOL <= address < PACKET_POOL + 0x28 or
                ORDER_TABLE <= address < ORDER_TABLE + 0x2000 or
                TITLE_BASE + 0x2CC04 <= address < TITLE_BASE + 0x2CC08 or
                TITLE_BASE + 0x2D328 <= address < TITLE_BASE + 0x2D32C or
                TITLE_BASE + 0x2DF40 <= address < TITLE_BASE + 0x2DF44):
            self.native_writes.append({"phase": self.phase, "pc": hex(self.uc.reg_read(UC_X86_REG_EIP)),
                                       "address": hex(address), "size": size,
                                       "value": hex(value & ((1 << (size * 8)) - 1))})


def invoke(uc, trace: NativeTrace, entry: int, args: tuple[int, ...], label: str) -> dict:
    sp = STACK_TOP - 0x100 - (len(args) + 1) * 4
    uc.mem_write(sp, struct.pack("<I", RETURN_SENTINEL))
    for index, value in enumerate(args):
        uc.mem_write(sp + 4 + 4 * index, struct.pack("<I", value & 0xFFFFFFFF))
    uc.reg_write(UC_X86_REG_EAX, 0x11111111)
    uc.reg_write(UC_X86_REG_EBX, 0x22222222)
    uc.reg_write(UC_X86_REG_ECX, 0x33333333)
    uc.reg_write(UC_X86_REG_EDX, 0x44444444)
    uc.reg_write(UC_X86_REG_ESI, 0x55555555)
    uc.reg_write(UC_X86_REG_EDI, 0x66666666)
    uc.reg_write(UC_X86_REG_EBP, 0x77777777)
    uc.reg_write(UC_X86_REG_ESP, sp)
    uc.reg_write(UC_X86_REG_EIP, entry)
    trace.unexpected_pc = None
    before_events = len(trace.events)
    before_callbacks = len(trace.callback_trace)
    trace.phase = label
    try:
        uc.emu_start(entry, RETURN_SENTINEL, count=INSTRUCTION_BUDGET)
    except Exception as error:
        pc = uc.reg_read(UC_X86_REG_EIP)
        raise RuntimeError(f"{label}: native execution fault at PC {pc:#x}: {error}") from error
    eip = uc.reg_read(UC_X86_REG_EIP)
    if trace.unexpected_pc is not None:
        raise RuntimeError(f"{label}: unknown code endpoint {trace.unexpected_pc:#x}; no stub was installed")
    if eip != RETURN_SENTINEL:
        raise RuntimeError(f"{label}: did not return to sentinel; EIP={eip:#x}")
    if uc.reg_read(UC_X86_REG_ESP) != sp + 4:
        raise RuntimeError(f"{label}: unexpected cdecl stack result")
    return {"eax": hex(uc.reg_read(UC_X86_REG_EAX)),
            "esp_before": hex(sp), "esp_after": hex(uc.reg_read(UC_X86_REG_ESP)),
            "events": trace.events[before_events:],
            "callbacks": trace.callback_trace[before_callbacks:]}


def write_ref_config(uc, matrix, translation, center, focal):
    raw = bytearray(44)
    struct.pack_into("<9h", raw, 0, *matrix)
    struct.pack_into("<3i", raw, 20, *translation)
    struct.pack_into("<2iI", raw, 32, center[0], center[1], focal)
    uc.mem_write(REFERENCE_CONFIG, bytes(raw))


def eval_reference(uc, trace, function, point):
    uc.mem_write(REFERENCE_POINT, struct.pack("<3h", *point))
    result = invoke(uc, trace, function,
                    (REFERENCE_CONFIG, REFERENCE_POINT, REFERENCE_RESULT),
                    "projection_reference")
    raw = bytes(uc.mem_read(REFERENCE_RESULT, 20))
    x, y, z, sx, sy, eax = struct.unpack("<iiiHHI", raw)
    return {"camera_xyz": [x, y, z], "projected_xy_words": [sx, sy],
            "raw_eax": hex(eax), "call_eax": result["eax"]}


def configure_case(uc, trace, case):
    uc.mem_write(SYNTH_BASE, b"\xA5" * SYNTH_SIZE)
    uc.mem_write(STACK_BASE, b"\xCC" * STACK_SIZE)
    uc.mem_write(GAME_STATE, b"\xA5" * GAME_STATE_SIZE)
    uc.mem_write(OBJECT, b"\0" * 0x100)
    stream = case["resource_bytes"]
    uc.mem_write(RESOURCE, bytes(stream) + b"\0" * (0x100 - len(stream)))
    uc.mem_write(PACKET_POOL, b"\xA5" * 0x28)
    uc.mem_write(ORDER_TABLE, b"\0" * 0x2000)
    object_fields = case["object_fields"]
    write_u32(uc, OBJECT + 0x54, object_fields["flags"])
    for offset, value in object_fields["words"].items():
        write_i16(uc, OBJECT + int(offset, 16), value)
    for offset, value in zip((0x1C, 0x28, 0x34), case["initial_camera_xyz"]):
        uc.mem_write(GAME_STATE + offset, struct.pack("<i", value))
    view = ORDER_TABLE
    write_u32(uc, TITLE_BASE + 0x2CC04, view)
    write_u32(uc, TITLE_BASE + 0x2D328, 1)
    write_u32(uc, TITLE_BASE + 0x2DF40, PACKET_POOL)
    for cell_rva, target_rva in TITLE_CALLBACK_CELLS.items():
        write_u32(uc, TITLE_BASE + cell_rva, EXE_BASE + target_rva)
    ot_index = 0x51A - object_fields["words"]["0x10"]
    if not (0 <= ot_index < 0x2000 // 4):
        raise RuntimeError(f"synthetic OT index out of range: {ot_index}")
    ot_head = ORDER_TABLE + ot_index * 4
    write_u32(uc, ot_head, case["old_ot_head"])
    trace.current_case = case["name"]
    trace.events.clear()
    trace.callback_trace.clear()
    trace.point_reads.clear()
    trace.native_writes.clear()
    trace.source_stream_reads.clear()
    trace.batch_inputs = trace.batch_camera = trace.batch_xy = None
    trace.single_input = trace.single_result = None
    trace.snapshot_translation = trace.loaded_matrix = None
    trace.phase = "setup"
    setup_center = invoke(uc, trace, TITLE_BASE + 0xCB10,
                          (case["center"][0], case["center"][1]), "set_center")
    setup_focal = invoke(uc, trace, TITLE_BASE + 0xCB30,
                         (case["focal"],), "set_focal")
    return {"center_setter": setup_center, "focal_setter": setup_focal,
            "order_table_head_address": ot_head, "order_table_index": ot_index}


def run_case(exe: PE, title: PE, projection: PE, projection_function: int, case: dict) -> dict:
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    exe_exec = map_pe(uc, exe, EXE_BASE)
    title_exec = map_pe(uc, title, TITLE_BASE)
    projection_exec = map_pe(uc, projection, REFERENCE_BASE)
    uc.mem_map(SYNTH_BASE, SYNTH_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    trace = NativeTrace(uc, projection_exec)
    # Reference code ranges are explicit; every reached original code address
    # must belong to one of the hash-pinned producer/helper/callback spans.
    trace.allowed = []
    for module, start, end in ALLOWED_RANGES:
        base = TITLE_BASE if module == "TITLE" else EXE_BASE
        trace.allowed.append((base + start, base + end, module, start))
    for start, end in projection_exec:
        trace.allowed.append((start, end, "PROJECTION_REFERENCE", start - REFERENCE_BASE))
    setup = configure_case(uc, trace, case)
    before_cursor = read_u32(uc, TITLE_BASE + 0x2DF40)
    # The producer uses arg3 as its packet iteration count. The dynamic caller
    # also passes exactly three arguments to the selected mode-table target.
    run = invoke(uc, trace, TITLE_BASE + 0x181E0,
                 (RESOURCE, OBJECT, case["packet_count"]), "title_mode5_181e0")
    if len(trace.batch_inputs or []) != 3 or trace.batch_camera is None or trace.batch_xy is None:
        raise RuntimeError(f"{case['name']}: expected one complete 102/107/117 batch")
    if trace.single_input is None or trace.single_result is None:
        raise RuntimeError(f"{case['name']}: expected one complete 101/106/114 single-point route")
    source_points = [row["xyz_words"] for row in trace.batch_inputs] + [trace.single_input["xyz_words"]]
    matrix = list(struct.unpack("<9h", bytes(uc.mem_read(GAME_STATE + 0x5C, 18))))
    translation = [struct.unpack("<i", bytes(uc.mem_read(GAME_STATE + offset, 4)))[0]
                   for offset in (0x70, 0x74, 0x78)]
    center = [struct.unpack("<i", bytes(uc.mem_read(GAME_STATE + offset, 4)))[0]
              for offset in (0x4C, 0x50)]
    focal = read_u32(uc, GAME_STATE + 0x54)
    write_ref_config(uc, matrix, translation, center, focal)
    reference_rows = [eval_reference(uc, trace, projection_function, point)
                      for point in source_points]
    native_rows = [{"camera_xyz": trace.batch_camera[index],
                    "projected_xy_words": trace.batch_xy[index]}
                   for index in range(3)] + [trace.single_result]
    for index, (native, reference) in enumerate(zip(native_rows, reference_rows)):
        if native["camera_xyz"] != reference["camera_xyz"] or \
                native["projected_xy_words"] != reference["projected_xy_words"]:
            raise RuntimeError(f"{case['name']}: public projection reference mismatch at point {index}: "
                               f"native={native} reference={reference}")
    final_cursor = read_u32(uc, TITLE_BASE + 0x2DF40)
    packet = bytes(uc.mem_read(PACKET_POOL, 0x28))
    corners = [[read_u16(uc, PACKET_POOL + offset), read_u16(uc, PACKET_POOL + offset + 2)]
               for offset in (0x08, 0x10, 0x18, 0x20)]
    expected_corners = [row["projected_xy_words"] for row in native_rows]
    if corners != expected_corners:
        raise RuntimeError(f"{case['name']}: FT4 corners do not match callback outputs: {corners} != {expected_corners}; "
                           f"batch_inputs={trace.batch_inputs}; batch_camera={trace.batch_camera}; "
                           f"batch_xy={trace.batch_xy}; single={trace.single_result}; "
                           f"matrix={matrix}; translation={translation}; reference={reference_rows}; "
                           f"callbacks={trace.callback_trace}; packet_ptrs="
                           f"{[row for row in trace.events if row.get('kind') == 'batch_117_return']}")
    head_after = read_u32(uc, setup["order_table_head_address"])
    record_link = read_u32(uc, PACKET_POOL)
    if final_cursor != PACKET_POOL + 0x28 * case["packet_count"]:
        raise RuntimeError(f"{case['name']}: pool cursor did not advance by one FT4 stride")
    if head_after != PACKET_POOL or record_link != case["old_ot_head"]:
        raise RuntimeError(f"{case['name']}: OT head insertion/link mismatch")
    callbacks = [row["slot"] for row in trace.callback_trace
                 if row.get("slot") in (102, 107, 117, 101, 106, 114)]
    if callbacks != [102, 107, 117, 101, 106, 114]:
        raise RuntimeError(f"{case['name']}: callback sequence mismatch: {callbacks}")
    source_records = [
        {"identity": f"batch_vertex_{index}", **trace.batch_inputs[index],
         "native_result": native_rows[index], "reference_result": reference_rows[index]}
        for index in range(3)
    ] + [{"identity": "single_fourth_vertex", **trace.single_input,
          "native_result": native_rows[3], "reference_result": reference_rows[3]}]
    value_groups = {}
    pointer_groups = {}
    for row in source_records:
        value_groups.setdefault(tuple(row["xyz_words"]), []).append(row["identity"])
        pointer_groups.setdefault(row["pointer"], []).append(row)
    same_xyz_roles = [roles for roles in value_groups.values() if len(roles) > 1]
    same_pointer_role_groups = [rows for rows in pointer_groups.values() if len(rows) > 1]
    pointer_reused_with_different_values = [
        {"pointer": rows[0]["pointer"], "identities": [row["identity"] for row in rows],
         "xyz_words": [row["xyz_words"] for row in rows]}
        for rows in same_pointer_role_groups
        if len({tuple(row["xyz_words"]) for row in rows}) > 1
    ]
    return {
        "name": case["name"],
        "synthetic_context": {
            "object_address": hex(OBJECT), "resource_address": hex(RESOURCE),
            "resource_bytes": case["resource_bytes"], "object_fields": case["object_fields"],
            "producer_packet_count_arg3": case["packet_count"],
            "initial_camera_xyz_for_B840": case["initial_camera_xyz"],
            "center": center, "focal": focal, "old_ot_head": hex(case["old_ot_head"]),
        },
        "setup": setup,
        "callback_order": trace.callback_trace,
        "constructed_source_points": source_records,
        "distinct_xyz_triplet_count": len({tuple(point) for point in source_points}),
        "source_identity_aliases": {
            "identity_key": "producer callback role plus callback-time input pointer and signed-WORD value",
            "same_xyz_role_groups": same_xyz_roles,
            "same_pointer_role_groups": [
                {"pointer": rows[0]["pointer"], "identities": [row["identity"] for row in rows],
                 "xyz_words": [row["xyz_words"] for row in rows]}
                for rows in same_pointer_role_groups],
            "same_pointer_reused_with_different_values": pointer_reused_with_different_values,
        },
        "matrix_row_major": matrix,
        "translation_after_B840": translation,
        "batch_screen_lanes": trace.batch_xy,
        "packet": {
            "address": hex(PACKET_POOL), "stride": 0x28, "initial_fill_byte": "0xa5",
            "bytes_hex": packet.hex(),
            "link": hex(record_link), "color_command_dword": hex(read_u32(uc, PACKET_POOL + 4)),
            "command_byte": packet[7], "corners_xy": corners,
            "uv_vertex0": list(packet[0x0C:0x10]),
            "uv_vertex1": list(packet[0x14:0x18]),
            "uv_vertex2": list(packet[0x1C:0x20]),
            "uv_vertex3": list(packet[0x24:0x28]),
        },
        "order_table": {"head_address": hex(setup["order_table_head_address"]),
                        "previous_head": hex(case["old_ot_head"]),
                        "head_after": hex(head_after), "inserted_packet": hex(PACKET_POOL)},
        "pool_cursor": {"before": hex(before_cursor), "after": hex(final_cursor),
                        "delta": final_cursor - before_cursor},
        "native_game_state_write_count": len(trace.native_writes),
        "native_writes": trace.native_writes,
        "source_stream_reads": trace.source_stream_reads,
        "input_memory_reads": trace.point_reads,
        "producer_return": run,
        "assertion_count": 11,
    }


def main() -> int:
    output = ARGS.output
    output.mkdir(parents=True, exist_ok=False)
    exe = PE(ARGS.exe)
    title = PE(ARGS.title)
    if exe.image_base != EXE_BASE or title.image_base != TITLE_BASE:
        raise RuntimeError("original image bases differ from the pinned preferred bases")
    for name, (module, rva, size, expected) in ORIGINAL_SPANS.items():
        image = title if module == "TITLE" else exe
        actual = sha256_bytes(image.read_rva(rva, size))
        if actual != expected:
            raise RuntimeError(f"{module}+{rva:#x} span hash mismatch: {actual}")
    projection, build = compile_projection_reference(output / "projection_reference_build")
    projection_func = next(symbol.address + REFERENCE_BASE
                           for symbol in projection.pe.DIRECTORY_ENTRY_EXPORT.symbols
                           if symbol.name and symbol.name.decode("ascii", "replace") == "hercules_projection_eval")
    center = (320, 240)
    common = {"packet_count": 1,
              "old_ot_head": 0x0C050000,
              "center": center, "focal": 256,
              "object_fields": {"flags": 0x84,
                                "words": {"0x10": 10, "0x14": 0, "0x1e": 0,
                                          "0x26": 0, "0x3a": 256, "0x3c": 256,
                                          "0x3e": 256, "0x46": 0, "0x4a": 80,
                                          "0x70": 0, "0x72": 0, "0x74": 0}}}
    cases = [
        {"name": "title_mode5_resource_byte2_clear", "resource_bytes": [10, 20, 0, 7, 13, 21, 29],
         "initial_camera_xyz": [100, -50, 1024], **common},
        {"name": "title_mode5_resource_byte2_bit7_payload", "resource_bytes": [10, 20, 0x97, 7, 13, 21, 29],
         "initial_camera_xyz": [100, -50, 1024], **common},
    ]
    case_results = [run_case(exe, title, projection, projection_func, case) for case in cases]
    first, second = case_results
    # The 0x97 input exercises bit7 with nonzero payload bits in the two masks
    # at TITLE+0x18318/+0x1831f. Report whether geometry, UV, or both changed.
    geometry0 = [{"camera_xyz": row["native_result"]["camera_xyz"],
                  "projected_xy_words": row["native_result"]["projected_xy_words"]}
                 for row in first["constructed_source_points"]]
    geometry1 = [{"camera_xyz": row["native_result"]["camera_xyz"],
                  "projected_xy_words": row["native_result"]["projected_xy_words"]}
                 for row in second["constructed_source_points"]]
    geometry_changed = geometry0 != geometry1
    uv_keys = ("uv_vertex0", "uv_vertex1", "uv_vertex2", "uv_vertex3")
    uv0 = [first["packet"][key] for key in uv_keys]
    uv1 = [second["packet"][key] for key in uv_keys]
    uv_changed = uv0 != uv1
    if not geometry_changed and not uv_changed:
        raise RuntimeError("resource byte2=0x97 did not change any geometry or FT4 UV fields")

    runtime = verify_runtime(unicorn, ARGS.deps)
    model_runtime = ARGS.projection_metadata.get("runtime", {})
    if runtime["unicorn_version"] != model_runtime.get("unicorn_version") or \
            runtime["native_dll_sha256"] != model_runtime.get("native_dll_sha256"):
        raise RuntimeError("current Unicorn runtime differs from the supplied public projection report")
    prediction_relative = "renderer_contract/fixtures/title_geometry_association/prediction.md"
    input_hashes = {
        "renderer_contract/evidence/upstream_geometry_modules.json": sha256_file(ROOT / "renderer_contract" / "evidence" / "upstream_geometry_modules.json"),
        "renderer_contract/evidence/title_handlers.json": sha256_file(ROOT / "renderer_contract" / "evidence" / "title_handlers.json"),
        "renderer_contract/evidence/module_packets.json": sha256_file(ROOT / "renderer_contract" / "evidence" / "module_packets.json"),
        "renderer_contract/evidence/geometry.json": sha256_file(ROOT / "renderer_contract" / "evidence" / "geometry.json"),
        "renderer_contract/cross_module_geometry.md": sha256_file(ROOT / "renderer_contract" / "cross_module_geometry.md"),
        "renderer_contract/hercules_projection.c": sha256_file(ROOT / "renderer_contract" / "hercules_projection.c"),
        "renderer_contract/hercules_projection.h": sha256_file(ROOT / "renderer_contract" / "hercules_projection.h"),
        "renderer_contract/hercules_host_abi.h": sha256_file(ROOT / "renderer_contract" / "hercules_host_abi.h"),
        "renderer_contract/hercules_types.h": sha256_file(ROOT / "renderer_contract" / "hercules_types.h"),
        "renderer_contract/tests/oracle_common.py": sha256_file(ROOT / "renderer_contract" / "tests" / "oracle_common.py"),
        "renderer_contract/tests/test_original_projection.py": sha256_file(ROOT / "renderer_contract" / "tests" / "test_original_projection.py"),
        "renderer_contract/tests/test_original_title_geometry_association.py": sha256_file(SCRIPT),
        "renderer_contract/fixtures/title_geometry_association/prediction.md": sha256_file(PREDICTION),
        "scripts/pe.py": sha256_file(ROOT / "scripts" / "pe.py"),
    }
    report_relative = ARGS.projection_report.relative_to(ROOT).as_posix()
    report = {
        "schema_version": 1,
        "status": "TITLE_MODE5_NATIVE_PRODUCER_BOUNDED_PASS",
        "scope": "Unchanged original TITLE.DLL+0x181e0 executed in Unicorn with explicit synthetic object/resource/context; original HERC callbacks and the current public C89 single-point projection model are used. No gameplay, original process, or rendered pixels.",
        "prediction_path": prediction_relative,
        "prediction_sha256": input_hashes[prediction_relative],
        "originals": {"HERCULES.EXE": {"path": str(ARGS.exe), "sha256": sha256_file(ARGS.exe), "preferred_base": hex(EXE_BASE)},
                      "TITLE.DLL": {"path": str(ARGS.title), "sha256": sha256_file(ARGS.title), "preferred_base": hex(TITLE_BASE)},
                      "windows_process_started": False, "original_code_modified": False},
        "runtime": {"unicorn_version": runtime["unicorn_version"],
                    "engine": runtime["engine"], "native_dll_sha256": runtime["native_dll_sha256"],
                    "python_executable": sys.executable,
                    "python_executable_sha256": sha256_file(Path(sys.executable))},
        "public_projection_reference": build,
        "public_projection_model": {
            "report_path": report_relative,
            "report_sha256": ARGS.projection_report_sha256,
            "report_status": ARGS.projection_metadata["status"],
            "current_report_harness_sha256": ARGS.projection_metadata["harness"]["sha256"],
            "current_shared_width_header_sha256": ARGS.projection_metadata["harness"]["shared_width_header_sha256"],
            "current_host_abi_header_sha256": ARGS.projection_metadata["harness"]["host_abi_header_sha256"],
            "classification": "single-point model exercised as a valid positive case on eight points observed at native batch and single-point callbacks; this is not general coverage of native batch math",
        },
        "source_hashes": input_hashes,
        "pinned_spans": [{"name": key, "module": value[0], "rva": hex(value[1]),
                          "size": value[2], "sha256": value[3]}
                         for key, value in ORIGINAL_SPANS.items()],
        "shared_native_callback_table": {
            "TITLE_table_base_rva": "0x2bb40",
            "synthetic_slot_cells": {str(slot): {"cell_rva": hex(cell), "host_target_rva": hex(target)}
                                      for slot, (cell, target) in zip(
                                          (101, 102, 103, 106, 107, 108, 114, 117, 164, 165, 174, 175),
                                          TITLE_CALLBACK_CELLS.items())},
            "initialization": "Only evidence-pinned callback cells used by the selected producer/helper closure are set to their original HERCULES.EXE preferred-base targets. No unknown callback endpoint receives a stub.",
        },
        "cases": case_results,
        "resource_byte2_bit7_discriminator": {
            "result": "PASS",
            "resource_byte2": [cases[0]["resource_bytes"][2], cases[1]["resource_bytes"][2]],
            "camera_xyz_or_projected_words_changed": geometry_changed,
            "uv_bytes_changed": uv_changed,
            "geometry_before": geometry0,
            "geometry_after": geometry1,
            "ft4_uv_bytes": [uv0, uv1],
            "interpretation": "This records only the observed bit7 stream branch effect for these bytes and this mode-5 context; it does not define general texture encoding.",
        },
        "summary": {"case_count": len(case_results),
                    "constructed_source_records_per_case": 4,
                    "distinct_xyz_triplet_count_per_case": [row["distinct_xyz_triplet_count"] for row in case_results],
                    "callback_sequence_per_case": [102, 107, 117, 101, 106, 114],
                    "public_projection_reference_points_compared": 8,
                    "resource_byte2_bit7_discriminator_assertions": 1,
                    "assertion_count": sum(row["assertion_count"] for row in case_results) + 1},
        "proof_boundary": [
            "This executes the original TITLE producer and the bounded, hash-pinned original HERC callback/helper closure in Unicorn; it is not a source/function match.",
            "The four source points are identities of the observed callback inputs and local stack addresses in this synthetic context. They are not asserted as world-space objects or general TITLE mesh semantics.",
            "TITLE's conditionally computed matrix is exercised through original slot 164/165 helpers and slot 108; B840 copies the explicitly seeded camera globals into translation cells.",
            "Projection center/focal are set through original TITLE wrappers and HERC setters because 0x181e0 does not call slots 174/175 itself.",
            "The public C89 comparison evaluates each captured point independently. Agreement here is a positive case for the single-point model, not proof of general batch-math coverage.",
            "No host visibility/culling, all-mode coverage, production buffer capacity, live game setup, packet consumer/raster result, or pixel behavior is claimed.",
        ],
    }
    report_path = output / "title_geometry_association.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
    report_md = output / "title_geometry_association.md"
    report_md.write_text(
        "# TITLE mode-5 geometry association\n\n"
        f"Status: **{report['status']}**.\n\n"
        "The unchanged original TITLE+0x181e0 producer exposed four role-addressed signed-WORD XYZ inputs "
        "at native callback boundaries; the per-case receipt records any equal coordinate triplets explicitly. "
        "Slots 102/107/117 produced corners 0–2; slots 101/106/114 produced corner 3. The output packet, "
        "texture fields, OT link/insertion, and 0x28 cursor step are recorded per case. The current public C89 "
        "single-point model matched eight independently evaluated point outputs sampled at native batch/single "
        "callbacks; this is a valid positive case for that model, not general coverage of native batch math. "
        "Resource byte2=0x97 "
        "was compared with byte2=0 and its geometry/UV deltas are recorded explicitly. For the clear-byte case, "
        f"same-XYZ role groups were {json.dumps(first['source_identity_aliases']['same_xyz_role_groups'])}; "
        f"same-stack reuse was {json.dumps(first['source_identity_aliases']['same_pointer_reused_with_different_values'])}. "
        "The identity key includes the callback role and time because TITLE reuses one local stack slot between "
        "the third batch input and the fourth single-point input. The runner requires a passing public projection "
        "report and validates its current harness, shared-width header, host ABI header, toolchain, and runtime "
        "hashes dynamically; paths are supplied at run time.\n\n"
        "This is a finite Unicorn producer execution against pinned original bytes, not a claim about source identity, "
        "world units, general visibility, all TITLE modes, or pixels.\n\n"
        f"Receipt SHA-256: `{sha256_file(report_path)}`.\n",
        encoding="utf-8", newline="\n")
    print(json.dumps({"status": report["status"], "cases": len(case_results),
                      "assertions": report["summary"]["assertion_count"],
                      "receipt": str(report_path), "sha256": sha256_file(report_path)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
