import argparse, hashlib, json, os, re, struct, subprocess, sys
from pathlib import Path
sys.path.insert(0, r'C:\tools\hercules\python')
import pefile
sys.path.insert(0, r'C:\tools\hercules\renderer-oracle-python')
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
 UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS)

DEFAULT_ORACLE=Path(r'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE')
EXPECTED='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
BASE=0x400000; STACK_BASE=0xc00000; STACK_SIZE=0x20000; STACK_TOP=STACK_BASE+STACK_SIZE-0x100; SENTINEL=STACK_BASE+0x1fff0
API_BASE=0xd00000; API_ENDPOINT={'GetAsyncKeyState':API_BASE+0x10,'joyGetPos':API_BASE+0x20,'joyGetDevCapsA':API_BASE+0x30,'joyGetPosEx':API_BASE+0x40}
FNV=16777619
ORIG_ADDR={'avail':0xb9e8a0,'caps':0xb9e8b0,'count':0xba0218,'config':0xb9e858,'state0':0x96e5d0,'state1':0x96e5d4,'pending':0xba0234,'gate':0x43a594,'table_a':0x96e4f0,'table_b':0x96e560}
ORIG_FUNCS={'initialize':BASE+0x7bb0,'sample':BASE+0x7c50,'joystick':BASE+0x7e90,'set_state':BASE+0x7f10,'keyboard':BASE+0x7f30,'collect':BASE+0x8040}
TOOLCHAIN=Path(r'C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953')

def repository_root(start):
 for candidate in (start, *start.parents):
  if (candidate/'scripts').is_dir() and (candidate/'toolchains'/'manifest.json').is_file():
   return candidate
 raise RuntimeError('cannot derive source root from runner/script installation location')

class Model:
 def __init__(self):
  self.pos_ok=0;self.caps_ok=0;self.ex_status=0;self.ex_write=True;self.x=500;self.y=500;self.buttons=0;self.keys=[0]*256;self.events=[];self.calls=[];self.hash=2166136261
 def reset_events(self): self.events=[];self.calls=[];self.hash=2166136261
 def record(self,kind,a=0,b=0,c=0):
  vals=(kind,a&0xffffffff,b&0xffffffff,c&0xffffffff);self.events.append(vals)
  h=self.hash
  for v in vals:h=((h^v)*FNV)&0xffffffff
  self.hash=h

