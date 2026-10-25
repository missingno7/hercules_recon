"""Native/source resize and actual coalescence/copy comparisons; no Windows game process."""
import argparse,hashlib,json,re,struct,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,DEFAULT_TOOLCHAIN,verify_toolchain
from masked import masked_compare
from coff import COFF
from pe import PE
from crt_copy_evidence import members
from title_behavioral import context_files
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
BASE=0x400000;POOL=BASE+0x7a29c0;DATA=0x61000000;OWNER=DATA+0x400;HEAP=0x64000040;STACK=0x63000000;ESP=STACK+0x8000;STOP=STACK+0xf000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]
PROFILE={'resize':('_shrink_relocate',0x4690,192),'coalesce':('_coalesce_pool',0x4830,96),'diagnostic':('_host_diagnostic',0x63c0,16),'copy':('_memmove',0x10fb0,821)}
SPECS=[('_g_host_pool',0x7a29c0,48),('_g_fmt_init_pool',0x3608c,32),('_g_fmt_alloc',0x360ac,30),('_g_fmt_alloc_failed',0x360cc,36),('_g_fmt_free',0x36110,29),('_g_fmt_remalloc',0x360f0,32)]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();pack=lambda v:struct.pack('<I',v&0xffffffff)
def u32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def cstring(u,a):
 b=bytearray()
 for i in range(128):
  v=u.mem_read(a+i,1)[0]
  if not v:return b.decode('ascii')
  b.append(v)
 raise AssertionError('unterminated diagnostic')

def verified_copy(oracle,out):
 """Identify an unchanged library member, checking every native relocation operand."""
 verify_toolchain(DEFAULT_TOOLCHAIN);out.mkdir(parents=True,exist_ok=True);assert sha(oracle)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
 archive=DEFAULT_TOOLCHAIN/'lib/libcmt.lib';assert sha(archive)=='7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b'
 raw=next(m for m in members(archive.read_bytes())if hashlib.sha256(m).hexdigest()=='0f29aab09e6fc7a2a24f13e6be7a9404c5226ab776a9f60b7d7bcc33a29b3078');lib=out/'crt_memmove.obj';lib.write_bytes(raw);co=COFF(lib);section=next(s for s in co.sections if s['flags']&0x20);index=co.sections.index(section)+1
 assert section['size']==821 and len(section['relocations'])==46;target=PE(oracle).read_rva(0x10fb0,821);covered=set()
 for rel in section['relocations']:
  s=next(s for s in co.symbols.values()if s['name']==rel['symbol']and s['section']==index);assert rel['type']==6;at=rel['offset'];addend=struct.unpack_from('<I',section['data'],at)[0];assert struct.unpack_from('<I',target,at)[0]==(BASE+0x10fb0+s['value']+addend)&0xffffffff;covered.update(range(at,at+4))
 assert all(a==b for i,(a,b)in enumerate(zip(section['data'],target))if i not in covered)
 return dict(archive=str(archive),archive_sha256=sha(archive),member_sha256=sha(lib),object=str(lib),complete_bytes=821,local_operands_verified=46,nonrelocation_bytes_equal=True,variant_limit='LIBC/LIBCMT and memcpy/memmove code are identical here;original public name/runtime variant is unproved.')

