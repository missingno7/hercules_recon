"""Original/source music and SFX loading with actual archive-byte replays; modeled I/O/device boundaries."""
import argparse,hashlib,json,re,struct,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,DEFAULT_TOOLCHAIN,verify_toolchain
from masked import masked_compare
from coff import COFF
from pe import PE
from title_behavioral import context_files
from host_resize_differential import verified_copy,SPECS
from host_music_paths_data import verify_data
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source',type=Path,default=ROOT/'src/pc/host_music.c');parser.add_argument('--data',type=Path,default=ROOT/'calibration/host_music_storage.c')
parser.add_argument('--paths-source',type=Path,default=ROOT/'calibration/host_music_paths.c');parser.add_argument('--paths-definition',type=Path,default=ROOT/'evidence/host_music_paths_data.json')
parser.add_argument('--sfx-source',type=Path,default=ROOT/'src/pc/host_sfx.c');parser.add_argument('--sfx-data',type=Path,default=ROOT/'calibration/host_sfx_storage.c')
parser.add_argument('--pool-source',type=Path,default=ROOT/'calibration/host_pool_repaired.c')
parser.add_argument('--output',type=Path,required=True);parser.add_argument('--original-root',type=Path,default=Path('D:/Prog/hercules_recon'))
args=parser.parse_args();OUT=args.output.resolve();assert OUT.is_relative_to(ROOT/'work');OUT.mkdir(parents=True,exist_ok=True)
ORACLE=args.original_root/'work/discs/pc_install/HERCULES.EXE'
ARCHIVE=args.original_root/'assets/pc/HERCULES/HERCULES.FS';ARCHIVE_BYTES=ARCHIVE.read_bytes();assert hashlib.sha256(ARCHIVE_BYTES).hexdigest()=='affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4'
MUSIC={'apply':(0x1000,144),'signal':(0x1090,32),'set_bases':(0x10b0,48),'initialize':(0x10e0,48),'load':(0x1110,544),'bank_register':(0xc040,80),'bank_preload':(0xc090,80)}
PROFILE={k:('_host_music_'+k+'_'+format(rv,'x'),rv,n)for k,(rv,n)in MUSIC.items()}
PROFILE.update(sync=('_host_file_sync',0x3b50,256),length=('_resource_file_length',0x40d0,176),prepare=('_host_file_prepare',0x1d60,32),empty=('_empty_host_callback',0xbe30,16),diagnostic=('_host_diagnostic',0x63c0,16),zero=('_host_zero_callback',0xa340,16),true=('_return_true',0xc240,16))
PROFILE.update(resize=('_shrink_relocate',0x4690,192),coalesce=('_coalesce_pool',0x4830,96),copy=('_memmove',0x10fb0,821))
PROFILE.update(load_sfx=('_host_sfx_load_1330',0x1330,672),program_attributes=('_host_sfx_program_attributes_be60',0xbe60,96),tone_attributes=('_host_sfx_tone_attributes_bec0',0xbec0,96))
PROFILE.update(allocate=('_host_allocate',0x4570,288))
FILE_DATA=[('_g_resource_filename_format',0x36024,5),('_g_resource_loading_format',0x36008,27),('_g_resource_async_open_failure_format',0x36050,24),('_g_resource_sync_open_failure_format',0x35ff4,20),('_g_resource_sync_loaded_format',0x35fe8,11),('_g_resource_close_failure_format',0x36044,11),('_g_resource_opening_format',0x3602c,12),('_g_resource_close_success_format',0x36038,11)]
MUSIC_DATA=[('_g_host_music_init_text',0x35bc4),('_g_host_music_load_text',0x35c18),('_g_host_music_filename',0x35c0c),('_g_host_music_open_error',0x35bf4),('_g_host_music_download_error',0x35bd8)]
SFX_DATA=[('_g_host_sfx_'+name,rv)for name,rv in [('load_text',0x35d1c),('filename',0x35d14),('warning_begin',0x35cd4),('warning_detail',0x35c98),('warning_program',0x35c6c),('warning_end',0x35c2c)]]
MODELS={'format':('_sprintf',0x10f40),'open':('_virtual_file_open',0xa210),'read':('_virtual_file_read',0xa2f0),'close':('_virtual_file_close',0xa350),'seek':('_virtual_file_seek',0xa280),'preload':('_host_sound_bank_initialize_f3d0',0xf3d0)}
BASE=0x400000;TABLE=BASE+0x361b8;PATH_TABLE=BASE+0x35178;BANKS=BASE+0x68e790;FILENAME=BASE+0x215c10;SCALAR=BASE+0x352b0
POOL=BASE+0x7a29c0
DATA=0x61000000;CONTEXTS=[DATA+0x10020,DATA+0x20020,DATA+0x30020];CTX_SIZE=0x1b34;PATHS=[DATA+0x4000,DATA+0x4200]
QUERY_OUT=DATA+0x4800
STACK=0x63000000;ESP=STACK+0x8000;STOP=STACK+0xf000;MODEL=0x65000000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();pack=lambda v:struct.pack('<I',v&0xffffffff)
def u32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def cstring(u,a):
 b=bytearray()
 for i in range(512):
  v=u.mem_read(a+i,1)[0]
  if not v:return b.decode('ascii')
  b.append(v)
 raise AssertionError('unterminated fixture string')

