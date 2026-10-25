"""Audit four complete initialized Entry owners and compose three real callers.

Uses the frozen boss-Entry instruction harness, with additional per-owner read
bounds. No original OS module or game starts; no instruction/file is patched.
"""
import argparse
import itertools
import json
import os
from pathlib import Path
import re
import struct
import subprocess

from env import ROOT
from coff import COFF
from match import DEFAULT_TOOLCHAIN, compile_inputs, compile_source, compare, digest, verify_toolchain
from masked import masked_compare
from pe import PE
from refinery_forms import replace_function
from title_link import read_map
import boss_entries_differential as base

SOURCE = ROOT/'calibration/eng1/player_scene_3a040.c'
OWNER = ROOT/'src/eng1/behavioral/attachment_entry_data.c'
TABLES = {'_g_entries_64510':(0x64510,464), '_g_entries_646e0':(0x646e0,240),
          '_g_entries_647d0':(0x647d0,144), '_g_level_entries_5e688':(0x5e688,272)}
CALLERS = {'_eng1_3a730':(0x3a730,'_g_entries_64510'),
           '_eng1_3a8a0':(0x3a8a0,'_g_entries_646e0'),
           '_eng1_3a910':(0x3a910,'_g_entries_647d0')}


def locate(pe, payload, size):
    if len(payload) != size or size % 16 or payload[-16:] != b'\xff'*16:
        raise ValueError('Complete owner, record stride and full terminal record required')
    if any(struct.unpack_from('<H',payload,n+14)[0]==0xffff for n in range(0,size-16,16)):
        raise ValueError('Early sentinel changes owner extent')
    hits, start = [], 0
    while True:
        at = pe.data.find(payload,start)
        if at < 0:
            break
        hits.append(pe.pe.get_rva_from_offset(at))
        start = at+1
    if len(hits) != 1:
        raise ValueError('Complete source payload needs one native occurrence')
    return hits[0]