class ImageEmu:
 def __init__(self,mode,oracle_path,fixture_exe=None,fixture_map=None):
  self.mode=mode
  if mode=='original':
   data=oracle_path.read_bytes();digest=hashlib.sha256(data).hexdigest()
   if digest!=EXPECTED:raise RuntimeError('retail oracle hash mismatch '+digest)
   self.functions=ORIG_FUNCS.copy();self.address=ORIG_ADDR.copy();self.image_hash=digest
   pe=pefile.PE(data=data);self.pe=pe
  elif mode=='candidate':
   data=fixture_exe.read_bytes();self.image_hash=hashlib.sha256(data).hexdigest();pe=pefile.PE(data=data);self.pe=pe
   symbols={}
   for line in fixture_map.read_text(encoding='ascii',errors='replace').splitlines():
    m=re.match(r'^\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\S+)\s+([0-9A-Fa-f]{8})',line)
    if m:symbols[m.group(1)]=int(m.group(2),16)
   def sym(n):
    k='_'+n
    if k not in symbols:raise RuntimeError('map symbol missing '+k)
    return symbols[k]
   self.functions={n:sym('host_input_'+name) for n,name in {'initialize':'initialize_7bb0','sample':'sample_7c50','joystick':'joystick_7e90','set_state':'set_last_state_7f10','keyboard':'keyboard_7f30','collect':'collect_8040'}.items()}
   self.address={n:sym(name) for n,name in {'avail':'g_host_input_joy_available_rva_79e8a0','caps':'g_host_input_joy_caps_rva_79e8b0','count':'g_host_input_joy_count_rva_7a0218','state0':'g_host_input_last_state_first_rva_56e5d0','state1':'g_host_input_last_state_second_rva_56e5d4','pending':'g_host_input_pending_rva_7a0234','gate':'g_host_input_gate_rva_03a594','table_a':'g_config_table_a_rva_56e4f0','table_b':'g_config_table_b_rva_56e560','config':'g_registry_config'}.items()}
  else:raise ValueError(mode)
  self.iat={};self.unexpected_iat={};next_unexpected=API_BASE+0x100
  imported=set()
  for ent in pe.DIRECTORY_ENTRY_IMPORT:
   dll=ent.dll.decode('ascii','replace').lower()
   for item in ent.imports:
    name=item.name.decode('ascii','replace') if item.name else f'ordinal:{item.ordinal}'
    if name in API_ENDPOINT:
     expected_dll='user32.dll' if name=='GetAsyncKeyState' else 'winmm.dll'
     if dll!=expected_dll:raise RuntimeError(f'{name} imported from unexpected DLL {dll}')
     if name in imported:raise RuntimeError(f'duplicate expected import {name}')
     imported.add(name);self.iat[name]=item.address
     # IAT endpoints are installed after memory mapping below.
    else:
     self.unexpected_iat[next_unexpected]={'dll':dll,'name':name,'iat':item.address}
     next_unexpected+=0x10
  if set(self.iat)!=set(API_ENDPOINT):raise RuntimeError('input IAT imports incomplete: '+repr(self.iat))
  self.uc=Uc(UC_ARCH_X86,UC_MODE_32)
  size=max(0x800000,self.pe.OPTIONAL_HEADER.SizeOfImage+0x10000)
  size=(size+0xfff)&~0xfff
  self.uc.mem_map(BASE,size)
  for sec in self.pe.sections:
   raw=data[sec.PointerToRawData:sec.PointerToRawData+sec.SizeOfRawData]
   if raw:self.uc.mem_write(BASE+sec.VirtualAddress,raw)
  self.uc.mem_map(STACK_BASE,STACK_SIZE);self.uc.mem_map(API_BASE,0x1000)
  for name,cell in self.iat.items():self.w32(cell,API_ENDPOINT[name])
  for endpoint,row in self.unexpected_iat.items():self.w32(row['iat'],endpoint)
  self.model=Model();self.write_log=[];self.capture_writes=False
  self.uc.hook_add(UC_HOOK_CODE,self.api_hook)
  self.uc.hook_add(UC_HOOK_MEM_WRITE,self.mem_write_hook)
  self.reset_state()
 def u32(self,a):return struct.unpack('<I',self.uc.mem_read(a,4))[0]
 def w32(self,a,v):self.uc.mem_write(a,struct.pack('<I',v&0xffffffff))
 def w16(self,a,v):self.uc.mem_write(a,struct.pack('<H',v&0xffff))
 def w8(self,a,v):self.uc.mem_write(a,bytes([v&255]))
 def write(self,a,b):self.uc.mem_write(a,b)
 def read(self,a,n):return bytes(self.uc.mem_read(a,n))
 def mem_write_hook(self,uc,access,address,size,value,user_data):
  if self.capture_writes:self.write_log.append((address,size,value&((1<<(min(size,8)*8))-1)))
 def reset_state(self):
  a=self.address;self.write(a['avail'],bytes(16));self.write(a['caps'],bytes(16*404));self.w32(a['count'],0)
  self.write(a['config'],bytes(72));self.w32(a['state0'],0);self.w32(a['state1'],0);self.w32(a['pending'],0);self.w8(a['gate'],1)
  self.write(a['table_a'],bytes(28*4));self.write(a['table_b'],bytes(28*4));self.model=Model();self.write_log=[]
 def api_hook(self,uc,addr,size,user_data):
  name=next((n for n,v in API_ENDPOINT.items() if v==addr),None)
  if name is None:
   if addr in self.unexpected_iat:
    row=self.unexpected_iat[addr]
    raise RuntimeError(f'forbidden/unmodeled imported endpoint entered: {row["dll"]}!{row["name"]}')
   return
  esp=uc.reg_read(UC_X86_REG_ESP);ret=self.u32(esp);m=self.model;eax=0;pop=0;raw_args=[];norm={}
  if name=='GetAsyncKeyState':
   key=self.u32(esp+4);key=key if key<0x80000000 else key-0x100000000
   value=m.keys[key]&0xffff if 0<=key<256 else 0
   if 0<=key<256:m.keys[key]&=0x8000
   m.record(4,key,value,0);raw_args=[key];norm={'key':key,'return':value};eax=value;pop=4
  elif name=='joyGetPos':
   dev=self.u32(esp+4);ptr=self.u32(esp+8);m.record(1,dev,0,0);raw_args=[dev]
   for off in (0,4,8,12):self.w32(ptr+off,0)
   okay=dev<32 and ((m.pos_ok>>dev)&1);eax=0 if okay else 1;pop=8
   norm={'device':dev,'output_bytes':16,'output_region':'stack','output_buffer_fits_stack':STACK_BASE<=ptr and ptr+16<=STACK_BASE+STACK_SIZE,'output_fields':[('wXpos',0,4),('wYpos',4,4),('wZpos',8,4),('wButtons',12,4)],'return':eax}
  elif name=='joyGetDevCapsA':
   dev=self.u32(esp+4);ptr=self.u32(esp+8);cb=self.u32(esp+12);m.record(2,dev,cb,0);raw_args=[dev,cb]
   okay=dev<32 and ((m.caps_ok>>dev)&1)
   if okay:
    self.w32(ptr+0x24,0);self.w32(ptr+0x28,1000);self.w32(ptr+0x2c,50);self.w32(ptr+0x30,1050)
   eax=0 if okay else 1;pop=12
   norm={'device':dev,'caps_offset':ptr-self.address['caps'],'cbSize':cb,'write_fields':({'wXmin':0,'wXmax':1000,'wYmin':50,'wYmax':1050} if okay else {}),'field_specs':[('wXmin',0x24,4),('wXmax',0x28,4),('wYmin',0x2c,4),('wYmax',0x30,4)],'return':eax}
  elif name=='joyGetPosEx':
   dev=self.u32(esp+4);ptr=self.u32(esp+8);cb=self.u32(ptr);flags=self.u32(ptr+4);m.record(3,dev,cb,flags);raw_args=[dev,cb,flags]
   if m.ex_write:
    self.w32(ptr+8,m.x);self.w32(ptr+12,m.y);self.w32(ptr+0x20,m.buttons)
   eax=m.ex_status;pop=8
   norm={'device':dev,'dwSize':cb,'dwFlags':flags,'output_region':'stack','output_buffer_fits_stack':STACK_BASE<=ptr and ptr+52<=STACK_BASE+STACK_SIZE,'write':m.ex_write,'output_fields':([('dwXpos',8,4),('dwYpos',12,4),('dwButtons',0x20,4)] if m.ex_write else []),'return':eax}
  m.calls.append({'api':name,'args':raw_args,'normalized':norm,'callee_pop_bytes':pop})
  uc.reg_write(UC_X86_REG_EAX,eax&0xffffffff);uc.reg_write(UC_X86_REG_ESP,esp+4+pop);uc.reg_write(UC_X86_REG_EIP,ret)
 def call(self,key,args=(),track=False):
  pc=self.functions[key];stack=STACK_TOP-4*(1+len(args));vals=[SENTINEL,*[v&0xffffffff for v in args]]
  self.write(stack,struct.pack('<'+'I'*len(vals),*vals))
  for r,v in [(UC_X86_REG_EAX,0x11111111),(UC_X86_REG_EBX,0x22222222),(UC_X86_REG_ECX,0x33333333),(UC_X86_REG_EDX,0x44444444),(UC_X86_REG_ESI,0x55555555),(UC_X86_REG_EDI,0x66666666),(UC_X86_REG_EBP,0x77777777),(UC_X86_REG_ESP,stack),(UC_X86_REG_EFLAGS,0x202),(UC_X86_REG_EIP,pc)]:self.uc.reg_write(r,v)
  self.capture_writes=track;self.write_log=[]
  self.uc.emu_start(pc,SENTINEL,count=2000000)
  self.capture_writes=False
  if self.uc.reg_read(UC_X86_REG_EIP)!=SENTINEL:raise RuntimeError(f'{self.mode} {key} did not return, EIP={self.uc.reg_read(UC_X86_REG_EIP):#x}')
  return self.uc.reg_read(UC_X86_REG_EAX)&0xffffffff
 def mask(self,addr,n):return sum(1<<i for i,b in enumerate(self.read(addr,n)) if b)
 def fnv(self,data):
  h=2166136261
  for b in data:h=((h^b)*FNV)&0xffffffff
  return h
 def summary(self,name,value):
  a=self.address;ev=self.model.events
  return {'name':name,'value':value&0xffffffff,'pending':self.u32(a['pending']),'count':self.u32(a['count']),'avail':self.mask(a['avail'],16),'state0':self.u32(a['state0']),'state1':self.u32(a['state1']),'events':len(ev),'trace':self.model.hash,'first':ev[0] if ev else None,'last':ev[-1] if ev else None,'event_sequence':ev.copy(),'api_calls':self.model.calls.copy(),'event_sequence_sha256':hashlib.sha256(json.dumps(ev,separators=(',',':')).encode()).hexdigest(),'api_sequence_sha256':hashlib.sha256(json.dumps(self.model.calls,separators=(',',':')).encode()).hexdigest()}
 def init_snapshot(self):
  a=self.address;cap=self.read(a['caps'],16*404);avail=self.read(a['avail'],16);keys=struct.pack('<256H',*[x&0xffff for x in self.model.keys])
  return {'availability_hex':avail.hex(),'availability_sha256':hashlib.sha256(avail).hexdigest(),'capability_bytes':len(cap),'capability_sha256':hashlib.sha256(cap).hexdigest(),'count':self.u32(a['count']),'cache_state_dwords':[self.u32(a['state0']),self.u32(a['state1'])],'async_key_cache_bytes':len(keys),'async_key_cache_sha256':hashlib.sha256(keys).hexdigest(),'async_key_cache_nonzero_count':sum(1 for i in range(256) if self.model.keys[i]&0xffff)}