def build(source,data,out):
 verify_toolchain(DEFAULT_TOOLCHAIN);out.mkdir(parents=True,exist_ok=True)
 assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
 runtime=Path('C:/tools/hercules/renderer-oracle-python/unicorn/lib/unicorn.dll');assert sha(runtime)=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
 files=[source,ROOT/'src/pc/host_file_callbacks.c',ROOT/'calibration/host_file_prepare.c',ROOT/'src/pc/interface_callbacks.c',ROOT/'src/pc/host_diagnostic.c',ROOT/'src/pc/host_zero_callback.c',ROOT/'src/pc/exe_helpers.c',data,ROOT/'calibration/host_file_callback_formats.c',ROOT/'calibration/host_resource_storage.c']
 files.extend([ROOT/'calibration/host_pool_shrink.c',args.pool_source.resolve(),ROOT/'calibration/host_pool_storage.c',ROOT/'calibration/host_pool_formats.c',ROOT/'calibration/host_pool_shrink_format.c'])
 files.extend([args.sfx_source.resolve(),args.sfx_data.resolve()])
 library=verified_copy(ORACLE,out);watched={Path(__file__),ORACLE,runtime,ROOT/'recovery.json',ROOT/'toolchains/manifest.json',ROOT/'scripts/host_resize_differential.py',ROOT/'scripts/crt_copy_evidence.py',Path(library['archive'])}
 paths=verify_data(args.paths_source.resolve(),args.paths_definition.resolve(),ORACLE,out/'paths');watched.update(Path(p)for p in paths['inputs'])
 watched.add(ARCHIVE)
 for p in files:watched.update(context_files(p))
 for name in ['match.py','masked.py','coff.py','pe.py','env.py','title_behavioral.py']:watched.add(ROOT/'scripts'/name)
 pins={str(p):sha(p)for p in watched};compiles=[];coffs=[];measurements=[];native=PE(ORACLE)
 for p in files:
  obj,cmd=compile_source(p,['/O2']);coffs.append(COFF(obj));compiles.append(dict(source=str(p),source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=cmd))
 objects=[x['object']for x in compiles]+[paths['object'],library['object']];coffs.extend([COFF(paths['object']),COFF(library['object'])])
 for role,(sym,rv,n)in PROFILE.items():
  if role=='copy':measurements.append(dict(role=role,symbol=sym,rva=hex(rv),native_size=n,compiled_size=821,library_identity=True));continue
  owners=[i for i,c in enumerate(coffs)if any(s['name']==sym and s['section']>0 for s in c.symbols.values())];assert len(owners)==1,(role,owners);i=owners[0]
  raw=compare(Path(compiles[i]['object']),sym,ORACLE,rv,n);d=masked_compare(Path(compiles[i]['object']),sym,ORACLE,rv,n)
  if role in ['empty','diagnostic','zero','true']:assert raw['exact'],role
  measurements.append(dict(role=role,symbol=sym,rva=hex(rv),native_size=n,compiled_size=coffs[i].function(sym)['size'],raw=raw,diagnostic=d))
 fields=[];spec=[(i,*v)for i,vv in [(8,FILE_DATA)]for v in vv]
 for name,rv in MUSIC_DATA:spec.append((7,name,rv,len(native.read_rva(rv,128).split(b'\0')[0])+1))
 for name,rv in SFX_DATA:spec.append((16,name,rv,len(native.read_rva(rv,256).split(b'\0')[0])+1))
 spec.extend([(7,'_g_host_music_bank_headers_rva_68e790',0x68e790,64),(9,'_g_resource_prepare_scalar',0x352b0,4)])
 for name,rv,n in SPECS:
  i=next(i for i,c in enumerate(coffs)if any(s['name']==name and(s['section']>0 or s['section']==0 and s['value']>0)for s in c.symbols.values()));spec.append((i,name,rv,n))
 for i,name,rv,n in spec:
  c=coffs[i];s=next(s for s in c.symbols.values()if s['name']==name and(s['section']>0 or s['section']==0 and s['value']>0))
  if s['section']==0:assert s['value']==n;raw=bytes(n)
  else:
   sec=c.sections[s['section']-1];assert s['value']+n<=sec['size']and not sec['relocations'];raw=sec['data'][s['value']:s['value']+n]if sec['data']else bytes(n)
  ns=next(s for s in native.pe.sections if s.VirtualAddress<=rv and rv+n<=s.VirtualAddress+s.Misc_VirtualSize);virtual=rv>=ns.VirtualAddress+ns.SizeOfRawData
  assert raw==(bytes(n)if virtual else native.read_rva(rv,n)),name
  fields.append(dict(symbol=name,rva=hex(rv),bytes=n,data=raw.hex(),sha256=hashlib.sha256(raw).hexdigest(),origin='virtual_zero'if virtual else'file_backed'))
 assert pins=={str(p):sha(p)for p in watched}
 return dict(inputs=pins,compiles=compiles,objects=objects,library=library,paths=paths,measurements=measurements,data=fields)

