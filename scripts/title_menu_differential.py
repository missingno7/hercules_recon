"""Compare native and source-built TITLE menu input/dispatch in an isolated process."""
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))

from coff import COFF
from match import DEFAULT_TOOLCHAIN, verify_toolchain
from pe import PE
from title_integration import ROOT, ORIGINAL_SHA256, load_report, parse_title_link_map, sha256


LANE = ROOT / 'work/title_behavioral/differential/menu'
FIXTURE = ROOT / 'tests/title_menu_differential.c'
ORIGINAL = ROOT / 'assets/pc/HERCULES/TITLE.DLL'

# Native data identities, not placement constraints for the reconstructed DLL.
BINDINGS = (
    ('G_INTERFACE', 0x2bb40, '_g_engine_interface'),
    ('G_BUFFER_CURRENT', 0x2cc04, '_g_2cc04'),
    ('G_BUFFER_PAIR', 0x2cc08, '_g_2cc08'),
    ('G_OBJECT_A', 0x2a004, '_g_2a004'),
    ('G_OBJECT_B', 0x29ff4, '_g_29ff4'),
    ('G_BUSY', 0x29f98, '_g_29f98'),
    ('G_PHASE', 0x2a07c, '_g_2a07c'),
    ('G_BACKGROUND', 0x2a080, '_g_2a080'),
    ('G_MIX', 0x2a018, '_g_2a018'),
    ('G_X', 0x2a044, '_g_2a044'),
    ('G_Y', 0x2a03c, '_g_2a03c'),
    ('G_OVERLAY', 0x2a020, '_g_2a020'),
    ('G_IDLE', 0x2a038, '_g_2a038'),
    ('G_SUPPRESS', 0x2a068, '_g_2a068'),
    ('G_REPEAT', 0x2a040, '_g_2a040'),
    ('G_SELECTION', 0x2a030, '_g_2a030'),
    ('G_LIMIT', 0x2a074, '_g_2a074'),
    ('G_ACTION', 0x29ff0, '_g_29ff0'),
    ('G_EDGE', 0x29fec, '_g_29fec'),
    ('G_HELD', 0x2a028, '_g_2a028'),
    ('G_DEMO', 0x2bb30, '_g_2bb30'),
    ('G_SOUND_ID', 0x29da8, '_g_29da8'),
    ('G_SOUND_BANK', 0x29db0, '_g_29db0'),
    ('G_SOUND_SAMPLE', 0x29db4, '_g_29db4'),
    ('D_PAGE_A', 0x29178, '_s_slot_tpage2s'),
    ('D_PAGE_B', 0x29288, '_s_slot_tpage1'),
    ('D_SPRITE_A', 0x293d8, '_s_slot_sprt2_buf'),
    ('D_SPRITE_B', 0x29480, '_s_pair_sprt1'),
)


def private_binding(name, symbols, objects, image):
    if name in symbols:
        return symbols[name] - image.image_base
    candidates = set()
    for obj in objects.values():
        for symbol in obj.symbols.values():
            if not symbol['type'] & 0x20 or symbol['section'] <= 0 or symbol['name'] not in symbols:
                continue
            section = obj.sections[symbol['section'] - 1]
            for relocation in section['relocations']:
                if relocation['type'] != 6 or relocation['symbol'].split('$')[0] != name:
                    continue
                addend = struct.unpack_from('<I', section['data'], relocation['offset'])[0]
                operand_rva = (symbols[symbol['name']] - image.image_base +
                               relocation['offset'] - symbol['value'])
                operand = struct.unpack('<I', image.read_rva(operand_rva, 4))[0]
                candidates.add(operand - addend - image.image_base)
    if len(candidates) != 1:
        raise ValueError('Missing or ambiguous named COFF data binding: ' + name)
    return candidates.pop()


def require_section(image, rva, size, flags):
    for section in image.sections:
        start = section['virtual_address']
        if start <= rva and rva + size <= start + section['virtual_size'] and \
                section['characteristics'] & flags == flags:
            return
    raise ValueError('Unexpected PE section for fixture binding: ' + hex(rva))