def field(v):return f'{v:08x}'
def render(row):
 s=(f"{row['name']} value={field(row['value'])} pending={field(row['pending'])} count={row['count']} avail={field(row['avail'])} state0={field(row['state0'])} state1={field(row['state1'])} events={row['events']} trace={field(row['trace'])}")
 if row['first'] is not None:
  def e(x):return f'{x[0]}:{field(x[1])}:{field(x[2])}:{field(x[3])}'
  s+=f' first={e(row["first"])} last={e(row["last"])}'
 return s

def write_contract(o):
 a=o.address;caps=(a['caps'],a['caps']+16*404);avail=(a['avail'],a['avail']+16);count=(a['count'],a['count']+4);s0=(a['state0'],a['state0']+4);s1=(a['state1'],a['state1']+4)
 rows=[]
 for addr,n,val in o.write_log:
  if addr>=caps[0] and addr+n<=caps[1]:region='caps'
  elif addr>=avail[0] and addr+n<=avail[1]:region='availability'
  elif addr>=count[0] and addr+n<=count[1]:region='count'
  elif addr>=s0[0] and addr+n<=s0[1]:region='cache0'
  elif addr>=s1[0] and addr+n<=s1[1]:region='cache1'
  elif STACK_BASE<=addr and addr+n<=STACK_BASE+STACK_SIZE:region='stack'
  else:region='outside'
  rows.append({'address':hex(addr),'size':n,'region':region})
 regions=('caps','availability','count','cache0','cache1','stack','outside')
 extents={}
 for region in regions:
  selected=[x for x in rows if x['region']==region]
  covered=set()
  for x in selected:covered.update(range(int(x['address'],16),int(x['address'],16)+x['size']))
  extents[region]={'writes':len(selected),'bytes_covered':len(covered),'min_address':hex(min(covered)) if covered else None,'end_address':hex(max(covered)+1) if covered else None,'write_sizes':sorted(set(x['size'] for x in selected))}
 bases={'caps':a['caps'],'availability':a['avail'],'count':a['count'],'cache0':a['state0'],'cache1':a['state1'],'stack':STACK_BASE}
 relative_trace=[[x['region'],int(x['address'],16)-bases.get(x['region'],0),x['size']] for x in rows]
 return {'tracked_write_count':len(rows),'normalized_relative_trace_sha256':hashlib.sha256(json.dumps(relative_trace,separators=(',',':')).encode()).hexdigest(),'normalized_relative_trace_count':len(relative_trace),'region_counts':{r:sum(1 for x in rows if x['region']==r) for r in regions},'region_extents':extents,'outside_write_examples':[x for x in rows if x['region']=='outside'][:12]}

