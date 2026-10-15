"""Finite original/source file callback comparisons; explicit synthetic I/O and layout."""
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
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source',type=Path,default=ROOT/'src/pc/host_file_callbacks.c')
parser.add_argument('--formats',type=Path,default=ROOT/'calibration/host_file_callback_formats.c')
parser.add_argument('--open-close-source',type=Path,default=ROOT/'src/pc/host_file_open_close.c')
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--original-root',type=Path,default=Path('D:/Prog/hercules_recon'))
args=parser.parse_args();OUT=args.output.resolve();OUT.mkdir(parents=True,exist_ok=True)
assert OUT.is_relative_to(ROOT/'work')
ORACLE=args.original_root/'work/discs/pc_install/HERCULES.EXE'
PROFILE={'sync':('_host_file_sync',0x3b50,256),'async':('_host_file_async',0x3d90,272),
 'cancel':('_host_cancel_load',0x3ea0,96),'progress':('_host_file_progress',0x3f00,320),
 'length':('_resource_file_length',0x40d0,176),'prepare':('_host_file_prepare',0x1d60,32),
 'empty':('_empty_host_callback',0xbe30,16),'diagnostic':('_host_diagnostic',0x63c0,16),
 'open_only':('_host_file_open_only',0x3c50,144),'close_state':('_host_frame_close_state',0x3d30,96),
 'sector_seek':('_host_sector_seek',0x3ce0,32),'sector_read':('_host_sector_read',0x3d00,48)}
FORMATS=[('_g_resource_filename_format',0x36024,5),('_g_resource_loading_format',0x36008,27),
 ('_g_resource_async_open_failure_format',0x36050,24),('_g_resource_sync_open_failure_format',0x35ff4,20),
 ('_g_resource_sync_loaded_format',0x35fe8,11),('_g_resource_close_failure_format',0x36044,11),
 ('_g_resource_opening_format',0x3602c,12),('_g_resource_close_success_format',0x36038,11)]
MODELS={'format':('_sprintf',0x10f40),'open':('_virtual_file_open',0xa210),
 'read':('_virtual_file_read',0xa2f0),'close':('_virtual_file_close',0xa350),'seek':('_virtual_file_seek',0xa280)}
BASE=0x400000;TABLE=BASE+0x361b8;FILENAME=BASE+0x215c10;SCALAR=BASE+0x352b0
DATA=0x61000000;CONTEXTS=[DATA+0x10020,DATA+0x20020,DATA+0x30020];SIZE=0x1b34
PREFIX=DATA+0x4000;SUFFIX=DATA+0x4200;DEST=0x62000020;STACK=0x63000000;ESP=STACK+0x8000;STOP=STACK+0xf000
MODEL=0x65000000;COMPLETE=MODEL+0x1000;FAILED=MODEL+0x1020
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
pack=lambda x:struct.pack('<I',x&0xffffffff)
def u32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def cstring(u,a):
 b=bytearray()
 for i in range(512):
  v=u.mem_read(a+i,1)[0]
  if not v:return b.decode('ascii')
  b.append(v)
 raise AssertionError('Unterminated fixture string')

