"""Bounded 33e10 composition: real reset/attachment helpers, modeled WORD interpreter.

The model tests caller ordering/reloads only. No original OS module starts.
"""
import argparse
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess

from env import ROOT
from coff import COFF
from match import DEFAULT_TOOLCHAIN, compile_inputs, compile_source, compare, digest
from masked import masked_compare
from pe import PE
from title_link import read_map
import attachment_entries_differential as entries

base=entries.base
SOURCE=ROOT/'src/eng1/behavioral/entry_level_initializer.c'
WORDS=ROOT/'src/eng1/behavioral/frame_data_config_word_pool_55478.c'
FRAME=ROOT/'src/shared/frame_state.c'
Uc=base.Uc


def build(output):
    base.verify_toolchain(DEFAULT_TOOLCHAIN)
    library=Path(base.unicorn.__file__).parent/'lib/unicorn.dll'
    assert base.unicorn.__version__=='2.1.4' and digest(library.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(base.ORACLE.read_bytes())=='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
    pins={str(p):digest(p.read_bytes()) for p in [Path(__file__).resolve(),Path(entries.__file__),Path(base.__file__),base.ORACLE,library,ROOT/'toolchains/manifest.json']};objects=[];commands=[];data={}
    for source in [SOURCE,FRAME,base.HELPER,entries.OWNER,WORDS]:
        pins.update({str(p):h for p,h in compile_inputs(source,['/O2'],DEFAULT_TOOLCHAIN).items()});obj,command=compile_source(source,['/O2']);commands.append(command);coff=COFF(obj)
        if source==SOURCE:assert masked_compare(obj,'_eng1_33e10',base.ORACLE,0x33e10,112)['masked_equal']
        if source==FRAME:assert compare(obj,'_reset_frame_state',base.ORACLE,0x29260,16)['exact']
        if source==base.HELPER:assert compare(obj,'_attach_entries',base.ORACLE,0x26700,48)['exact']
        if source in [entries.OWNER,WORDS]:
            name='_g_level_entries_5e688' if source==entries.OWNER else '_g_eng1_config_word_pool_55478';size=272 if source==entries.OWNER else 192
            symbol,=[s for s in coff.symbols.values() if s['name']==name];section=coff.sections[symbol['section']-1];payload=section['data'][symbol['value']:symbol['value']+size];assert len(payload)==size and not section['relocations'];native=PE(base.ORACLE)
            if source==entries.OWNER:assert entries.locate(native,payload,size)==0x5e688
            else:
                assert native.data.count(payload)==1 and native.pe.get_rva_from_offset(native.data.find(payload))==0x55478
            data[name]=dict(size=size,payload_sha256=digest(payload))
        objects.append(str(obj))
    model=output/'word_model.c';model.write_text('/* Intercepted fixture endpoint only; never production or executed. */\nvoid eng1_interpret_word_stream(void *p){for(;;){}}\n');pins[str(model)]=digest(model.read_bytes());obj,command=compile_source(model,['/O2']);objects.append(str(obj));commands.append(command)
    image,mapfile=output/'fixture.dll',output/'fixture.map';command=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile),'/EXPORT:eng1_33e10',*objects];commands.append(command)
    env=os.environ.copy()
    for name in ['CL','_CL_','LINK','_LINK_']:env.pop(name,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','');linked=subprocess.run(command,cwd=output,env=env,capture_output=True,text=True,timeout=60);(output/'link.log').write_text(linked.stdout+linked.stderr);assert linked.returncode==0,linked.stdout+linked.stderr
    return dict(input_sha256=pins,commands=commands,data=data,image_sha256=digest(image.read_bytes()),real_helper_raw_exact=True),PE(base.ORACLE),PE(image),read_map(mapfile)


def execute(pe,symbols,case,original):
    u=Uc(base.UC_ARCH_X86,base.UC_MODE_32);extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff;u.mem_map(pe.image_base,extent);u.mem_write(pe.image_base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'));u.mem_map(base.ACTOR-0x1000,0x3000);u.mem_map(base.STACK,0x10000)
    entry,reset,word,attach,table,program=[pe.image_base+r for r in [0x33e10,0x29260,0x28820,0x26700,0x5e688,0x554f0]] if original else [symbols[n]+add for n,add in [('_eng1_33e10',0),('_reset_frame_state',0),('_eng1_interpret_word_stream',0),('_attach_entries',0),('_g_level_entries_5e688',0),('_g_eng1_config_word_pool_55478',120)]]
    raw=bytearray((n*37+case['seed'])&255 for n in range(0x3000));struct.pack_into('<II',raw,0x1050,case['flags'],case['render']);u.mem_write(base.ACTOR-0x1000,bytes(raw));u.mem_write(table+case['sentinel']*16+14,b'\xff\xff');frame=base.pack(base.STOP)+base.pack(base.ACTOR)+b'\x93'*24;u.mem_write(base.SP,frame);u.reg_write(base.UC_X86_REG_ESP,base.SP)
    for reg,value in zip(base.NV,base.SEEDS):u.reg_write(reg,value)
    calls=[];writes=[];reads=[];model_events=[]
    r32=lambda at:struct.unpack('<I',u.mem_read(at,4))[0]
    def code(uc,address,size,user):
        if address==base.STOP:uc.emu_stop();return
        if address in [reset,word,attach]:
            esp=uc.reg_read(base.UC_X86_REG_ESP);assert r32(esp+4)==base.ACTOR
            calls.append('reset' if address==reset else 'word_model' if address==word else 'attach')
            if address==attach:assert r32(esp+8)==table
        if address==word:
            assert r32(base.ACTOR+0x64)==program and bytes(uc.mem_read(base.ACTOR+0x30,4))==b'\0'*4
            assert uc.mem_read(base.ACTOR+0x22,2)==b'\x02\x04'
            model_events.append(['word_input',r32(base.ACTOR+0x50),r32(base.ACTOR+0x54)])
            if case['mutate']:
                uc.mem_write(base.ACTOR+0x36,b'\x77\x77');uc.mem_write(base.ACTOR+0x98,b'\xef\xbe');uc.mem_write(base.ACTOR+0x4c,base.pack(99));uc.mem_write(base.ACTOR+0x50,base.pack(r32(base.ACTOR+0x50)^0x10000));uc.mem_write(base.ACTOR+0x64,base.pack(program+2));uc.mem_write(base.ACTOR+0x22,b'\x09');model_events.append(['mutated_count_parameter_flags_image_category'])
            esp=uc.reg_read(base.UC_X86_REG_ESP);destination=r32(esp)
            from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX
            for reg,value in [(base.UC_X86_REG_EAX,0xface0000),(UC_X86_REG_ECX,0xabad1dea),(UC_X86_REG_EDX,0xdeadc0de)]:uc.reg_write(reg,value)
            uc.reg_write(base.UC_X86_REG_ESP,esp+4);uc.reg_write(base.UC_X86_REG_EIP,destination);return
        assert any(at<=address and address+size<=at+length for at,length in [(entry,112),(reset,16),(attach,48)]),hex(address)
    def read(uc,access,address,size,value,user):
        if base.STACK<=address and address+size<=base.STACK+0x10000 or base.ACTOR<=address and address+size<=base.ACTOR+0x134:return
        assert table<=address and address+size<=table+272 and size==2 and (address-table)%16==14
        reads.append([(address-table)//16,int.from_bytes(uc.mem_read(address,size),'little')])
    def write(uc,access,address,size,value,user):
        if base.STACK<=address and address+size<=base.SP:return
        assert base.ACTOR<=address and address+size<=base.ACTOR+0x134
        if address==base.ACTOR+0x64:assert size==4 and value==program;value='word_program'
        if address==base.ACTOR+0x5c:assert size==4 and value==table;value='entry_table'
        writes.append([address-base.ACTOR,size,value])
    u.hook_add(base.UC_HOOK_CODE,code);u.hook_add(base.UC_HOOK_MEM_READ,read);u.hook_add(base.UC_HOOK_MEM_WRITE,write);u.emu_start(entry,0,count=2000)
    assert u.reg_read(base.UC_X86_REG_EIP)==base.STOP and u.reg_read(base.UC_X86_REG_ESP)==base.SP+4 and bytes(u.mem_read(base.SP,len(frame)))==frame and [u.reg_read(r) for r in base.NV]==base.SEEDS
    assert calls==['reset','word_model','attach'] and [row for row,_ in reads]==list(range(case['sentinel']+1)) and reads[-1][1]==65535
    assert int.from_bytes(u.mem_read(base.ACTOR+0x98,2),'little')==case['sentinel'] and r32(base.ACTOR+0x4c)==6
    assert r32(base.ACTOR+0x5c)==table and r32(base.ACTOR+0x64)==program+(2 if case['mutate'] else 0)
    memory=bytearray(u.mem_read(base.ACTOR-0x1000,0x3000));assert memory[:0x1000]==raw[:0x1000] and memory[0x1134:]==raw[0x1134:];memory[0x105c:0x1060]=b'\0'*4;memory[0x1064:0x1068]=base.pack(2 if case['mutate'] else 0)
    return dict(memory_sha256=digest(memory),ordered_guest_writes=writes,tag_reads=reads,calls=calls,model_events=model_events)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/entry_level_initializer_differential');args=parser.parse_args();out=args.output.resolve()
    if not out.is_relative_to(ROOT/'work'):parser.error('Fixture output belongs under ignored work/')
    out.mkdir(parents=True,exist_ok=True);built,native,source,symbols=build(out);checks=[]
    for flags,render,seed,sentinel,mutate in itertools.product([0,0xffffffff,0x20800,0x12345678],[0,0xffffffff,0x20000000,0x87654321],[0,79,255],[0,1,8,15,16],[False,True]):
        case=dict(flags=flags,render=render,seed=seed,sentinel=sentinel,mutate=mutate);a=execute(native,symbols,case,True);b=execute(source,symbols,case,False);checks.append(dict(case=case,equal=a==b,native=a,source=b))
    for path,sha in built['input_sha256'].items():assert digest(Path(path).read_bytes())==sha
    raw=json.dumps(checks,sort_keys=True).encode()+b'\n';(out/'cases.json').write_bytes(raw);report=dict(scope=__doc__,cases=len(checks),all_equal=all(c['equal'] for c in checks),failed_cases=sum(not c['equal'] for c in checks),cases_sha256=digest(raw),build=built,limits=['WORD interpreter is an explicit model; its own commands/side effects are not proved by these cases.','Real RAW reset/attachment helpers execute; raw pointers are checked before role normalization.','No later rendering, complete gameplay or original OS startup claim.']);(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['cases','all_equal','failed_cases']}))
    if not report['all_equal']:raise SystemExit(1)


if __name__=='__main__':main()