def run_suite(o):
 rows=[];a=o.address;m=o.model
 o.reset_state();m=o.model;o.write(a['caps'],b'\xa5'*(16*404));o.write(a['avail'],b'\x5a'*16);o.w32(a['count'],0x1234);o.w32(a['state0'],0x11111111);o.w32(a['state1'],0x22222222)
 m.pos_ok=(1<<0)|(1<<1)|(1<<2)|(1<<15);m.caps_ok=(1<<0)|(1<<2)|(1<<15)
 for i in range(256):m.keys[i]=0x0001 if i&1 else 0x8000
 v=o.call('initialize',track=True);init=o.init_snapshot();init_writes=write_contract(o);row=o.summary('initialize',o.fnv(o.read(a['caps'],16*404)));rows.append(row)
 initline=f"initialize_caps_hash={field(row['value'])} key0={m.keys[0]:04x} key1={m.keys[1]:04x}"
 calls=row['api_calls'];init['api_write_contract']={'api_counts':{n:sum(1 for c in calls if c['api']==n) for n in API_ENDPOINT},'joyGetDevCapsA_cbSize_values':sorted(set(c['normalized']['cbSize'] for c in calls if c['api']=='joyGetDevCapsA')),'joyGetCaps_offsets':[c['normalized']['caps_offset'] for c in calls if c['api']=='joyGetDevCapsA'],'api_sequence_sha256':row['api_sequence_sha256']}
 # sampling boundary, joyGetPosEx failure and unavailable slot
 for name,x,y,buttons,status,write in [('sample_thresholds',335,665,0xf0000001,0,True),('sample_outside',330,670,0xf0000000,0,True),('sample_failure',500,500,0,1,False)]:
  o.reset_state();m=o.model;o.w8(a['avail'],1);o.w32(a['caps']+0x24,0);o.w32(a['caps']+0x28,1000);o.w32(a['caps']+0x2c,0);o.w32(a['caps']+0x30,1000);m.x=x;m.y=y;m.buttons=buttons;m.ex_status=status;m.ex_write=write
  v=o.call('sample',(0,));rows.append(o.summary(name,v))
 o.reset_state();o.w8(a['avail'],1);rows.append(o.summary('sample_unavailable',o.call('sample',(1,))))
 # mapping and disabled id
 o.reset_state();m=o.model;o.w8(a['avail'],1);o.w32(a['caps']+0x24,0);o.w32(a['caps']+0x28,1000);o.w32(a['caps']+0x2c,0);o.w32(a['caps']+0x30,1000);o.w16(a['config']+0x16,0);o.w32(a['table_a'],3);o.w32(a['table_b'],0x4000);m.x=670;m.y=500;m.buttons=3;rows.append(o.summary('joystick_remap',o.call('joystick')))
 o.reset_state();o.w16(a['config']+0x16,0xffff);rows.append(o.summary('joystick_disabled',o.call('joystick')))
 # keyboard disabled, Alt edge, fixed + configurable mappings
 o.reset_state();o.w8(a['config']+0x14,0);rows.append(o.summary('keyboard_disabled',o.call('keyboard')))
 o.reset_state();m=o.model;o.w8(a['config']+0x14,1);m.keys[0x12]=1;rows.append(o.summary('keyboard_alt_edge',o.call('keyboard')));altline=f'keyboard_alt_edge_left={m.keys[0x12]:04x}'
 o.reset_state();m=o.model;o.w8(a['config']+0x14,1);o.w32(a['state0'],0x20);o.w32(a['state1'],0x80)
 for i in range(9):o.w16(a['config']+0x1c+2*i,65+i)
 m.keys[0x1b]=m.keys[0x20]=m.keys[0x0d]=0x8000
 for i in range(9):m.keys[65+i]=0x8000
 m.keys[66]=1;rows.append(o.summary('keyboard_mapping',o.call('keyboard')))
 # sentinel setter
 o.reset_state();o.w32(a['state0'],0x12345678);o.w32(a['state1'],0x87654321);o.call('set_state',(0xffffffff,0x0badc0de));rows.append(o.summary('last_state_sentinel',0))
 # collector scenarios
 def prepare_collect():
  o.reset_state();m=o.model;o.w8(a['config']+0x14,1);o.w16(a['config']+0x1c,65);o.w8(a['avail'],1);o.w32(a['caps']+0x24,0);o.w32(a['caps']+0x28,1000);o.w32(a['caps']+0x2c,0);o.w32(a['caps']+0x30,1000);o.w16(a['config']+0x16,0);o.w32(a['table_a'],1);o.w32(a['table_b'],0x40);m.keys[65]=0x8000;m.buttons=1;return m
 m=prepare_collect();rows.append(o.summary('collect_priority',o.call('collect')))
 m=prepare_collect();o.w32(a['pending'],1);m.keys[65]=0x8000;rows.append(o.summary('collect_pending_with_key',o.call('collect')))
 m=prepare_collect();o.w32(a['pending'],1);m.keys[65]=0;rows.append(o.summary('collect_pending_without_key',o.call('collect')))
 m=prepare_collect();o.w8(a['gate'],0);m.keys[65]=0x8000;rows.append(o.summary('collect_gate_zero',o.call('collect')))
 lines=[render(rows[0]),initline]+[render(r) for r in rows[1:9]]+[altline]+[render(r) for r in rows[9:11]]+[render(r) for r in rows[11:]]
 return {'rows':rows,'lines':lines,'initialization_state':init,'initialization_writes':init_writes}

