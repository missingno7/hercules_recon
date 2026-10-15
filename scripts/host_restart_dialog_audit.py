import sys,json,struct,hashlib,argparse
from pathlib import Path
root=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file());sys.path[:0]=[str(root/'scripts'),'C:/tools/hercules/python']
from coff import COFF
from pe import PE
from masked import _compare_section
parser=argparse.ArgumentParser(description='Fresh PC Restart Level code/class/data audit;no modal/game execution')
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args();lane=args.output.resolve();assert lane.is_relative_to(root/'work');lane.mkdir(parents=True,exist_ok=True)
from match import compile_source,compare,DEFAULT_TOOLCHAIN,verify_toolchain
from masked import masked_compare
from title_behavioral import context_files
verify_toolchain(DEFAULT_TOOLCHAIN)
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
assert hashlib.sha256(ORACLE.read_bytes()).hexdigest()=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
profile={
'restart.cpp':('src/pc/host_restart_dialog.cpp',[
('constructor','??0CHostRestartDialog@@QAE@PAVCWnd@@@Z',0xef60,32),
('getmap','?GetMessageMap@CHostRestartDialog@@MBEPBUAFX_MSGMAP@@XZ',0xefb0,16),
('init','?OnInitDialog@CHostRestartDialog@@UAEHXZ',0xf100,720),
('compiler','??1CHostRestartDialog@@UAE@XZ',0x9730,16),
('compiler','??_GCHostRestartDialog@@UAEPAXI@Z',0xef80,32)]),
'frame.cpp':('src/pc/host_frame_remaining.cpp',[(name,'?OnRva_'+sym+'@CMainFrame@@'+tail,rva,n)for name,sym,tail,rva,n in [
('open','000096C0','QAEXXZ',0x96c0,112),('close','000097F0','QAEXXZ',0x97f0,48),('toggle','00009990','QAEXXZ',0x9990,16),('keys','00009B40','QAEXIII@Z',0x9b40,128)]]),
'ddx.cpp':('src/pc/host_restart_dialog_ddx.cpp',[("ddx","?DoDataExchange@CHostRestartDialog@@MAEXPAVCDataExchange@@@Z",0xefa0,16)]),
'selector.c':('src/pc/host_restart_selector.c',[('selector','_cleanup_native_4b50',0x4b50,112)])}
for i,rva in enumerate([0xefc0,0xefe0,0xf000,0xf020,0xf040,0xf060,0xf080,0xf0a0,0xf0c0,0xf0e0]):profile['restart.cpp'][1].append(('level_'+str(i),'?OnRva_%08X@CHostRestartDialog@@QAEXXZ'%rva,rva,32))
watched={Path(__file__),ORACLE,root/'toolchains/manifest.json'}
for source,rows in profile.values():watched.update(context_files(root/source))
pins={str(p):hashlib.sha256(p.read_bytes()).hexdigest()for p in watched}
m=dict(compiles=[],functions=[])
for short,(source,rows)in profile.items():
 flags=['/O2']if source.endswith('.c')else['/O2','/MT','/GX','/I'+str(DEFAULT_TOOLCHAIN/'mfc/include')]
 obj,cmd=compile_source(root/source,flags);m['compiles'].append(dict(source=str(root/source),short=short,sha256=hashlib.sha256((root/source).read_bytes()).hexdigest(),object=str(obj),object_sha256=hashlib.sha256(obj.read_bytes()).hexdigest(),command=cmd))
 for role,sym,rva,n in rows:
  raw=compare(obj,sym,ORACLE,rva,n);diag=masked_compare(obj,sym,ORACLE,rva,n)
  if role!='selector':assert diag['masked_equal'],(role,diag)
  m['functions'].append(dict(role=role,symbol=sym,rva=hex(rva),size=n,source=short,raw=raw,diagnostic=diag))
(lane/'measurement.json').write_bytes((json.dumps(m,indent=2)+'\n').encode())
oracle=PE('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE');bindings={};data=[];functions=[]
coffs={x['short']:COFF(x['object'])for x in m['compiles']}
def bind(co,name,rva):
 s=next(x for x in co.symbols.values()if x['name']==name);key=name if s['storage']in [2,105]else(str(id(co)),name)
 assert key not in bindings or bindings[key]==rva,(name,bindings.get(key),rva)
 bindings[key]=rva