def build(source,fmt,out):
 library=verified_copy(ORACLE,out);archive=Path(library['archive']);lib=Path(library['object']);co=COFF(lib)
 files=[source,ROOT/'src/pc/host_pool_repaired.c',ROOT/'src/pc/host_diagnostic.c',ROOT/'src/pc/host_pool_storage.c',ROOT/'src/pc/host_pool_formats.c',fmt]
 runtime=Path('C:/tools/hercules/renderer-oracle-python/unicorn/lib/unicorn.dll');assert sha(runtime)=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
 watched={Path(__file__),ORACLE,archive,runtime,ROOT/'recovery.json',ROOT/'toolchains/manifest.json',ROOT/'scripts/crt_copy_evidence.py'}
 for p in files:watched.update(context_files(p))
 for name in ['match.py','coff.py','pe.py','masked.py','env.py','title_behavioral.py']:watched.add(ROOT/'scripts'/name)
 pins={str(p):sha(p)for p in watched};compiles=[];coffs=[]
 for p in files:
  obj,cmd=compile_source(p,['/O2']);coffs.append(COFF(obj));compiles.append(dict(source=str(p),source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=cmd))
 coffs.append(co);objects=[x['object']for x in compiles]+[str(lib)];measurements=[]
 for role,(name,rv,n)in PROFILE.items():
  if role=='copy':
   measurements.append(dict(role=role,symbol=name,rva=hex(rv),native_size=n,compiled_size=821,library_identity=True));continue
  i=next(i for i,c in enumerate(coffs)if any(s['name']==name and s['section']>0 for s in c.symbols.values()));rr=compare(Path(objects[i]),name,ORACLE,rv,n)
  if role=='diagnostic':assert rr['exact']
  measurements.append(dict(role=role,symbol=name,rva=hex(rv),native_size=n,compiled_size=coffs[i].function(name)['size'],raw=rr,diagnostic=masked_compare(Path(objects[i]),name,ORACLE,rv,n)))
 data=[];native=PE(ORACLE)
 for name,rv,n in SPECS:
  candidates=[(c,s)for c in coffs for s in c.symbols.values()if s['name']==name and(s['section']>0 or s['section']==0 and s['value']>0)];assert len(candidates)==1,name;c,s=candidates[0]
  if s['section']==0:assert s['value']==n;raw=bytes(n)
  else:sec=c.sections[s['section']-1];assert s['value']+n<=sec['size']and not sec['relocations'];raw=sec['data'][s['value']:s['value']+n]if sec['data']else bytes(n)
  ns=next(s for s in native.pe.sections if s.VirtualAddress<=rv and rv+n<=s.VirtualAddress+s.Misc_VirtualSize);virtual=rv>=ns.VirtualAddress+ns.SizeOfRawData;assert raw==(bytes(n)if virtual else native.read_rva(rv,n))
  data.append(dict(symbol=name,rva=hex(rv),bytes=n,data=raw.hex(),sha256=hashlib.sha256(raw).hexdigest()))
 assert pins=={str(p):sha(p)for p in watched}
 return dict(inputs=pins,compiles=compiles,objects=objects,measurements=measurements,data=data,library=library)

def prepare(b):
 symbols={};segments=[];where={};coffs=[COFF(p)for p in b['objects']];cursor=0x20000000
 for x in b['data']:symbols[x['symbol']]=BASE+int(x['rva'],0);segments.append((symbols[x['symbol']],bytes.fromhex(x['data'])))
 for k,c in enumerate(coffs):
  for i,s in enumerate(c.sections,1):
   if s['flags']&0x20 and s['size']:where[k,i]=cursor;cursor+=(s['size']+15)&~15
  for s in c.symbols.values():
   if s['storage']==2 and s['section']>0 and(k,s['section'])in where:assert s['name']not in symbols;symbols[s['name']]=where[k,s['section']]+s['value']
 for k,c in enumerate(coffs):
  for i,s in enumerate(c.sections,1):
   if(k,i)not in where:continue
   raw=bytearray(s['data']);assert len(raw)==s['size']
   for rel in s['relocations']:
    local=next((v for v in c.symbols.values()if v['name']==rel['symbol']and v['section']>0),None);target=where[k,local['section']]+local['value']if local else symbols[rel['symbol']];at=rel['offset'];assert rel['type']in [6,20];value=struct.unpack_from('<I',raw,at)[0]+target-(where[k,i]+at+4 if rel['type']==20 else 0);struct.pack_into('<I',raw,at,value&0xffffffff)
   segments.append((where[k,i],bytes(raw)))
 addresses={role:symbols[name]for role,(name,rv,n)in PROFILE.items()};allowed=[(addresses[x['role']],addresses[x['role']]+x['compiled_size'])for x in b['measurements']]
 return dict(segments=segments,addresses=addresses,allowed=allowed)

