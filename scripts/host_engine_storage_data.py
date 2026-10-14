"""Fresh VC5 audit of five original host loader/CRT storage prefixes."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'scripts'), 'C:/tools/hercules/python']
from match import compile_source, DEFAULT_TOOLCHAIN, verify_toolchain
from coff import COFF
from pe import PE

ORACLE_SHA256 = '587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
FIELDS = {
    '_g_host_engine_entry_91c45c': (0x51c45c, 4),
    '_g_host_engine_gate_91c460': (0x51c460, 4),
    '_g_host_engine_selector_91c464': (0x51c464, 4),
    '_g_host_engine_module_91c468': (0x51c468, 4),
    '_g_host_engine_jump_buffer_91bfd8': (0x51bfd8, 64),
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-exe', type=Path,
                        default=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE'))
    parser.add_argument('--output', type=Path, default=ROOT / 'work/host_engine_storage_data')
    args = parser.parse_args()
    out = args.output.resolve()
    if not out.is_relative_to((ROOT / 'work').resolve()):
        parser.error('--output must remain under ignored work/')
    out.mkdir(parents=True, exist_ok=True)
    source = ROOT / 'calibration/host_engine_storage.c'
    header = ROOT / 'calibration/host_engine_cells.h'
    oracle = args.native_exe.resolve()
    if sha(oracle) != ORACLE_SHA256:
        raise ValueError('pinned retail oracle changed')
    toolchain = verify_toolchain(DEFAULT_TOOLCHAIN)
    sdk = DEFAULT_TOOLCHAIN / 'include/setjmp.h'
    watched = [Path(__file__).resolve(), source, header, oracle, sdk,
               ROOT / 'toolchains/manifest.json', ROOT / 'scripts/match.py',
               ROOT / 'scripts/coff.py', ROOT / 'scripts/pe.py']
    pins = {str(p): sha(p) for p in watched}
    obj, command = compile_source(source, ['/O2'])
    coff, pe = COFF(obj), PE(oracle)
    definitions = [s for s in coff.symbols.values()
                   if s['storage'] == 2 and (s['section'] > 0 or s['section'] == 0 and s['value'] > 0)]
    if {s['name'] for s in definitions} != set(FIELDS) or len(definitions) != len(FIELDS):
        raise ValueError('public provider definition set differs')
    rows, intervals, used_sections = [], [], set()
    for symbol in definitions:
        rva, size = FIELDS[symbol['name']]
        if symbol['type'] & 0x20 or symbol['section'] <= 0:
            raise ValueError('provider must define data in an ordinary zero-fill section')
        section = coff.sections[symbol['section'] - 1]
        if not section['flags'] & 0x80 or section['flags'] & 0x20 or section['relocations'] or section['data']:
            raise ValueError('provider has code, relocations or initialized data')
        start, end = symbol['value'], symbol['value'] + size
        if end > section['size']:
            raise ValueError('field exceeds source contribution')
        if any(symbol['section'] == sec and start < hi and lo < end for sec, lo, hi in intervals):
            raise ValueError('source fields overlap')
        intervals.append((symbol['section'], start, end))
        used_sections.add(symbol['section'])
        target = next(s for s in pe.sections if s['virtual_address'] <= rva and
                      rva + size <= s['virtual_address'] + s['virtual_size'])
        if not target['characteristics'] & 0x80000000 or rva < target['virtual_address'] + target['raw_size']:
            raise ValueError('target prefix is not wholly writable virtual loader-zero backing')
        rows.append(dict(symbol=symbol['name'], rva=hex(rva), size=size,
                         section=section['name'], compiled_offset=start,
                         loaded_zero_equal=True, initial_sha256=hashlib.sha256(bytes(size)).hexdigest()))
    for number, section in enumerate(coff.sections, 1):
        if section['name'] == '.drectve' or section['name'].startswith('.debug') or not section['size']:
            continue
        if number not in used_sections:
            raise ValueError('unexpected unreviewed source contribution')
        credited = sum(hi - lo for sec, lo, hi in intervals if sec == number)
        if credited != section['size']:
            raise ValueError('unreviewed storage or padding in provider')
    if pins != {str(p): sha(p) for p in watched}:
        raise ValueError('a watched input changed during verification')
    report = dict(scope='Fresh data-only source/original prefix comparison; no function, module layout, original ownership, initial runtime state or game acceptance.',
                  source=str(source), source_sha256=sha(source), header_sha256=sha(header),
                  object=str(obj), object_sha256=sha(obj), command=command,
                  oracle_sha256=sha(oracle), fields=rows, total_prefix_bytes=80,
                  exact_public_definition_set=True, no_code_or_relocations=True,
                  toolchain=toolchain, sdk_setjmp_sha256=sha(sdk), inputs=pins,
                  limit='64 bytes express the pinned SDK jmp_buf contract, not independently observed original allocation capacity. Current dispatcher uses extra-count0; optional CRT unwind slots and real SEH behavior are not exercised here.')
    destination = out / 'host_engine_storage_data.json'
    destination.write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(json.dumps(dict(fields=5, prefix_bytes=80, loaded_zero_equal=True,
                          no_code_or_relocations=True, report=str(destination)), indent=2))


if __name__ == '__main__':
    main()
