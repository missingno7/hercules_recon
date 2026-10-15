"""Original/source archive pipeline; actual game code with read-only modeled CRT streams."""
import argparse,hashlib,json,re,struct,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,DEFAULT_TOOLCHAIN,verify_toolchain
from masked import masked_compare
from coff import COFF
from pe import PE
from title_behavioral import context_files
from host_resize_differential import verified_copy
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
BASE=0x400000;DIRECTORY=BASE+0x5177d0;POSITIONS=BASE+0x51a7d0;SCRATCH=BASE+0x51b7d0;STREAM_CELL=BASE+0x51bbd0;COUNT_CELL=BASE+0x51bbd4;LAST=BASE+0x365a4;PUBLIC_COUNT=BASE+0x56e638;SLOTS=BASE+0x56e640
DATA=0x61000000;BUFFER=DATA+0x10000;BUFFER_BYTES=0x2000;STACK=0x63000000;ESP=STACK+0x8000;STOP=STACK+0xf000;MODEL=0x65000000;STREAM=0x66000040
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]
SOURCE_FILES=['calibration/virtual_file.c','calibration/archive_backend.c','calibration/archive_names.c','calibration/archive_read.c','calibration/archive_positions.c','calibration/archive_storage.c','calibration/archive_array_prefixes.c','calibration/public_file_handles.c']
PROFILE={'vopen':('_virtual_file_open',0xa210,112),'vseek':('_virtual_file_seek',0xa280,112),'vread':('_virtual_file_read',0xa2f0,80),'vclose':('_virtual_file_close',0xa350,80),'aopen':('_archive_open',0x5130,192),'aseek':('_archive_seek',0x52c0,128),'aread':('_archive_read',0x5210,176),'aclose':('_archive_close',0x51f0,32),'position':('_archive_position',0x5340,16),'normalize':('_archive_normalize_name',0x4e10,464),'hash':('_name_hash',0x4fe0,64),'copy':('_memmove',0x10fb0,821)}
MODELS={'format':('_sprintf',0x10f40),'crt_seek':('_fseek',0x112f0),'crt_read':('_fread',0x10dc0),'fatal':('_archive_fatal',0x10460)}
proof=json.loads((ROOT/'evidence/archive_storage_closure_2026_10_11.json').read_bytes())
DATA_SPECS=[('_'+x['name'],int(x['rva'],0),x['bytes'])for x in proof['fresh_canonical']['providers']['calibration/archive_storage.c']['strings']]
DATA_SPECS +=[('_g_archive_stream',0x51bbd0,4),('_g_archive_directory_count',0x51bbd4,4),('_g_archive_last_stream_handle',0x365a4,4),('_g_public_file_count',0x56e638,4),('_g_public_file_handles',0x56e640,64),('_g_archive_directory',0x5177d0,12288),('_g_archive_logical_positions',0x51a7d0,4096)]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();pack=lambda v:struct.pack('<I',v&0xffffffff)
def u32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def read_cstring(u,a,limit=512):
 out=bytearray()
 for i in range(limit):
  b=u.mem_read(a+i,1)[0]
  if b==0:return bytes(out)
  out.append(b)
 raise AssertionError('unterminated string')
def describe_data(coffs,oracle,specs=None):
 pe=PE(oracle);fields=[]
 for name,rv,n in (DATA_SPECS if specs is None else specs):
  owners=[(c,s)for c in coffs for s in c.symbols.values()if s['name']==name and(s['section']>0 or s['section']==0 and s['value']>0)];assert len(owners)==1,name;c,s=owners[0]
  if s['section']==0:assert s['value']==n;raw=bytes(n)
  else:section=c.sections[s['section']-1];assert s['value']+n<=section['size']and not section['relocations'];raw=section['data'][s['value']:s['value']+n]if section['data']else bytes(n)
  sec=next(t for t in pe.pe.sections if t.VirtualAddress<=rv and rv+n<=t.VirtualAddress+t.Misc_VirtualSize);virtual=rv>=sec.VirtualAddress+sec.SizeOfRawData;assert raw==(bytes(n)if virtual else pe.read_rva(rv,n)),name
  fields.append(dict(symbol=name,rva=hex(rv),bytes=n,data=raw.hex(),sha256=hashlib.sha256(raw).hexdigest(),origin='virtual_zero'if virtual else'file_backed'))
 return fields
