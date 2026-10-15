"""Audit the owner of host callback 0x79A0: the application object's MFC
OnIdle override, dispatched from CWinThread::Run through vtable slot 26.

Read-only: decodes the pinned unpatched HERCULES.EXE with Capstone/pefile and
never loads or executes it. The lead came from an REA/Ghidra session
(prototypes/rea_eval); every claim below is re-derived here from bytes.

  python renderer_contract/tests/audit_idle_owner.py --exe <HERCULES.EXE>
      [--output work/idle_owner/idle_owner.json]
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
import pefile  # noqa: E402

PINNED = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
BASE = 0x400000
APP_VTABLE = 0x42E1D8
SLOT_INIT_INSTANCE, SLOT_RUN, SLOT_ON_IDLE, SLOT_EXIT_INSTANCE = 22, 23, 26, 28


class Image:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        self.sha256 = hashlib.sha256(self.data).hexdigest()
        self.pe = pefile.PE(data=self.data)
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        text = next(s for s in self.pe.sections if s.Name.rstrip(b"\0") == b".text")
        self.text_lo = BASE + text.VirtualAddress
        self.text_hi = self.text_lo + text.Misc_VirtualSize
        self.imports = {i.address: (i.name or b"").decode()
                        for d in self.pe.DIRECTORY_ENTRY_IMPORT for i in d.imports}

    def read(self, va, n):
        return self.pe.get_data(va - BASE, n)

    def dword(self, va):
        return struct.unpack("<I", self.read(va, 4))[0]

    def span(self, va, n):
        return list(self.md.disasm(self.read(va, n), va))

    def until_ret(self, va, limit=0x400):
        out = []
        for ins in self.md.disasm(self.read(va, limit), va):
            out.append(ins)
            if ins.mnemonic == "ret":
                break
        return out

    def sweep(self):
        code = self.read(self.text_lo, self.text_hi - self.text_lo)
        off = 0
        while off < len(code):
            got = False
            for ins in self.md.disasm(code[off:], self.text_lo + off):
                got = True
                yield ins
                off = ins.address + ins.size - self.text_lo
            if not got:
                off += 1

    def fmt(self, ins):
        text = f"{ins.address:#x}: {ins.mnemonic} {ins.op_str}".strip()
        if "dword ptr [0x" in ins.op_str:
            target = int(ins.op_str.split("[0x")[1].split("]")[0], 16)
            if target in self.imports:
                text += f"  ; {self.imports[target]}"
        return text


def stack_pushes_before(img, site):
    """Count pushes between the previous call/ret/jmp and the site."""
    window = img.span(site - 0x24, 0x24)
    window = [i for i in window if i.address < site]
    count = 0
    for ins in reversed(window):
        if ins.mnemonic in ("call", "ret", "jmp") or ins.mnemonic.startswith("j"):
            break
        if ins.mnemonic == "push":
            count += 1
    return count


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", required=True)
    ap.add_argument("--output", default=str(REPO / "work" / "idle_owner" / "idle_owner.json"))
    args = ap.parse_args()
    output = Path(args.output).resolve()
    output.relative_to((REPO / "work").resolve())  # receipts stay in ignored work/
    img = Image(args.exe)
    if img.sha256 != PINNED:
        sys.exit(f"unexpected EXE {img.sha256}")
    checks = []

    def check(name, ok, detail):
        checks.append({"check": name, "ok": bool(ok), "detail": detail})

    # Object, constructor, vtable.
    init = img.span(0x407660, 10)
    check("static_init_object", init[0].op_str == "ecx, 0x96e428" and init[1].op_str == "0x407620",
          [img.fmt(i) for i in init[:2]])
    ctor = img.until_ret(0x4075E0)
    check("ctor_stores_vptr", any(i.op_str == f"dword ptr [esi], {APP_VTABLE:#x}" for i in ctor),
          [img.fmt(i) for i in ctor])
    base_ctor = next(int(i.op_str, 16) for i in ctor if i.mnemonic == "call")
    vt = [img.dword(APP_VTABLE + 4 * k) for k in range(41)]
    gmm = img.span(vt[12], 6)
    check("slot12_get_message_map", gmm[0].op_str == "eax, 0x42e170" and gmm[1].mnemonic == "ret",
          [img.fmt(i) for i in gmm[:2]])
    base_vptr = [i for i in img.until_ret(base_ctor, 0x200)
                 if i.mnemonic == "mov" and i.op_str.startswith("dword ptr [esi], 0x")]
    base_vt = int(base_vptr[-1].op_str.split(", ")[1], 16)
    overrides = [k for k in range(41) if img.dword(base_vt + 4 * k) != vt[k]]
    check("cwinapp_base_vtable", base_vt == 0x4309F4, {"base_ctor": hex(base_ctor), "vtable": hex(base_vt)})
    check("override_slots", overrides == [1, 12, SLOT_INIT_INSTANCE, SLOT_ON_IDLE, SLOT_EXIT_INSTANCE],
          {k: hex(vt[k]) for k in overrides})
    check("legacy_span_is_vtable_tail", 0x42E210 - APP_VTABLE == 14 * 4 and img.dword(APP_VTABLE + 41 * 4) == 0,
          "renderer_contract's earlier 0x2E210..0x2E27F span = app vtable slots 14..40")

    # Base OnIdle and the override.
    base_idle = img.dword(base_vt + 4 * SLOT_ON_IDLE)
    body = img.span(base_idle, 0x66)
    check("base_slot26_cwinapp_onidle",
          base_idle == 0x425886 and any("[esi + 0x80]" in i.op_str for i in body)
          and any(i.mnemonic == "setl" for i in body) and body[-1].op_str == "4",
          [img.fmt(i) for i in body[-6:]])
    idle = img.span(0x4079A0, 0xFC)
    check("override_is_0x4079a0_thiscall_ret4",
          vt[SLOT_ON_IDLE] == 0x4079A0
          and any(i.address == 0x407A98 and i.mnemonic == "ret" and i.op_str == "4" for i in idle)
          and any(i.op_str == "dword ptr [ebp - 0x420], ecx" for i in idle),
          [img.fmt(i) for i in idle if 0x407A75 <= i.address <= 0x407A98])
    check("override_tick_and_wait",
          any(i.op_str == "0x405f20" for i in idle) and any("0xba5abc" in i.op_str for i in idle),
          [img.fmt(i) for i in idle if i.mnemonic == "call"])
    check("waitmessage_import", img.imports.get(0xBA5ABC) == "WaitMessage", img.imports.get(0xBA5ABC))
    seh = img.span(0x407A1D, 0x3F)
    check("override_seh_reports_fatal",
          img.dword(0x42E280) == 0xFFFFFFFF and img.dword(0x42E284) == 0x407A1D
          and img.dword(0x42E288) == 0x407A27 and any(i.op_str == "0x410460" for i in seh)
          and img.read(0x436C38, 33) == b"Exception %08lX at Address %08lX\0",
          [img.fmt(i) for i in seh])

    # Dispatch: CWinThread::Run idle loop, and every other +0x68 virtual call.
    sites = [ins.address for ins in img.sweep()
             if ins.mnemonic == "call" and ins.op_str.startswith("dword ptr [e")
             and ins.op_str.endswith(" + 0x68]")]
    run_loop = img.span(0x425A22, 0x4B)
    check("cwinthread_run_idle_loop",
          0x425A56 in sites and any("PeekMessageA" in img.fmt(i) for i in run_loop)
          and img.dword(base_vt + 4 * SLOT_RUN) == 0x4257B6,
          [img.fmt(i) for i in run_loop])
    classified = {}
    for s in sites:
        classified[hex(s)] = {"stack_pushes": stack_pushes_before(img, s)}
    classified["0x4064ce"]["note"] = "COM object at [0x96E3EC] slot 26 after TextOutA (IDirectDrawSurface::ReleaseDC shape)"
    classified["0x425a56"]["note"] = "CWinThread::Run idle loop: OnIdle(lIdleCount++)"
    one_arg = [s for s, v in classified.items() if v["stack_pushes"] == 1]
    check("only_run_matches_onidle_arity", sorted(classified) == sorted(
        ["0x4064ce", "0x4118bf", "0x4241a3", "0x425a56", "0x42a465", "0x42b6b8"]), classified)

    # InitInstance ends with the direct DirectDraw/engine-load chain.
    ii = img.until_ret(vt[SLOT_INIT_INSTANCE], 0x300)
    calls = [i for i in ii if i.mnemonic == "call"]
    check("initinstance_last_call_is_5e10",
          vt[SLOT_INIT_INSTANCE] == 0x407670 and calls[-1].op_str == "0x405e10"
          and "UpdateWindow" in img.fmt(calls[-2]),
          [img.fmt(i) for i in calls[-3:]])

    result = {"kind": "idle_owner_audit", "exe_sha256": img.sha256,
              "all_ok": all(c["ok"] for c in checks), "checks": checks,
              "one_push_sites": one_arg}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=1) + "\n")
    for c in checks:
        print(("PASS " if c["ok"] else "FAIL ") + c["check"])
    print("ALL_OK" if result["all_ok"] else "NOT_OK")
    return 0 if result["all_ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