for row in m['functions']:
 if row['role']=='selector':continue
 co=coffs[row['source']];assert row['diagnostic']['masked_equal'];bind(co,row['symbol'],int(row['rva'],0))
 for name,rva in row['diagnostic']['symbols'].items():bind(co,name,int(rva,0))
 functions.append(dict(symbol=row['symbol'],rva=row['rva'],bytes=row['size'],raw_equal=row['raw']['exact']))
own=coffs['restart.cpp']
def symbol(co,name):return next(x for x in co.symbols.values()if x['name']==name and x['section']>0)
def audit(co,name,rva,n,kind):
 s=symbol(co,name);sec=co.sections[s['section']-1];off=s['value'];assert off+n<=sec['size']and len(sec['data'])==sec['size']
 left=bytearray(sec['data'][off:off+n]);right=bytearray(oracle.read_rva(rva,n));rels=[];bind(co,name,rva)
 for rel in sec['relocations']:
  if not off<=rel['offset']<off+n:continue
  at=rel['offset']-off;assert rel['type']==6 and at+4<=n
  addend=struct.unpack_from('<i',left,at)[0];address=struct.unpack_from('<I',right,at)[0];target=(address-0x400000-addend)&0xffffffff
  bind(co,rel['symbol'],target);rels.append(dict(offset=at,symbol=rel['symbol'],rva=hex(target),addend=addend));left[at:at+4]=bytes(4);right[at:at+4]=bytes(4)
 assert left==right,(name,'scalar bytes')
 expected=set(range(0,n,4))if kind=='vtable'else{0,4}if kind=='message_map'else{20+i*24 for i in range(n//24-1)}if kind=='entries'else set()
 assert {x['offset']for x in rels}==expected,(name,'source pointer fields')
 record=dict(symbol=name,rva=hex(rva),bytes=n,kind=kind,relocations=rels,all_scalar_bytes_equal=True);data.append(record);return record
map_data=audit(own,'?messageMap@CHostRestartDialog@@1UAFX_MSGMAP@@B',0x2e970,8,'message_map')
entries=audit(own,'?_messageEntries@CHostRestartDialog@@0PBUAFX_MSGMAP_ENTRY@@B',0x2e978,264,'entries')
table=audit(own,'??_7CHostRestartDialog@@6B@',0x2ea80,216,'vtable')
assert[(struct.unpack('<6I',oracle.read_rva(0x2e978+i*24,24))[2],x['rva'])for i,x in enumerate(entries['relocations'])]==[(0x470,'0xf080'),(0x46c,'0xf000'),(0x46b,'0xefe0'),(0x46e,'0xf040'),(0x46f,'0xf060'),(0x46d,'0xf020'),(0x471,'0xf0a0'),(0x472,'0xf0c0'),(0x46a,'0xefc0'),(0x473,'0xf0e0')]
archive=Path('C:/tools/hercules/msvc500-8abf95ce980161ad87b0b02402269cce76988953/mfc/lib/nafxcw.lib');blob=archive.read_bytes();assert hashlib.sha256(blob).hexdigest()=='5d9c3585552412a9b94a9a0b7c3a4208061a76d48a6c3283518bfe9f674c9e22';pins[str(archive)]=hashlib.sha256(blob).hexdigest();pos,index=8,0;member=None
while pos<len(blob):
 h=blob[pos:pos+60];assert h[58:]==b'`\n';n=int(h[48:58])
 if index==124:
  member=lane/'mfc124.obj';member.write_bytes(blob[pos+60:pos+60+n]);break
 pos+=60+n+(n&1);index+=1
lib=COFF(member);ctor='??0CDialog@@QAE@IPAVCWnd@@@Z';s=symbol(lib,ctor);sec=lib.sections[s['section']-1];assert s['value']==0 and sec['size']==57
d=_compare_section(lib,sec,s['section'],oracle,0x21d83,57);assert d['masked_equal']
for name,rva in d['symbols'].items():bind(lib,name,int(rva,0))
base_vt=int(d['symbols']['??_7CDialog@@6B@'],0);base=audit(lib,'??_7CDialog@@6B@',base_vt,216,'vtable')
overrides=[]
for a,b in zip(table['relocations'],base['relocations']):
 assert a['offset']==b['offset']
 if a['rva']==b['rva']:assert a['symbol']==b['symbol'],(a,b)
 else:overrides.append(dict(slot=a['offset']//4,symbol=a['symbol'],rva=a['rva']))
assert [(x['slot'],x['rva'])for x in overrides]==[(1,'0xef80'),(12,'0xefb0'),(35,'0xefa0'),(49,'0xf100')]
getrt=struct.unpack('<I',oracle.read_rva(base_vt,4))[0]-0x400000;code=oracle.read_rva(getrt,6);assert code[0]==0xb8 and code[5]==0xc3
rt=struct.unpack('<I',code[1:5])[0]-0x400000;record=struct.unpack('<6I',oracle.read_rva(rt,24));assert record[1]==92 and record[2]==0xffff and record[3]==0 and oracle.read_rva(record[0]-0x400000,8)==b'CDialog\0'
weak=next(s for s in own.symbols.values()if s['name']=='??_ECHostRestartDialog@@UAEPAXI@Z');assert weak['storage']==105
alias,search=struct.unpack_from('<II',weak['aux'][0]);assert search==2 and own.symbols[alias]['name']=='??_GCHostRestartDialog@@UAEPAXI@Z'
fields=[]
for row in m['functions']:
 if row['role']=='selector':continue
 co=coffs[row['source']]
 for name,rva in row['diagnostic']['symbols'].items():
  if not name.startswith('??_C@'):continue
  s=symbol(co,name);raw=co.sections[s['section']-1]['data'][s['value']:];n=raw.index(0)+1;assert oracle.read_rva(int(rva,0),n)==raw[:n]
  fields.append(dict(symbol=name,rva=rva,bytes=n,text=raw[:n-1].decode()))
library_specs=[('BaseCtor','??0CDialog@@QAE@IPAVCWnd@@@Z',0x21d83,57,124),('BaseDtor','??1CDialog@@UAE@XZ',0x21b15,62,124),('Modal','?DoModal@CDialog@@UAEHXZ',0x21e31,357,124),('EndDialog','?EndDialog@CDialog@@QAEXH@Z',0x21f96,35,124),('BaseInit','?OnInitDialog@CDialog@@UAEHXZ',0x220bd,101,124),('GetDlgItem','?GetDlgItem@CWnd@@QBEPAV1@H@Z',0x24faa,42,104),('SetText','?SetWindowTextA@CWnd@@QAEXPBD@Z',0x2509d,39,104),('Enable','?EnableWindow@CWnd@@QAEHH@Z',0x25155,39,104),('Default','?Default@CWnd@@IAEJXZ',0x225ec,39,148),('FrameClose','?OnClose@CFrameWnd@@IAEXXZ',0x2a3af,265,147)]
library=[];pos,index=8,0
while pos<len(blob):
 h=blob[pos:pos+60];assert h[58:]==b'`\n';n=int(h[48:58])
 for role,name,rva,width,member_index in library_specs:
  if index!=member_index:continue
  path=lane/('mfc%d.obj'%index);path.write_bytes(blob[pos+60:pos+60+n]);co=COFF(path);ss=symbol(co,name);sec=co.sections[ss['section']-1]
  assert ss['value']==0 and sec['size']==width
  proof=_compare_section(co,sec,ss['section'],oracle,rva,width);assert proof['masked_equal'],(role,proof)
  library.append(dict(role=role,symbol=name,rva=hex(rva),bytes=width,member=index,member_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),diagnostic=proof))
 pos+=60+n+(n&1);index+=1
assert len(library)==10 and pins=={path:hashlib.sha256(Path(path).read_bytes()).hexdigest()for path in pins}
result=dict(scope='Fresh static Restart Level class/map/vtable/code/literal audit. Derived source name is provisional;inherited runtime identity is CDialog. No modal/UI/game execution.',measurement_sha256=hashlib.sha256((lane/'measurement.json').read_bytes()).hexdigest(),functions=functions,data=data,own_data_bytes=488,virtual_slots=54,overrides=overrides,inherited_runtime=dict(name='CDialog',bytes=92,rva=hex(rt),get_runtime_rva=hex(getrt),has_factory=False),base_constructor=dict(symbol=ctor,bytes=57,diagnostic=d),library=library,library_member=124,library_member_sha256=hashlib.sha256((lane/'mfc124.obj').read_bytes()).hexdigest(),mfc_archive_sha256=hashlib.sha256(blob).hexdigest(),inputs=pins,compiles=m['compiles'],measurements=m['functions'],literals=fields,compiler_destructor_alias='Generated storage105 search-library2 _Eto_G fallback;not a source/link alias trick.',native_relocations='Absent/stripped;source pointer schemas and consistent target bindings checked only.')
(lane/'class_audit.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print(json.dumps(dict(functions=len(functions),own_data_bytes=488,virtual_slots=54,overrides=len(overrides),literal_occurrences=len(fields))))
