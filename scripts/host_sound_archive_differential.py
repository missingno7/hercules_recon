"""Real PC ESF archive-to-PCM comparison; CRT streams and DirectSound are modeled."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
import host_archive_pipeline_differential as ap
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
BASE=ap.BASE;STACK=ap.STACK;ESP=ap.ESP;STOP=ap.STOP
DEVICE=0x66000000;DVT=DEVICE+0x1000;BVT=DEVICE+0x2000;BUFFERS=DEVICE+0x3000;PCM=0x67000020;PCM_MAP=0x67000000;SECOND=0x67100020
HEADER=BASE+0x699838;SCRATCH=BASE+0x699848;BANK=BASE+0x799908;COUNTERS=BASE+0x698ed8;LOOPS=BASE+0x698a18;DEVICE_CELL=BASE+0x79e440;RELEASE_FLAG=BASE+0x79e450
SOUND_FILES=['src/pc/host_sound_bank.cpp','calibration/host_sound_bank_storage.c','src/pc/host_sound_decoder.c','calibration/host_sound_decoder_tables.c','calibration/audio_bank_release.cpp','calibration/host_sound_channel_storage.cpp','calibration/sound_bank_storage.cpp','calibration/host_sound_scratch_storage.c']
PROFILE={**ap.PROFILE,'constructor':('_host_sound_bank_initialize_f3d0',0xf3d0,832),'decoder':('_host_sound_decode_2410',0x2410,288),'release':('_cleanup_native_f710',0xf710,80)}
EXTRA_DATA=[('_g_host_sound_header_699838',0x699838,16),('_g_host_sound_bank_counters_698ed8',0x698ed8,2400),('_g_host_sound_scratch_699848',0x699848,0x100000),('_g_host_sound_loop_flags_698a18',0x698a18,1200),('_g_exe_799908',0x799908,19200),('_g_exe_79e440',0x79e440,4),('_g_audio_cleanup_iteration_flag',0x79e450,1),('_g_host_sound_filename_format',0x3a510,len(b'm:\\language\\sonix\\pc_%ld.esf')+1),('_g_host_sound_create_error',0x3a500,16),('_g_host_sound_index_deltas_35e08',0x35e08,64),('_g_host_sound_steps_35e48',0x35e48,356)]
METHODS={'Create':(0xc,12),'Duplicate':(0x14,8),'Stop':(0x48,0),'Release':(8,0),'Position':(0x34,4),'Frequency':(0x44,4),'Pan':(0x40,4),'Volume':(0x3c,4),'Lock':(0x2c,28),'Unlock':(0x4c,16)}
pack=ap.pack;u32=ap.u32;sha=ap.sha

def catalogue(archive):
 lookup={struct.unpack_from('<III',archive,i*12)[0]:(i,*struct.unpack_from('<III',archive,i*12)[1:])for i in range(821)};members=[]
 for index in range(1200):
  path='m:\\language\\sonix\\pc_%d.esf'%index;raw=path.upper().encode();key=(sum(v<<((i%4)*8)for i,v in enumerate(raw))+len(raw))&0xffffffff
  if key not in lookup:continue
  row,offset,length=lookup[key];payload=archive[offset:offset+length];magic,samples,rate,flags=struct.unpack_from('<4I',payload)
  assert magic==0x03465345 and length==16+samples//4 and 0<samples<=0xfffff and samples+samples//4<=0xfffff
  members.append(dict(index=index,path=path,row=row,key=hex(key),offset=offset,bytes=length,sample_bytes=samples,rate_word=rate,flags=flags,sha256=hashlib.sha256(payload).hexdigest()))
 physical=set()
 for i in range(821):
  key,offset,length=struct.unpack_from('<III',archive,i*12)
  if archive[offset:offset+4]==b'ESF\x03':physical.add(i)
 assert physical=={x['row']for x in members}and len(physical)==len(members)
 return members

def build(oracle,out,overrides=None):
 files=[*ap.SOURCE_FILES,*SOUND_FILES];files=[str((overrides or{}).get(p,p))for p in files]
 b=ap.build(oracle,out,source_files=files,profile=PROFILE,data_specs=[*ap.DATA_SPECS,*EXTRA_DATA]);b['inputs'][str(Path(__file__).resolve())]=sha(Path(__file__));return b

def complement(ranges):
 cursor=1
 for lo,hi in sorted(ranges):
  assert cursor<=lo<hi
  if cursor<lo:yield cursor,lo-1
  cursor=hi
 if cursor<=0xffffffff:yield cursor,0xffffffff

def execute(native,b,p,image,archive,s):
 row=s['member'];index=row['index'];sample_bytes=row['sample_bytes'];pcm_bytes=sample_bytes//(2 if row['flags']&0xff00 else 1)
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(BASE,(len(image)+4095)&~4095)
 if native:u.mem_write(BASE,image)
 u.mem_map(0x20000000,0x100000);u.mem_map(STACK,0x10000);u.mem_map(ap.MODEL,0x1000);u.mem_map(DEVICE,0x10000);u.mem_map(PCM_MAP,0x200000)
 if native:addresses={r:BASE+rv for r,(sym,rv,n)in PROFILE.items()};endpoints={BASE+rv:r for r,(sym,rv)in ap.MODELS.items()};allowed=[(BASE+rv,BASE+rv+n)for sym,rv,n in PROFILE.values()]
 else:
  addresses=p['addresses'];endpoints=dict(p['endpoints']);allowed=p['allowed']
  for a,raw in p['segments']:u.mem_write(a,raw)
 u.mem_write(ap.DIRECTORY-32,bytes([0xd1])*32+archive[:12288]+bytes([0xff])*4096+bytes([0xa7])*256+bytes([0xd2])*32);u.mem_write(ap.STREAM_CELL,pack(ap.STREAM)+pack(821));u.mem_write(ap.LAST,pack(-1));u.mem_write(ap.PUBLIC_COUNT,pack(0));u.mem_write(ap.SLOTS-4,bytes([0xe1])*4+bytes(64)+bytes([0xe2])*32)
 u.mem_write(COUNTERS-16,bytes([0xc1])*16+bytes([0x97])*2400+bytes([0x83])*16+bytes([0x5d])*0x100000+bytes([0xc2])*32);u.mem_write(SCRATCH+0xfffff,bytes([s.get('tail',0)]));u.mem_write(LOOPS-16,bytes([0xc3])*16+bytes([0x96])*1200+bytes([0xc1])*16);u.mem_write(BANK-32,bytes([0xb1])*32+bytes(19200)+bytes([0xb2])*32);u.mem_write(DEVICE_CELL,pack(0 if s.get('no_device')else DEVICE));u.mem_write(RELEASE_FLAG,bytes([s.get('release_flag',1)]))
 u.mem_write(PCM-32,bytes([0xf1])*32+bytes([0x91])*pcm_bytes+bytes([0xf2])*32);u.mem_write(SECOND-32,bytes([0xf3])*32+bytes([0x92])*32+bytes([0xf4])*32);u.mem_write(DEVICE,pack(DVT))
 for i in range(4):u.mem_write(BUFFERS+i*16,pack(BVT)+pack(i)+bytes(8))
 for i,(name,(offset,extra))in enumerate(METHODS.items()):
  a=ap.MODEL+0x100+i*16;endpoints[a]=name;u.mem_write((DVT if name in ['Create','Duplicate']else BVT)+offset,pack(a))
 events=[];entries=[];pending=[];errors=[];cursor=0;argument_end=ESP+8;terminal=None
 writable=[(ap.POSITIONS,ap.POSITIONS+4096),(ap.SCRATCH,ap.SCRATCH+256),(ap.LAST,ap.LAST+4),(BANK,BANK+19200),(COUNTERS,COUNTERS+2400),(HEADER,HEADER+16),(SCRATCH,SCRATCH+0x100000),(LOOPS,LOOPS+1200),(RELEASE_FLAG,RELEASE_FLAG+1),(PCM,PCM+pcm_bytes),(ESP-0x3000,ESP+8)]
 def on_write(machine,access,a,n,value,_):errors.append(('write',hex(a),n));machine.emu_stop()
 def on_code(machine,a,n,_):
  if a==STOP or a in endpoints:pending.append(a);machine.emu_stop()
  elif a in [addresses['constructor'],addresses['decoder'],addresses['release']]:entries.append(next(k for k in ['constructor','decoder','release']if addresses[k]==a))
  elif not any(lo<=a and a+n<=hi for lo,hi in allowed):errors.append(('code',hex(a)));machine.emu_stop()
 # Unicorn filters these disjoint intervals before invoking Python. Legitimate
 # constructor/decoder instructions and writes run normally. Code elsewhere
 # is checked against the full function ranges; forbidden writes still stop.
 hot=sorted((lo,hi)for lo,hi in allowed if lo in [addresses['decoder'],addresses['constructor']])
 for lo,hi in complement(hot):u.hook_add(UC_HOOK_CODE,on_code,begin=lo,end=hi)
 for role in ['constructor','decoder']:u.hook_add(UC_HOOK_CODE,on_code,begin=addresses[role],end=addresses[role])
 # Merge touching allowed data regions before installing their complement.
 merged=[]
 for lo,hi in sorted(writable):
  if merged and lo<=merged[-1][1]:merged[-1]=(merged[-1][0],max(hi,merged[-1][1]))
  else:merged.append((lo,hi))
 for lo,hi in complement(merged):u.hook_add(UC_HOOK_MEM_WRITE,on_write,begin=lo,end=hi)
 for command in s.get('commands',['constructor']):
  u.mem_write(ESP,pack(STOP)+pack(index)+bytes([0xf7])*32);u.reg_write(UC_X86_REG_ESP,ESP);u.reg_write(UC_X86_REG_EFLAGS,0x202)
  for reg,v in zip(ap.NV,ap.SEEDS):u.reg_write(reg,v)
  pc=addresses[command]
  for _ in range(100):
   pending.clear();u.emu_start(pc,STOP,count=max(1000000,sample_bytes*150))
   assert not errors,(s,errors)
   if u.reg_read(UC_X86_REG_EIP)==STOP:break
   assert pending,(s,'instruction cap');role=endpoints[pending[-1]];sp=u.reg_read(UC_X86_REG_ESP);arg=lambda n:u32(u,sp+4*n);cleanup=0
   if role=='fatal':terminal=ap.read_cstring(u,arg(1)).decode('latin-1');events.append(dict(call=role,message=terminal));break
   if role=='format':
    pattern=ap.read_cstring(u,arg(2));assert pattern in [b'm:\\language\\sonix\\pc_%ld.esf',b'PC_fopen("%s")'];raw=pattern.replace(b'%ld',str(arg(3)).encode())if b'%ld'in pattern else pattern.replace(b'%s',ap.read_cstring(u,arg(3)));assert ESP-0x3000<=arg(1)and arg(1)+len(raw)+1<ESP;u.mem_write(arg(1),raw+b'\0');result=len(raw);events.append(dict(call=role,text=raw.decode()))
   elif role=='crt_seek':
    assert arg(1)==ap.STREAM and arg(3)==0 and arg(2)<=len(archive);cursor=arg(2);result=0;events.append(dict(call=role,offset=cursor))
   elif role=='crt_read':
    dst,size,count,stream=[arg(i)for i in [1,2,3,4]];assert stream==ap.STREAM;items=min(count,(len(archive)-cursor)//size)if size else 0;n=items*size;assert (dst==HEADER and n==16)or SCRATCH<=dst and dst+n<=SCRATCH+0x100000;raw=archive[cursor:cursor+n];u.mem_write(dst,raw);events.append(dict(call=role,destination='header'if dst==HEADER else dst-SCRATCH,size=size,count=count,items=items,offset=cursor,sha256=hashlib.sha256(raw).hexdigest()));cursor+=n;result=items
   else:
    receiver=arg(1);cleanup=METHODS[role][1]+4;result=s.get('com_result',0);event=dict(call=role)
    if role in ['Create','Duplicate']:assert receiver==DEVICE
    else:assert receiver in [BUFFERS+i*16 for i in range(4)];event['buffer']=(receiver-BUFFERS)//16
    if role=='Create':
     desc,target,outer=arg(2),arg(3),arg(4);assert target==BANK+index*16 and outer==0;raw=bytearray(u.mem_read(desc,20));fmt=u32(u,desc+16);assert ESP-0x3000<=desc and desc+20<ESP and ESP-0x3000<=fmt and fmt+16<ESP;raw[16:20]=bytes(4);event.update(descriptor=raw.hex(),format=bytes(u.mem_read(fmt,16)).hex());bits=8 if row['flags']&0xff00 else 16;rate=row['rate_word']&65535;assert bytes(raw)==struct.pack('<5I',20,0xe2 if index<1000 else 0xe8,pcm_bytes,0,0)and bytes(u.mem_read(fmt,16))==struct.pack('<HHIIHH',1,1,rate,rate*(bits//8),bits//8,bits)
     if not s.get('create_null'):u.mem_write(target,pack(BUFFERS))
    elif role=='Duplicate':
     source,target=arg(2),arg(3);lane=(target-BANK-index*16)//4;assert lane in [1,2,3]and source in [BUFFERS+i*16 for i in range(4)];u.mem_write(target,pack(0 if lane==s.get('duplicate_null')else BUFFERS+lane*16));event.update(source=(source-BUFFERS)//16,lane=lane)
    elif role=='Lock':
     assert arg(2)==0 and arg(3)==pcm_bytes and arg(8)==0
     for at,value in [(arg(4),0 if s.get('null_block')else PCM),(arg(5),0 if s.get('empty_lock')else pcm_bytes-s.get('split',0)),(arg(6),SECOND if s.get('split')else 0),(arg(7),s.get('split',0))]:assert ESP-0x3000<=at and at+4<ESP;u.mem_write(at,pack(value))
     event.update(bytes=pcm_bytes,split=s.get('split',0))
    elif role=='Unlock':assert arg(2)==PCM and arg(3)==pcm_bytes-s.get('split',0) and arg(4)==(SECOND if s.get('split')else 0)and arg(5)==s.get('split',0);event['bytes']=arg(3)
    elif role in ['Position','Frequency','Pan','Volume']:assert arg(2)==0;event['value']=0
    events.append(event)
   pc=u32(u,sp);u.reg_write(UC_X86_REG_ESP,sp+4+cleanup);u.reg_write(UC_X86_REG_EAX,result&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xdeadbeef);u.reg_write(UC_X86_REG_EDX,0xcafebabe)
  else:raise AssertionError('boundary cap')
  if terminal is not None:break
  assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4 and not u.reg_read(UC_X86_REG_EFLAGS)&0x400 and all(u.reg_read(r)==v for r,v in zip(ap.NV,ap.SEEDS))
  assert bytes(u.mem_read(ESP,4))==pack(STOP)and bytes(u.mem_read(ESP+8,32))==bytes([0xf7])*32
 for a,n,v in [(ap.DIRECTORY-32,32,0xd1),(ap.SCRATCH+256,32,0xd2),(ap.SLOTS-4,4,0xe1),(ap.SLOTS+64,32,0xe2),(COUNTERS-16,16,0xc1),(SCRATCH+0x100000,32,0xc2),(LOOPS-16,16,0xc3),(LOOPS+1200,16,0xc1),(BANK-32,32,0xb1),(BANK+19200,32,0xb2),(PCM-32,32,0xf1),(PCM+pcm_bytes,32,0xf2),(SECOND-32,32,0xf3),(SECOND+32,32,0xf4)]:assert bytes(u.mem_read(a,n))==bytes([v])*n,(s,hex(a),'guard')
 digest=lambda a,n:hashlib.sha256(bytes(u.mem_read(a,n))).hexdigest()
 return dict(last_decoded_sample=struct.unpack('<h',u.mem_read(SCRATCH+(sample_bytes&~1)-2,2))[0],events=events,entries=entries,terminal_fatal=terminal,physical_cursor=cursor,pcm=digest(PCM,pcm_bytes),second=digest(SECOND,32),sound_state={name:digest(a,n)for name,a,n in [('header',HEADER,16),('scratch',SCRATCH,0x100000),('counters',COUNTERS,2400),('loops',LOOPS,1200),('bank',BANK,19200)]},archive=dict(directory=digest(ap.DIRECTORY,12288),positions=digest(ap.POSITIONS,4096),scratch=digest(ap.SCRATCH,256),count=u32(u,ap.COUNT_CELL),stream=u32(u,ap.STREAM_CELL),last=u32(u,ap.LAST),public_count=u32(u,ap.PUBLIC_COUNT),slots=digest(ap.SLOTS,64)),guards_whitelists_stack_nonvolatiles_direction=True)

def scenarios(members):
 result=[]
 for member in members:
  result.append(dict(name='member_'+str(member['index']),member=member))
  if member['sample_bytes']%4:
   for tail in [0x70,0xf0]:result.append(dict(name='member_'+str(member['index'])+'_tail_'+str(tail),member=member,tail=tail))
 first=members[0];protected=next(x for x in members if x['index']==824)
 for name,values in [('null_device',dict(no_device=True)),('null_created_buffer',dict(create_null=True)),('loaded_twice',dict(commands=['constructor','constructor'])),('release_reload',dict(commands=['constructor','release','constructor'])),('split_lock',dict(split=7)),('empty_lock',dict(empty_lock=True)),('null_lock',dict(null_block=True)),('ignored_com_result',dict(com_result=0x80004005))]:result.append(dict(name=name,member=first,**values))
 for lane in [1,2,3]:result.append(dict(name='duplicate_fail_'+str(lane),member=first,duplicate_null=lane))
 for flag in [0,1,128]:result.append(dict(name='protected_duplicate_fail_'+str(flag),member=protected,duplicate_null=2,release_flag=flag))
 missing=dict(first,index=0,path='m:\\language\\sonix\\pc_0.esf')
 assert not any(x['index']==0 for x in members)
 result.append(dict(name='missing_member',member=missing,expected_fatal='PC_fopen("m:\\language\\sonix\\pc_0.esf")'))
 return result

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--original-root',type=Path,default=Path('D:/Prog/hercules_recon'));parser.add_argument('--limit',type=int);args=parser.parse_args();assert args.limit is None or args.limit>0;out=args.output.resolve();assert out.is_relative_to(ROOT/'work');out.mkdir(parents=True,exist_ok=True)
 oracle=args.original_root/'work/discs/pc_install/HERCULES.EXE';fs=args.original_root/'assets/pc/HERCULES/HERCULES.FS';archive=fs.read_bytes();assert hashlib.sha256(archive).hexdigest()=='affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4';members=catalogue(archive);(out/'catalogue.json').write_bytes((json.dumps(members,indent=2)+'\n').encode());print(json.dumps(dict(members=len(members),max_sample_bytes=max(x['sample_bytes']for x in members),odd=sum(x['sample_bytes']%4!=0 for x in members))),flush=True)
 b=build(oracle,out/'build');p=ap.prepare(b,PROFILE);image=pefile.PE(str(oracle)).get_memory_mapped_image();cases=[];ss=scenarios(members)
 if args.limit:ss=ss[:args.limit]
 for s in ss:
  member=s['member'];n=execute(True,b,p,image,archive,s);c=execute(False,b,p,image,archive,s);assert n==c,(s,n,c);assert n['terminal_fatal']==s.get('expected_fatal'),(s,n['terminal_fatal'])
  if not s.get('expected_fatal'):
   header_only=s.get('no_device')or s.get('create_null');assert n['physical_cursor']==member['offset']+(16 if header_only else member['bytes'])and n['archive']['last']==member['row']+1
   reads=[e for e in n['events']if e['call']=='crt_read'];cycles=2 if s['name']=='release_reload'else 1;assert len(reads)==cycles*(1 if header_only else 2)
   for i in range(cycles):
    first=reads[i*(1 if header_only else 2)];assert first['offset']==member['offset']and first['items']==1
    if not header_only:second=reads[i*2+1];assert second['offset']==member['offset']+16 and second['items']==1
  cases.append(dict(scenario=s,equal=True,native=n,source=c))
  if len(cases)%50==0:print(json.dumps(dict(done=len(cases),index=member['index'])),flush=True)
 negatives=[]
 if args.limit is None:
  ctor=(ROOT/'src/pc/host_sound_bank.cpp').read_text();decoder=(ROOT/'src/pc/host_sound_decoder.c').read_text();reader=(ROOT/'calibration/archive_read.c').read_text()
  variants=[
   ('pcm8_bias','src/pc/host_sound_bank.cpp',ctor.replace('(BYTE)(127-','(BYTE)(128-'),ss[0]),
   ('scratch_offset','src/pc/host_sound_bank.cpp',ctor.replace('+0xfffff-','+0x100000-'),ss[0]),
   ('low_nibble_first','src/pc/host_sound_decoder.c',decoder.replace('code = cached;','code = cached >> 4;').replace('code = cached >> 4;\n        }','code = cached;\n        }'),ss[0]),
   ('archive_read_offset_off_by_one','calibration/archive_read.c',reader.replace('.offset + position)', '.offset + position + 1)'),ss[0])]
  for name,owner,code,s in variants:
   assert code!=(ROOT/owner).read_text();lane=out/name;lane.mkdir(exist_ok=True);source=lane/Path(owner).name
   import re
   code=re.sub(r'#include "([^"]+)"',lambda m:'#include "'+((ROOT/owner).parent/m[1]).resolve().as_posix()+'"',code);source.write_bytes(code.encode());bad=build(oracle,lane/'build',{owner:source});bp=ap.prepare(bad,PROFILE);n=execute(True,b,p,image,archive,s);c=execute(False,bad,bp,image,archive,s);assert n!=c,name;negatives.append(dict(name=name,detected=True,scenario=s,native=n,wrong=c,build=bad))
  tail_groups={}
  for c in cases:
   if c['scenario']['name'].startswith('member_'):tail_groups.setdefault(c['scenario']['member']['index'],set()).add(c['native']['last_decoded_sample'])
  tail_effects=sum(len(v)>1 for v in tail_groups.values());assert tail_effects>0
 else:tail_effects=None
 assert b['inputs']=={p:sha(Path(p))for p in b['inputs']};report=dict(scope=__doc__,limits=['Actual original/source archive normalization,hash,search,read/close,constructor,decoder and bankrelease execute. CRTseek/read,sprintf and DirectSoundCOM effects remain models.','All559physical ESFmembers have unique verified names in the constructor index domain. Only this actual file/header/size domain and directed effects are tested.','Archive state is explicit post-initialization input. Scratch end-byte,PCMblock and other guard patterns are fixture inputs;original capacities,TU/layout and arbitrary header/pointer safety are not inferred.','Two shared-scratch tail variants preserve native final-nibble consumption. A split lock copies only its first block. This is behavioral evidence,not a safety repair or audio-device proof.','Fatal comparison stops at first entry. OS I/O,DirectSounddevice,actual game startup/menu/input/gameplay and renderer do not execute.'],build=b,case_count=len(cases),all_equal=True,cases=cases,catalogue_members=len(members),full_catalogue=args.limit is None,tail_effect_members=tail_effects,negative_controls=negatives,archive_sha256=hashlib.sha256(archive).hexdigest());(out/'host_sound_archive_differential.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(all_equal=True,cases=len(cases),tail_effect_members=tail_effects,negative_controls=len(negatives))),flush=True)
if __name__=='__main__':main()