def prepare(b):
 symbols={'_g_host_engine_interface':TABLE,'_g_resource_filename':FILENAME,'_g_host_music_paths_rva_35178':PATH_TABLE};segments=[];where={};coffs=[COFF(x)for x in b['objects']];cursor=0x20000000
 for x in b['data']:symbols[x['symbol']]=BASE+int(x['rva'],0);segments.append((symbols[x['symbol']],bytes.fromhex(x['data'])))
 for x in b['paths']['fixture_segments']:segments.append((BASE+int(x['rva'],0),bytes.fromhex(x['data'])))
 endpoints={MODEL+i*32:role for i,role in enumerate(MODELS)}
 for a,role in endpoints.items():symbols[MODELS[role][0]]=a
 for k,c in enumerate(coffs):
  for i,s in enumerate(c.sections,1):
   if s['flags']&0x20 and s['size']:where[k,i]=cursor;cursor+=(s['size']+15)&~15
  for s in c.symbols.values():
   if s['storage']==2 and s['section']>0 and(k,s['section'])in where:
    if s['name']in [v[0]for v in MODELS.values()]:
     role=next(r for r,v in MODELS.items()if v[0]==s['name']);old=next(a for a,r in endpoints.items()if r==role);del endpoints[old];endpoints[where[k,s['section']]+s['value']]=role
    else:assert s['name']not in symbols,s['name']
    symbols[s['name']]=where[k,s['section']]+s['value']
 for k,c in enumerate(coffs):
  for i,s in enumerate(c.sections,1):
   if(k,i)not in where:continue
   raw=bytearray(s['data']);assert len(raw)==s['size']
   for rel in s['relocations']:
    local=next((x for x in c.symbols.values()if x['name']==rel['symbol']and x['section']>0),None);target=where[k,local['section']]+local['value']if local else symbols[rel['symbol']]
    at=rel['offset'];assert rel['type']in [6,20];value=struct.unpack_from('<I',raw,at)[0]+target-(where[k,i]+at+4 if rel['type']==20 else 0);struct.pack_into('<I',raw,at,value&0xffffffff)
   segments.append((where[k,i],bytes(raw)))
 addresses={role:symbols[sym]for role,(sym,rv,n)in PROFILE.items()};allowed=[(addresses[x['role']],addresses[x['role']]+x['compiled_size'],x['role'])for x in b['measurements']]
 return dict(segments=segments,endpoints=endpoints,addresses=addresses,allowed=allowed)

