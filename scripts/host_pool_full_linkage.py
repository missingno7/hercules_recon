"""Natural full pool dependency link; no original execution or acceptance writes."""
import json
import os
import re
import subprocess

from env import ROOT
from match import compile_source, DEFAULT_TOOLCHAIN, digest, verify_toolchain
from coff import COFF
from pe import PE
from region import contribution_identity


def main():
    lane = ROOT / 'work/host_pool_full_replay'
    lane.mkdir(parents=True, exist_ok=True)
    core = json.loads((ROOT / 'evidence/host_pool_region.json').read_text())
    target = PE(ROOT / core['target'])
    if digest(target.data) != core['target_sha256']:
        raise ValueError('Changed host oracle')
    units = [
        ('host_pool', 'calibration/host_pool_repaired.c', 'evidence/regions/pool_predicates.json'),
        ('host_pool_counts', 'calibration/host_pool_counts.c', 'evidence/regions/host_pool_counts_common.json'),
        ('host_pool_lifecycle', 'calibration/host_pool_lifecycle_repaired.c', 'evidence/regions/bulk_owner_guard.json'),
        ('host_arena_reset', 'calibration/host_arena_reset.c', 'evidence/regions/host_arena_reset_first.json'),
        ('host_pool_shrink', 'calibration/host_pool_shrink.c', 'evidence/regions/host_pool_shrink_common.json'),
        ('pool_reverse_copy', 'calibration/pool_reverse_copy.c', 'evidence/regions/pool_reverse_copy_common.json'),
    ]
    specifications = [json.loads((ROOT / f'evidence/{name}_region.json').read_text()) for name, _, _ in units]
    strings = {}
    for spec in specifications:
        for name, value in spec.get('symbol_rvas', {}).items():
            if not name.startswith('_g_fmt_'):
                continue
            if name in strings and strings[name]['target_rva'] != value:
                raise ValueError('Inconsistent format reference')
            data = bytearray()
            rva = int(value, 0)
            for offset in range(256):
                char = target.read_rva(rva+offset,1)
                if char == b'\0':
                    break
                data += char
            else:
                raise ValueError('Unterminated observed string')
            strings[name] = dict(target_rva=value,text=data.decode('ascii'),sha256=digest(bytes(data)+b'\0'),bytes=len(data)+1)
    prefix = (ROOT / 'calibration/host_pool_repaired.c').read_text().split('void __cdecl save_arena')[0]
    data_source = lane / 'pool_data.c'
    data_text = prefix + '\nHostPoolState g_host_pool;\n'
    for name, row in strings.items():
        data_text += f'const char {name[1:]}[] = {json.dumps(row["text"])};\n'
    data_source.write_text(data_text,encoding='utf-8',newline='\n')
    objects = []
    source_rows = []
    functions = []
    for (name, source_path, receipt_path), spec in zip(units, specifications):
        source = ROOT / source_path
        obj, command = compile_source(source, ['/O2'])
        objects.append(obj)
        coff = COFF(obj)
        receipt = json.loads((ROOT / receipt_path).read_text())
        frozen = {row['symbol']:row['output_id'] for row in receipt['functions']}
        for row in spec['functions']:
            contribution = coff.function(row['symbol'])
            identity = contribution_identity(contribution)
            if identity != frozen[row['symbol']]:
                raise ValueError('Changed frozen contribution: '+row['symbol'])
            functions.append(dict(symbol=row['symbol'],rva=row['rva'],target_size=row['size'],candidate_size=contribution['size'],identity=identity))
        source_rows.append(dict(path=source_path,sha256=digest(source.read_bytes()),command=command,object=str(obj.relative_to(ROOT)),object_sha256=digest(obj.read_bytes())))
    diagnostic_source = ROOT / 'src/pc/host_diagnostic.c'
    diagnostic, command = compile_source(diagnostic_source, ['/O2'])
    diagnostic_code = COFF(diagnostic).function('_host_diagnostic')
    if diagnostic_code['data'] != target.read_rva(0x63c0,16) or diagnostic_code['relocations']:
        raise ValueError('Diagnostic raw match regressed')
    objects.append(diagnostic)
    source_rows.append(dict(path='src/pc/host_diagnostic.c',sha256=digest(diagnostic_source.read_bytes()),command=command))
    functions.append(dict(symbol='_host_diagnostic',rva='0x63c0',target_size=16,candidate_size=16,identity=contribution_identity(diagnostic_code)))
    data_obj, data_command = compile_source(data_source, ['/O2'])
    objects.append(data_obj)
    archive = DEFAULT_TOOLCHAIN / 'lib/libc.lib'
    if digest(archive.read_bytes()) != '4ae22c6f35a87c55c453ad388ada649ea3ba190614be3d1ab95935ba0faa7416':
        raise ValueError('Changed experimental CRT provider')
    command = [str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:NOREF','/OUT:'+str(lane/'pool.dll'),'/MAP:'+str(lane/'pool.map'),*('/EXPORT:'+row['symbol'][1:] for row in functions),*(str(obj) for obj in objects),str(archive)]
    environment = os.environ.copy()
    for key in ('CL','_CL_','LINK','_LINK_'):
        environment.pop(key,None)
    environment.update(PATH=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+environment.get('PATH',''),LIB=str(DEFAULT_TOOLCHAIN/'lib'))
    result = subprocess.run(command,cwd=lane,env=environment,capture_output=True,text=True,timeout=60)
    (lane/'link.log').write_text(result.stdout+result.stderr)
    if result.returncode:
        raise ValueError(result.stdout+result.stderr)
    linked = PE(lane/'pool.dll')
    exports = {entry.name.decode():entry.address for entry in linked.pe.DIRECTORY_ENTRY_EXPORT.symbols}
    map_text = (lane/'pool.map').read_text()
    symbols = {name:int(address,16)-linked.image_base for name,address in re.findall(r'\s+[0-9a-fA-F]+:[0-9a-fA-F]+\s+(\S+)\s+([0-9a-fA-F]{8})',map_text)}
    for row in functions:
        rva = exports[row['symbol'][1:]]
        data = linked.read_rva(rva,row['candidate_size'])
        expected = target.read_rva(int(row['rva'],0),row['target_size'])
        row.update(linked_rva=hex(rva),raw_equal=data==expected,raw_differences=sum(data[n:n+1]!=expected[n:n+1] for n in range(max(len(data),len(expected)))),linked_sha256=digest(data))
    for name,row in strings.items():
        rva = symbols[name]
        if digest(linked.read_rva(rva,row['bytes'])) != row['sha256']:
            raise ValueError('Changed linked string')
        row['linked_rva'] = hex(rva)
    state_rva = symbols['_g_host_pool']
    state = next(s for s in linked.sections if s['virtual_address']<=state_rva and state_rva+48<=s['virtual_address']+s['virtual_size'])
    if state_rva-state['virtual_address'] < state['raw_size']:
        raise ValueError('State is not virtual zero fill')
    provider = next(line.strip() for line in map_text.splitlines() if '_memmove' in line)
    if 'libc:memmove.obj' not in provider:
        raise ValueError('Unexpected copy provider')
    report = dict(scope='Fresh natural diagnostic link only; no placement flags, stubs, aliases, patched objects, original execution or FUNCTION_MATCH. Original TU, declarations/layout and CRT variant remain unproved.',toolchain=verify_toolchain(DEFAULT_TOOLCHAIN),sources=source_rows,data_source_sha256=digest(data_source.read_bytes()),data_command=data_command,link_command=command,linked_sha256=digest(linked.data),functions=functions,strings=strings,state=dict(bytes=48,virtual_zero_fill=True,linked_rva=hex(state_rva)),experimental_library=dict(path=str(archive),sha256=digest(archive.read_bytes()),provider=provider,original_variant_proven=False),builds=dict(source_compiles=8,natural_links=1),unresolved_externals=0)
    (lane/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(exports=len(functions),preserved_prelink_functions=12,strings=len(strings),state_bytes=48,unresolved_externals=0,raw_equal=sum(row['raw_equal'] for row in functions))))
    return report


if __name__ == '__main__':
    main()