def build(oracle,out,normalizer=None,backend=None,read_source=None,virtual_source=None,source_files=None,profile=None,data_specs=None):
 verify_toolchain(DEFAULT_TOOLCHAIN);assert sha(oracle)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d';library=verified_copy(oracle,out)
 files=[ROOT/p for p in (SOURCE_FILES if source_files is None else source_files)]+[ROOT/'src/pc/exe_helpers.c']
 replacements={'archive_names.c':normalizer,'archive_backend.c':backend,'archive_read.c':read_source,'virtual_file.c':virtual_source};files=[Path(replacements.get(p.name)or p).resolve()for p in files]
 runtime=Path('C:/tools/hercules/renderer-oracle-python/unicorn/lib/unicorn.dll');assert sha(runtime)=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
 watched={Path(__file__),oracle,runtime,ROOT/'recovery.json',ROOT/'toolchains/manifest.json',ROOT/'evidence/archive_storage_closure_2026_10_11.json',ROOT/'evidence/host_music_paths_data.json',oracle.parents[3]/'assets/pc/HERCULES/HERCULES.FS',ROOT/'scripts/host_resize_differential.py',ROOT/'scripts/crt_copy_evidence.py',Path(library['archive']),*[ROOT/'scripts'/p for p in ['match.py','coff.py','masked.py','pe.py','env.py','title_behavioral.py']]}
 for p in files:watched.update(context_files(p))
 pins={str(p):sha(p)for p in watched};compiles=[];coffs=[]
 for p in files:
  obj,cmd=compile_source(p,['/O2']);coffs.append(COFF(obj));compiles.append(dict(source=str(p),source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=cmd))
 objects=[x['object']for x in compiles]+[library['object']];coffs.append(COFF(library['object']));measurements=[]
 for role,(sym,rv,n)in (PROFILE if profile is None else profile).items():
  if role=='copy':measurements.append(dict(role=role,symbol=sym,rva=hex(rv),native_size=n,compiled_size=821,library_identity=True));continue
  i=next(i for i,c in enumerate(coffs)if any(s['name']==sym and s['section']>0 for s in c.symbols.values()));raw=compare(Path(objects[i]),sym,oracle,rv,n)
  if role=='hash':assert raw['exact']
  measurements.append(dict(role=role,symbol=sym,rva=hex(rv),native_size=n,compiled_size=coffs[i].function(sym)['size'],raw=raw,diagnostic=masked_compare(Path(objects[i]),sym,oracle,rv,n)))
 fields=describe_data(coffs,oracle,data_specs);assert pins=={p:sha(p)for p in pins}
 return dict(inputs=pins,compiles=compiles,objects=objects,measurements=measurements,data=fields,library=library)