def execute(native,b,p,s):
 image=pefile.PE(str(ORACLE));u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(BASE,(image.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
 if native:u.mem_write(BASE,image.get_memory_mapped_image())
 u.mem_map(0x20000000,0x100000);u.mem_map(DATA,0x10000);u.mem_map(HEAP-64,0x10000);u.mem_map(STACK,0x10000)
 if native:addresses={role:BASE+rv for role,(name,rv,n)in PROFILE.items()};allowed=[(BASE+rv,BASE+rv+n)for name,rv,n in PROFILE.values()]
 else:
  addresses=p['addresses'];allowed=p['allowed']
  for a,raw in p['segments']:u.mem_write(a,raw)
 before=s.get('left',0);after=s.get('right',0);words=s['words'];total=before+words+after;header=HEAP+before*4;payload=header+8;end=HEAP+total*4
 raw=bytearray(bytes([0x5d])*(total*4+4))
 if before:struct.pack_into('<I',raw,0,0x80000000|before)
 struct.pack_into('<II',raw,before*4,(0x40000000 if s.get('pinned')else 0)|words,OWNER)
 if after:struct.pack_into('<I',raw,(before+words)*4,0x80000000|after)
 struct.pack_into('<I',raw,total*4,1)
 owner=OWNER
 if s.get('inside_owner'):
  owner=payload+s['inside_owner'];struct.pack_into('<I',raw,before*4+4,owner);struct.pack_into('<I',raw,owner-HEAP,payload)
 u.mem_write(HEAP-32,bytes([0xd1])*32+raw+bytes([0xd2])*32);u.mem_write(OWNER-32,bytes([0xc1])*32+pack(payload)+bytes([0xc2])*32)
 pool=bytearray(48);id=s.get('pool',0);struct.pack_into('<I',pool,id*4,HEAP);struct.pack_into('<I',pool,16+id*4,end);u.mem_write(POOL,bytes(pool))
 arg0=0 if s.get('null')else payload;frame=pack(STOP)+pack(arg0)+pack(s['request'])+pack(id)+pack(s['owner_only'])+bytes([0xf7])*32;u.mem_write(ESP,frame);u.reg_write(UC_X86_REG_ESP,ESP);u.reg_write(UC_X86_REG_EFLAGS,0x202)
 for r,v in zip(NV,SEEDS):u.reg_write(r,v)
 events=[];illegal=[];writes=[];count=0
 def write_hook(machine,access,address,n,value,user):
  if not any(lo<=address and address+n<=hi for lo,hi in [(HEAP,end+4),(OWNER,OWNER+4),(ESP-0x1000,ESP),(ESP+4,ESP+20)]):writes.append([hex(address),n]);machine.emu_stop()
 def code_hook(machine,address,n,user):
  nonlocal count
  count+=1
  if count>50000:illegal.append('instruction cap');machine.emu_stop();return
  if address==STOP:machine.emu_stop();return
  sp=machine.reg_read(UC_X86_REG_ESP)
  if address==addresses['copy']:events.append(dict(call='copy',destination=hex(u32(machine,sp+4)),source=hex(u32(machine,sp+8)),bytes=u32(machine,sp+12)))
  elif address==addresses['coalesce']:events.append(dict(call='coalesce',pool=u32(machine,sp+4),owner=hex(u32(machine,owner)),heap_before=hashlib.sha256(bytes(machine.mem_read(HEAP,total*4+4))).hexdigest()))
  elif address==addresses['diagnostic']:events.append(dict(call='diagnostic',format=cstring(machine,u32(machine,sp+4)),pool=u32(machine,sp+8),payload=hex(u32(machine,sp+12)),bytes=u32(machine,sp+16),owner=hex(u32(machine,owner))))
  if not any(lo<=address<hi for lo,hi in allowed):illegal.append(hex(address));machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,code_hook);u.hook_add(UC_HOOK_MEM_WRITE,write_hook);u.emu_start(addresses['resize'],STOP,count=50001)
 assert not illegal and not writes,(s,illegal,writes)
 assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4 and all(u.reg_read(r)==v for r,v in zip(NV,SEEDS))
 assert u.reg_read(UC_X86_REG_EFLAGS)&0x400==0
 assert bytes(u.mem_read(ESP,4))==pack(STOP)and bytes(u.mem_read(ESP+20,32))==bytes([0xf7])*32
 assert bytes(u.mem_read(HEAP-32,32))==bytes([0xd1])*32 and bytes(u.mem_read(end+4,32))==bytes([0xd2])*32
 assert bytes(u.mem_read(OWNER-32,32))==bytes([0xc1])*32 and bytes(u.mem_read(OWNER+4,32))==bytes([0xc2])*32
 result=dict(return_value=hex(u.reg_read(UC_X86_REG_EAX)),owner=hex(u32(u,owner)),external_owner=hex(u32(u,OWNER)),heap=bytes(u.mem_read(HEAP,total*4+4)).hex(),pool=bytes(u.mem_read(POOL,48)).hex(),events=events,ABI_guards_whitelists_and_direction_flag=True)
 return result

def scenarios():
 cases=[]
 for words in [3,10,17,64]:
  capacity=words*4-8
  for n in sorted(set([0,1,3,4,max(0,capacity-4),capacity-3,capacity,capacity+1,0xfffffffd,0xffffffff])):
   for pin in [False,True]:
    for mode in [0,1,0x80000000]:cases.append(dict(name='base_%d_%x_%d_%x'%(words,n,pin,mode),words=words,request=n,pinned=pin,owner_only=mode))
 for pool in [1,2,3]:
  for left,right in [(5,0),(0,7),(5,7)]:
   for mode in [0,1]:cases.append(dict(name='neighbors_%d_%d_%d_%d'%(pool,left,right,mode),words=10,request=27,pinned=True,owner_only=mode,pool=pool,left=left,right=right))
 for mode in [0,1]:
  for req in [0,9,27]:cases.append(dict(name='inside_%d_%d'%(mode,req),words=10,request=req,pinned=True,owner_only=mode,inside_owner=8))
 cases.append(dict(name='null',words=10,request=7,pinned=True,owner_only=0,null=True))
 return cases

def main():
 global ORACLE
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--source',type=Path,default=ROOT/'src/pc/host_pool_shrink.c');parser.add_argument('--format',type=Path,default=ROOT/'src/pc/host_pool_shrink_format.c');parser.add_argument('--output',type=Path,required=True);parser.add_argument('--original-root',type=Path,default=Path('D:/Prog/hercules_recon'))
 args=parser.parse_args();OUT=args.output.resolve();assert OUT.is_relative_to(ROOT/'work');OUT.mkdir(parents=True,exist_ok=True);ORACLE=args.original_root/'work/discs/pc_install/HERCULES.EXE'
 b=build(args.source.resolve(),args.format.resolve(),OUT/'build');p=prepare(b);ss=scenarios();cases=[]
 for s in ss:
  a=execute(True,b,p,s);c=execute(False,b,p,s);assert a==c,(s,a,c);cases.append(dict(scenario=s,equal=True,native=a,source=c))
 text=args.source.resolve().read_text();wrong={
  'cached_owner_word':(text.replace('new_header[1].owner_slot = old_header[1].owner_slot;','new_header[1].owner_slot = owner_slot;'),'base_10_1c_1_1'),
  'returns_new_payload':(text.replace('    return old_payload;\n}', '    return new_payload;\n}'),'base_10_0_1_1'),
  'loses_pinned_flag':(text.replace('retained_flags = old_descriptor & POOL_PINNED;','retained_flags = 0;'),'base_10_0_1_1'),
  'copy_omits_final_word':(text.replace('memmove(&new_header[1], &old_header[1], (new_words << 2) - 4);','memmove(&new_header[1], &old_header[1], (new_words << 2) - 8);'),'base_10_1c_1_0'),
  'omits_coalescence':(text.replace('    coalesce_pool(pool_id);','    /* deliberately wrong: no coalescence */'),'neighbors_1_5_7_1')};negatives=[]
 for name,(code,case)in wrong.items():
  assert code!=text,name
  lane=OUT/name;lane.mkdir(exist_ok=True);source=lane/'wrong.c';source.write_bytes(code.encode())
  bad=build(source,args.format.resolve(),lane/'build');plan=prepare(bad);s=next(x for x in ss if x['name']==case);a=execute(True,b,p,s);c=execute(False,bad,plan,s);assert a!=c,name
  negatives.append(dict(name=name,detected=True,scenario=s,native=a,wrong=c,build=bad))
 assert b['inputs']=={path:sha(path)for path in b['inputs']}
 result=dict(scope=__doc__,build=b,case_count=len(cases),all_equal=True,cases=cases,negative_controls=negatives,limits=['Actual original/source resize,coalescer and identified unmodified CRT copy execute. Data/heap/owners are explicit fixture preconditions.','Remainder1owner-word corruption is preserved. Original declarations,TU,library name/runtime variant and module placement remain unproved.','No real game process,Windows heap,archive,audio or gameplay execution.'])
 dest=OUT/'host_resize_differential.json';dest.write_bytes((json.dumps(result,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,negatives=len(negatives),report=str(dest))))
if __name__=='__main__':main()
