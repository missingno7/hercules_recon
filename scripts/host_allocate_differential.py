"""Guarded native/source historical pool allocation and initialization; no game process."""
import argparse,hashlib,json,re,struct,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,DEFAULT_TOOLCHAIN,verify_toolchain
from masked import masked_compare
from coff import COFF
from pe import PE
from title_behavioral import context_files
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
BASE=0x400000;POOL=BASE+0x7a29c0;HEAP=0x64000040;DATA=0x61000000;OWNER=DATA+0x400;STACK=0x63000000;ESP=STACK+0x8000;STOP=STACK+0xf000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]
PROFILE={'allocate':('_host_allocate',0x4570,288),'initialize':('_initialize_pool',0x4510,96),'diagnostic':('_host_diagnostic',0x63c0,16)}
SPECS=[('_g_host_pool',0x7a29c0,48),('_g_fmt_init_pool',0x3608c,32),('_g_fmt_alloc',0x360ac,30),('_g_fmt_alloc_failed',0x360cc,36),('_g_fmt_free',0x36110,29)]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();pack=lambda v:struct.pack('<I',v&0xffffffff)
def u32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def text_at(u,a):
 data=bytes(u.mem_read(a,128));return data.split(b'\0')[0].decode('ascii')
def build(source,out):
 verify_toolchain(DEFAULT_TOOLCHAIN);out.mkdir(parents=True,exist_ok=True);assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
 runtime=Path('C:/tools/hercules/renderer-oracle-python/unicorn/lib/unicorn.dll');assert sha(runtime)=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
 files=[source,ROOT/'src/pc/host_diagnostic.c',ROOT/'src/pc/host_pool_storage.c',ROOT/'src/pc/host_pool_formats.c'];watched={Path(__file__),ORACLE,runtime,ROOT/'recovery.json',ROOT/'toolchains/manifest.json',*[ROOT/'scripts'/p for p in ['match.py','masked.py','coff.py','pe.py','env.py','title_behavioral.py']]}
 for p in files:watched.update(context_files(p))
 pins={str(p):sha(p)for p in watched};compiles=[];coffs=[]
 for p in files:
  obj,cmd=compile_source(p,['/O2']);coffs.append(COFF(obj));compiles.append(dict(source=str(p),source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=cmd))
 measurements=[]
 for role,(symbol,rv,n)in PROFILE.items():
  i=next(i for i,c in enumerate(coffs)if any(s['name']==symbol and s['section']>0 for s in c.symbols.values()));raw=compare(Path(compiles[i]['object']),symbol,ORACLE,rv,n)
  if role=='diagnostic':assert raw['exact']
  measurements.append(dict(role=role,symbol=symbol,rva=hex(rv),native_size=n,compiled_size=coffs[i].function(symbol)['size'],raw=raw,diagnostic=masked_compare(Path(compiles[i]['object']),symbol,ORACLE,rv,n)))
 data=[];native=PE(ORACLE)
 for name,rv,n in SPECS:
  c,s=next((c,s)for c in coffs for s in c.symbols.values()if s['name']==name and(s['section']>0 or s['section']==0 and s['value']>0))
  if s['section']==0:assert s['value']==n;raw=bytes(n)
  else:sec=c.sections[s['section']-1];assert not sec['relocations']and s['value']+n<=sec['size'];raw=sec['data'][s['value']:s['value']+n]if sec['data']else bytes(n)
  ns=next(s for s in native.pe.sections if s.VirtualAddress<=rv and rv+n<=s.VirtualAddress+s.Misc_VirtualSize);virtual=rv>=ns.VirtualAddress+ns.SizeOfRawData;assert raw==(bytes(n)if virtual else native.read_rva(rv,n))
  data.append(dict(symbol=name,rva=hex(rv),bytes=n,data=raw.hex(),sha256=hashlib.sha256(raw).hexdigest()))
 assert pins=={p:sha(p)for p in pins}
 return dict(inputs=pins,compiles=compiles,measurements=measurements,data=data)
def prepare(b):
 symbols={};segments=[];where={};coffs=[COFF(x['object'])for x in b['compiles']];cursor=0x20000000
 for x in b['data']:symbols[x['symbol']]=BASE+int(x['rva'],0);segments.append((symbols[x['symbol']],bytes.fromhex(x['data'])))
 for k,c in enumerate(coffs):
  for i,s in enumerate(c.sections,1):
   if s['flags']&0x20 and s['size']:where[k,i]=cursor;cursor+=(s['size']+15)&~15
  for s in c.symbols.values():
   if s['storage']==2 and s['section']>0 and(k,s['section'])in where:assert s['name']not in symbols;symbols[s['name']]=where[k,s['section']]+s['value']
 for k,c in enumerate(coffs):
  for i,s in enumerate(c.sections,1):
   if(k,i)not in where:continue
   raw=bytearray(s['data'])
   for r in s['relocations']:
    local=next((v for v in c.symbols.values()if v['name']==r['symbol']and v['section']>0),None);target=where[k,local['section']]+local['value']if local else symbols[r['symbol']];at=r['offset'];assert r['type']in [6,20];value=struct.unpack_from('<I',raw,at)[0]+target-(where[k,i]+at+4 if r['type']==20 else 0);struct.pack_into('<I',raw,at,value&0xffffffff)
   segments.append((where[k,i],bytes(raw)))
 addresses={role:symbols[sym]for role,(sym,rv,n)in PROFILE.items()};allowed=[(addresses[x['role']],addresses[x['role']]+x['compiled_size'])for x in b['measurements']]
 return dict(segments=segments,addresses=addresses,allowed=allowed)