def prepare(b,profile=None):
 symbols={'_g_archive_scratch':SCRATCH};segments=[];where={};coffs=[COFF(p)for p in b['objects']];cursor=0x20000000
 for x in b['data']:symbols[x['symbol']]=BASE+int(x['rva'],0);segments.append((symbols[x['symbol']],bytes.fromhex(x['data'])))
 endpoints={MODEL+i*32:role for i,role in enumerate(MODELS)}
 for a,r in endpoints.items():symbols[MODELS[r][0]]=a
 for k,c in enumerate(coffs):
  for i,s in enumerate(c.sections,1):
   if s['size']and(s['flags']&0x20 or s['name'].startswith('.rdata')and s['relocations']):where[k,i]=cursor;cursor+=(s['size']+15)&~15
  for s in c.symbols.values():
   if s['storage']==2 and s['section']>0 and(k,s['section'])in where:assert s['name']not in symbols;symbols[s['name']]=where[k,s['section']]+s['value']
 for k,c in enumerate(coffs):
  for i,s in enumerate(c.sections,1):
   if(k,i)not in where:continue
   raw=bytearray(s['data'])
   for r in s['relocations']:
    local=next((v for v in c.symbols.values()if v['name']==r['symbol']and v['section']>0),None);target=where[k,local['section']]+local['value']if local and(k,local['section'])in where else symbols[r['symbol']];at=r['offset'];assert r['type']in [6,20];value=struct.unpack_from('<I',raw,at)[0]+target-(where[k,i]+at+4 if r['type']==20 else 0);struct.pack_into('<I',raw,at,value&0xffffffff)
   segments.append((where[k,i],bytes(raw)))
 addresses={role:symbols[sym]for role,(sym,rv,n)in (PROFILE if profile is None else profile).items()};allowed=[(addresses[x['role']],addresses[x['role']]+x['compiled_size'])for x in b['measurements']]
 return dict(segments=segments,addresses=addresses,endpoints=endpoints,allowed=allowed)
