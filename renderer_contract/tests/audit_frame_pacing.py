"""Audit how the original host paces engine frames (OnIdle 0x79A0 -> 0x5F20 ->
0x5EC0 -> PC_DLLEngineMain) and whether presentation can change simulation rate.

Read-only: decodes the pinned unpatched HERCULES.EXE and the three engine DLLs
with Capstone/pefile; nothing is loaded or executed. Every claim in
renderer_contract/frame_pacing.md is re-derived here from bytes. The final
section runs a timing model whose constants are the values decoded from 0x5F20;
the model itself (host-loop period, vblank blocking) is an assumption, labelled
as such in the receipt.

  python renderer_contract/tests/audit_frame_pacing.py --exe <HERCULES.EXE>
      [--dll-dir <dir with ENG1/ENG3/TITLE.DLL>] [--output work/frame_pacing/frame_pacing.json]
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "scripts"))
import env  # noqa: F401,E402
import capstone  # noqa: E402
from capstone import x86  # noqa: E402
import pefile  # noqa: E402

PINNED = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "ENG3.DLL": "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
    "TITLE.DLL": "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
}
# evidence/host_timing_dispatch_recovery.json oracle.body_sha256 (173 bytes at RVA 0x5F20).
TICK_BODY_SHA256 = "2da2abca95296d27f38f85a3e65df55f188a014d1a3f0470f26cb39d47224802"
SOURCE_TABLE = 0x4361B8
TABLE_WORDS = 250
APP_VTABLE = 0x42E1D8
# Copied 250-DWORD interface tables (static `rep movsd` destination in each export).
MODULE_TABLE = {"ENG1.DLL": 0x10070F20, "ENG3.DLL": 0x100407A0, "TITLE.DLL": 0x1002BB40}

CLOCK_APIS = [
    "timeGetTime", "timeBeginPeriod", "timeEndPeriod", "timeSetEvent", "timeKillEvent",
    "timeGetSystemTime", "QueryPerformanceCounter", "QueryPerformanceFrequency",
    "GetTickCount", "GetTickCount64", "GetSystemTimeAsFileTime", "GetSystemTime",
    "GetLocalTime", "Sleep", "SleepEx", "WaitForSingleObject", "WaitForSingleObjectEx",
    "WaitForMultipleObjects", "WaitForMultipleObjectsEx", "MsgWaitForMultipleObjects",
    "MsgWaitForMultipleObjectsEx", "CreateWaitableTimerA", "SetWaitableTimer",
    "SetTimer", "KillTimer", "WaitMessage", "GetMessageTime", "CreateThread",
    "SuspendThread", "ResumeThread", "SetThreadPriority",
]


def sha(b):
    return hashlib.sha256(b).hexdigest()


class Image:
    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        self.sha256 = sha(self.data)
        self.pe = pefile.PE(data=self.data)
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.mem = self.pe.get_memory_mapped_image()
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.md.detail = True
        text = next(s for s in self.pe.sections if s.Name.rstrip(b"\0") == b".text")
        self.lo = self.base + text.VirtualAddress
        self.hi = self.lo + text.Misc_VirtualSize
        self.text = self.mem[self.lo - self.base:self.hi - self.base]
        self.imports = {}
        for d in getattr(self.pe, "DIRECTORY_ENTRY_IMPORT", []):
            for i in d.imports:
                if i.name:
                    self.imports[i.address] = i.name.decode()
        self.iat = {v: k for k, v in self.imports.items()}
        self.exports = {e.name.decode(): self.base + e.address
                        for e in getattr(self.pe, "DIRECTORY_ENTRY_EXPORT", None).symbols} \
            if hasattr(self.pe, "DIRECTORY_ENTRY_EXPORT") else {}
        self._fn = {}

    def read(self, va, n):
        return self.mem[va - self.base:va - self.base + n]

    def dword(self, va):
        return struct.unpack("<I", self.read(va, 4))[0]

    def insn(self, va):
        return next(self.md.disasm(self.read(va, 16), va), None)

    def span(self, va, n):
        return list(self.md.disasm(self.read(va, n), va))

    def fmt(self, ins):
        text = f"{ins.address:#x}: {ins.mnemonic} {ins.op_str}".strip()
        for op in ins.operands:
            if op.type == x86.X86_OP_MEM and op.mem.base == 0 and op.mem.index == 0 \
                    and op.mem.disp in self.imports:
                text += f"  ; {self.imports[op.mem.disp]}"
        return text

    def rel_callers(self, target):
        """Every E8/E9 rel32 in .text whose destination is target, kept only
        when the decoded instruction at that byte is that call/jmp."""
        out = []
        t = self.text
        for i in range(len(t) - 5):
            if t[i] in (0xE8, 0xE9):
                dest = (self.lo + i + 5 + struct.unpack_from("<i", t, i + 1)[0]) & 0xFFFFFFFF
                if dest == target:
                    ins = self.insn(self.lo + i)
                    if ins and ins.mnemonic in ("call", "jmp") and ins.op_str == hex(target):
                        out.append(self.lo + i)
        return out

    def abs_refs(self, value):
        """Instructions in .text that carry value as an absolute operand.
        Candidate starts are every byte up to 7 before the immediate; a candidate
        is kept only if it decodes to an instruction covering the four bytes and
        naming the value."""
        pat = struct.pack("<I", value)
        out, i = [], 0
        while True:
            i = self.text.find(pat, i)
            if i < 0:
                break
            best, rank = None, 0
            for back in range(1, 8):
                ins = self.insn(self.lo + i - back)
                if not ins or ins.address + ins.size < self.lo + i + 4:
                    continue
                r = 0
                for op in ins.operands:
                    if op.type == x86.X86_OP_MEM and op.mem.disp == value:
                        r = 2
                    elif op.type == x86.X86_OP_IMM and op.imm == value and r < 1:
                        r = 1
                if r > rank:
                    best, rank = ins, r
            out.append(best if best else self.lo + i)
            i += 1
        return out

    def fn(self, entry):
        """Bounded recursive-descent walk of one function. Follows branches,
        not calls; stops at ret and at unresolved indirect jumps (recorded)."""
        if entry in self._fn:
            return self._fn[entry]
        seen, work = set(), [entry]
        calls, apis, icalls, ijmps = set(), set(), [], []
        while work:
            a = work.pop()
            while self.lo <= a < self.hi and a not in seen:
                ins = self.insn(a)
                if ins is None:
                    break
                seen.add(a)
                nxt = a + ins.size
                m = ins.mnemonic
                if m == "call":
                    op = ins.operands[0]
                    if op.type == x86.X86_OP_IMM:
                        calls.add(op.imm)
                    elif op.type == x86.X86_OP_MEM and op.mem.base == 0 and op.mem.index == 0 \
                            and op.mem.disp in self.imports:
                        apis.add(self.imports[op.mem.disp])
                    else:
                        icalls.append(a)
                    a = nxt
                    continue
                if m == "ret":
                    break
                if m == "jmp":
                    op = ins.operands[0]
                    if op.type == x86.X86_OP_IMM:
                        a = op.imm
                        continue
                    if op.type == x86.X86_OP_MEM and op.mem.base == 0 and op.mem.index == 0 \
                            and op.mem.disp in self.imports:
                        apis.add(self.imports[op.mem.disp])
                    else:
                        ijmps.append(a)
                    break
                if m.startswith("j") or m.startswith("loop"):
                    work.append(ins.operands[0].imm)
                a = nxt
        r = {"insns": seen, "calls": calls, "apis": apis, "icalls": icalls, "ijmps": ijmps}
        self._fn[entry] = r
        return r

    def register_apis(self, entry):
        """Imports loaded into a register (mov reg, [IAT]) inside a function;
        VC5 hoists repeated Sleep/SetThreadPriority calls this way."""
        out = set()
        for a in self.fn(entry)["insns"]:
            ins = self.insn(a)
            if ins.mnemonic == "mov" and len(ins.operands) == 2 \
                    and ins.operands[0].type == x86.X86_OP_REG \
                    and ins.operands[1].type == x86.X86_OP_MEM \
                    and ins.operands[1].mem.base == 0 and ins.operands[1].mem.index == 0 \
                    and ins.operands[1].mem.disp in self.imports:
                out.add(self.imports[ins.operands[1].mem.disp])
        return out

    def closure(self, root):
        out, st = set(), [root]
        while st:
            a = st.pop()
            if a in out or not (self.lo <= a < self.hi):
                continue
            out.add(a)
            st.extend(self.fn(a)["calls"])
        return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", required=True)
    ap.add_argument("--dll-dir", help="directory holding ENG1.DLL, ENG3.DLL, TITLE.DLL (default: EXE's)")
    ap.add_argument("--output", default=str(REPO / "work" / "frame_pacing" / "frame_pacing.json"))
    args = ap.parse_args()
    output = Path(args.output).resolve()
    output.relative_to((REPO / "work").resolve())  # receipts stay in ignored work/
    exe_path = Path(args.exe).resolve()
    dll_dir = Path(args.dll_dir).resolve() if args.dll_dir else exe_path.parent
    img = Image(exe_path)
    mods = {m: Image(dll_dir / m) for m in ("ENG1.DLL", "ENG3.DLL", "TITLE.DLL")}
    for name, im in [("HERCULES.EXE", img)] + list(mods.items()):
        if im.sha256 != PINNED[name]:
            sys.exit(f"unexpected {name} {im.sha256}")
    checks = []

    def check(name, ok, detail):
        checks.append({"check": name, "ok": bool(ok), "detail": detail})

    table = [img.dword(SOURCE_TABLE + 4 * k) for k in range(TABLE_WORDS)]

    # ---------------------------------------------------------------- 1. clock APIs
    present = {}
    for name, im in [("HERCULES.EXE", img)] + list(mods.items()):
        present[name] = sorted(a for a in CLOCK_APIS if a in im.iat)
    check("exe_clock_imports",
          present["HERCULES.EXE"] == sorted([
              "timeGetTime", "timeBeginPeriod", "timeEndPeriod", "Sleep", "SetTimer", "KillTimer",
              "WaitMessage", "GetMessageTime", "CreateThread", "SuspendThread", "ResumeThread",
              "SetThreadPriority"]),
          {"present": present["HERCULES.EXE"],
           "absent": sorted(set(CLOCK_APIS) - set(present["HERCULES.EXE"]))})
    strings = {}
    for name, im in mods.items():
        strings[name] = [a for a in CLOCK_APIS if a.encode() in im.data]
    check("engine_dlls_have_no_clock_or_wait_api",
          all(not present[m] for m in mods) and all(not strings[m] for m in mods),
          {"imports": {m: present[m] for m in mods},
           "name_strings_anywhere_in_file": strings,
           "note": "DLLs import GetProcAddress (CRT); no clock/wait API name exists in their bytes to resolve"})
    exe_counts = {a: img.data.count(a.encode() + b"\0") for a in present["HERCULES.EXE"]}
    check("exe_clock_names_only_in_import_table",
          all(v == 1 for v in exe_counts.values()), exe_counts)

    # ---------------------------------------------------------------- 2. OnIdle 0x79A0
    vt26 = img.dword(APP_VTABLE + 4 * 26)
    idle = [i for i in img.span(0x4079A0, 0xFC) if i.address <= 0x407A98]
    idle_calls = [img.fmt(i) for i in idle if i.mnemonic == "call"]
    gate = [img.fmt(i) for i in idle if 0x4079EA <= i.address <= 0x4079FC]
    check("onidle_tick_bracket",
          vt26 == 0x4079A0
          and [i.op_str for i in idle if i.mnemonic == "call"][:6]
          == ["0x405a50", "0x406bb0", "0x405f20", "0x406bb0", "0x405a50", "0x410f40"]
          and gate[0].endswith("mov al, byte ptr [0xba0238]")
          and "0xba0228" in gate[3] and gate[-1].endswith("call 0x405f20"),
          {"calls": idle_calls, "gate": gate})
    tail = [i for i in idle if i.address >= 0x407A63]
    check("onidle_waits_only_when_tick_gate_closed",
          [i.mnemonic for i in tail[:7]] == ["mov", "test", "je", "mov", "test", "je", "call"]
          and img.imports.get(0xBA5ABC) == "WaitMessage" and "0xba5abc" in tail[6].op_str
          and int(tail[2].op_str, 16) == tail[6].address and int(tail[5].op_str, 16) == tail[7].address,
          [img.fmt(i) for i in tail[:8]])
    base_idle = img.span(0x425886, 0x66)
    ret_tail = [img.fmt(i) for i in tail[7:10]] + [img.fmt(i) for i in base_idle if i.address >= 0x4258DE][:6]
    check("onidle_returns_base_onidle_0_true",
          tail[7].op_str == "0" and tail[9].op_str == "0x425886"
          and any(i.op_str == "edi, dword ptr [ebp + 8]" for i in base_idle)
          and any(i.mnemonic == "setl" for i in base_idle)
          and not any(i.mnemonic in ("mov", "xor", "or", "and") and i.op_str.startswith("eax")
                      for i in idle if i.address > 0x407A83),
          {"detail": ret_tail,
           "meaning": "OnIdle(n) -> CWinApp::OnIdle(0) == (0 < 1) == TRUE, so CWinThread::Run keeps calling OnIdle without blocking"})
    gate_closure = set()
    for f in (0x405A50, 0x406BB0):
        for g in img.closure(f):
            gate_closure |= img.fn(g)["apis"] | img.register_apis(g)
    check("onidle_side_calls_do_not_sleep",
          not ({"Sleep", "WaitMessage", "SleepEx"} & gate_closure),
          sorted(a for a in gate_closure if a in CLOCK_APIS))

    # ---------------------------------------------------------------- 3. tick 0x5F20
    body = img.read(0x405F20, 173)
    tick = img.span(0x405F20, 173)
    tick_txt = [img.fmt(i) for i in tick]
    consts = {
        "clamp_hi": [i for i in tick if i.address == 0x405F62][0].op_str,
        "clamp_hi_store": [i for i in tick if i.address == 0x405F6D][0].op_str,
        "clamp_lo": [i for i in tick if i.address == 0x405F78][0].op_str,
        "clamp_lo_store": [i for i in tick if i.address == 0x405F7D][0].op_str,
        "step_test": [i for i in tick if i.address == 0x405F8C][0].op_str,
        "step_sub_loop": [i for i in tick if i.address == 0x405FA2][0].op_str,
        "loop_bound": [i for i in tick if i.address == 0x405FA6][0].op_str,
        "step_sub_final": [i for i in tick if i.address == 0x405FBC][0].op_str,
    }
    CLAMP = int(consts["clamp_hi"].split(", ")[1], 16)
    STEP = int(consts["step_test"].split(", ")[1], 16)
    BOUND = int(consts["loop_bound"].split(", ")[1], 16)
    check("tick_body_is_reconstructed_oracle",
          sha(body) == TICK_BODY_SHA256 and tick[-1].mnemonic == "ret",
          {"sha256": sha(body), "bytes": len(body)})
    check("tick_order_helper_then_timegettime",
          tick_txt[2].endswith("call 0x409cd0") and tick_txt[3].endswith("timeGetTime"),
          tick_txt[:4])
    check("tick_constants",
          (CLAMP, STEP, BOUND) == (0x42, 0x21, 2)
          and consts["clamp_lo"] == "ecx, -0x42" and consts["clamp_lo_store"] == "ecx, 0xffffffbe"
          and consts["step_sub_loop"] == "ecx, 0x21" and consts["step_sub_final"] == "ecx, 0x21",
          {**consts, "decoded": {"clamp_ms": CLAMP, "step_ms": STEP, "max_flagged_per_tick": BOUND}})
    check("tick_flag_protocol",
          any(i.address == 0x405F8A and i.op_str == "bl, 1" for i in tick)
          and any(i.address == 0x405F91 and i.op_str == "byte ptr [0xba01fc], bl" for i in tick)
          and any(i.address == 0x405F97 and i.op_str == "0x405ec0" for i in tick)
          and any(i.address == 0x405FB3 and i.op_str == "byte ptr [0xba01fc], 0" for i in tick)
          and any(i.address == 0x405FBA and i.mnemonic == "jl" for i in tick)
          and any(i.address == 0x405FC5 and i.op_str == "0x405ec0" for i in tick),
          [t for t in tick_txt if "0xba01fc" in t or "0x405ec0" in t or t.split(": ")[0] in ("0x405fba",)])
    check("tick_reset_branch",
          [i.op_str for i in tick if 0x405F2D <= i.address <= 0x405F4F] == [
              "edx, dword ptr [0x91c474]", "edx, edx", "0x405f41", "ecx, dword ptr [0x91c46c]",
              "ecx, ecx", "0x405f51", "dword ptr [0x91c46c], 0", "edx, eax", "ecx, ecx", "0x405f57"],
          [t for t in tick_txt if "0x405f2d" <= t.split(":")[0] <= "0x405f4f"])

    # clock/flag cell ownership
    cell_refs = {}
    for cell in (0x91C470, 0x91C474, 0xBA01FC, 0x91C46C):
        cell_refs[hex(cell)] = [img.fmt(r) if not isinstance(r, int) else f"raw@{r:#x}" for r in img.abs_refs(cell)]
    in_tick = lambda s: 0x405F20 <= int(s.split(":")[0].replace("raw@", ""), 16) < 0x405FD0
    check("accumulator_and_previous_sample_private_to_tick",
          all(in_tick(s) for s in cell_refs["0x91c470"] + cell_refs["0x91c474"])
          and len(cell_refs["0x91c470"]) == 7 and len(cell_refs["0x91c474"]) == 2,
          {k: cell_refs[k] for k in ("0x91c470", "0x91c474")})
    flag_writes = [s for s in cell_refs["0xba01fc"] if s.split(": ")[1].startswith("mov byte ptr [0xba01fc]")]
    flag_reads = [s for s in cell_refs["0xba01fc"] if s not in flag_writes]
    check("catchup_flag_written_only_by_tick",
          len(flag_writes) == 2 and all(in_tick(s) for s in flag_writes)
          and sorted(int(s.split(":")[0], 16) for s in flag_reads)
          == [0x405EFB, 0x40AC60, 0x40ACCF, 0x40B057, 0x40B17B, 0x40B2C2],
          {"writes": flag_writes, "reads": flag_reads})

    # ---------------------------------------------------------------- 4. reset writers
    reset_sites = {}
    for s in cell_refs["0x91c46c"]:
        a = int(s.split(":")[0], 16)
        if "mov dword ptr [0x91c46c]" in s and not in_tick(s):
            reset_sites[hex(a)] = s
    fn_of = {0x405AED: 0x405A50, 0x4069A1: 0x4069A0}
    cd_play = img.fn(0x409E60)
    cd_site = [a for a in cd_play["insns"] if "0x91c46c" in img.insn(a).op_str]
    check("clock_reset_writers",
          sorted(int(a, 16) for a in reset_sites) == sorted([0x405AED, 0x4069A1] + cd_site)
          and len(cd_site) == 1
          and all(a in img.fn(f)["insns"] for a, f in fn_of.items()),
          {"sites": reset_sites,
           "owners": {"0x405aed": "0x5A50 platform sync (OnIdle side call; depth/mode request ba0230 != 0)",
                      "0x4069a1": "0x69A0 palette refresh",
                      hex(cd_site[0]) if cd_site else "?": "0x9E60 CD play (also stamps 96e624 with timeGetTime)"},
           "effect": "next 0x5F20 discards elapsed time: accumulator=0, one unflagged step"})
    pal = img.fn(0x4069A0)
    pal_ins = sorted(pal["insns"])
    wfvb = []
    for a in pal_ins:
        ins = img.insn(a)
        if ins.mnemonic == "call" and ins.op_str.endswith("+ 0x58]"):
            pushes = [img.insn(b) for b in pal_ins if a - 9 <= b < a and img.insn(b).mnemonic == "push"]
            wfvb.append({"site": hex(a), "pushes": [p.op_str for p in pushes]})
    pal_calls = [img.fmt(img.insn(a)) for a in pal_ins if img.insn(a).mnemonic == "call"]
    check("palette_refresh_waits_vblank_and_presents",
          [w["pushes"][:2] for w in wfvb] == [["0", "1"], ["0", "4"], ["0", "1"]]
          and pal_calls[:3] == ["0x4069ab: call 0x407140", "0x4069b6: call 0x406ee0", "0x4069be: call 0x406620"],
          {"WaitForVerticalBlank_vtbl_0x58": wfvb, "calls": pal_calls,
           "flags": "1=DDWAITVB_BLOCKBEGIN, 4=DDWAITVB_BLOCKEND (pinned ddraw.h); DirectDraw-mode branch only"})
    callers_69a0 = [hex(a) for a in img.rel_callers(0x4069A0)]
    s145 = table[145]
    s148 = table[148]
    check("palette_slot145_and_movie_slot148_reach_69a0",
          s145 == 0x405760 and s148 == 0x405450
          and "0x4057da" in callers_69a0 and "0x4059ef" in callers_69a0
          and img.insn(0x4057D8).op_str == "1" and img.insn(0x4059ED).op_str == "1"
          and {0x4069A0, 0x403140, 0x4037A0} <= img.fn(0x405450)["calls"],
          {"slot145": hex(s145), "slot148": hex(s148), "callers_of_69a0": callers_69a0,
           "slot148_calls": sorted(hex(c) for c in img.fn(0x405450)["calls"]),
           "note": "0x4059EF is slot 145's switch path (indirect jmp table at 0x405A04)"})

    caller_owner = {}
    for a in img.rel_callers(0x4069A0):
        caller_owner[hex(a)] = [hex(f) for f in (0x405450, 0x405760, 0x406BB0, 0x4071A0)
                                if a in img.fn(f)["insns"]]
    switch_ok = (img.insn(0x4057FF).op_str == "dword ptr [eax*4 + 0x405a04]"
                 and all(0x405806 <= img.dword(0x405A04 + 4 * k) <= 0x4059F7 for k in range(18))
                 and img.insn(0x4057F6).op_str == "eax, 0x11" and img.insn(0x4059ED).op_str == "1")
    caller_owner["0x4059ef"] = ["0x405760 (switch path)"] if switch_ok else []
    no_refresh = [k - 6 for k in range(18) if img.dword(0x405A04 + 4 * k) == 0x4059F7]
    check("palette_refresh_callers_owned",
          all(caller_owner.values()) and len(caller_owner) == 6 and no_refresh == [0, 4],
          {"callers": caller_owner,
           "slot145_preset_indices_returning_without_refresh": no_refresh,
           "note": "slot 145 with a NULL path switches on index+6 (0..0x11); out-of-range indices also return at 0x4059F7"})
    cd = sorted(img.fn(0x409E60)["insns"])
    cd_sleep = [f"{img.fmt(img.insn(a))} / {img.fmt(img.insn(b))}" for a, b in zip(cd, cd[1:])
                if img.insn(a).op_str == "0x64" and img.insn(a).mnemonic == "push"
                and img.insn(b).mnemonic == "call" and img.insn(b).op_str == "ebp"]
    check("cd_play_retry_sleeps_100ms",
          len(cd_sleep) == 1 and img.insn(0x409E83).op_str == "ebp, dword ptr [0xba58b0]",
          {"sites": cd_sleep, "sleep_loaded": img.fmt(img.insn(0x409E83))})

    # ---------------------------------------------------------------- 5. dispatch + exports
    disp = img.span(0x405EC0, 0x54)
    disp_txt = [img.fmt(i) for i in disp]
    gp = img.span(0x405C89, 0x1A)
    check("dispatch_passes_table_and_flag_to_export",
          any(i.op_str == "edx, dword ptr [0x4365a0]" for i in disp)
          and any(i.op_str == "cl, byte ptr [0xba01fc]" for i in disp)
          and any(i.mnemonic == "call" and i.op_str == "eax" for i in disp)
          and img.dword(0x4365A0) == SOURCE_TABLE
          and "GetProcAddress" in img.fmt(gp[3]) and gp[5].op_str == "dword ptr [0x91c45c], eax"
          and img.read(0x436868, 17) == b"PC_DLLEngineMain\0",
          {"dispatch": [t for t in disp_txt if "0x405ef3" <= t.split(":")[0] <= "0x405f05"],
           "binding": [img.fmt(i) for i in gp[:6]], "proc_name": "PC_DLLEngineMain"})
    export_latch = {}
    for m, im in mods.items():
        ex = im.exports["PC_DLLEngineMain"]
        ins = im.span(ex, 0x80)
        seq = None
        for k in range(len(ins) - 4):
            w = ins[k:k + 5]
            if w[0].op_str == "edx, dword ptr [esp + 0x10]" and w[1].op_str == f"eax, dword ptr [{MODULE_TABLE[m] + 4:#x}]" \
                    and w[2].op_str == "edx, 1" and w[3].op_str == "dword ptr [eax + 0x40], edx" \
                    and w[4].mnemonic == "call":
                seq = w
        export_latch[m] = [im.fmt(i) for i in seq] if seq else None
    check("exports_latch_flag_into_context_0x40",
          all(export_latch.values()), export_latch)
    eng1 = mods["ENG1.DLL"]
    d = [i for i in eng1.span(0x10020300, 0x368)]
    ebx_writes = [eng1.fmt(i) for i in d
                  if i.mnemonic not in ("push", "pop", "cmp", "test") and i.operands
                  and i.operands[0].type == x86.X86_OP_REG and i.reg_name(i.operands[0].reg) == "ebx"]
    gated = []
    for k, i in enumerate(d):
        if i.mnemonic == "cmp" and i.op_str in ("dword ptr [eax + 0x40], ebx", "dword ptr [ecx + 0x40], ebx",
                                                "dword ptr [edx + 0x40], ebx"):
            gated.append({"load": eng1.fmt(d[k - 1]), "test": eng1.fmt(i), "branch": eng1.fmt(d[k + 1]),
                          "call": eng1.fmt(d[k + 2]),
                          "skip_lands_after_call": int(d[k + 1].op_str, 16) == d[k + 3].address})
    zero_write = [eng1.fmt(i) for i in d if i.mnemonic == "mov" and i.op_str.endswith("+ 0x40], ebx")]
    check("eng1_flag_skips_two_calls_per_step",
          len(ebx_writes) == 1 and ebx_writes[0].endswith("xor ebx, ebx")
          and [g["call"].split("call ")[1] for g in gated] == ["0x10032bd0", "0x1002da90"]
          and all(g["load"].endswith("dword ptr [0x10070f24]") and g["branch"].split(": ")[1].startswith("jne")
                  and g["skip_lands_after_call"] for g in gated)
          and zero_write == ["0x10020498: mov dword ptr [ecx + 0x40], ebx"],
          {"ebx_zero": ebx_writes, "ebx_note": "only the prologue writes ebx (besides the epilogue pop); ebx is callee-saved across the calls",
           "gated": gated, "restart_clears": zero_write,
           "unchanged_on_flagged_steps": "every other iteration call, including 0x2C620 order-table submission to slot 89 and the context tick increment at +0x38"})
    # host AC60 gates
    ac = img.span(0x40AC60, 0x690)
    gates = {}
    for k, i in enumerate(ac):
        if "0xba01fc" in i.op_str:
            j = next((x for x in ac[k + 1:k + 4] if x.mnemonic.startswith("j")), None)
            gates[hex(i.address)] = [img.fmt(x) for x in ac[k:k + 3]] + ([img.fmt(j)] if j and img.fmt(j) not in [img.fmt(x) for x in ac[k:k + 3]] else [])
    ac_ok = (
        img.insn(0x40AC6B).op_str == "0x40ac97" and img.insn(0x40AC8A).op_str == "0x406ee0"
        and img.insn(0x40AC92).op_str == "0x406ff0"
        and img.insn(0x40ACD7).op_str == "0x40aca2"
        and img.insn(0x40B180).op_str == "0x40aca2"
        and img.insn(0x40B2C9).op_str == "0x40b2e2"
        and [img.insn(a).op_str for a in (0x40B2D0, 0x40B2D8, 0x40B2DD, 0x40B2E2)]
        == ["0x4070c0", "0x407140", "0x406620", "0x40bd80"])
    b057 = img.span(0x40B057, 0x10)
    check("slot89_flag_gates",
          ac_ok and table[89] == 0x40AC60 and b057[1].mnemonic == "test" and b057[2].mnemonic == "jne",
          {"gates": gates,
           "meaning": {"0x40ac60": "flag=1 skips color fill 0x6EE0 and surface lock 0x6FF0",
                       "0x40accf": "class 0x20 primitives skipped (walk continues)",
                       "0x40b057": "class 0x60 primitives skipped: " + img.fmt(b057[2]),
                       "0x40b17b": "class 0x40: no effect on either path",
                       "0x40b2c2": "flag=1 skips 0x70C0(0xFF) LUT, 0x7140 unlock, 0x6620 present; 0xBD80 runs on both"}})
    # present/lock services
    pres = img.span(0x406620, 0xD5)
    flip = [i for i in pres if i.address in (0x406664, 0x406666, 0x406668, 0x406669)]
    lock = [i for i in img.span(0x40704D, 0x11)]
    check("present_flip_wait_and_lock_wait",
          [i.op_str for i in flip] == ["1", "0", "eax", "dword ptr [ecx + 0x2c]"]
          and any("StretchDIBits" in img.fmt(i) for i in pres) and any("GdiFlush" in img.fmt(i) for i in pres)
          and [i.op_str for i in lock[1:5]] == ["0", "edx, [esp + 8]", "1", "edx"]
          and lock[-1].op_str == "dword ptr [ecx + 0x64]",
          {"flip": [img.fmt(i) for i in flip], "lock": [img.fmt(i) for i in lock],
           "meaning": "IDirectDrawSurface::Flip(NULL, DDFLIP_WAIT) on 0x96E3E8; Lock(NULL,&desc,DDLOCK_WAIT,NULL) on 0x96E3EC; GDI path StretchDIBits+GdiFlush (no vblank wait)"})
    # host VSync-style slot and callback invocation
    s248 = img.span(table[248], 0x10)
    check("slot248_is_a_call_counter",
          table[248] == 0x40AC10
          and [img.fmt(i) for i in s248[:5]] == [
              "0x40ac10: mov ecx, dword ptr [0xa8e700]", "0x40ac16: mov eax, ecx", "0x40ac18: inc ecx",
              "0x40ac19: mov dword ptr [0xa8e700], ecx", "0x40ac1f: ret"],
          {"slot248": [img.fmt(i) for i in s248[:5]],
           "meaning": "returns the previous count and increments it; ignores its argument; never waits"})
    wrappers248 = {}
    for m, im in mods.items():
        k = im.text.find(bytes([0xFF, 0x15]) + struct.pack("<I", MODULE_TABLE[m] + 4 * 248))
        w = im.span(im.lo + k - 5, 16)[:4] if k >= 0 else []
        wrappers248[m] = [im.fmt(i) for i in w]
    check("engine_slot248_wrappers",
          all(len(v) == 4 and v[0].endswith("eax, dword ptr [esp + 4]") and v[1].endswith("push eax")
              and "call dword ptr" in v[2] for v in wrappers248.values())
          and [v[0].split(":")[0] for v in wrappers248.values()] == ["0x1002cc30", "0x10020170", "0x1000cc70"],
          wrappers248)
    cb_inv = {hex(t): [hex(a) for a in img.rel_callers(t)] for t in (0x40BD80, 0x40BDB0)}
    check("engine_callbacks_invoked_synchronously",
          table[92] == 0x40BDA0 and table[249] == 0x40BDD0
          and cb_inv["0x40bdb0"] == ["0x40bd21"] and "0x40b2e2" in cb_inv["0x40bd80"],
          {"setters": {"slot92": hex(table[92]), "slot249": hex(table[249])}, "invokers": cb_inv,
           "note": "invokers are reached by direct calls only (no pointer to them in the image)",
           "absolute_pointer_occurrences": {hex(t): img.mem.count(struct.pack('<I', t)) for t in (0x40BD80, 0x40BDB0)}})

    # ---------------------------------------------------------------- 6. timing call sites
    # owner functions: rel-call targets + table targets + app vtable + thread start
    roots = set()
    t = img.text
    for i in range(len(t) - 5):
        if t[i] == 0xE8:
            dest = (img.lo + i + 5 + struct.unpack_from("<i", t, i + 1)[0]) & 0xFFFFFFFF
            if img.lo <= dest < img.hi:
                roots.add(dest)
    roots |= {v for v in table if img.lo <= v < img.hi}
    roots |= {img.dword(APP_VTABLE + 4 * k) for k in range(41)}
    roots.add(0x4033E0)
    sites = {}
    for api in ("timeGetTime", "timeBeginPeriod", "timeEndPeriod", "Sleep"):
        out = []
        for r in img.abs_refs(img.iat[api]):
            if isinstance(r, int):
                out.append({"site": hex(r), "owner": None})
                continue
            own = sorted(f for f in (0x405F20, 0x409CD0, 0x409D60, 0x409DB0, 0x409E60, 0x409F60,
                                     0x40A0B0, 0x403140, 0x403680, 0x4037A0, 0x4033E0, 0x405E10,
                                     0x405E90, 0x4035D0) if r.address in img.fn(f)["insns"])
            if not own:
                own = sorted(f for f in roots if r.address in img.fn(f)["insns"])[:3]
            out.append({"site": img.fmt(r), "owner": [hex(o) for o in own]})
        sites[api] = out
    tbp = [s for s in sites["timeBeginPeriod"]]
    check("timer_resolution_1ms_for_engine_lifetime",
          sorted(o for s in tbp for o in s["owner"]) == ["0x403140", "0x405e10"]
          and img.insn(0x405E10).op_str == "1" and img.insn(0x405E90).op_str == "1"
          and "timeEndPeriod" in img.fmt(img.insn(0x405E92)),
          {"timeBeginPeriod": tbp, "timeEndPeriod": sites["timeEndPeriod"],
           "meaning": "0x5E10 (InitInstance tail, engine startup) requests 1 ms; 0x5E90 fatal/shutdown chain releases it; the movie stream open/close pair 0x3140/0x3680 nests a second request"})
    tgt_owners = sorted({o for s in sites["timeGetTime"] for o in s["owner"]})
    check("timegettime_call_sites_classified",
          all(s["owner"] for s in sites["timeGetTime"]) and len(sites["timeGetTime"]) == 11
          and set(tgt_owners) <= {"0x405f20", "0x409cd0", "0x409d60", "0x409db0", "0x409e60", "0x409f60",
                                  "0x40a0b0", "0x4037a0", "0x4033e0"},
          {"sites": sites["timeGetTime"],
           "classes": {"0x405f20": "frame clock", "0x409c00-0x40a100": "CD-audio track timer (MCI)",
                       "0x4033e0/0x4037a0": "movie stream thread / frame wait"}})
    check("sleep_call_sites_classified",
          all(s["owner"] for s in sites["Sleep"])
          and {o for s in sites["Sleep"] for o in s["owner"]} <= {"0x403140", "0x403680", "0x4037a0", "0x409e60"},
          {"sites": sites["Sleep"],
           "meaning": "movie stream open/close/frame wait and CD play retry (Sleep(100) on MCI failure); none in OnIdle's own calls, slot 89 or present; 0x5F20 reaches Sleep only via 0x9CD0 -> 0x9E60 (see tick_closure_waits)"})
    tick_cl = img.closure(0x405F20)
    tick_apis = set()
    for f in tick_cl:
        tick_apis |= img.fn(f)["apis"] | img.register_apis(f)
    check("tick_closure_waits",
          {"Sleep", "timeGetTime"} <= tick_apis and 0x409E60 in tick_cl
          and not ({"WaitMessage", "SleepEx", "WaitForSingleObject"} & tick_apis),
          {"clock_or_wait_apis_reachable_from_0x5f20_by_direct_calls": sorted(a for a in tick_apis if a in CLOCK_APIS),
           "only_sleep_path": "0x5F20 -> 0x9CD0 (CD track ended) -> 0x9E60 -> Sleep(100) retry; engine-export calls are indirect and not followed"})
    thread = img.closure(0x4033E0)
    thread_writes = []
    for f in thread:
        for a in img.fn(f)["insns"]:
            op = img.insn(a).op_str
            if any(h in op for h in ("0x91c46c", "0x91c470", "0x91c474", "0xba01fc", "0x91c45c")):
                thread_writes.append(img.fmt(img.insn(a)))
    check("stream_thread_does_not_touch_frame_clock",
          img.insn(0x403354).op_str == "0x4033e0" and not thread_writes
          and not ({0x40BD80, 0x40BDB0, 0x405EC0, 0x405F20} & thread),
          {"thread_start": "0x4033e0 (CreateThread at 0x403379)", "functions": len(thread), "clock_refs": thread_writes})

    # ---------------------------------------------------------------- 7. palette call sites in engines
    pal_callers = {}
    for m, im in mods.items():
        tb = MODULE_TABLE[m] + 4 * 145
        wrap = None
        pat = b"\xFF\x15" + struct.pack("<I", tb)
        k = im.text.find(pat)
        if k >= 0:
            wrap = im.lo + k - 10  # wrapper: mov eax,[esp+8]; mov ecx,[esp+4]; push; push; call [slot]
            w0 = im.span(wrap, 10)
            if not (w0[0].op_str == "eax, dword ptr [esp + 8]" and w0[1].op_str == "ecx, dword ptr [esp + 4]"):
                wrap = None
        pal_callers[m] = {"wrapper": hex(wrap) if wrap else None,
                          "callers": [hex(a) for a in im.rel_callers(wrap)] if wrap else None}
    check("engine_palette_slot145_callers",
          pal_callers["ENG1.DLL"]["callers"] == ["0x100203f6"]
          and pal_callers["ENG3.DLL"]["callers"] and len(pal_callers["ENG3.DLL"]["callers"]) == 1
          and pal_callers["TITLE.DLL"]["callers"] and len(pal_callers["TITLE.DLL"]["callers"]) == 7,
          {**pal_callers,
           "ENG1_context": "0x100203F6 lies in the dispatcher's state-0 initial block (0x20325..0x203FE), run once per module entry, not in the iteration"})

    # ---------------------------------------------------------------- 8. timing model
    model = run_model(CLAMP, STEP, BOUND)
    check("model_constants_from_bytes", (CLAMP, STEP, BOUND) == (66, 33, 2),
          {"clamp": CLAMP, "step": STEP, "bound": BOUND})

    result = {
        "kind": "frame_pacing_audit",
        "inputs": {n: PINNED[n] for n in PINNED},
        "all_ok": all(c["ok"] for c in checks),
        "checks": checks,
        "model": model,
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=1) + "\n")
    for c in checks:
        print(("PASS " if c["ok"] else "FAIL ") + c["check"])
    print("ALL_OK" if result["all_ok"] else "NOT_OK")
    return 0 if result["all_ok"] else 1


# ---------------------------------------------------------------------- model
def next_vblank(now, vb):
    """First vblank strictly after now (epsilon guards float multiples of vb)."""
    return (int(now / vb + 1e-9) + 1) * vb


class Tick:
    """Python transcription of 0x5F20's arithmetic (constants passed in from the
    decoded bytes). Callbacks are assumed not to re-enter or mutate the cells."""

    def __init__(self, clamp, step, bound):
        self.clamp, self.step, self.bound = clamp, step, bound
        self.acc = 0
        self.prev = 0
        self.reset = 0

    def __call__(self, now):
        if self.prev == 0 or self.reset:
            self.reset = 0
            acc = 0
        else:
            acc = self.acc + ((now - self.prev) & 0xFFFFFFFF)
            acc = acc - (1 << 32) if acc >= 1 << 31 else acc
        self.prev = now
        acc = max(-self.clamp, min(self.clamp, acc))
        steps = []
        i = 0
        while acc >= self.step and i < self.bound:
            steps.append(1)
            acc -= self.step
            i += 1
        if acc >= 0:
            acc -= self.step
            steps.append(0)
        self.acc = acc
        return steps  # 1 = flagged catch-up, 0 = presenting step


def run_fixed_period(clamp, step, bound, period, duration=120_000, start=1000):
    tk = Tick(clamp, step, bound)
    now, flagged, shown = start, 0, 0
    while now < start + duration:
        for s in tk(now):
            flagged += s
            shown += 1 - s
        now += period
    steps = flagged + shown
    return {"period_ms": period, "speed": round(steps * step / duration, 4),
            "steps_per_s": round(steps * 1000 / duration, 3),
            "presents_per_s": round(shown * 1000 / duration, 3),
            "flagged_fraction": round(flagged / steps, 4) if steps else None}


def run_vsync(clamp, step, bound, refresh_hz, cpu_ms, sim_ms=0.0, duration=120_000.0, start=1000.0):
    """Host loop: idle iterations are free; a presenting step costs cpu_ms and
    then blocks until the next vblank; a flagged step costs sim_ms. The clock is
    integer milliseconds as timeGetTime returns."""
    tk = Tick(clamp, step, bound)
    vb = 1000.0 / refresh_hz
    now, flagged, shown, idle = start, 0, 0, 0
    while now < start + duration:
        steps = tk(int(now))
        if not steps:
            # Exact skip: a stepless tick only adds elapsed time, so the next
            # step happens at the first integer ms where acc reaches zero.
            idle += 1
            now = max(float(tk.prev - tk.acc), float(int(now) + 1))
            continue
        for s in steps:
            if s:
                flagged += 1
                now += sim_ms
            else:
                shown += 1
                now += cpu_ms
                now = next_vblank(now, vb)
    total = flagged + shown
    return {"refresh_hz": refresh_hz, "cpu_ms": cpu_ms, "speed": round(total * step / duration, 4),
            "presents_per_s": round(shown * 1000 / duration, 3),
            "flagged_fraction": round(flagged / total, 4)}


def run_reset_every_present(clamp, step, bound, refresh_hz, waits_vblank):
    """Worst case for the clock-reset writers: every presenting step also calls
    0x69A0 (palette refresh), which resets the clock and, in DirectDraw mode,
    waits BLOCKBEGIN, BLOCKEND, BLOCKBEGIN (about one refresh) after presenting."""
    tk = Tick(clamp, step, bound)
    vb = 1000.0 / refresh_hz
    now, total, duration, start = 1000.0, 0, 60_000.0, 1000.0
    while now < start + duration:
        steps = tk(int(now))
        if not steps:
            now = max(float(tk.prev - tk.acc), float(int(now) + 1))
            continue
        total += len(steps)
        tk.reset = 1
        if waits_vblank:
            now = next_vblank(now, vb)      # Flip(DDFLIP_WAIT) inside 0x69A0's present
            now = next_vblank(now, vb)      # BLOCKEND + BLOCKBEGIN
        else:
            now += 0.5                          # GDI path: no vblank wait
    return {"refresh_hz": refresh_hz, "vblank_waits": waits_vblank,
            "speed": round(total * step / duration, 3)}


def run_model(clamp, step, bound):
    fixed = [run_fixed_period(clamp, step, bound, p) for p in range(1, 141)]
    full = [r["period_ms"] for r in fixed if r["speed"] >= 0.999]
    first_slow = next(r for r in fixed if r["speed"] < 0.999)
    vs = [run_vsync(clamp, step, bound, hz, cpu) for hz in (50, 60, 75, 85, 100, 120, 144)
          for cpu in (2.0, 12.0, 20.0)]
    vs_half = [run_vsync(clamp, step, bound, hz, cpu) for hz, cpu in ((60, 17.0), (60, 34.0), (60, 50.0), (30, 2.0), (20, 2.0))]
    reset = [run_reset_every_present(clamp, step, bound, hz, w) for hz, w in ((60, True), (75, True), (120, True), (60, False))]
    return {
        "assumption": "No callback re-enters 0x5F20 or mutates its cells; idle loop iterations take ~0 ms; vblank model is synchronous (a presenting step returns at the next vblank after its CPU work). Driver flip-queue depth is not modelled.",
        "nominal": {"step_ms": step, "steps_per_s": round(1000 / step, 3)},
        "fixed_period_full_speed_contiguous_max_ms": first_slow["period_ms"] - 1,
        "fixed_period_full_speed_isolated_above": [p for p in full if p > first_slow["period_ms"]],
        "fixed_period_first_slowdown": first_slow,
        "fixed_period_samples": [r for r in fixed if r["period_ms"] in (1, 16, 17, 33, 34, 40, 50, 60, 66, 67, 70, 80, 99, 100, 120, 140)],
        "vsync_blocking": vs,
        "vsync_slow_frames": vs_half,
        "reset_every_present_worst_case": reset,
    }


if __name__ == "__main__":
    sys.exit(main())