def compact_api(c):
 n=c['normalized'];extra=[]
 if c['api']=='joyGetPos':extra=[n['output_bytes'],n['output_fields']]
 elif c['api']=='joyGetDevCapsA':extra=[n['caps_offset'],n['cbSize'],n['field_specs']]
 elif c['api']=='joyGetPosEx':extra=[n['dwSize'],n['dwFlags'],n['write'],n['output_fields']]
 return [c['api'],c['args'],n.get('return'),c['callee_pop_bytes'],extra]

def api_bounds(rows):
 checks=[];pop={'GetAsyncKeyState':4,'joyGetPos':8,'joyGetDevCapsA':12,'joyGetPosEx':8};
 for row in rows:
  for c in row['api_calls']:
   n=c['api'];v=c['normalized'];ok=c['callee_pop_bytes']==pop[n]
   if n=='GetAsyncKeyState':ok=ok and len(c['args'])==1 and 0<=v['return']<=0xffff
   elif n=='joyGetPos':ok=ok and v['output_bytes']==16 and v['output_buffer_fits_stack'] and all(off+width<=16 for _,off,width in v['output_fields'])
   elif n=='joyGetDevCapsA':ok=ok and v['cbSize']==404 and v['caps_offset']==v['device']*404 and 0<=v['caps_offset'] and v['caps_offset']+v['cbSize']<=16*404 and all(off+width<=v['cbSize'] for _,off,width in v['field_specs'])
   elif n=='joyGetPosEx':ok=ok and v['dwSize']==52 and v['dwFlags']==0x83 and v['output_buffer_fits_stack'] and all(off+width<=v['dwSize'] for _,off,width in v['output_fields'])
   checks.append({'api':n,'okay':ok,'args':c['args'],'normalized':v,'callee_pop_bytes':c['callee_pop_bytes']})
 counts={n:sum(1 for c in checks if c['api']==n) for n in API_ENDPOINT}
 def unique(name,selector):
  vals=[]
  for c in checks:
   if c['api']==name:
    v=selector(c)
    if v not in vals:vals.append(v)
  return vals
 return {'all_calls_within_SDK_argument_and_buffer_contract':all(c['okay'] for c in checks),'call_count':len(checks),'counts_by_api':counts,
  'callee_pop_bytes_by_api':pop,
  'JOYINFO_contract':{'bytes':16,'field_offsets_widths':[['wXpos',0,4],['wYpos',4,4],['wZpos',8,4],['wButtons',12,4]],'observed_call_count':counts['joyGetPos']},
  'JOYCAPSA_contract':{'bytes':404,'observed_cbSize_values':unique('joyGetDevCapsA',lambda c:c['normalized']['cbSize']),'observed_record_offsets':unique('joyGetDevCapsA',lambda c:c['normalized']['caps_offset']),'field_offsets_widths':[['wXmin',36,4],['wXmax',40,4],['wYmin',44,4],['wYmax',48,4]],'observed_return_codes':unique('joyGetDevCapsA',lambda c:c['normalized']['return'])},
  'JOYINFOEX_contract':{'bytes':52,'observed_dwSize_values':unique('joyGetPosEx',lambda c:c['normalized']['dwSize']),'observed_dwFlags_values':unique('joyGetPosEx',lambda c:c['normalized']['dwFlags']),'field_offsets_widths':[['dwXpos',8,4],['dwYpos',12,4],['dwButtons',32,4]],'observed_return_codes':unique('joyGetPosEx',lambda c:c['normalized']['return']),'observed_write_modes':unique('joyGetPosEx',lambda c:c['normalized']['write'])},
  'GetAsyncKeyState_contract':{'arg_count':1,'return_type':'SHORT','observed_count':counts['GetAsyncKeyState']},
  'failed_checks':[c for c in checks if not c['okay']]}