def execute(native,b,p,oracle,archive,s):
 image=pefile.PE(str(oracle));u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(BASE,(image.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
 if native:u.mem_write(BASE,image.get_memory_mapped_image())
 u.mem_map(0x20000000,0x100000);u.mem_map(DATA,0x20000);u.mem_map(STACK,0x10000);u.mem_map(MODEL,0x1000)
 if native:addresses={r:BASE+rv for r,(sym,rv,n)in PROFILE.items()};endpoints={BASE+rv:r for r,(sym,rv)in MODELS.items()};allowed=[(BASE+rv,BASE+rv+n)for sym,rv,n in PROFILE.values()]
 else:
  addresses=p['addresses'];endpoints=p['endpoints'];allowed=p['allowed']
  for a,raw in p['segments']:u.mem_write(a,raw)
 u.mem_write(DIRECTORY-32,bytes([0xd1])*32+archive[:12288]+bytes([0xff])*4096+bytes([0xa7])*256+bytes([0xd2])*32);u.mem_write(STREAM_CELL,pack(STREAM)+pack(821));u.mem_write(LAST,pack(-1));u.mem_write(PUBLIC_COUNT,pack(0));u.mem_write(SLOTS-4,bytes([0xe1])*4+bytes(64)+bytes([0xe2])*32);u.mem_write(BUFFER-32,bytes([0xc1])*32+bytes([0xcc])*BUFFER_BYTES+bytes([0xc2])*32)
 cursor=0;events=[];pending=[];terminal=None;illegal=[];writes=[];instructions=0;argument_end=ESP+4;vars={};calls=[]
 def on_write(machine,access,a,n,value,user):
  ranges=[(POSITIONS,POSITIONS+4096),(SCRATCH,SCRATCH+256),(PUBLIC_COUNT,PUBLIC_COUNT+4),(SLOTS,SLOTS+64),(LAST,LAST+4),(BUFFER,BUFFER+BUFFER_BYTES),(ESP-0x2000,ESP),(ESP+4,argument_end)]
  if not any(lo<=a and a+n<=hi for lo,hi in ranges):writes.append((hex(a),n));machine.emu_stop()
 def on_code(machine,a,n,user):
  nonlocal instructions
  instructions+=1
  if instructions>200000:illegal.append('instruction cap');machine.emu_stop();return
  if a==STOP:machine.emu_stop();return
  if a in endpoints:pending.append(a);machine.emu_stop();return
  if not any(lo<=a<hi for lo,hi in allowed):illegal.append(hex(a));machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write)
 for index,command in enumerate(s['commands']):
  op=command['op'];path=command.get('path');pa=0
  if path is not None:
   raw=bytes.fromhex(command['path_hex'])if 'path_hex'in command else path.encode('latin-1');pa=DATA+0x4000+index*512;assert pa+len(raw)+1<BUFFER-32;u.mem_write(pa,raw+b'\0')
  h=vars.get(command.get('handle'),command.get('handle',0));dest=BUFFER+command.get('out',0)
  argv={'normalize':[pa],'hash':[pa],'vopen':[pa,command.get('mode',0),0xabcdef01],'aopen':[pa,BASE+0x35fe4],'vclose':[h],'aclose':[h],'position':[h],'vseek':[h,command.get('offset',0),command.get('origin',0)],'aseek':[h,command.get('offset',0),command.get('origin',0)],'vread':[h,dest,command.get('count',0)],'aread':[dest,command.get('size',1),command.get('count',0),h]}[op]
  frame=pack(STOP)+b''.join(pack(v)for v in argv)+bytes([0xf7])*32;argument_end=ESP+len(frame)-32;u.mem_write(ESP,frame);u.reg_write(UC_X86_REG_ESP,ESP);u.reg_write(UC_X86_REG_EFLAGS,0x202)
  for r,v in zip(NV,SEEDS):u.reg_write(r,v)
  pc=addresses[op]
  for _ in range(200):
   pending.clear();u.emu_start(pc,STOP,count=200001)
   if illegal or writes or u.reg_read(UC_X86_REG_EIP)==STOP:break
   assert pending;role=endpoints[pending[-1]];sp=u.reg_read(UC_X86_REG_ESP)
   if role=='fatal':terminal=read_cstring(u,u32(u,sp+4)).decode('latin-1');events.append(dict(call='fatal',message=terminal));break
   if role=='format':
    dst,fmt,ptr=[u32(u,sp+i)for i in [4,8,12]];pattern=read_cstring(u,fmt);assert pattern==b'PC_fopen("%s")';raw=pattern.replace(b'%s',read_cstring(u,ptr));assert ESP-0x2000<=dst and dst+len(raw)+1<ESP;u.mem_write(dst,raw+b'\0');result=len(raw);events.append(dict(call='format',text=raw.decode('latin-1')))
   elif role=='crt_seek':
    stream,offset,origin=[u32(u,sp+i)for i in [4,8,12]];assert stream==STREAM and origin==0 and offset<=len(archive);result=s.get('seek_status',0)
    if result==0:cursor=offset
    events.append(dict(call='crt_seek',offset=offset,origin=origin,result=result&0xffffffff))
   else:
    dst,size,count,stream=[u32(u,sp+i)for i in [4,8,12,16]];assert stream==STREAM;items=min(count,(len(archive)-cursor)//size)if size else 0;items=min(items,s.get('read_item_limit',items));length=items*size;assert BUFFER<=dst and dst+length<=BUFFER+BUFFER_BYTES;raw=archive[cursor:cursor+length];u.mem_write(dst,raw);events.append(dict(call='crt_read',buffer_offset=dst-BUFFER,size=size,count=count,offset=cursor,items=items,sha256=hashlib.sha256(raw).hexdigest()));cursor+=length;result=items
   pc=u32(u,sp);u.reg_write(UC_X86_REG_ESP,sp+4);u.reg_write(UC_X86_REG_EAX,result&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xdeadbeef);u.reg_write(UC_X86_REG_EDX,0xcafebabe)
  else:raise AssertionError('boundary cap')
  assert not illegal and not writes,(s,illegal,writes)
  if terminal is not None:break
  assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4 and all(u.reg_read(r)==v for r,v in zip(NV,SEEDS))and not u.reg_read(UC_X86_REG_EFLAGS)&0x400
  assert bytes(u.mem_read(ESP,4))==pack(STOP)and bytes(u.mem_read(argument_end,32))==bytes([0xf7])*32
  value=u.reg_read(UC_X86_REG_EAX)
  if command.get('store'):vars[command['store']]=value
  normalized=None if op!='normalize'or value==0 else read_cstring(u,value).hex()
  calls.append(dict(op=op,result=hex(value),normalized=normalized,buffer_sha256=hashlib.sha256(bytes(u.mem_read(BUFFER,BUFFER_BYTES))).hexdigest(),buffer_head=bytes(u.mem_read(BUFFER,64)).hex(),physical_cursor=cursor))
 assert bytes(u.mem_read(DIRECTORY-32,32))==bytes([0xd1])*32 and bytes(u.mem_read(SCRATCH+256,32))==bytes([0xd2])*32
 assert bytes(u.mem_read(SLOTS-4,4))==bytes([0xe1])*4 and bytes(u.mem_read(SLOTS+64,32))==bytes([0xe2])*32
 assert bytes(u.mem_read(BUFFER-32,32))==bytes([0xc1])*32 and bytes(u.mem_read(BUFFER+BUFFER_BYTES,32))==bytes([0xc2])*32
 digest=lambda a,n:hashlib.sha256(bytes(u.mem_read(a,n))).hexdigest()
 return dict(calls=calls,terminal_fatal=terminal,events=events,physical_cursor=cursor,directory=digest(DIRECTORY,12288),positions=bytes(u.mem_read(POSITIONS,4096)).hex(),scratch=bytes(u.mem_read(SCRATCH,256)).hex(),public_count=u32(u,PUBLIC_COUNT),slots=bytes(u.mem_read(SLOTS,64)).hex(),last_handle=u32(u,LAST),stream=u32(u,STREAM_CELL),directory_count=u32(u,COUNT_CELL),buffer=digest(BUFFER,BUFFER_BYTES),guards_and_code_write_whitelists=True)

def scenarios(archive):
 data=json.loads((ROOT/'evidence/host_music_paths_data.json').read_bytes());literals={r['rva']:r['literal']for r in data['literals']};lookup={}
 for i in range(1024):
  key,offset,n=struct.unpack_from('<III',archive,i*12)
  if not key:break
  lookup[key]=(i,offset,n)
 cases=[];members=[];seen=set()
 for row in data['records']:
  for filename in ['SFX.SFX','MUSIC.MUS']:
   path=literals[row['path_rva']]+filename;normalized='M:\\LANGUAGE\\SONIX'+path[2:];raw=normalized.upper().encode();key=(sum(v<<((i%4)*8)for i,v in enumerate(raw))+len(raw))&0xffffffff
   if key not in lookup or lookup[key][0]in seen:continue
   member,offset,n=lookup[key];seen.add(member);members.append(dict(path=path,index=member,offset=offset,bytes=n,bank=row['index'],filename=filename))
   cases.append(dict(name='member_'+str(member),commands=[dict(op='vopen',path=path,store='a'),dict(op='vseek',handle='a',offset=0,origin=2),dict(op='vseek',handle='a',offset=0,origin=0),dict(op='vread',handle='a',count=n),dict(op='vclose',handle='a')],expected_member=member))
 a='q:\\TRAINING\\SFX.SFX';b='q:\\GAUNTLET\\SFX.SFX'
 for path in [None,'',a,a.lower(),'r:\\training\\sfx.sfx','M:\\LANGUAGE\\SONIX\\TRAINING\\SFX.SFX','\\TEST\\FILE','TEST\\FILE','o:\\MAP\\X','p:\\CHOP\\X','n:\\ROOT\\X','s:\\ROOT\\X']:
  cases.append(dict(name='normalize_'+str(len(cases)),commands=[dict(op='normalize',path=path)]))
 for offset,origin in [(0,0),(5,0),(-1,0),(-1,1),(-1,2),(9999,0),(5,7)]:cases.append(dict(name='seek_'+str(offset)+'_'+str(origin),commands=[dict(op='vopen',path=a,store='a'),dict(op='vseek',handle='a',offset=offset,origin=origin),dict(op='vread',handle='a',count=8)]))
 cases.append(dict(name='item_sizes',commands=[dict(op='aopen',path=a,store='a'),dict(op='aread',handle='a',size=3,count=1000),dict(op='aread',handle='a',size=3,count=1),dict(op='aread',handle='a',size=1,count=8),dict(op='aread',handle='a',size=0,count=8)]))
 cases.append(dict(name='shared_cache_after_other_seek',commands=[dict(op='vopen',path=a,store='a'),dict(op='vread',handle='a',count=4),dict(op='vopen',path=b,store='b'),dict(op='vseek',handle='b',offset=0x44,origin=0),dict(op='vread',handle='a',count=4)]))
 cases.append(dict(name='public_hole_read',commands=[dict(op='vopen',path=a,store='a'),dict(op='vopen',path=b,store='b'),dict(op='vclose',handle='a'),dict(op='vread',handle='b',count=4)]))
 cases.append(dict(name='public_hole_close',commands=[dict(op='vopen',path=a,store='a'),dict(op='vopen',path=b,store='b'),dict(op='vclose',handle='a'),dict(op='vclose',handle='b')]))
 cases.append(dict(name='missing_path',commands=[dict(op='vopen',path='q:\\PLAYROOM\\SFX.SFX')]))
 cases.append(dict(name='bad_mode',commands=[dict(op='vopen',path=a,mode=1)]))
 cases.append(dict(name='capacity',commands=[dict(op='vopen',path=a)]*17))
 cases.append(dict(name='short_read',read_item_limit=7,commands=[dict(op='vopen',path=a,store='a'),dict(op='vread',handle='a',count=452),dict(op='position',handle=598)]))
 cases.append(dict(name='failed_seek',seek_status=-1,commands=[dict(op='vopen',path=a,store='a'),dict(op='vseek',handle='a',offset=5,origin=0)]))
 for letter in 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz':
  cases.append(dict(name='drive_'+letter,commands=[dict(op='normalize',path=letter+':\\Root\\file.bin'),dict(op='normalize',path='M:\\x')]))
 for raw in [b'q:\\x\\\x80az',b'q:\\x\\\xffaz']:
  cases.append(dict(name='high_byte_'+raw.hex(),commands=[dict(op='normalize',path='',path_hex=raw.hex())]))
 cases.append(dict(name='shared_member_position',commands=[dict(op='vopen',path=a,store='a'),dict(op='vread',handle='a',count=4),dict(op='vopen',path=a,store='b'),dict(op='vread',handle='a',count=8),dict(op='vclose',handle='b'),dict(op='position',handle=598)]))
 cases.append(dict(name='same_member_alias',commands=[dict(op='vopen',path=a,store='a'),dict(op='vopen',path='r:\\training\\sfx.sfx',store='b'),dict(op='vread',handle='b',count=8),dict(op='vread',handle='a',count=8)]))
 return cases,members
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original-root',type=Path,default=Path('D:/Prog/hercules_recon'));p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();assert out.is_relative_to(ROOT/'work');out.mkdir(parents=True,exist_ok=True);oracle=a.original_root/'work/discs/pc_install/HERCULES.EXE';fs=a.original_root/'assets/pc/HERCULES/HERCULES.FS';archive=fs.read_bytes();assert hashlib.sha256(archive).hexdigest()=='affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4'
 b=build(oracle,out/'build');plan=prepare(b);ss,members=scenarios(archive);cases=[]
 for s in ss:
  n=execute(True,b,plan,oracle,archive,s);c=execute(False,b,plan,oracle,archive,s);assert n==c,(s,n,c)
  if 'expected_member'in s:
   row=next(x for x in members if x['index']==s['expected_member']);expected=bytearray(bytes([0xcc])*BUFFER_BYTES);expected[:row['bytes']]=archive[row['offset']:row['offset']+row['bytes']];assert n['buffer']==hashlib.sha256(expected).hexdigest()and n['public_count']==0
  cases.append(dict(scenario=s,equal=True,native=n,source=c))
 cache=next(c for c in cases if c['scenario']['name']=='shared_cache_after_other_seek')['native'];assert len([x for x in cache['events']if x['call']=='crt_seek'])==2
 reads=[x for x in cache['events']if x['call']=='crt_read'];assert reads[-1]['offset']!=reads[0]['offset']+4 and reads[-1]['sha256']!=reads[0]['sha256']
 texts={p:(ROOT/p).read_text()for p in ['calibration/archive_names.c','calibration/archive_backend.c','calibration/archive_read.c','calibration/virtual_file.c']};negative_specs=[
  ('uppercase_removed','calibration/archive_names.c',texts['calibration/archive_names.c'].replace("value = (unsigned char)(value - (unsigned char)('a' - 'A'));","value = value;"),next(s['name']for s in ss if s['commands'][0].get('path')=='q:\\training\\sfx.sfx'and s['commands'][0]['op']=='normalize'),None),
  ('archive_handle_zero_based','calibration/archive_backend.c',texts['calibration/archive_backend.c'].replace('return row + 1;','return row;'),ss[0]['name'],None),
  ('negative_seek_clamps_signed','calibration/archive_backend.c',texts['calibration/archive_backend.c'].replace('(unsigned long)g_archive_logical_positions[handle - 1] > file_length','g_archive_logical_positions[handle - 1] > (long)file_length'),'seek_-1_0',None),
  ('seek_repairs_read_cache','calibration/archive_backend.c',texts['calibration/archive_backend.c'].replace('return fseek(g_archive_stream,','g_archive_last_stream_handle = -1;\n    return fseek(g_archive_stream,'),'shared_cache_after_other_seek',None),
  ('handle_validation_uses16','calibration/virtual_file.c',texts['calibration/virtual_file.c'].replace('handle>g_public_file_count','handle>16'),'public_hole_read',None),
  ('read_ceil_instead_of_floor','calibration/archive_read.c',texts['calibration/archive_read.c'].replace('(file_length - position) / divisor','(file_length - position + divisor - 1) / divisor'),'item_sizes',2)]
 negatives=[]
 for name,owner,code,case,command_limit in negative_specs:
  assert code!=texts[owner],name;lane=out/name;lane.mkdir(exist_ok=True);source=lane/'wrong.c';source.write_bytes(code.encode());kwargs={dict([('archive_names.c','normalizer'),('archive_backend.c','backend'),('archive_read.c','read_source'),('virtual_file.c','virtual_source')])[Path(owner).name]:source};bad=build(oracle,lane/'build',**kwargs);bp=prepare(bad);s=dict(next(x for x in ss if x['name']==case))
  if command_limit:s['commands']=s['commands'][:command_limit]
  n=execute(True,b,plan,oracle,archive,s);c=execute(False,bad,bp,oracle,archive,s);assert n!=c,name;negatives.append(dict(name=name,detected=True,scenario=s,native=n,wrong=c,build=bad))
 assert b['inputs']=={p:sha(p)for p in b['inputs']}
 report=dict(scope=__doc__,build=b,case_count=len(cases),all_equal=True,cases=cases,members=members,archive_sha256=hashlib.sha256(archive).hexdigest(),negative_controls=negatives,limits=['Actual normalization/hash/search,virtual handles,logical positioning,read/seek/close game code executes. CRT seek/read/format effects are modeled;comparison stops at first fatal entry.','Directory and logical positions are explicit post-initialization state from the pinned real821-row archive and existing1024-entry prefixes. Initialization,CRT locking,FILElayout,OS I/O and termination do not execute.','Scratch256bytes and destination buffer sizes are bounded fixture backing,not original capacity/ownership proofs. Only full-item CRT read effects are modeled.','Finite agreement and native quirks do not grant raw matches,unique source identity,TU/layout or safe arbitrary handles/pointers.'])
 dest=out/'host_archive_pipeline_differential.json';dest.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),members=len(members),all_equal=True,report=str(dest))))
if __name__=='__main__':main()