def build(source,formats,out,open_close_source=None):
 verify_toolchain(DEFAULT_TOOLCHAIN);out.mkdir(parents=True,exist_ok=True)
 assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
 runtime=Path('C:/tools/hercules/renderer-oracle-python/unicorn/lib/unicorn.dll')
 assert sha(runtime)=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
 paths=[source,open_close_source or args.open_close_source.resolve(),ROOT/'calibration/host_sector_io.c',ROOT/'calibration/host_file_prepare.c',ROOT/'src/pc/interface_callbacks.c',ROOT/'src/pc/host_diagnostic.c',formats,ROOT/'calibration/host_resource_storage.c']
 watched={Path(__file__),ORACLE,runtime,ROOT/'recovery.json',ROOT/'toolchains/manifest.json'}
 for p in paths:watched.update(context_files(p))
 for name in ['match.py','masked.py','coff.py','pe.py','env.py','title_behavioral.py']:watched.add(ROOT/'scripts'/name)
 pins={str(p):sha(p)for p in watched};compiles=[];coffs=[];measurements=[];native=PE(ORACLE)
 for p in paths:
  obj,cmd=compile_source(p,['/O2']);co=COFF(obj);coffs.append(co)
  compiles.append(dict(source=str(p),source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=cmd))
 for role,(sym,rva,n)in PROFILE.items():
  owners=[i for i,co in enumerate(coffs)if any(s['name']==sym and s['section']>0 for s in co.symbols.values())];assert len(owners)==1,(role,owners);index=owners[0]
  obj=Path(compiles[index]['object']);raw=compare(obj,sym,ORACLE,rva,n);diag=masked_compare(obj,sym,ORACLE,rva,n)
  if role in ['prepare','empty','diagnostic','sector_seek','sector_read']:assert diag['masked_equal']
  if role in ['empty','diagnostic']:assert raw['exact']
  measurements.append(dict(role=role,symbol=sym,rva=hex(rva),target_size=n,compiled_size=coffs[index].function(sym)['size'],raw=raw,diagnostic=diag))
 data=[]
 for name,rva,n in [*FORMATS,('_g_resource_prepare_scalar',0x352b0,4)]:
  co=coffs[6 if name!='_g_resource_prepare_scalar'else 7]
  s=next(s for s in co.symbols.values()if s['name']==name and s['section']>0);sec=co.sections[s['section']-1]
  raw=sec['data'][s['value']:s['value']+n];assert len(raw)==n and not sec['relocations']and raw==native.read_rva(rva,n)
  data.append(dict(symbol=name,rva=hex(rva),bytes=n,data=raw.hex(),sha256=hashlib.sha256(raw).hexdigest()))
 assert pins=={str(p):sha(p)for p in watched}
 return dict(inputs=pins,compiles=compiles,measurements=measurements,data=data)

def prepare(b):
 symbols={'_g_host_engine_interface':TABLE,'_g_resource_filename':FILENAME};segments=[];addr={};coffs=[COFF(x['object'])for x in b['compiles'][:6]];cursor=0x20000000
 for x in b['data']:symbols[x['symbol']]=BASE+int(x['rva'],0);segments.append((symbols[x['symbol']],bytes.fromhex(x['data'])))
 endpoints={MODEL+i*32:role for i,role in enumerate(MODELS)}
 for a,role in endpoints.items():symbols[MODELS[role][0]]=a
 for k,co in enumerate(coffs):
  for i,s in enumerate(co.sections,1):
   if not s['flags']&0x20 or not s['size']:continue
   addr[k,i]=cursor;cursor+=(s['size']+15)&~15
  for s in co.symbols.values():
   if s['storage']==2 and s['section']>0:
    assert (k,s['section'])in addr and s['name']not in symbols,s['name']
    symbols[s['name']]=addr[k,s['section']]+s['value']
 for k,co in enumerate(coffs):
  for i,s in enumerate(co.sections,1):
   if(k,i)not in addr:continue
   raw=bytearray(s['data']);assert len(raw)==s['size']
   for rel in s['relocations']:
    name=rel['symbol'];local=next((x for x in co.symbols.values()if x['name']==name and x['section']>0),None)
    target=addr[k,local['section']]+local['value']if local else symbols[name]
    off=rel['offset'];assert rel['type']in [6,20]
    value=struct.unpack_from('<I',raw,off)[0]+target-(addr[k,i]+off+4 if rel['type']==20 else 0)
    struct.pack_into('<I',raw,off,value&0xffffffff)
   segments.append((addr[k,i],bytes(raw)))
 addresses={role:symbols[sym]for role,(sym,rva,n)in PROFILE.items()}
 allowed=[(addresses[x['role']],addresses[x['role']]+x['compiled_size'])for x in b['measurements']]
 return dict(segments=segments,endpoints=endpoints,addresses=addresses,allowed=allowed)

