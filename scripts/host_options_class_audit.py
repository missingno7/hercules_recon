"""Static PC Options class/code/data audit; no UI, process or runtime acceptance."""
import argparse,hashlib,json,re,struct,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python']
from match import compile_source,compare,verify_toolchain,DEFAULT_TOOLCHAIN
from masked import masked_compare
from coff import COFF
from pe import PE
from title_behavioral import context_files
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'work/options_class_audit')
args=parser.parse_args();OUT=args.output.resolve();assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
SOURCES=[ROOT/'src/pc/host_options_classes.cpp',ROOT/'src/pc/host_frame_options.cpp']
HEADER=ROOT/'calibration/host_options_classes.h'
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
CLASSES=[('CKeyboardPage',744,0x2e408,0x2e470,62),('CJoystickPage',560,0x2e290,0x2e310,62),
 ('CSoundPage',328,0x2eb88,0x2ebf0,62),('CGamePlayPage',388,0x2e008,0x2e070,62),('COptionsSheet',2156,0x2ece8,0x2ed20,51)]
CONTROLS=['CEdit','CButton','CListBox','CSliderCtrl','CStatic']
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()


def run():
 assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
 verify_toolchain(DEFAULT_TOOLCHAIN)
 state=json.loads((ROOT/'recovery.json').read_bytes());rows=[]
 for item in state['functions']:
  if item['source']not in ['src/pc/host_options_classes.cpp','src/pc/host_frame_options.cpp']:continue
  name=item['symbol'];cls=next(c for c in [x[0]for x in CLASSES]+['CMainFrame']if c+'@@'in name)
  role=('ctor'if name.startswith('??0')else'dtor'if name.startswith('??1')else'deleter'if name.startswith('??_G')else'factory'if name.startswith('?CreateObject@')else'runtime'if name.startswith('?GetRuntimeClass@')else'getmap'if name.startswith('?GetMessageMap@')else'ddx'if name.startswith('?DoDataExchange@')else'options')
  rows.append(dict(cls=cls,role=role,symbol=name,rva=item['rva'],size=item['size'],flags=item['flags']))
 assert len(rows)==34

 watched={ORACLE,Path(__file__),ROOT/'toolchains/manifest.json',ROOT/'recovery.json',DEFAULT_TOOLCHAIN/'mfc/lib/nafxcw.lib'}
 for p in SOURCES:watched.update(context_files(p))
 watched.update(ROOT/'scripts'/p for p in ['match.py','coff.py','masked.py','pe.py','env.py','title_behavioral.py'])
 pins={str(p):sha(p)for p in watched};pe=PE(ORACLE);measurements=[];compiles=[];coffs=[];bindings={};binding_records=[];data=[];strings={}
 native_relocations={e.rva for b in getattr(pe.pe,'DIRECTORY_ENTRY_BASERELOC',[])for e in b.entries if e.type}
 has_native_relocations=pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size!=0
 def symbol(co,name):
  candidates=[x for x in co.symbols.values()if x['name']==name]
  assert candidates,name
  return next((x for x in candidates if x['section']>0),candidates[0])
 def key(co,name):
  s=symbol(co,name)
  return name if s['storage']in [2,105]else(str(id(co)),name)
 def bind(co,name,address,where):
  k=key(co,name)
  assert k not in bindings or bindings[k]==address,('inconsistent symbol binding',name,hex(bindings.get(k,0)),hex(address),where)
  bindings[k]=address;binding_records.append(dict(symbol=name,rva=hex(address),where=where,global_symbol=isinstance(k,str)))
 def executable(rva):return any(s['characteristics']&0x20000000 and s['virtual_address']<=rva<s['virtual_address']+s['raw_size']for s in pe.sections)
 def measure(co,obj,row):
  rva=int(row['rva'],0);raw=compare(obj,row['symbol'],ORACLE,rva,row['size']);diag=masked_compare(obj,row['symbol'],ORACLE,rva,row['size'])
  assert diag['masked_equal'],(row,diag)
  bind(co,row['symbol'],rva,row['role'])
  for name,address in diag['symbols'].items():bind(co,name,int(address,0),row['symbol'])
  return dict(**row,raw=raw,diagnostic=diag)
 for index,p in enumerate(SOURCES):
  flags=[x['flags']for x in rows if(x['cls']=='CMainFrame')==(index==1)];assert flags and all(x==flags[0]for x in flags)
  obj,command=compile_source(p,flags[0]);co=COFF(obj);coffs.append((co,obj))
  compiles.append(dict(source=str(p),source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=command))
  for row in rows:
   if(row['cls']=='CMainFrame')==(index==1):measurements.append(measure(co,obj,row))
 assert len(measurements)==34
 def audit(co,name,rva,n,kind):
  s=symbol(co,name);assert s['section']>0;s0=s['value'];sec=co.sections[s['section']-1]
  assert s0+n<=sec['size']and len(sec['data'])==sec['size']
  left=bytearray(sec['data'][s0:s0+n]);right=bytearray(pe.read_rva(rva,n));relocations=[]
  bind(co,name,rva,kind)
  for rel in sec['relocations']:
   if s0<=rel['offset']<s0+n:
    off=rel['offset']-s0;assert off+4<=n and rel['type']==6
    addend=struct.unpack_from('<i',left,off)[0];value=struct.unpack_from('<I',right,off)[0];target=(value-pe.image_base-addend)&0xffffffff
    if kind=='vtable':assert executable(target),(name,off,hex(target))
    bind(co,rel['symbol'],target,name)
    relocations.append(dict(offset=off,symbol=rel['symbol'],target_rva=hex(target),addend=addend))
    left[off:off+4]=bytes(4);right[off:off+4]=bytes(4)
  assert left==right,(name,kind,'scalar/data bytes differ')
  offsets={x['offset']for x in relocations}
  if kind=='vtable':expected_offsets=set(range(0,n,4))
  elif kind=='runtime_descriptor':expected_offsets={0,16}|({12}if struct.unpack_from('<I',pe.read_rva(rva,n),12)[0]else set())
  elif kind=='message_map':expected_offsets={0,4}
  elif kind=='message_entries':expected_offsets={20+i*24 for i in range(n//24-1)}
  else:expected_offsets=set()
  assert offsets==expected_offsets,(name,'typed pointer-field footprint')
  if has_native_relocations:assert {rva+x for x in offsets}=={x for x in native_relocations if rva<=x<rva+n},(name,'native base-relocation footprint')
  item=dict(symbol=name,kind=kind,rva=hex(rva),bytes=n,source_section=s['section'],source_offset=s0,relocations=relocations,all_scalar_bytes_equal=True)
  data.append(item);return item
 # Genuine base-class archive code establishes the inherited slot identities.
 archive=(DEFAULT_TOOLCHAIN/'mfc/lib/nafxcw.lib').read_bytes();pos,index=8,0
 while pos<len(archive):
  h=archive[pos:pos+60];assert h[58:]==b'`\n';n=int(h[48:58])
  if index==121:break
  pos+=60+n+(n&1);index+=1
 assert index==121;libobj=OUT/'mfc_member_121.obj';libobj.write_bytes(archive[pos+60:pos+60+n]);lib=COFF(libobj);library=[]
 for name,rva,n in [('??0CPropertyPage@@QAE@II@Z',0x2877d,79),('??0CPropertySheet@@QAE@PBDPAVCWnd@@I@Z',0x28dd8,100)]:
  library.append(measure(lib,libobj,dict(cls='MFC',role='base_constructor',symbol=name,rva=hex(rva),size=n)))
 base_tables={}
 for cls,rva,slots in [('CPropertyPage',0x2fb4c,62),('CPropertySheet',0x2fc4c,51)]:
  item=audit(lib,'??_7'+cls+'@@6B@',rva,slots*4,'vtable');base_tables[cls]=item
  for role in ['GetMessageMap','GetRuntimeClass']:
   rel=next(x for x in item['relocations']if x['symbol'].startswith('?'+role+'@'))
   name=rel['symbol'];n=lib.function(name)['size'];library.append(measure(lib,libobj,dict(cls='MFC',role=role,symbol=name,rva=rel['target_rva'],size=n)))
  s='?class'+cls+'@'+cls+'@@2UCRuntimeClass@@B';audit(lib,s,bindings[key(lib,s)],24,'runtime_descriptor')
 own=coffs[0][0];own_data=[];overrides=[];compiler_thunks=[]
 for cls,size,rt,vt,slots in CLASSES:
  descriptor=audit(own,'?class'+cls+'@'+cls+'@@2UCRuntimeClass@@B',rt,24,'runtime_descriptor');own_data.append(descriptor)
  native_rt=struct.unpack('<6I',pe.read_rva(rt,24));assert native_rt[1]==size and native_rt[2]==0xffff and native_rt[5]==0
  nameptr=native_rt[0]-pe.image_base;namebytes=pe.read_rva(nameptr,len(cls)+1);assert namebytes==cls.encode()+b'\0'
  rel=next(x for x in descriptor['relocations']if x['offset']==0);strings[(id(own),rel['symbol'])]=(own,rel['symbol'],nameptr,len(cls)+1)
  getter=next(x for x in measurements if x['cls']==cls and x['role']=='getmap');code=pe.read_rva(int(getter['rva'],0),6);assert code[0]==0xb8 and code[5]==0xc3
  map_rva=struct.unpack('<I',code[1:5])[0]-pe.image_base;base_map,entry_address=struct.unpack('<2I',pe.read_rva(map_rva,8));entries_rva=entry_address-pe.image_base;count=0
  while any(pe.read_rva(entries_rva+count*24,24)):count+=1;assert count<20
  header=audit(own,'?messageMap@'+cls+'@@1UAFX_MSGMAP@@B',map_rva,8,'message_map');own_data.append(header)
  entries=audit(own,'?_messageEntries@'+cls+'@@0PBUAFX_MSGMAP_ENTRY@@B',entries_rva,(count+1)*24,'message_entries');own_data.append(entries)
  for rel in entries['relocations']:
   assert rel['offset']%24==20 and executable(int(rel['target_rva'],0))
   sig=struct.unpack_from('<I',pe.read_rva(entries_rva,(count+1)*24),rel['offset']-4)[0]
   suffix='@@QAEXXZ'if sig==12 else'@@QAEXI@Z'if sig==13 else None
   if rel['symbol']=='??_9@$BNM@AE':
    assert cls=='CKeyboardPage'and sig==12 and rel['target_rva']=='0x9410'
    entry=pe.read_rva(entries_rva+rel['offset']-20,20)
    assert struct.unpack('<5I',entry)==(0x111,0,0x44b,0x44b,12)
    thunk=own.function(rel['symbol']);assert thunk['size']==8 and not thunk['relocations']
    raw=compare(coffs[0][1],rel['symbol'],ORACLE,0x9410,8);assert raw['exact']
    assert struct.unpack('<I',thunk['data'][4:8])[0]==0xdc
    assert struct.unpack('<I',pe.read_rva(vt+0xdc,4))[0]-pe.image_base==0x9330
    compiler_thunks.append(dict(symbol=rel['symbol'],rva='0x9410',bytes=8,virtual_slot=55,virtual_target_rva='0x9330',raw=raw,following_gap_bytes_excluded=8))
   else:
    assert suffix is not None and rel['symbol'].endswith('@'+cls+suffix),(cls,rel,sig)
  table=audit(own,'??_7'+cls+'@@6B@',vt,slots*4,'vtable');s=symbol(own,table['symbol']);assert s['value']==0 and own.sections[s['section']-1]['size']==slots*4
  base='CPropertySheet'if cls=='COptionsSheet'else'CPropertyPage';base_table=base_tables[base];assert len(table['relocations'])==slots
  for rel,previous in zip(table['relocations'],base_table['relocations']):
   assert rel['offset']==previous['offset']
   if rel['target_rva']==previous['target_rva']:assert rel['symbol']==previous['symbol'],(cls,'inherited slot',rel,previous)
   else:
    assert cls+'@@'in rel['symbol'],(cls,'override owner',rel)
    overrides.append(dict(cls=cls,slot=rel['offset']//4,symbol=rel['symbol'],rva=rel['target_rva']))
  # Compiler-generated search-library weak deleting-destructor fallback, not a source/linker trick.
  weak=symbol(own,'??_E'+cls+'@@UAEPAXI@Z');assert weak['storage']==105
  alias_index,search=struct.unpack_from('<II',weak['aux'][0]);assert search==2
  fallback=own.symbols[alias_index]['name'];assert fallback=='??_G'+cls+'@@UAEPAXI@Z'
  assert bindings[key(own,weak['name'])]==bindings[key(own,fallback)]
 # Whole 496-byte own descriptor/map aggregate is covered, without assuming
 # that this convenient region was one original translation unit or layout.
 section=own_data[0]['source_section'];covered=[]
 for item in own_data:
  assert item['source_section']==section;covered.extend(range(item['source_offset'],item['source_offset']+item['bytes']))
 assert len(covered)==len(set(covered))and sorted(covered)==list(range(own.sections[section-1]['size']))
 for cls in CONTROLS:
  name='??_7'+cls+'@@6B@';s=symbol(own,name);n=own.sections[s['section']-1]['size']-s['value'];vt=bindings[key(own,name)]
  fn=struct.unpack('<I',pe.read_rva(vt,4))[0]-pe.image_base;code=pe.read_rva(fn,6);assert code[0]==0xb8 and code[5]==0xc3
  rt=struct.unpack('<I',code[1:5])[0]-pe.image_base;nameptr,size=struct.unpack('<2I',pe.read_rva(rt,8));assert size==60 and pe.read_rva(nameptr-pe.image_base,len(cls)+1)==cls.encode()+b'\0'
  audit(own,name,vt,n,'vtable')
 for co,obj in coffs:
  for s in co.symbols.values():
   if s['section']>0 and s['name'].startswith('??_C@')and key(co,s['name'])in bindings:
    sec=co.sections[s['section']-1];value=sec['data'][s['value']:];n=value.index(0)+1
    assert pe.read_rva(bindings[key(co,s['name'])],n)==value[:n]
    strings[(id(co),s['name'])]=(co,s['name'],bindings[key(co,s['name'])],n)
 for co,name,rva,n in strings.values():audit(co,name,rva,n,'string_literal')
 negative_header=OUT/'wrong_control.h';header=HEADER.read_text();assert header.count('CEdit m_control_0416;')==1
 negative_header.write_bytes(header.replace('CEdit m_control_0416;','CButton m_control_0416;').encode())
 negative_source=OUT/'wrong_control.cpp';text,count=re.subn(r'#include "[^"]*host_options_classes.h"','#include "'+negative_header.as_posix()+'"',SOURCES[0].read_text());assert count==1;negative_source.write_bytes(text.encode())
 wrong_obj,wrong_command=compile_source(negative_source,['/O2','/MT','/GX','/I'+str(DEFAULT_TOOLCHAIN/'mfc/include')]);wrong=COFF(wrong_obj)
 bad=masked_compare(wrong_obj,'??0CKeyboardPage@@QAE@XZ',ORACLE,0x8aa0,320)
 assert bad['compiled_size']==320 and bad['differing_bytes']==0 and bad['conflicts']and not bad['masked_equal']
 runtime_symbol=symbol(wrong,'?classCKeyboardPage@CKeyboardPage@@2UCRuntimeClass@@B');sec=wrong.sections[runtime_symbol['section']-1]
 assert struct.unpack_from('<I',sec['data'],runtime_symbol['value']+4)[0]==744
 negative=dict(description='Wrong same-size first keyboard control CEdit->CButton',class_bytes_unchanged=744,constructor_bytes_unchanged=320,unmasked_instruction_differences=0,relocation_binding_conflicts=bad['conflicts'],detected=True,command=wrong_command,source_sha256=sha(negative_source),header_sha256=sha(negative_header),object_sha256=sha(wrong_obj))
 assert pins=={p:sha(p)for p in pins}
 result=dict(scope=__doc__,inputs=pins,compiles=compiles,measurements=measurements,library_proofs=library,archive_member_sha256=sha(libobj),data=data,overrides=overrides,symbol_bindings=binding_records,
  compiler_thunks=compiler_thunks,negative_control=negative,codegen_equal_functions=len(measurements),own_classes=len(CLASSES),own_aggregate_bytes=496,native_base_relocation_directory_present=has_native_relocations,relocation_check_scope='Typed source pointer-field footprint and consistent native addresses;native PE base relocations absent/stripped when directory is zero.',ui_or_game_executed=False)
 path=OUT/'options_class_audit.json';path.write_bytes((json.dumps(result,indent=2)+'\n').encode());print(json.dumps(dict(functions=len(measurements),classes=len(CLASSES),data_objects=len(data),overrides=len(overrides),report=str(path))))


if __name__=='__main__':run()