def execute(native,b,p,s):
 pe=pefile.PE(str(ORACLE));u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(BASE,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
 if native:u.mem_write(BASE,pe.get_memory_mapped_image())
 u.mem_map(0x20000000,0x100000);u.mem_map(DATA,0x50000);u.mem_map(0x62000000,0x40000);u.mem_map(STACK,0x10000);u.mem_map(MODEL,0x1000)
 if native:addresses={role:BASE+rv for role,(sym,rv,n)in PROFILE.items()};endpoints={BASE+rv:role for role,(sym,rv)in MODELS.items()};allowed=[(BASE+rv,BASE+rv+n,role)for role,(sym,rv,n)in PROFILE.items()]
 else:
  addresses=p['addresses'];endpoints=p['endpoints'];allowed=p['allowed']
  for a,raw in p['segments']:u.mem_write(a,raw)
 slot=s.get('slot',1)&255;header_off=s.get('header_offset',0x100);sample_off=s.get('sample_offset',0x280);file_bytes=s.get('file_bytes',0x400)
 capacity=0x9000 if s.get('allocation_fail')else (file_bytes&0xfffff800)+0x1000;assert 0x1000<=capacity<=0x9000
 heap_start=0x62000040;heap_bytes=capacity+16 if s.get('allocation_fail')else 0x10000;heap_end=heap_start+heap_bytes-8
 BLOBS=[0x62020040 if s.get('allocation_fail')else heap_end-capacity*2-8,heap_end-capacity]
 content=bytearray(max(0x400,min(file_bytes,0x8000)))
 for i,v in enumerate([sample_off,header_off,0x10,0x20,0x30,0x40,0x50]):struct.pack_into('<I',content,i*4,v)
 for i,v in enumerate(s.get('samples',[])):struct.pack_into('<h',content,header_off+0x44+i*2,v)
 for i,v in enumerate(s.get('parameters',[])):content[header_off+0x144+i]=v
 if 'archive_member'in s:
  key,offset,n=struct.unpack_from('<III',ARCHIVE_BYTES,s['archive_member']*12);assert n==file_bytes
  content=bytearray(ARCHIVE_BYTES[offset:offset+n]);sample_off,header_off=struct.unpack_from('<II',content);assert header_off+0x1c4<=len(content)
 contents=[bytes(content),bytes(content)];alternate=bytearray(content)
 if s.get('preload_swap'):struct.pack_into('<h',alternate,header_off+0x46,9)
 contents[1]=bytes(alternate)
 u.mem_write(heap_start-32,bytes([0xd1])*32+bytes([0x5d])*(heap_end+4-heap_start)+bytes([0xd2])*32)
 u.mem_write(heap_start,pack(0x80000000+((heap_end-heap_start)>>2)));u.mem_write(heap_end,pack(1))
 if s.get('fragmented_pool'):
  assert not s.get('allocation_fail');barrier=heap_start+0x2000;high=barrier+16;u.mem_write(heap_start,pack(0x80000800));u.mem_write(barrier,pack(0x40000004)+pack(DATA+0x4900));u.mem_write(DATA+0x4900,pack(barrier+8));u.mem_write(high,pack(0x80000000+((heap_end-high)>>2)))
 pool=bytearray(48);struct.pack_into('<I',pool,0,heap_start);struct.pack_into('<I',pool,16,heap_end);u.mem_write(POOL,bytes(pool))
 table=bytearray(bytes([0x2d])*1000);table[4:8]=pack(CONTEXTS[0]);u.mem_write(TABLE,bytes(table));u.mem_write(FILENAME-32,bytes([0xe1])*32+bytes([0xe2])*512+bytes([0xe3])*32)
 for a,text in zip(PATHS,[b'X:\\first\\\0',b'Y:\\second\\\0']):u.mem_write(a,text)
 if not s.get('native_paths'):
  for i in range(256):u.mem_write(PATH_TABLE+i*8,pack(PATHS[0])+pack(0x11110000+i))
 bank_cells=bytearray(64)
 for i in s.get('occupied',[]):bank_cells[i*4:i*4+4]=pack(BLOBS[0]+header_off)
 if s['role']in ['bank_preload','program_attributes','tone_attributes']and not s.get('null_header'):bank_cells[(s.get('slot_word',1)&15)*4:(s.get('slot_word',1)&15)*4+4]=pack(BLOBS[0]+header_off)
 u.mem_write(BANKS-32,bytes([0xb1])*32+bank_cells+bytes([0xb2])*32)
 for i,a in enumerate(CONTEXTS):
  raw=bytearray(bytes([0xa1+i])*CTX_SIZE)
  fields={0x9c:('H',0),0xb8:('I',10+i),0x1afc:('I',s.get('handle',0)&0xffffffff),0x1b04:('I',s.get('music_base',128)),0x1b08:('I',s.get('effects_base',128)),0x1b0c:('I',s.get('music_user',256)),0x1b10:('I',s.get('effects_user',256)),0x1b14:('I',0x22334455),0x1b18:('I',0x33445566),0x1b1c:('H',s.get('cached',0)&65535),0x13c+slot*4:('I',BLOBS[1])}
  for off,(fmt,v)in fields.items():struct.pack_into('<'+fmt,raw,off,v)
  if i==2:struct.pack_into('<I',raw,0x13c+slot*4,0)
  if s['role']=='load_sfx' or 'load_sfx'in s.get('commands',[]):
   ids={struct.unpack_from('<h',content,header_off+0x44+j*2)[0]for j in range(128)}
   if s.get('preload_swap'):ids.add(9)
   for value in ids:
    if value:
     assert 0<=0x2f0+value*3<CTX_SIZE-2,(s,value);raw[0x2f0+value*3]=s.get('initial_claim',0xff)
  u.mem_write(a-32,bytes([0xc1])*32+raw+bytes([0xc2])*32)
 output=BLOBS[0]+header_off+s['output_alias']if 'output_alias'in s else QUERY_OUT
 u.mem_write(QUERY_OUT-32,bytes([0xe4])*32+bytes([0xcc])*32+bytes([0xe5])*32)
 def frame_for(role):
  header=0 if s.get('register_null')else BLOBS[0]+header_off
  args={'allocate':[CONTEXTS[2]+0x13c+slot*4,capacity,0x10],'apply':[],'signal':[],'set_bases':[s.get('set_music',128),s.get('set_effects',256)],'initialize':[],'load':[s.get('bank',0),s.get('slot',1)],'load_sfx':[s.get('bank',0),s.get('slot',1)],'bank_register':[header,s.get('slot_word',-1)],'bank_preload':[BLOBS[0]+sample_off,s.get('slot_word',1)],'program_attributes':[s.get('slot_word',1),s.get('program_word',0),0 if s.get('null_output')else output],'tone_attributes':[s.get('slot_word',1),s.get('program_word',0),s.get('tone_word',0),0 if s.get('null_output')else output]}[role]
  return pack(STOP)+b''.join(pack(v)for v in args)+bytes([0xf7])*32
 trace=[];counts={};pending=[];illegal=[];writes=[];instructions=0;handles={};argument_end=ESP+4;phase='preparation'
 def effect(role):
  key=role+'_'+str(counts.get(role,0));index=s.get('effects',{}).get(key)
  if index is not None:u.mem_write(TABLE+4,pack(CONTEXTS[index]));trace.append(dict(effect=key,context=index))
 def on_write(machine,access,address,n,value,user):
  ranges=[(a,a+CTX_SIZE)for a in CONTEXTS]+[(BANKS,BANKS+64),(SCALAR,SCALAR+4),(heap_start,heap_end+4),(QUERY_OUT,QUERY_OUT+32),(ESP-0x1000,ESP),(ESP+4,argument_end)]
  if not any(lo<=address and address+n<=hi for lo,hi in ranges):writes.append([hex(address),n]);machine.emu_stop()
 def on_code(machine,address,n,user):
  nonlocal instructions
  instructions+=1
  if instructions>200000:illegal.append('instruction cap');machine.emu_stop();return
  if address==STOP:machine.emu_stop();return
  if address in endpoints:pending.append(address);machine.emu_stop();return
  if address==addresses['allocate']:
   sp=machine.reg_read(UC_X86_REG_ESP);owner,n,flags=[u32(machine,sp+i)for i in [4,8,12]];trace.append(dict(call='allocate',phase=phase,owner=hex(owner),requested=n,flags=flags,previous=hex(u32(machine,owner))))
  elif address in [addresses['program_attributes'],addresses['tone_attributes']]:
   sp=machine.reg_read(UC_X86_REG_ESP);trace.append(dict(call='program_attributes'if address==addresses['program_attributes']else'tone_attributes',bank=u32(machine,sp+4)&65535,program=u32(machine,sp+8)&65535))
  elif address==addresses['resize']:
   sp=machine.reg_read(UC_X86_REG_ESP);old,n,pool,owner_only=[u32(machine,sp+i)for i in [4,8,12,16]];trace.append(dict(call='resize',old=hex(old),requested=n,pool=pool,owner_only=owner_only))
  elif address==addresses['coalesce']:
   sp=machine.reg_read(UC_X86_REG_ESP);trace.append(dict(call='coalesce',pool=u32(machine,sp+4),heap_before=hashlib.sha256(bytes(machine.mem_read(heap_start,heap_end+4-heap_start))).hexdigest()))
  elif address==addresses['copy']:
   sp=machine.reg_read(UC_X86_REG_ESP);trace.append(dict(call='copy',destination=hex(u32(machine,sp+4)),source=hex(u32(machine,sp+8)),bytes=u32(machine,sp+12)))
  elif address==addresses['empty']:
   sp=machine.reg_read(UC_X86_REG_ESP);caller=u32(machine,sp);site=next(tag for lo,hi,tag in allowed if lo<=caller<hi);n=3 if site=='apply'else 1 if site=='signal'else 0
   trace.append(dict(call='empty',site=site,defined_word_arguments=[u32(machine,sp+4+i*4)&65535 for i in range(n)]))
  elif address==addresses['diagnostic']:
   sp=machine.reg_read(UC_X86_REG_ESP);fmt=cstring(machine,u32(machine,sp+4));event=dict(call='diagnostic',phase=phase,format=fmt)
   if fmt.startswith('LoadMusic')or fmt.startswith('LoadSfx')or 'attribute number::'in fmt:event['arguments']=[u32(machine,sp+8),u32(machine,sp+12)]
   if '%s'in fmt:event['text']=cstring(machine,u32(machine,sp+8))
   if fmt.startswith('Loading'):event.update(destination=hex(u32(machine,sp+12)),requested=u32(machine,sp+16))
   if fmt.startswith('ReMalloc'):event.update(pool=u32(machine,sp+8),payload=hex(u32(machine,sp+12)),bytes=u32(machine,sp+16))
   if fmt.startswith('Malloc Heap'):event.update(pool=u32(machine,sp+8),payload=hex(u32(machine,sp+12)),bytes=u32(machine,sp+16))
   if fmt.startswith('Malloc FAILED'):event.update(owner=hex(u32(machine,sp+8)),previous=hex(u32(machine,sp+12)))
   trace.append(event)
  elif address in [addresses['zero'],addresses['true']]:
   sp=machine.reg_read(UC_X86_REG_ESP);trace.append(dict(call='zero'if address==addresses['zero']else'true',argument=u32(machine,sp+4)&65535))
  if not any(lo<=address<hi for lo,hi,tag in allowed):illegal.append(hex(address));machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write)
 def invoke(function_role):
  nonlocal argument_end
  frame=frame_for(function_role);u.mem_write(ESP,frame);u.reg_write(UC_X86_REG_ESP,ESP)
  u.reg_write(UC_X86_REG_EFLAGS,0x202)
  argument_end=ESP+len(frame)-32
  for reg,v in zip(NV,SEEDS):u.reg_write(reg,v)
  pc=addresses[function_role]
  for _ in range(400):
   pending.clear();u.emu_start(pc,STOP,count=200001)
   if illegal or writes or u.reg_read(UC_X86_REG_EIP)==STOP:break
   assert pending,'unexpected stop';address=pending[-1];sp=u.reg_read(UC_X86_REG_ESP);role=endpoints[address];counts[role]=counts.get(role,0)+1;number=counts[role]
   if role=='format':
    dest,fmt,a,c=[u32(u,sp+i)for i in [4,8,12,16]];assert dest==FILENAME and cstring(u,fmt)=='%s%s';text=cstring(u,a)+cstring(u,c);assert len(text)<512;u.mem_write(dest,text.encode()+b'\0');result=len(text);trace.append(dict(call=role,text=text))
   elif role=='open':
    path=cstring(u,u32(u,sp+4));result=2+number;handles[result]=1 if path.startswith('Y:')else 0;trace.append(dict(call=role,path=path,mode=u32(u,sp+8),flags=u32(u,sp+12),result=result))
   elif role=='seek':
    result=file_bytes;trace.append(dict(call=role,handle=u32(u,sp+4),offset=u32(u,sp+8),origin=u32(u,sp+12),result=result))
    if s.get('path_swap')and number==1:u.mem_write(PATH_TABLE+(s.get('bank',0)&255)*8,pack(PATHS[1]))
   elif role=='close':result=0;trace.append(dict(call=role,handle=u32(u,sp+4),result=0))
   elif role=='read':
    handle,dest,n=[u32(u,sp+i)for i in [4,8,12]];payload=contents[handles[handle]][:n];assert heap_start+8<=dest and dest+len(payload)<=heap_end;u.mem_write(dest,payload);result=len(payload);trace.append(dict(call=role,handle=handle,destination=hex(dest),requested=n,returned=result))
   else:
    value=u32(u,sp+4);result=0x12345678;trace.append(dict(call='sample_preload',index=value))
    if s.get('preload_swap')and number==1:u.mem_write(BANKS+(s.get('slot_word',slot)&15)*4,pack(BLOBS[1]+header_off))
   effect(role);pc=u32(u,sp);u.reg_write(UC_X86_REG_ESP,sp+4);u.reg_write(UC_X86_REG_EAX,result&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xdeadbeef);u.reg_write(UC_X86_REG_EDX,0xcafebabe)
  else:raise AssertionError('endpoint cap')
  assert not illegal and not writes,(s,illegal,writes)
  assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4
  assert u.reg_read(UC_X86_REG_EFLAGS)&0x400==0
  assert all(u.reg_read(reg)==v for reg,v in zip(NV,SEEDS))
  assert bytes(u.mem_read(ESP,4))==pack(STOP)and bytes(u.mem_read(argument_end,32))==bytes([0xf7])*32
  for a in CONTEXTS:assert bytes(u.mem_read(a-32,32))==bytes([0xc1])*32 and bytes(u.mem_read(a+CTX_SIZE,32))==bytes([0xc2])*32
  assert bytes(u.mem_read(BANKS-32,32))==bytes([0xb1])*32 and bytes(u.mem_read(BANKS+64,32))==bytes([0xb2])*32
  assert bytes(u.mem_read(FILENAME-32,32))==bytes([0xe1])*32 and bytes(u.mem_read(FILENAME+512,32))==bytes([0xe3])*32
  assert bytes(u.mem_read(QUERY_OUT-32,32))==bytes([0xe4])*32 and bytes(u.mem_read(QUERY_OUT+32,32))==bytes([0xe5])*32
  assert bytes(u.mem_read(heap_start-32,32))==bytes([0xd1])*32 and bytes(u.mem_read(heap_end+4,32))==bytes([0xd2])*32,(s,bytes(u.mem_read(heap_start-32,32)).hex(),bytes(u.mem_read(heap_end+4,32)).hex())
  value=u.reg_read(UC_X86_REG_EAX)
  return value&65535 if function_role in ['bank_register','bank_preload','program_attributes','tone_attributes']else value if function_role in ['allocate','load','load_sfx']else None
 reserved=invoke('allocate');assert reserved==BLOBS[1],(s,hex(reserved),hex(BLOBS[1]))
 preparation=dict(reserved_payload=hex(reserved),pool=bytes(u.mem_read(POOL,48)).hex(),heap=hashlib.sha256(bytes(u.mem_read(heap_start,heap_end+4-heap_start))).hexdigest(),valid_explicit_free_pool_and_actual_reservation=True)
 for a,raw in zip(BLOBS,contents):u.mem_write(a,raw)
 phase='caller'
 calls=[]
 for role in s.get('commands',[s['role']]):calls.append(dict(role=role,result=invoke(role),context_hashes=[hashlib.sha256(bytes(u.mem_read(a,CTX_SIZE))).hexdigest()for a in CONTEXTS]))
 digest=lambda a,n:hashlib.sha256(bytes(u.mem_read(a,n))).hexdigest()
 return dict(events=trace,preparation=preparation,calls=calls,query_output=bytes(u.mem_read(output,32)).hex(),contexts=[digest(a,CTX_SIZE)for a in CONTEXTS],table=digest(TABLE,1000),banks=digest(BANKS,64),paths=digest(PATH_TABLE,312 if s.get('native_paths')else 2048),path_literals=[digest(BASE+int(x['rva'],0),x['bytes'])for x in b['paths']['literals']]if s.get('native_paths')else [],filename=digest(FILENAME,512),blobs=[digest(a,len(content))for a in BLOBS],heap=digest(heap_start,heap_end+4-heap_start),pool=digest(POOL,48),prepare_scalar=u32(u,SCALAR),guards_stack_nonvolatiles_and_whitelists=True)