def execute(native,b,p,s):
 pe=pefile.PE(str(ORACLE));u=Uc(UC_ARCH_X86,UC_MODE_32);image_size=(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095
 u.mem_map(BASE,image_size)
 if native:u.mem_write(BASE,pe.get_memory_mapped_image())
 u.mem_map(0x20000000,0x100000);u.mem_map(DATA,0x50000);u.mem_map(DEST-32,0x20000);u.mem_map(STACK,0x10000);u.mem_map(MODEL,0x2000)
 if native:
  addresses={role:BASE+rva for role,(sym,rva,n)in PROFILE.items()};endpoints={BASE+rva:role for role,(sym,rva)in MODELS.items()}
  allowed=[(BASE+rva,BASE+rva+n)for sym,rva,n in PROFILE.values()]
 else:
  addresses=p['addresses'];endpoints=p['endpoints'];allowed=p['allowed']
  for address,raw in p['segments']:u.mem_write(address,raw)
 table=bytearray(bytes([0x2d])*1000);table[4:8]=pack(CONTEXTS[0]);u.mem_write(TABLE,bytes(table))
 u.mem_write(FILENAME-32,bytes([0xe1])*32+bytes([0xe2])*512+bytes([0xe3])*32)
 u.mem_write(PREFIX,b'\\TITLE\\');u.mem_write(PREFIX+7,b'\0');u.mem_write(SUFFIX,b'ENGINE\\TITLE\0')
 u.mem_write(DEST-32,bytes([0xd1])*32+bytes([0x5d])*0x10000+bytes([0xd2])*32)
 for index,a in enumerate(CONTEXTS):
  raw=bytearray(bytes([0xa1+index])*SIZE)
  fields={0x9c:('H',s.get('mode',1)),0x9e:('H',s.get('remaining',0)&0xffff),0xa0:('H',0x1234),
   0xa4:('I',DEST+index*0x100),0xac:('I',0x12340000+index),0xb0:('I',0 if s.get('no_complete')else COMPLETE),
   0xb4:('I',0 if s.get('no_failed')else FAILED),0xb8:('I',10+index),0xbc:('I',s.get('countdown',0)),
   0xd8:('I',s.get('transfer',0)&0xffffffff),0xdc:('H',0x55aa),0xe0:('B',s.get('suppress',0)),0x1b00:('I',0x33445566)}
  for off,(fmt,v)in fields.items():struct.pack_into('<'+fmt,raw,off,v)
  u.mem_write(a-32,bytes([0xc1])*32+raw+bytes([0xc2])*32)
 def frame_for(role):
  arguments={'sync':[PREFIX,SUFFIX,DEST,s.get('bytes',11)],'async':[PREFIX,SUFFIX,s.get('sector',0),DEST,s.get('bytes',11)],'cancel':[],'progress':[],'length':[PREFIX,SUFFIX],'open_only':[PREFIX,SUFFIX],'close_state':[],'sector_seek':[s.get('sector',0)],'sector_read':[DEST,s.get('sectors',1)]}[role]
  return pack(STOP)+b''.join(pack(v)for v in arguments)+bytes([0xf7])*32
 trace=[];counts={};pending=[];illegal=[];writes=[];instructions=0
 def effect(role):
  key=role+'_'+str(counts.get(role,0));index=s.get('effects',{}).get(key)
  if index is not None:u.mem_write(TABLE+4,pack(CONTEXTS[index]));trace.append(dict(effect=key,context=index))
 def hook_write(machine,access,address,n,value,user):
  ranges=[(a,a+SIZE)for a in CONTEXTS]+[(SCALAR,SCALAR+4),(ESP-0x1000,ESP)]
  if not any(lo<=address and address+n<=hi for lo,hi in ranges):writes.append([hex(address),n]);machine.emu_stop()
 def hook_code(machine,address,n,user):
  nonlocal instructions
  instructions+=1
  if instructions>100000:illegal.append('instruction cap');machine.emu_stop();return
  if address==STOP:machine.emu_stop();return
  if address in endpoints or address in [COMPLETE,FAILED]:pending.append(address);machine.emu_stop();return
  if address==addresses['diagnostic']:
   sp=machine.reg_read(UC_X86_REG_ESP);fmt=cstring(machine,u32(machine,sp+4));event=dict(call='diagnostic',format=fmt)
   if '%s' in fmt:event['text']=cstring(machine,u32(machine,sp+8))
   if fmt.startswith('Loading '):event.update(destination=hex(u32(machine,sp+12)),requested=u32(machine,sp+16))
   elif '(%d)'in fmt:event['requested']=u32(machine,sp+12)
   trace.append(event)
  if not any(lo<=address<hi for lo,hi in allowed):illegal.append(hex(address));machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,hook_code);u.hook_add(UC_HOOK_MEM_WRITE,hook_write)
 def invoke(function_role):
  frame=frame_for(function_role);u.mem_write(ESP,frame);u.reg_write(UC_X86_REG_ESP,ESP)
  for reg,value in zip(NV,SEEDS):u.reg_write(reg,value)
  pc=addresses[function_role]
  for _ in range(100):
   pending.clear();u.emu_start(pc,STOP,count=100001)
   if illegal or writes or u.reg_read(UC_X86_REG_EIP)==STOP:break
   assert pending,'Unexpected emulator stop';address=pending[-1];sp=u.reg_read(UC_X86_REG_ESP);role=endpoints.get(address,'complete'if address==COMPLETE else'failed');counts[role]=counts.get(role,0)+1;number=counts[role]
   if role=='format':
    dest,fmt,a,c=[u32(u,sp+i)for i in [4,8,12,16]];assert dest==FILENAME and cstring(u,fmt)=='%s%s'
    text=cstring(u,a)+cstring(u,c);assert len(text)<512;u.mem_write(dest,text.encode()+b'\0');result=len(text);trace.append(dict(call=role,prefix=cstring(u,a),suffix=cstring(u,c),text=text))
   elif role=='open':
    path,mode,flags=[u32(u,sp+i)for i in [4,8,12]];seq=s.get('open_results',[3]);assert number<=len(seq),'Open retry escaped bounded fixture';result=seq[number-1];trace.append(dict(call=role,path=cstring(u,path),mode=mode,flags=flags,result=result&0xffffffff))
   elif role=='read':
    handle,dest,n=[u32(u,sp+i)for i in [4,8,12]];result=s.get('read_result',min(3,n));assert 0<=result<=16
    if result:assert DEST<=dest and dest+result<=DEST+0x10000;u.mem_write(dest,bytes([0x70+number])*result)
    trace.append(dict(call=role,handle=handle,destination=hex(dest),requested=n,returned=result))
   elif role=='close':
    seq=s.get('close_results',[0]);assert number<=len(seq),'Close retry escaped bounded fixture';result=seq[number-1];trace.append(dict(call=role,handle=u32(u,sp+4),result=result&0xffffffff))
   elif role=='seek':
    seq=s.get('seek_results',[0x5000]);assert number<=len(seq);result=seq[number-1];trace.append(dict(call=role,handle=u32(u,sp+4),offset=u32(u,sp+8),origin=u32(u,sp+12),result=result&0xffffffff))
   else:
    assert u32(u,sp+4)==0;result=0x12345678;context=u32(u,TABLE+4);trace.append(dict(call=role,context=CONTEXTS.index(context),argument=0,mode=struct.unpack('<H',u.mem_read(context+0x9c,2))[0],status=struct.unpack('<H',u.mem_read(context+0xdc,2))[0]))
   effect(role);pc=u32(u,sp);u.reg_write(UC_X86_REG_ESP,sp+4);u.reg_write(UC_X86_REG_EAX,result&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xdeadbeef);u.reg_write(UC_X86_REG_EDX,0xcafebabe)
  else:raise AssertionError('Endpoint cap')
  assert not illegal and not writes,(s,illegal,writes)
  assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4
  assert all(u.reg_read(reg)==v for reg,v in zip(NV,SEEDS))
  assert bytes(u.mem_read(ESP,len(frame)))==frame
  assert all(bytes(u.mem_read(a-32,32))==bytes([0xc1])*32 and bytes(u.mem_read(a+SIZE,32))==bytes([0xc2])*32 for a in CONTEXTS)
  assert bytes(u.mem_read(FILENAME-32,32))==bytes([0xe1])*32 and bytes(u.mem_read(FILENAME+512,32))==bytes([0xe3])*32
  assert bytes(u.mem_read(DEST-32,32))==bytes([0xd1])*32 and bytes(u.mem_read(DEST+0x10000,32))==bytes([0xd2])*32
  return None if function_role=='progress'else u.reg_read(UC_X86_REG_EAX)
 calls=[]
 for role in s.get('commands',[s['role']]):
  result=invoke(role);context=u32(u,TABLE+4)
  calls.append(dict(role=role,result=result,context=CONTEXTS.index(context),mode=struct.unpack('<H',u.mem_read(context+0x9c,2))[0],remaining=struct.unpack('<h',u.mem_read(context+0x9e,2))[0],countdown=u32(u,context+0xbc),cursor=hex(u32(u,context+0xa4)),context_hashes=[hashlib.sha256(bytes(u.mem_read(a,SIZE))).hexdigest()for a in CONTEXTS]))
 digest=lambda a,n:hashlib.sha256(bytes(u.mem_read(a,n))).hexdigest()
 return dict(events=trace,calls=calls,result=calls[-1]['result'],contexts=[digest(a,SIZE)for a in CONTEXTS],table=digest(TABLE,1000),destination=digest(DEST,0x10000),filename=digest(FILENAME,512),prepare_scalar=u32(u,SCALAR),guards_stack_nonvolatiles_and_whitelists=True)