def run():
    LANE.mkdir(parents=True, exist_ok=True)
    report, linked, linked_hash = load_report()
    preferred_base, symbols = parse_title_link_map(report)
    if sha256(ORIGINAL) != ORIGINAL_SHA256['TITLE.DLL'] or sha256(ORIGINAL) != report['original']:
        raise ValueError('Native TITLE oracle identity changed')
    image, native = PE(linked), PE(ORIGINAL)
    if image.image_base != preferred_base:
        raise ValueError('Source image/map base differs')
    objects, object_inputs = {}, {}
    for row in report['compiled_sources']:
        path = Path(next(arg[3:] for arg in row['command'] if arg.startswith('/Fo'))).resolve()
        if not path.is_relative_to(ROOT / 'build') and not path.is_relative_to(ROOT / 'work'):
            raise ValueError('Compiler object escaped ignored workspace outputs')
        if sha256(path) != row['object_sha256']:
            raise ValueError('Existing compiler object changed: ' + row['source'])
        objects[row['source']] = COFF(path)
        object_inputs[path.relative_to(ROOT).as_posix()] = row['object_sha256']

    bindings = []
    for name, native_rva, symbol in BINDINGS:
        linked_rva = private_binding(symbol, symbols, objects, image)
        size = 1000 if name == 'G_INTERFACE' else 4
        if name.startswith('D_'):
            size = 96 if 'PAGE' in name else 160
        require_section(native, native_rva, size, 0x80000000)
        require_section(image, linked_rva, size, 0x80000000)
        bindings.append(dict(name=name, symbol=symbol, native_rva=native_rva, linked_rva=linked_rva))
    menu_rva = symbols['_title_07510'] - preferred_base
    require_section(native, 0x7510, 1, 0x20000000)
    require_section(image, menu_rva, 1, 0x20000000)
    header = '\n'.join('#define %s %d' % (row['name'], index)
                       for index, row in enumerate(bindings)) + '\n'
    header += ('#define SCALAR_FIRST G_BUSY\n#define SCALAR_COUNT 19\n'
               '#define DRAW_FIRST D_PAGE_A\n#define DRAW_COUNT 4\n#define DRAW_BYTES 512\n'
               '#define NATIVE_MENU_RVA 0x7510UL\n#define LINKED_MENU_RVA 0x%xUL\n' % menu_rva)
    for kind in ('native', 'linked'):
        header += ('static const DWORD %s_rvas[]={%s};\n' %
                   (kind, ','.join(hex(row[kind + '_rva']) for row in bindings)))
    header += 'static const DWORD draw_sizes[]={96,96,160,160};\n'
    generated = LANE / 'title_menu_bindings.h'
    generated.write_text(header, encoding='utf-8')
    prediction = dict(
        prediction='Equivalent state-2 input preserves menu state and ordered callbacks in native and linked TITLE.',
        falsifier='First differing compared byte, unexpected callback, or incorrect EXIT callback count.',
        limits='Busy flag skips animations; sound mapping rejects playback; exit spy returns; drawing callbacks record only.')
    (LANE / 'prediction.json').write_text(json.dumps(prediction, indent=2) + '\n')
    staged = LANE / 'TITLE_MENU_RECON.DLL'
    shutil.copyfile(linked, staged)
    if sha256(staged) != linked_hash:
        raise ValueError('Staged source image changed')

    toolchain = verify_toolchain(DEFAULT_TOOLCHAIN)
    environment = os.environ.copy()
    for key in ('CL', '_CL_', 'LINK', '_LINK_', 'HERCULES_TITLE_TRACE'):
        environment.pop(key, None)
    environment.update(PATH=str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + environment.get('PATH', ''),
                       INCLUDE=str(DEFAULT_TOOLCHAIN / 'include'), LIB=str(DEFAULT_TOOLCHAIN / 'lib'))
    executable, obj = LANE / 'menu_fixture.exe', LANE / 'menu_fixture.obj'
    if executable.exists():
        executable.unlink()
    commands = [
        [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/c', '/O2', '/Gy',
         '/I' + str(LANE), '/Fo' + str(obj), str(FIXTURE)],
        [str(DEFAULT_TOOLCHAIN / 'bin/link.exe'), '/nologo', '/INCREMENTAL:NO',
         '/out:' + str(executable), '/subsystem:console', str(obj), 'libc.lib', 'kernel32.lib'],
        [str(executable), str(ORIGINAL), str(staged)],
    ]
    executed = []
    for ordinal, command in enumerate(commands, 1):
        result = subprocess.run(command, cwd=LANE, env=environment, capture_output=True,
                                text=True, timeout=120)
        executed.append(command)
        (LANE / ('command_%d.log' % ordinal)).write_text(result.stdout + result.stderr)
        if result.returncode:
            break
    record = dict(scope='Isolated native/linked menu differential; no matching promotion or running-game memory edits.',
                  toolchain=toolchain, commands=executed, bindings=bindings, prediction=prediction,
                  inputs={path.relative_to(ROOT).as_posix(): sha256(path)
                          for path in (Path(__file__), FIXTURE, generated, ORIGINAL, linked, staged)},
                  existing_object_inputs=object_inputs,
                  fixture_sha256=sha256(executable) if executable.exists() else None,
                  exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr)
    (LANE / 'report.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps({key: record[key] for key in ('exit_code', 'stdout', 'stderr')}, indent=2))
    return result.returncode


if __name__ == '__main__':
    try:
        raise SystemExit(run())
    except (OSError, ValueError, KeyError, StopIteration, subprocess.SubprocessError) as error:
        print('menu differential error:', error, file=sys.stderr)
        raise SystemExit(2)