def scenarios():
 cases=[]
 def add(role,name,**values):cases.append(dict(role=role,name=name,**values))
 for base,user in [(0,256),(128,256),(0x10000,256),(0x10000,0x10000),(0xffffffff,0xffffffff)]:
  for cached in [0,128,65535]:
   for handle in [0xffffffff,0,0x1234ffff]:add('apply','gain_%x_%x_%x_%x'%(base,user,cached,handle),music_base=base,music_user=user,effects_base=user,effects_user=base,cached=cached,handle=handle)
 for h in [0xffffffff,65535,0x1234ffff,0,1]:add('signal','signal_'+str(h),handle=h)
 add('initialize','initialize',music_user=7,effects_user=9)
 for m,e in [(0,0),(128,256),(0xffffffff,0xffffffff),(0x10000,0x10000)]:add('set_bases','set_%x_%x'%(m,e),set_music=m,set_effects=e)
 for word in [-32768,-2,-1,0,1,15,16,32767,0xabcd0001]:
  for occupied in [[],list(range(16)),[0,7,15]]:add('bank_register','register_%x_%d'%(word&0xffffffff,len(occupied)),slot_word=word,occupied=occupied)
 add('bank_register','register_null',slot_word=-1,register_null=True)
 for slot in [0,1,15]:
  add('bank_preload','preload_null_'+str(slot),slot_word=slot,null_header=True)
  add('bank_preload','preload_empty_'+str(slot),slot_word=slot)
  add('bank_preload','preload_mixed_'+str(slot),slot_word=slot,samples=[1,0,-1,1199,1200])
 add('bank_preload','preload_full',slot_word=1,samples=[1]*128)
 add('bank_preload','preload_live_header',slot_word=1,samples=[7,2],preload_swap=True)
 for bank in [0,17,0x12340101,0xffffffff]:
  for slot in [0,1,15,16,255,0xabcd0001]:add('load','load_%x_%x'%(bank,slot),bank=bank,slot=slot)
 for n in [0,1,2047,2048,2049,0x7fffffff,0xfffff800,0xffffffff]:add('load','alloc_fail_%x'%n,file_bytes=n,allocation_fail=True)
 add('load','load_rounding_801',file_bytes=2049)
 for offset in [0,1,3,4,7,8,0x104]:add('load','load_shrink_offset_%x'%offset,header_offset=offset)
 add('load','load_samples',samples=[1,0,-1,1199,1200])
 add('load','load_path_reload',path_swap=True)
 add('load','load_context_reload',effects={'seek_1':1,'read_1':2})
 add('load','load_sample_context',samples=[1,2],effects={'preload_1':1})
 for bank in range(39):add('load','native_path_'+str(bank),bank=bank,native_paths=True)
 add('load','native_path_masked_38',bank=0xabcd0026,native_paths=True)
 add('sequence','controls_to_load',commands=['initialize','set_bases','load','apply','signal'])
 add('sequence','register_preload_apply',slot_word=1,samples=[1,2],commands=['bank_register','bank_preload','apply'])
 values=[0]*128;values[0]=7;values[1]=-1;values[126]=1199;values[127]=1200;parameters=[(j*7+9)&255 for j in range(128)]
 for role in ['program_attributes','tone_attributes']:
  for bank in [0,1,15,0xabcd0001]:
   for program in [-32768,-1,0,1,126,127,128,32767,0xabcd007f,0xabcdffff]:
    for missing in [False,True]:add(role,'query_%s_%x_%x_%d'%(role,bank,program&0xffffffff,missing),slot_word=bank,program_word=program,null_header=missing,samples=values,parameters=parameters,tone_word=0xdeadffff)
  add(role,'query_null_out_'+role,slot_word=1,program_word=-1,null_output=True)
  add(role,'query_alias_'+role,slot_word=1,program_word=0,samples=values,parameters=parameters,output_alias=0x40 if role=='program_attributes'else 0x140)
 for bank in [0,17,0x12340101,0xffffffff]:
  for slot in [0,1,15,16,255,0xabcd0001]:add('load_sfx','sfx_%x_%x'%(bank,slot),bank=bank,slot=slot)
 add('load_sfx','sfx_samples',samples=[7,0,-1,1199,1200],parameters=parameters)
 add('load_sfx','sfx_signed_sample',samples=[-1],parameters=parameters)
 add('load_sfx','sfx_duplicate_claim',samples=[7,7],parameters=parameters,initial_claim=1)
 add('load_sfx','sfx_last_program',samples=values,parameters=parameters)
 add('load_sfx','sfx_only_program127',samples=[0]*127+[7],parameters=parameters)
 add('load_sfx','sfx_all_programs',samples=[7]*128,parameters=parameters)
 add('load_sfx','sfx_header_reload',samples=[7,2],parameters=parameters,preload_swap=True)
 add('load_sfx','sfx_path_reload',path_swap=True)
 add('load_sfx','sfx_context_reload',samples=[7],parameters=parameters,effects={'seek_1':1,'read_1':2})
 add('load_sfx','sfx_sample_context',samples=[7],parameters=parameters,effects={'preload_1':1})
 add('load_sfx','sfx_rounding_801',file_bytes=2049,samples=[7],parameters=parameters)
 for n in [0,2049,0x7fffffff,0xffffffff]:add('load_sfx','sfx_alloc_fail_%x'%n,file_bytes=n,allocation_fail=True)
 for off in [0,4,8,0x104]:add('load_sfx','sfx_shrink_%x'%off,header_offset=off)
 add('sequence','sfx_twice',samples=[7,2],parameters=parameters,commands=['load_sfx','load_sfx'])
 add('sequence','music_then_sfx',samples=[7,2],parameters=parameters,commands=['load','load_sfx','apply'])
 add('load','load_fragmented_pool',fragmented_pool=True)
 add('load_sfx','sfx_fragmented_pool',samples=[7,2],parameters=parameters,fragmented_pool=True)
 add('sequence','load_until_exhausted',commands=['load']*17)
 lookup=__import__('host_music_paths_data').asset_lookup(args.paths_definition.resolve(),ORACLE,ARCHIVE);seen=set()
 for row in lookup['rows']:
  item=next(x for x in row['files']if x['filename']=='SFX.SFX')
  if item['found']and item['directory_record'][0]not in seen:
   member,offset,n=item['directory_record'];seen.add(member);add('load_sfx','sfx_archive_'+str(member),bank=row['index'],native_paths=True,file_bytes=n,archive_member=member)
 return cases