def scenarios():
 cases=[]
 def add(role,name,**values):cases.append(dict(role=role,name=name,**values))
 for n in [0,1,2047,2048,2049,0x3ffffff,0x7ffff800,0x7fffffff,0x80000000,0xfffff800,0xffffffff]:
  for mode in [0,1,4,0xffff]:add('async','async_%x_%x'%(n,mode),bytes=n,mode=mode)
 for sector in [1,-1,0x1fffff,0x200000,0x7fffffff,-0x80000000]:add('async','sector_'+str(sector),sector=sector)
 add('async','async_open_failed',open_results=[-1],mode=0)
 add('async','async_live_open_seek',sector=2,mode=0,effects={'open_1':1,'seek_1':2})
 for n in [0,1,11,2048,0xffffffff]:add('sync','sync_'+str(n),bytes=n)
 add('sync','sync_open_failed',open_results=[-1])
 add('sync','sync_reopen_failed',open_results=[3,-1],close_results=[-1])
 add('sync','sync_retry_twice',open_results=[3,4,5],close_results=[-1,1,0])
 add('sync','sync_live_read_close',effects={'open_1':1,'read_1':2,'close_1':0})
 add('length','length_normal')
 add('length','length_open_failed',open_results=[-1])
 add('length','length_reopen_failed',open_results=[3,-1],close_results=[-1])
 add('length','length_retry',open_results=[3,4,5],close_results=[-1,1,0],seek_results=[8,0x80000000,0xffffffff])
 add('length','length_live_seek_close',effects={'open_1':1,'seek_1':2,'close_1':0})
 for mode in [0,1,4,0xffff]:
  for result in [0,-1,1]:add('cancel','cancel_%x_%s'%(mode,result),mode=mode,close_results=[result],effects={'close_1':1})
 for count in [0,1,2,12,0x80000000,0xffffffff]:
  for mode in [0,1,4]:add('progress','count_%x_%x'%(count,mode),countdown=count,mode=mode,remaining=6)
 for n in [-32768,-1,0,1,4,5,6,32767]:
  for suppress in [0,1,0xff]:add('progress','progress_%s_%s'%(n,suppress),remaining=n,suppress=suppress,read_result=0 if n<0 else min(3,n*2048))
 for failure in [False,True]:
  add('progress','callback_null_'+str(failure),transfer=-1 if failure else 0,no_complete=True,no_failed=True)
  add('progress','callback_live_'+str(failure),transfer=-1 if failure else 0,effects={'failed_1':1,'complete_1':2})
 add('progress','progress_read_live',remaining=1,effects={'read_1':1,'close_1':2})
 add('progress','progress_close_failed',remaining=0,close_results=[-1],effects={'close_1':1})
 add('pipeline','pipeline_small_completion',mode=0,bytes=2049,commands=['async']+['progress']*14)
 add('pipeline','pipeline_three_chunks',mode=0,bytes=12*2048+1,commands=['async']+['progress']*16)
 add('pipeline','pipeline_cancel_mid_transfer',mode=0,bytes=6*2048,commands=['async','progress','cancel','progress'])
 add('pipeline','pipeline_transfer_failure',mode=0,bytes=2048,transfer=-1,commands=['async','progress','progress'])
 add('pipeline','pipeline_suppress_repeated_completion',mode=0,bytes=1,suppress=1,commands=['async']+['progress']*3)
 for mode in [0,1,2,4,0xffff]:
  for result in [0,3,7,-1,-2,0x80000001,0xffffffff]:add('open_only','open_%x_%x'%(mode,result&0xffffffff),mode=mode,open_results=[result])
 add('open_only','open_live_context',mode=4,open_results=[7],effects={'open_1':1})
 for mode in [0,1,4,0xffff]:
  for result in [0,1,-1,0x80000000]:add('close_state','close_%x_%x'%(mode,result&0xffffffff),mode=mode,close_results=[result],effects={'close_1':1})
 for sector in [0,1,0x1fffff,0x200000,0x7fffffff,0x80000000,0xffffffff]:add('sector_seek','sector_seek_%x'%sector,sector=sector)
 for n in [0,1,0x1fffff,0x200000,0xffffffff]:add('sector_read','sector_read_%x'%n,sectors=n,read_result=0)
 add('pipeline','pipeline_open_seek_read_close',mode=4,sector=3,sectors=2,commands=['open_only','sector_seek','sector_read','close_state'])
 add('pipeline','pipeline_open_live_close',mode=4,effects={'open_1':1,'close_1':2},commands=['open_only','close_state'])
 add('pipeline','pipeline_busy_open_preserves_async',mode=0,bytes=2049,commands=['async','open_only','progress'])
 return cases