def execute(native,b,p,s):
 image=pefile.PE(str(ORACLE));u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(BASE,(image.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
 if native:u.mem_write(BASE,image.get_memory_mapped_image())
 u.mem_map(0x20000000,0x100000);u.mem_map(HEAP-64,0x10000);u.mem_map(DATA,0x10000);u.mem_map(STACK,0x10000)
 if native:addresses={r:BASE+rv for r,(sym,rv,n)in PROFILE.items()};allowed=[(BASE+rv,BASE+rv+n)for sym,rv,n in PROFILE.values()]
 else:
  addresses=p['addresses'];allowed=p['allowed']
  for a,raw in p['segments']:u.mem_write(a,raw)
 role=s.get('role','allocate');flags=s.get('flags',0);pool_id=s.get('pool',flags&3);blocks=s.get('blocks',[]);total=sum(n for kind,n in blocks);width=s['bytes']if role=='initialize'else total*4+4
 raw=bytearray(bytes([0x5d])*width);cursor=0
 for kind,n in blocks:
  struct.pack_into('<I',raw,cursor,({'free':0x80000000,'free_pinned':0xc0000000,'used':0x40000000}[kind]|n));cursor+=n*4
 if role=='allocate':struct.pack_into('<I',raw,total*4,1)
 u.mem_write(HEAP-32,bytes([0xd1])*32+raw+bytes([0xd2])*32);u.mem_write(OWNER-32,bytes([0xc1])*32+pack(s.get('previous',0))+bytes([0xc2])*32)
 meta=bytearray(48)
 struct.pack_into('<I',meta,pool_id*4,HEAP);struct.pack_into('<I',meta,16+pool_id*4,HEAP+total*4);meta[32:]=pack(0x12340000)+pack(0x23450000)+pack(0x34560000)+pack(0x45670000);u.mem_write(POOL,bytes(meta))
 owner=HEAP+s['owner_heap_offset']if 'owner_heap_offset'in s else POOL+s['owner_pool_offset']if 'owner_pool_offset'in s else OWNER
 argv=[HEAP,s['bytes'],pool_id]if role=='initialize'else [owner,s['request'],flags]
 frame=pack(STOP)+b''.join(pack(v)for v in argv)+bytes([0xf7])*32;u.mem_write(ESP,frame);u.reg_write(UC_X86_REG_ESP,ESP);u.reg_write(UC_X86_REG_EFLAGS,0x202)
 for r,v in zip(NV,SEEDS):u.reg_write(r,v)
 events=[];illegal=[];writes=[];count=0
 def on_write(machine,access,a,n,value,user):
  if not any(lo<=a and a+n<=hi for lo,hi in [(HEAP,HEAP+width),(POOL,POOL+48),(OWNER,OWNER+4),(ESP-0x1000,ESP),(ESP+4,ESP+16)]):writes.append((hex(a),n));machine.emu_stop()
 def on_code(machine,a,n,user):
  nonlocal count
  count+=1
  if count>50000:illegal.append('instruction cap');machine.emu_stop();return
  if a==STOP:machine.emu_stop();return
  if a==addresses['diagnostic']:
   sp=machine.reg_read(UC_X86_REG_ESP);fmt=text_at(machine,u32(machine,sp+4));args=[u32(machine,sp+8),u32(machine,sp+12)]
   if u32(machine,sp+4)!=BASE+0x360cc:args.append(u32(machine,sp+16))
   events.append(dict(call='diagnostic',format=fmt,arguments=args,output=hex(u32(machine,owner))))
  if not any(lo<=a<hi for lo,hi in allowed):illegal.append(hex(a));machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write);u.emu_start(addresses[role],STOP,count=50001)
 assert not illegal and not writes,(s,illegal,writes)
 assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4 and all(u.reg_read(r)==v for r,v in zip(NV,SEEDS))
 assert not u.reg_read(UC_X86_REG_EFLAGS)&0x400
 assert bytes(u.mem_read(ESP,4))==pack(STOP)and bytes(u.mem_read(ESP+16,32))==bytes([0xf7])*32
 assert bytes(u.mem_read(HEAP-32,32))==bytes([0xd1])*32 and bytes(u.mem_read(HEAP+width,32))==bytes([0xd2])*32
 assert bytes(u.mem_read(OWNER-32,32))==bytes([0xc1])*32 and bytes(u.mem_read(OWNER+4,32))==bytes([0xc2])*32
 return dict(return_value=None if role=='initialize'else hex(u.reg_read(UC_X86_REG_EAX)),owner=hex(u32(u,owner)),external_owner=hex(u32(u,OWNER)),heap=bytes(u.mem_read(HEAP,width)).hex(),pool=bytes(u.mem_read(POOL,48)).hex(),events=events,ABI_guards_whitelists_and_direction_flag=True)
def scenarios():
 result=[];layouts=[[('free',2)],[('free',4)],[('free',8)],[('free',64)],[('free',40),('used',5),('free',12)],[('free',12),('used',3),('free_pinned',40)],[('used',16)],[]]
 for li,blocks in enumerate(layouts):
  capacity=max([n*4-8 for kind,n in blocks if kind.startswith('free')]+[0])
  for request in sorted(set([0,1,3,4,5,capacity,max(0,capacity-4),capacity+1,0xfffffffc,0xfffffffd,0xffffffff])):
   for flags in [0,0x10,0x20,0x30]:
    for previous in [0,0x11112222]:result.append(dict(name='matrix_%d_%x_%x_%x'%(li,request,flags,previous),blocks=blocks,request=request,flags=flags,previous=previous))
 for pool in range(4):
  for flags in [0x10,0x30,0x80000010,0xfffffff0]:
   for request in [0,9,999]:result.append(dict(name='pool_%d_%x_%x'%(pool,flags,request),blocks=[('free',12),('used',4),('free',10)],request=request,flags=flags|pool,previous=0x12345678))
 for offset in [0,4,12,20,24]:result.append(dict(name='owner_heap_'+str(offset),blocks=[('free',8)],request=8,flags=0x10,owner_heap_offset=offset))
 for offset in [0,16,32]:result.append(dict(name='owner_pool_'+str(offset),blocks=[('free',8)],request=8,flags=0x10,owner_pool_offset=offset))
 for pool in range(4):
  for n in [16,20,31,256,4096]:result.append(dict(name='initialize_%d_%d'%(pool,n),role='initialize',bytes=n,pool=pool))
 return result
def main():
 global ORACLE
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,default=ROOT/'src/pc/host_pool_repaired.c');p.add_argument('--original-root',type=Path,default=Path('D:/Prog/hercules_recon'));p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();assert out.is_relative_to(ROOT/'work');out.mkdir(parents=True,exist_ok=True);ORACLE=a.original_root/'work/discs/pc_install/HERCULES.EXE'
 b=build(a.source.resolve(),out/'build');plan=prepare(b);ss=scenarios();cases=[]
 for s in ss:
  native=execute(True,b,plan,s);source=execute(False,b,plan,s);assert native==source,(s,native,source);cases.append(dict(scenario=s,equal=True,native=native,source=source))
 text=a.source.resolve().read_text();wrong={
  'first_adequate_block':(text.replace('selected_words = words;','selected_words = words;\n                break;'),'matrix_4_5_10_0'),
  'pinned_flag_lost':(text.replace('pinned = POOL_PINNED;','pinned = 0;'),'matrix_2_5_10_0'),
  'preserve_failure_ignored':(text.replace('preserve_output = 1;','preserve_output = 0;'),'matrix_6_5_20_11112222'),
  'request_rounds_down':(text.replace('need_words = ((requested_bytes + 3) >> 2) + 2;','need_words = (requested_bytes >> 2) + 2;'),'matrix_2_5_10_0'),
  'exact_fit_rejected':(text.replace('if (words >= need_words)','if (words > need_words)'),'matrix_1_8_10_0')};negatives=[]
 for name,(code,case)in wrong.items():
  assert code!=text;lane=out/name;lane.mkdir(exist_ok=True);candidate=lane/'wrong.c';candidate.write_bytes(code.encode());bad=build(candidate,lane/'build');bp=prepare(bad);s=next(x for x in ss if x['name']==case);native=execute(True,b,plan,s);source=execute(False,bad,bp,s);assert native!=source,name;negatives.append(dict(name=name,detected=True,scenario=s,native=native,wrong=source,build=bad))
 assert b['inputs']=={p:sha(p)for p in b['inputs']}
 report=dict(scope=__doc__,build=b,case_count=len(cases),all_equal=True,cases=cases,negative_controls=negatives,limits=['Actual allocator/initializer and no-op diagnostic execute;no modeled callees. Heap,owner and metadata are explicit valid input/precondition data.','Owner/header/metadata aliases preserve native mutations,including malformed resulting ownership. No arbitrary-pointer,arena-size or whole-game safety claim.','Allocation flags,rounding wrap,zero/empty/exact/one-word cases and prior output values are finite comparisons. Raw differences remain reconstructed;no FUNCTION_MATCH.'])
 dest=out/'host_allocate_differential.json';dest.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),negatives=len(negatives),all_equal=True,report=str(dest))))
if __name__=='__main__':main()