def tool_env(toolchain):
 env=os.environ.copy()
 env.pop('CL',None);env.pop('_CL_',None)
 env['PATH']=str(toolchain/'bin')+os.pathsep+env.get('PATH','')
 env['INCLUDE']=str(toolchain/'include')
 env['LIB']=str(toolchain/'lib')
 return env

def run_checked(command,cwd,env):
 result=subprocess.run(command,cwd=cwd,env=env,capture_output=True,text=True)
 if result.returncode:
  raise RuntimeError('command failed (%d): %r\n%s\n%s'%(result.returncode,command,result.stdout,result.stderr))
 return {'command':command,'returncode':result.returncode,'stdout':result.stdout,'stderr':result.stderr}

def write_json(path,value):
 path.parent.mkdir(parents=True,exist_ok=True)
 path.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')

def main():
 parser=argparse.ArgumentParser(description='Fresh VC5 host-input compile, strict COFF diagnostics, and pinned-original synthetic-IAT differential.')
 parser.add_argument('--source-root',type=Path,default=repository_root(Path(__file__).resolve()))
 parser.add_argument('--native-exe',type=Path,default=DEFAULT_ORACLE,help='read-only pinned retail oracle executable')
 parser.add_argument('--output',type=Path,help='JSON report path under source-root/work')
 args=parser.parse_args()
 root=args.source_root.resolve()
 native=args.native_exe.resolve()
 lane=root/'work'/'host_input_recovery'
 build=lane/'runner_build'
 build.mkdir(parents=True,exist_ok=True)
 output=(args.output if args.output else lane/'runner_report.json')
 if not output.is_absolute():output=root/output
 output=output.resolve()
 try:output.relative_to((root/'work').resolve())
 except ValueError:raise SystemExit('--output must remain under source-root/work')
 source=root/'src/pc/host_input.c'
 providers=[root/'src/pc/host_startup_storage.c',root/'src/shared/registry_storage.c']
 fixture=build/'runner_fixture.c'
 fixture_text='''#include <windows.h>
#include <mmsystem.h>
void host_input_initialize_7bb0(void);
DWORD host_input_sample_7c50(int);
DWORD host_input_joystick_7e90(void);
void host_input_set_last_state_7f10(DWORD, DWORD);
DWORD host_input_keyboard_7f30(void);
DWORD host_input_collect_8040(void);
static volatile DWORD runner_sink;
int main(void)
{
    host_input_initialize_7bb0();
    runner_sink ^= host_input_sample_7c50(0);
    runner_sink ^= host_input_joystick_7e90();
    host_input_set_last_state_7f10(0xffffffffUL, 0xffffffffUL);
    runner_sink ^= host_input_keyboard_7f30();
    runner_sink ^= host_input_collect_8040();
    return (int)runner_sink;
}
'''
 fixture.write_text(fixture_text,encoding='utf-8',newline='\n')
 if not source.is_file():raise SystemExit('missing canonical source: '+str(source))
 for p in providers:
  if not p.is_file():raise SystemExit('missing measured storage provider: '+str(p))
 if not native.is_file():raise SystemExit('missing pinned native oracle: '+str(native))
 toolchain=TOOLCHAIN
 compiler=toolchain/'bin'/'cl.exe'
 if not compiler.is_file():raise SystemExit('pinned historical compiler not found: '+str(compiler))
 oracle_hash=hashlib.sha256(native.read_bytes()).hexdigest()
 if oracle_hash!=EXPECTED:raise SystemExit('pinned oracle SHA-256 mismatch: '+oracle_hash)
 headers={'windows.h':toolchain/'include'/'windows.h','mmsystem.h':toolchain/'include'/'mmsystem.h','string.h':toolchain/'include'/'string.h','host_startup_storage.h':root/'src/pc/host_startup_storage.h'}
 for p in headers.values():
  if not p.is_file():raise SystemExit('missing compile header: '+str(p))
 sys.path.insert(0,str(root/'scripts'))
 import match
 from coff import COFF
 from masked import masked_compare
 env=tool_env(toolchain)
 input_snapshot={str(p.resolve()):hashlib.sha256(p.read_bytes()).hexdigest() for p in [source,*providers,fixture,*headers.values(),native,compiler,root/'toolchains'/'manifest.json']}
 compile_records=[]
 source_paths=[source,*providers,fixture]
 objects=[]
 for path in source_paths:
  obj=build/(path.stem+'_fresh.obj')
  command=[str(compiler),'/nologo','/c','/Gy','/O2','/Fo'+str(obj),str(path.resolve())]
  rec=run_checked(command,build,env)
  rec.update(source=str(path.resolve()),source_sha256=input_snapshot[str(path.resolve())],object=str(obj.resolve()),object_sha256=hashlib.sha256(obj.read_bytes()).hexdigest())
  compile_records.append(rec);objects.append(obj)
 input_obj=objects[0]
 object_hash=hashlib.sha256(input_obj.read_bytes()).hexdigest()
 exe=build/'host_input_native_import_fixture.exe'
 map_file=build/'host_input_native_import_fixture.map'
 link_command=[str(compiler),'/nologo','/O2','/Gy','/Fe'+str(exe),*[str(p.resolve()) for p in objects],'/link','user32.lib','winmm.lib','/INCREMENTAL:NO','/OPT:REF','/MAP:'+str(map_file.resolve())]
 link_record=run_checked(link_command,build,env)
 if not exe.is_file() or not map_file.is_file():raise RuntimeError('VC5 link returned without fixture PE/map')
 linked_object_hash=hashlib.sha256(input_obj.read_bytes()).hexdigest()
 if linked_object_hash!=object_hash:raise RuntimeError('input COFF changed between raw comparison and fixture link')
 header_hashes={n:hashlib.sha256(p.read_bytes()).hexdigest() for n,p in headers.items()}
 raw_rows=[('host_input_initialize_7bb0',0x7bb0,0xa0),
  ('host_input_sample_7c50',0x7c50,0x240),
  ('host_input_joystick_7e90',0x7e90,0x80),
  ('host_input_set_last_state_7f10',0x7f10,0x20),
  ('host_input_keyboard_7f30',0x7f30,0x110),
  ('host_input_collect_8040',0x8040,0x50)]
 coff=COFF(input_obj)
 function_comparisons=[]
 for sym,rva,size in raw_rows:
  section=coff.function('_'+sym)
  function_comparisons.append({'symbol':'_'+sym,'rva':hex(rva),'target_extent':size,'object_contribution_bytes':section['size'],'object_relocations':section['relocations'],'raw':match.compare(input_obj,'_'+sym,native,rva,size),'masked_diagnostic':masked_compare(input_obj,'_'+sym,native,rva,size)})
 orig=ImageEmu('original',native)
 cand=ImageEmu('candidate',native,exe,map_file)
 ro=run_suite(orig);rc=run_suite(cand)
 line_equal=ro['lines']==rc['lines']
 rows_equal=[]
 full_scenario_traces=[]
 for x,y in zip(ro['rows'],rc['rows']):
  rows_equal.append({'scenario':x['name'],'summary_fields_equal':{k:x[k]==y[k] for k in ('value','pending','count','avail','state0','state1','events','trace')},'event_sequence_equal':x['event_sequence']==y['event_sequence'],'event_count':len(x['event_sequence']),'oracle_event_sequence_sha256':x['event_sequence_sha256'],'candidate_event_sequence_sha256':y['event_sequence_sha256'],'api_order_args_returns_and_abi_equal':x['api_calls']==y['api_calls'],'api_call_count':len(x['api_calls']),'oracle_api_sequence_sha256':x['api_sequence_sha256'],'candidate_api_sequence_sha256':y['api_sequence_sha256']})
  full_scenario_traces.append({'scenario':x['name'],'oracle_state':{k:x[k] for k in ('value','pending','count','avail','state0','state1','events','trace')},'candidate_state':{k:y[k] for k in ('value','pending','count','avail','state0','state1','events','trace')},'oracle_event_sequence':x['event_sequence'],'candidate_event_sequence':y['event_sequence'],'oracle_api_calls':x['api_calls'],'candidate_api_calls':y['api_calls']})
 all_event=all(x['event_sequence_equal'] for x in rows_equal)
 all_api=all(x['api_order_args_returns_and_abi_equal'] for x in rows_equal)
 oracle_api=api_bounds(ro['rows']);candidate_api=api_bounds(rc['rows'])
 expected_counts={'GetAsyncKeyState':322,'joyGetPos':16,'joyGetDevCapsA':4,'joyGetPosEx':8}
 init_exact=ro['initialization_state']==rc['initialization_state']
 init_write_equal=(ro['initialization_writes']['normalized_relative_trace_sha256']==rc['initialization_writes']['normalized_relative_trace_sha256'] and ro['initialization_writes']['normalized_relative_trace_count']==rc['initialization_writes']['normalized_relative_trace_count'])
 init_write_contract=rc['initialization_writes']
 region_extents=init_write_contract['region_extents']
 init_write_ok=(init_write_contract['region_counts']['outside']==0 and
  region_extents['caps']['bytes_covered']==16*404 and region_extents['availability']['bytes_covered']==16 and
  region_extents['count']['bytes_covered']==4 and region_extents['cache0']['bytes_covered']==4 and
  region_extents['cache1']['bytes_covered']==4)
 api_contract_ok=(oracle_api['all_calls_within_SDK_argument_and_buffer_contract'] and candidate_api['all_calls_within_SDK_argument_and_buffer_contract'] and
  oracle_api['call_count']==350 and candidate_api['call_count']==350 and
  oracle_api['counts_by_api']==expected_counts and candidate_api['counts_by_api']==expected_counts)
 if len(ro['rows'])!=15 or len(rc['rows'])!=15 or len(ro['lines'])!=17 or len(rc['lines'])!=17:raise RuntimeError('scenario matrix changed unexpectedly')
 if sum(len(r['api_calls']) for r in ro['rows'])!=350 or sum(len(r['api_calls']) for r in rc['rows'])!=350:raise RuntimeError('API contract count changed unexpectedly')
 final_snapshot={str(p.resolve()):hashlib.sha256(p.read_bytes()).hexdigest() for p in [source,*providers,fixture,*headers.values(),native,compiler,root/'toolchains'/'manifest.json']}
 if final_snapshot!=input_snapshot:raise RuntimeError('source/header/oracle/compiler input changed during the run')
 report={'scope':'Six recovered host-input functions freshly compiled with stock VC5 and actual SDK imports. The same exact input COFF object is raw-compared and linked with only source-owned shared storage providers; candidate PE and pinned retail PE are executed as bounded Unicorn functions with synthetic IAT endpoints. No retail process, original DLL initializer, real API, or fixture process is launched.',
  'runner_path':str(Path(__file__).resolve()),'runner_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'runner_argv':[str(x) for x in sys.argv],'source_root':str(root),'source':str(source.resolve()),'source_sha256':input_snapshot[str(source.resolve())],
  'provider_sources':[{'path':str(p.resolve()),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in providers],
  'header_sha256':header_hashes,'fixture_source':str(fixture.resolve()),'fixture_source_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),
  'input_object':str(input_obj.resolve()),'input_object_sha256':object_hash,'raw_comparison_object_sha256':object_hash,'linked_fixture_object_sha256':linked_object_hash,
  'compile_commands':compile_records,'link_command':link_record,'fixture_exe':str(exe.resolve()),'fixture_exe_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'fixture_map':str(map_file.resolve()),
  'native_exe':str(native),'native_exe_sha256':oracle_hash,'toolchain_id':match.verify_toolchain(toolchain),'toolchain_manifest_sha256':input_snapshot[str((root/'toolchains'/'manifest.json').resolve())],'compiler_path':str(compiler.resolve()),'compiler_sha256':input_snapshot[str(compiler.resolve())],'toolchain_tree_sha256':'29fc5aea67aa10a8135d65f8f371f7456bfc10ec8ef36dc0050ca5406bf9d92e',
  'functions':{k:hex(v) for k,v in cand.functions.items()},'candidate_sdk_iat':{k:hex(v) for k,v in cand.iat.items()},'fixture_only_unmodeled_import_count':len(cand.unexpected_iat),'fixture_only_unmodeled_endpoints':'All other PE imports are rebound to fail-on-entry synthetic endpoints; any invocation aborts the runner.',
  'function_comparisons':function_comparisons,
  'scenario_count':len(ro['rows']),'rendered_output_line_count':len(ro['lines']),'exact_rendered_line_equality':line_equal,'all_event_order_value_sequences_equal':all_event,'all_api_order_args_returns_and_abi_records_equal':all_api,'scenario_results':rows_equal,'full_scenario_traces':full_scenario_traces,
  'api_call_count':sum(len(r['api_calls']) for r in ro['rows']),'stock_api_contracts':{'expected_call_count':350,'expected_counts_by_api':expected_counts,'all_checks_passed':api_contract_ok,'oracle':oracle_api,'candidate':candidate_api},
  'initialization_complete_state':{'exact_equal':init_exact,'oracle':ro['initialization_state'],'candidate':rc['initialization_state']},
  'initializer_global_write_contract':{'exact_equal':init_write_equal,'checks_passed':init_write_ok,'oracle':ro['initialization_writes'],'candidate':rc['initialization_writes']},
  'input_failure_boundaries':{'enumeration_synthetic_results':'joyGetPos succeeds for devices 0,1,2,15 and fails for other queried slots; joyGetDevCapsA then fails for device 1 and succeeds for 0,2,15.','joyGetPosEx':'One case returns status 1 without writing its 52-byte JOYINFOEX; other sampled cases provide full synthetic x/y/buttons writes.','outside_contract':'Invalid configured device IDs except the 0xffff disabled sentinel, partial/overrunning driver writes, additional WinMM errors, unusual key values, and actual OS/driver behavior are not established.'},
  'oracle_lines':ro['lines'],'candidate_lines':rc['lines']}
 write_json(output,report)
 print(json.dumps({'report':str(output),'source_sha256':report['source_sha256'],'input_object_sha256':object_hash,'fixture_exe_sha256':report['fixture_exe_sha256'],'scenario_count':len(ro['rows']),'rendered_output_line_count':len(ro['lines']),'api_call_count':report['api_call_count'],'api_counts':expected_counts,'exact_rendered_line_equality':line_equal,'all_event_order_value_sequences_equal':all_event,'all_api_order_args_returns_and_abi_records_equal':all_api,'initialization_complete_state_equal':init_exact,'initializer_write_contract_passed':init_write_ok,'api_contracts_passed':api_contract_ok,'raw_exact_functions':[r['symbol'] for r in function_comparisons if r['raw']['exact']]},indent=2))
 if not(line_equal and all_event and all_api and init_exact and init_write_equal and init_write_ok and api_contract_ok):return 1
 return 0

if __name__=='__main__':
 try:sys.exit(main())
 except Exception as exc:
  print(type(exc).__name__+': '+str(exc),file=sys.stderr)
  raise