def main():
 b=build(args.source.resolve(),args.formats.resolve(),OUT/'build');p=prepare(b);ss=scenarios();cases=[]
 assert next(x for x in b['measurements']if x['role']=='close_state')['diagnostic']['masked_equal']
 for s in ss:
  native=execute(True,b,p,s);source=execute(False,b,p,s);assert native==source,(s,native,source)
  cases.append(dict(scenario=s,equal=True,native=native,source=source))
 text=args.source.resolve().read_text();wrong={
  'advance_returned_count':text.replace('(void)virtual_file_read(handle, destination, (size_t)byte_delta);','byte_delta = virtual_file_read(handle, destination, (size_t)byte_delta);'),
  'cached_read_context':text.replace('        context = g_host_engine_interface.context_004;\n        context->destination_cursor_a4 = (unsigned char *)','        context->destination_cursor_a4 = (unsigned char *)'),
  'unsigned_sector_rounding':text.replace('sector_count = (short)(signed_corrected_count / 2048L);','sector_count = (short)(corrected_count / 2048UL);')}
 start=text.index('int __cdecl host_cancel_load');end=text.index('void __cdecl host_file_progress',start)
 wrong['cancel_returns_one']=text[:start]+text[start:end].replace('return 0;','return 1;')+text[end:]
 names={'advance_returned_count':'progress_1_1','cached_read_context':'progress_read_live',
        'unsigned_sector_rounding':'async_80000000_0','cancel_returns_one':'cancel_0_0'}
 negatives=[]
 for name,code in wrong.items():
  assert code!=text,name
  lane=OUT/name;lane.mkdir(exist_ok=True)
  code=re.sub(r'#include "([^"]+)"',lambda m:'#include "'+(args.source.resolve().parent/m.group(1)).resolve().as_posix()+'"',code)
  source=lane/'wrong.c';source.write_bytes(code.encode());bad=build(source,args.formats.resolve(),lane/'build');plan=prepare(bad)
  s=next(x for x in ss if x['name']==names[name]);a=execute(True,b,p,s);c=execute(False,bad,plan,s);assert a!=c,name
  negatives.append(dict(name=name,detected=True,scenario=s,native=a,wrong=c,build=bad))
 extension=args.open_close_source.resolve();ext_text=extension.read_text();close_at=ext_text.index('int __cdecl host_frame_close_state')
 wrong_ext={'open_loses_return':ext_text.replace('return opened;','return 1;')}
 close_body=ext_text[close_at:];close_body=close_body.replace('{\n','{\n    ResourceCallbackContext *saved = g_host_engine_interface.context_004;\n',1)
 close_body=close_body.replace('g_host_engine_interface.context_004->unknown_0ac = 0;','saved->unknown_0ac = 0;').replace('g_host_engine_interface.context_004->mode_09c = 0;','saved->mode_09c = 0;')
 wrong_ext['close_caches_context']=ext_text[:close_at]+close_body
 for name,code in wrong_ext.items():
  assert code!=ext_text,name
  lane=OUT/name;lane.mkdir(exist_ok=True);code=re.sub(r'#include "([^"]+)"',lambda m:'#include "'+(extension.parent/m.group(1)).resolve().as_posix()+'"',code)
  source=lane/'wrong.c';source.write_bytes(code.encode());bad=build(args.source.resolve(),args.formats.resolve(),lane/'build',source);plan=prepare(bad)
  case='open_0_80000001'if name=='open_loses_return'else'close_4_0';s=next(x for x in ss if x['name']==case);a=execute(True,b,p,s);c=execute(False,bad,plan,s);assert a!=c,name
  negatives.append(dict(name=name,detected=True,scenario=s,native=a,wrong=c,build=bad))
 assert b['inputs']=={path:sha(path)for path in b['inputs']}
 report=dict(scope=__doc__,build=b,case_count=len(cases),all_equal=True,cases=cases,negative_controls=negatives,
  full_sequences=sum(x['scenario']['role']=='pipeline'for x in cases),
  actual_family_invocations=sum(len(x['native']['calls'])for x in cases),
  limits=['Source COFF receives ordinary DIR32/REL32 fixups in an explicit fixture layout, not a historical module link.',
          'Table1000,three context views,512-byte filename model and I/O/callback responses are synthetic; no production buffer capacity or real I/O is proved.',
          'Actual prepare,empty and diagnostic code executes. sprintf and virtual file adapters remain modeled.',
          'No process,filesystem,registry,window,exception or gameplay execution.'])
 dest=OUT/'host_file_callbacks_differential.json';dest.write_bytes((json.dumps(report,indent=2)+'\n').encode())
 print(json.dumps(dict(cases=len(cases),all_equal=True,negatives=len(negatives),report=str(dest))))
if __name__=='__main__':main()