def build(output):
    verify_toolchain(DEFAULT_TOOLCHAIN)
    library=Path(base.unicorn.__file__).parent/'lib/unicorn.dll'
    assert base.unicorn.__version__=='2.1.4' and digest(library.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(base.ORACLE.read_bytes())=='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
    pins={str(p):digest(p.read_bytes()) for p in [Path(__file__).resolve(),Path(base.__file__),base.ORACLE,library,ROOT/'toolchains/manifest.json']}
    objects, commands, payloads = [], [], {}
    for source in [SOURCE,base.HELPER,OWNER]:
        pins.update({str(p):h for p,h in compile_inputs(source,['/O2'],DEFAULT_TOOLCHAIN).items()})
        obj,command=compile_source(source,['/O2']);commands.append(command);coff=COFF(obj)
        if source==SOURCE:
            text=source.read_text()
            for symbol in coff.symbols.values():
                if symbol['section']>0 and symbol['type']&0x20 and symbol['name'] not in CALLERS:
                    text=replace_function(text,symbol['name'][1:],'')
            text=re.sub(r'#include "([^"]+)"',lambda m:'#include "'+source.parent.joinpath(m[1]).as_posix()+'"',text)
            fixture=output/'callers.c';fixture.write_text(text);pins[str(fixture)]=digest(fixture.read_bytes())
            obj,command=compile_source(fixture,['/O2']);commands.append(command);private=COFF(obj)
            for name,(rva,_) in CALLERS.items():
                a,b=coff.function(name),private.function(name)
                assert a['data']==b['data'] and a['relocations']==b['relocations']
                assert masked_compare(obj,name,base.ORACLE,rva,64)['masked_equal']
        elif source==base.HELPER:
            assert compare(obj,'_attach_entries',base.ORACLE,0x26700,48)['exact']
        else:
            definitions={s['name']:s for s in coff.symbols.values() if s['storage']==2 and s['section']>0}
            assert set(definitions)==set(TABLES) and all(not s['type']&0x20 for s in definitions.values())
            spans=[]
            for name,(_,size) in TABLES.items():
                symbol=definitions[name];section=coff.sections[symbol['section']-1];at=symbol['value']
                assert not section['relocations'] and at+size<=len(section['data'])
                spans.append((symbol['section'],at,at+size));payloads[name]=section['data'][at:at+size]
            assert len({s[0] for s in spans})==1
            intervals=sorted((a,b) for _,a,b in spans)
            assert intervals[0][0]==0 and all(a[1]==b[0] for a,b in zip(intervals,intervals[1:]))
            assert intervals[-1][1]==len(coff.sections[spans[0][0]-1]['data'])==1120
        objects.append(str(obj))
    native=PE(base.ORACLE);data={}
    for name,(rva,size) in TABLES.items():
        payload=payloads[name];assert locate(native,payload,size)==rva;rejections=[]
        for label,bad in [('changed_value',bytes([payload[0]^1])+payload[1:]),('truncated',payload[:-16])]:
            try:locate(native,bad,size)
            except ValueError:rejections.append(label)
            else:raise AssertionError(label)
        data[name]=dict(native_rva=hex(rva),size=size,rows=size//16,usable_rows=size//16-1,payload_sha256=digest(payload),unique_native_occurrence=True,negative_controls=rejections)
    image,mapfile=output/'fixture.dll',output/'fixture.map'
    command=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile),*['/EXPORT:'+n[1:] for n in CALLERS],*objects];commands.append(command)
    env=os.environ.copy()
    for name in ['CL','_CL_','LINK','_LINK_']:env.pop(name,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    linked=subprocess.run(command,cwd=output,env=env,capture_output=True,text=True,timeout=60);(output/'link.log').write_text(linked.stdout+linked.stderr);assert linked.returncode==0,linked.stdout+linked.stderr
    return dict(input_sha256=pins,commands=commands,data=data,complete_selected_contributions_equal=True,helper_raw_exact=True,image_sha256=digest(image.read_bytes())),native,PE(image),read_map(mapfile)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/attachment_entries_differential');args=parser.parse_args();output=args.output.resolve()
    if not output.is_relative_to(ROOT/'work'):parser.error('Fixture output belongs under ignored work/')
    output.mkdir(parents=True,exist_ok=True);built,native,source,symbols=build(output);checks=[]
    for name,(rva,table_name) in CALLERS.items():
        table,size=TABLES[table_name];last=size//16-1
        for flags,render,seed,sentinel in itertools.product([0,0xffffffff,0x800,0x12345678],[0,0xffffffff,0x20000600,0x87654321],[0,79,255],[0,1,last//2,last-1,last]):
            case=dict(flags=flags,render=render,seed=seed,sentinel=sentinel)
            a=base.execute(native,native.image_base+rva,native.image_base+0x26700,native.image_base+table,case)
            b=base.execute(source,symbols[name],symbols['_attach_entries'],symbols[table_name],case)
            # The shared harness has a larger generic table-read envelope.
            # Require every actual read to stay within this complete owner and
            # visit exactly its prefix through the selected first sentinel.
            for result in [a,b]:
                assert [row for row,_ in result['table_tag_reads']]==list(range(sentinel+1))
                assert all(0<=row<size//16 for row,_ in result['table_tag_reads'])
                assert all(tag!=0xffff for _,tag in result['table_tag_reads'][:-1])
                assert result['table_tag_reads'][-1][1]==0xffff
            checks.append(dict(symbol=name,case=case,equal=a==b,native=a,source=b))
    for path,sha in built['input_sha256'].items():assert digest(Path(path).read_bytes())==sha
    traces=json.dumps(checks,sort_keys=True).encode()+b'\n';(output/'cases.json').write_bytes(traces)
    report=dict(scope=__doc__,cases=len(checks),all_equal=all(c['equal'] for c in checks),failed_cases=sum(not c['equal'] for c in checks),by_function={name:sum(c['symbol']==name for c in checks) for name in CALLERS},cases_sha256=digest(traces),build=built,limits=['All four source payloads are audited. Only the three simple callers execute in these720 composition cases;33e10 remains outside the claim.','Real attach_entries executes with no modeled services. Pointer normalization follows separate raw-pointer checks.','Finite actor/sentinel cases establish no later coordinate/rendering/gameplay semantics.'])
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['cases','all_equal','failed_cases','by_function']}))
    if not report['all_equal']:raise SystemExit(1)


if __name__=='__main__':main()