def main():
 b=build(args.source.resolve(),args.data.resolve(),OUT/'build');p=prepare(b);ss=scenarios();cases=[]
 for role in ['set_bases','initialize','bank_register','bank_preload']:
  assert next(x for x in b['measurements']if x['role']==role)['diagnostic']['masked_equal']
 for s in ss:
  a=execute(True,b,p,s);c=execute(False,b,p,s);assert a==c,(s,a,c);cases.append(dict(scenario=s,equal=True,native=a,source=c))
 text=args.source.resolve().read_text();wrong={
  'gain_compares_only_cached_word':text.replace('(MusicU32)MUSIC_WORD(0x1b1c)!=MUSIC_DWORD(0x1b14)','(MusicU32)MUSIC_WORD(0x1b1c)!=MUSIC_WORD(0x1b14)'),
  'auto_slot_rejected':text.replace('if (slot < -1 || slot >= 16)','if ((unsigned short)slot >= 16)'),
  'allocation_rounds_up':text.replace('aligned=bytes&0xfffff800UL;','aligned=(bytes+2047UL)&0xfffff800UL;'),
  'bank_id_loses_upper_nibble':text.replace('bank &= 0xff;','bank &= 0x0f;')}
 a=text.index('short __cdecl host_music_bank_preload_c090');z=text.index('int __cdecl host_music_load_1110',a);block=text[a:z]
 block=block.replace('    int offset;','    int offset;\n    void *saved_header=g_host_music_bank_headers_rva_68e790[slot];').replace('(MusicU8 *)g_host_music_bank_headers_rva_68e790[slot]+offset','(MusicU8 *)saved_header+offset')
 wrong['preload_caches_header']=text[:a]+block+text[z:]
 names={'gain_compares_only_cached_word':'gain_10000_100_0_0','auto_slot_rejected':'register_ffffffff_0','allocation_rounds_up':'load_rounding_801','bank_id_loses_upper_nibble':'load_11_1','preload_caches_header':'preload_live_header'};negatives=[]
 for name,code in wrong.items():
  assert code!=text,name
  lane=OUT/name;lane.mkdir(exist_ok=True);code=re.sub(r'#include "([^"]+)"',lambda m:'#include "'+(args.source.resolve().parent/m.group(1)).resolve().as_posix()+'"',code);source=lane/'wrong.c';source.write_bytes(code.encode())
  bad=build(source,args.data.resolve(),lane/'build');plan=prepare(bad);s=next(x for x in ss if x['name']==names[name]);a=execute(True,b,p,s);c=execute(False,bad,plan,s);assert a!=c,name
  negatives.append(dict(name=name,detected=True,scenario=s,native=a,wrong=c,build=bad))
 text=args.sfx_source.resolve().read_text();wrong_sfx={
  'loader_includes_program127':(text.replace('program<127','program<=127'),'sfx_only_program127'),
  'unsigned_sample_test':(text.replace('attributes.sample_006!=0','attributes.sample_006>0'),'sfx_signed_sample'),
  'duplicate_same_slot_suppressed':(text.replace('SFX_BYTE(0x2f0+(long)attributes.sample_006*3)!=0xff','SFX_BYTE(0x2f0+(long)attributes.sample_006*3)!=0xff && SFX_BYTE(0x2f0+(long)attributes.sample_006*3)!=(MusicU8)slot'),'sfx_duplicate_claim'),
  'tone_output_clears_only16':(text.replace('memset(out,0,sizeof(*out));\n        out->byte_002','memset(out,0,16);\n        out->byte_002'),'query_tone_attributes_1_0_0'),
  'program_output_clears_only8':(text.replace('memset(out,0,sizeof(*out));\n        out->sample_006','memset(out,0,8);\n        out->sample_006'),'query_program_attributes_1_0_0'),
  'tone_captured_before_clear':(text.replace('memset(out,0,sizeof(*out));\n        out->byte_002=*((MusicU8 *)g_host_music_bank_headers_rva_68e790[bank]+0x144+program);','MusicU8 saved=*((MusicU8 *)g_host_music_bank_headers_rva_68e790[bank]+0x144+program);\n        memset(out,0,sizeof(*out));\n        out->byte_002=saved;'),'query_alias_tone_attributes')}
 original_sfx=args.sfx_source
 for name,(code,case)in wrong_sfx.items():
  assert code!=text,name
  lane=OUT/name;lane.mkdir(exist_ok=True);code=re.sub(r'#include "([^"]+)"',lambda m:'#include "'+(original_sfx.resolve().parent/m.group(1)).resolve().as_posix()+'"',code);source=lane/'wrong.c';source.write_bytes(code.encode());args.sfx_source=source
  bad=build(args.source.resolve(),args.data.resolve(),lane/'build');plan=prepare(bad);s=next(x for x in ss if x['name']==case);a=execute(True,b,p,s);c=execute(False,bad,plan,s);assert a!=c,name
  negatives.append(dict(name=name,detected=True,scenario=s,native=a,wrong=c,build=bad))
 args.sfx_source=original_sfx
 assert b['inputs']=={path:sha(path)for path in b['inputs']}
 members=[]
 for s in ss:
  if 'archive_member'in s:
   key,off,n=struct.unpack_from('<III',ARCHIVE_BYTES,s['archive_member']*12);payload=ARCHIVE_BYTES[off:off+n];members.append(dict(scenario=s['name'],bank=s['bank'],directory_index=s['archive_member'],key=hex(key),offset=off,bytes=n,sha256=hashlib.sha256(payload).hexdigest(),root_offsets=list(struct.unpack_from('<II',payload))))
 exhaustion=next(c for c in cases if c['scenario']['name']=='load_until_exhausted');assert [r['result']for r in exhaustion['native']['calls']]==[0]*15+[0xffffffff]*2
 report=dict(scope=__doc__,build=b,case_count=len(cases),all_equal=True,cases=cases,negative_controls=negatives,archive_members=members,archive_sha256=sha(ARCHIVE),native_path_cases=sum(bool(x['scenario'].get('native_paths'))for x in cases),full_sequences=sum(x['scenario']['role']=='sequence'for x in cases),actual_family_invocations=sum(len(x['native']['calls'])for x in cases),allocator_is_modeled=False,limits=['Actual music/SFX loaders,attribute queries,file-length/synchronous-load,prepare,empty,diagnostic,zero,return-true,pool allocator,resize and coalescer execute. Identified unchanged CRT copy is linked but these owner-only loaders do not call it;separate resize tests execute it.','Virtual I/O,formatting and sample-device entry remain models;23SFX cases replay exact original FS member bytes. Allocation has no entry/return hook. No original archive opener,Windows arena allocation or audio device executes.','Every case starts from explicit valid pool metadata/free-block inputs and runs one actual allocation reserving the alternate-context block. Caller allocations then execute normally. Failure cases exhaust the pool;repeated loading naturally exhausts the normal64KiBfixture. These are test arenas,not production capacity evidence.','Sixty-three cases consume the actual39-record initialized path prefix and25literal identities. Other cases substitute a256-record synthetic external table for adversarial/reload discriminators;this proves no production capacity or legal access beyond the prefix.','Context pointers,512-byte filename and pool backing are synthetic preconditions. Non-archive file bytes are synthetic. The two fragmented cases add a valid pinned barrier between free blocks.','Prior modeled allocation-return mutations are retired;path/context interventions now occur at the still-modeled seek/read boundaries. They test caller reloads,not actual callee side-effect reachability.','Only defined WORD arguments/returns are compared for narrow APIs. Native bank-query helpers do not bound bank indices;valid prefix indices and bounded context-accessing sample IDs remain preconditions.','Native1110/1330and allocator4570reuse incoming parameter slots as scratch. Return address,caller guard,ESP,nonvolatiles and direction flag are checked;argument-slot bytes are not promised unchanged.','SFX caller ignores query failures. Undefined local attributes on failed queries are not repaired;loader cases keep a valid registered header. Standalone query failures preserve output bytes.'])
 dest=OUT/'host_music_differential.json';dest.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,negatives=len(negatives),report=str(dest))))
if __name__=='__main__':main()
