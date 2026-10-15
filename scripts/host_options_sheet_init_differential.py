"""Offline PC Options tab initialization comparison; no Windows dialog execution."""
import argparse,hashlib,json,os,re,struct,subprocess,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,verify_toolchain,DEFAULT_TOOLCHAIN
from masked import masked_compare,_compare_section
from coff import COFF
from pe import PE
from title_behavioral import context_files
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--source',type=Path,default=ROOT/'src/pc/host_options_sheet_init.cpp')
args=parser.parse_args();OUT=args.output.resolve();SOURCE=args.source.resolve()
assert OUT.is_relative_to(ROOT/'work');OUT.mkdir(parents=True,exist_ok=True)
HEADER=ROOT/'calibration/host_options_classes.h'
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
SYM='?OnInitDialog@COptionsSheet@@UAEHXZ'
MFC={'BaseInit':('?OnInitDialog@CPropertySheet@@UAEHXZ',0x292f0,489,121),
     'FromHandle':('?FromHandle@CWnd@@SGPAV1@PAUHWND__@@@Z',0x22697,40,148)}
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
pack=lambda v:struct.pack('<I',v&0xffffffff)
DATA,FRAME,TAB,TAB2=0x61000000,0x61000100,0x61002000,0x61002100
STACK,ESP,STOP,API=0x62000000,0x62008000,0x6200f000,0x65000000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]
SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]

def build(source,out,accept=True):
 out.mkdir(parents=True,exist_ok=True);verify_toolchain(DEFAULT_TOOLCHAIN)
 assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
 graph=ROOT/'evidence/host_current_source_graph.json'
 libs=json.loads(graph.read_text())['libraries']
 assert Path(libs[0]['path']).name=='nafxcw.lib'and libs[0]['sha256']=='5d9c3585552412a9b94a9a0b7c3a4208061a76d48a6c3283518bfe9f674c9e22'
 assert all(sha(x['path'])==x['sha256']for x in libs)
 runtime=Path('C:/tools/hercules/renderer-oracle-python/unicorn/lib/unicorn.dll')
 assert sha(runtime)=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
 watched={Path(__file__),graph,ORACLE,runtime,ROOT/'toolchains/manifest.json',*context_files(source),*[Path(x['path'])for x in libs]}
 watched.update(ROOT/'scripts'/p for p in ['match.py','coff.py','masked.py','pe.py','env.py','title_behavioral.py'])
 pins={str(p):sha(p)for p in watched}
 anchor=out/'anchor.cpp';anchor.write_bytes(('#include "'+HEADER.as_posix()+'"\nextern "C" int main(){for(;;){}}\nextern "C" __declspec(dllexport) BOOL anchor(COptionsSheet*p){return p->COptionsSheet::OnInitDialog();}\n').encode())
 objects=[];compiles=[]
 for p in [source,anchor]:
  obj,cmd=compile_source(p,['/O2','/MT','/GX','/I'+str(DEFAULT_TOOLCHAIN/'mfc/include')]);objects.append(obj)
  compiles.append(dict(source=str(p),source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=cmd))
 raw=compare(objects[0],SYM,ORACLE,0x10960,192);diag=masked_compare(objects[0],SYM,ORACLE,0x10960,192)
 if accept:assert diag['masked_equal'],diag
 dll,mp=out/'fixture.dll',out/'fixture.map'
 cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/OUT:'+str(dll),'/MAP:'+str(mp),*map(str,objects),*[x['path']for x in libs]]
 env=os.environ.copy()
 for key in ['CL','_CL_','LINK','_LINK_']:env.pop(key,None)
 env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
 proc=subprocess.run(cmd,env=env,capture_output=True,text=True);(out/'link.log').write_bytes((proc.stdout+proc.stderr).encode())
 assert proc.returncode==0,proc.stdout+proc.stderr
 regex=re.compile(r'^\s*[\da-fA-F]{4}:[\da-fA-F]{8}\s+(\S+)\s+([\da-fA-F]{8})')
 symbols={m[1]:int(m[2],16)for line in mp.read_text().splitlines()if(m:=regex.match(line))}
 linked=pefile.PE(str(dll));oracle=PE(ORACLE)
 def linked_body(co,sec,name):
  data=bytearray(linked.get_data(symbols[name]-linked.OPTIONAL_HEADER.ImageBase,sec['size']));expected=bytearray(sec['data'])
  for rel in sec['relocations']:data[rel['offset']:rel['offset']+4]=bytes(4);expected[rel['offset']:rel['offset']+4]=bytes(4)
  assert data==expected
 co=COFF(objects[0]);s=next(x for x in co.symbols.values()if x['name']==SYM and x['section']>0)
 linked_body(co,co.sections[s['section']-1],SYM)
 literals=[]
 if accept:
  for name,rva in diag['symbols'].items():
   if name.startswith('??_C@'):
    s=next(x for x in co.symbols.values()if x['name']==name and x['section']>0);sec=co.sections[s['section']-1]
    data=sec['data'][s['value']:];assert not sec['relocations']and data.endswith(b'\0')
    assert oracle.read_rva(int(rva,0),len(data))==data
    assert linked.get_data(symbols[name]-linked.OPTIONAL_HEADER.ImageBase,len(data))==data
    literals.append(dict(symbol=name,native_rva=rva,bytes=len(data),text=data[:-1].decode()))
 archive=Path(libs[0]['path']).read_bytes();pos,index=8,0;proofs=[]
 while pos<len(archive):
  h=archive[pos:pos+60];assert h[58:]==b'`\n';n=int(h[48:58])
  for role,(name,rva,width,member)in MFC.items():
   if index!=member:continue
   obj=out/('mfc_'+str(index)+'.obj');obj.write_bytes(archive[pos+60:pos+60+n]);co=COFF(obj)
   s=next(x for x in co.symbols.values()if x['name']==name and x['section']>0);sec=co.sections[s['section']-1]
   assert s['value']==0 and sec['size']==width and len(sec['data'])==width
   d=_compare_section(co,sec,s['section'],oracle,rva,width);assert d['masked_equal'],(role,d)
   linked_body(co,sec,name)
   proofs.append(dict(role=role,symbol=name,rva=hex(rva),bytes=width,member=member,member_sha256=sha(obj),diagnostic=d,linked_code_diagnostic_equal=True,execution='modeled identified boundary'))
  pos+=60+n+(n&1);index+=1
 assert len(proofs)==2 and pins=={p:sha(p)for p in pins}
 return dict(inputs=pins,compiles=compiles,raw=raw,diagnostic=diag,literals=literals,mfc=proofs,libraries=libs,link_command=cmd,dll=str(dll),dll_sha256=sha(dll),map_sha256=sha(mp),symbols=symbols)

def execute(native,b,s):
 image=pefile.PE(str(ORACLE if native else b['dll']));base,size=image.OPTIONAL_HEADER.ImageBase,image.OPTIONAL_HEADER.SizeOfImage
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,image.get_memory_mapped_image().ljust(size,b'\0'))
 for addr in [DATA,STACK]:u.mem_map(addr,0x10000)
 u.mem_map(API,0x1000)
 read=lambda a:int.from_bytes(u.mem_read(a,4),'little')
 def p32(a,v):u.mem_write(a,pack(v))
 def string(a):return bytes(u.mem_read(a,80)).split(b'\0')[0].decode('latin1')
 u.mem_write(DATA,bytes((i*17+(i>>8)*7+31)&255 for i in range(0x10000)))
 for i,a in enumerate([FRAME,TAB,TAB2]):p32(a+0x1c,0x12340000+i*0x100)
 arena=bytes(u.mem_read(DATA,0x10000))
 start=0x410960 if native else b['symbols'][SYM];width=192 if native else b['raw']['compiled_size']
 endpoints={API:('SendMessageA',16)}
 for role,(name,rva,n,index)in MFC.items():endpoints[0x400000+rva if native else b['symbols'][name]]=(role,4 if role=='FromHandle'else 0)
 for d in image.DIRECTORY_ENTRY_IMPORT:
  for imp in d.imports:
   if imp.name==b'SendMessageA':p32(imp.address,API)
 events=[];errors=[];items=[];item_address=None;wrapper=TAB2 if s.get('alternate_wrapper')else TAB
 def ret(sp,n,value):
  target=read(sp);u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xc1c2c3c4);u.reg_write(UC_X86_REG_EDX,0xd1d2d3d4)
  u.reg_write(UC_X86_REG_ESP,sp+4+n);u.reg_write(UC_X86_REG_EIP,target)
 def code(machine,a,n,_):
  nonlocal item_address
  if a==STOP:machine.emu_stop();return
  if a not in endpoints:
   if not start<=a<start+width:errors.append(['execute',hex(a)]);machine.emu_stop()
   return
  name,cleanup=endpoints[a];sp=machine.reg_read(UC_X86_REG_ESP);argv=[read(sp+4+i*4)for i in range(cleanup//4)]
  if name=='BaseInit':
   assert machine.reg_read(UC_X86_REG_ECX)==FRAME
   if s.get('base_changes_hwnd'):p32(FRAME+0x1c,0xabcdef00)
   events.append([name]);result=s.get('base_result',1)
  elif name=='FromHandle':
   assert argv==[s.get('tab_handle',0x34567890)]
   events.append([name,*argv]);result=wrapper
  else:
   hwnd,msg,wp,lp=argv
   if msg==0x474:
    assert hwnd==read(FRAME+0x1c)and wp==0 and lp==0
    events.append([name,*argv]);result=s.get('tab_handle',0x34567890)
   else:
    assert msg==0x1306
    assert ESP-0x2000<=lp and lp+28<=ESP
    if item_address is None:item_address=lp
    assert item_address==lp
    fields=[read(lp+i*4)for i in range(7)];fields[3]=string(fields[3]);items.append(fields)
    events.append([name,hwnd,msg,wp,fields]);result=s.get('set_result',1)
    if s.get('mutate_each_item'):
     p32(wrapper+0x1c,0x45670000+len(items)*0x100)
     for i in range(7):p32(lp+i*4,0x65430000+len(items)*0x100+i)
  ret(sp,cleanup,result)
 def write(machine,access,a,n,value,_):
  if not ESP-0x2000<=a or a+n>ESP+4:errors.append(['write',hex(a),n]);machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,write)
 u.mem_write(ESP-0x2000,bytes([0xa7])*0x2100);p32(ESP,STOP);u.reg_write(UC_X86_REG_ESP,ESP);u.reg_write(UC_X86_REG_ECX,FRAME)
 for reg,v in zip(NV,SEEDS):u.reg_write(reg,v)
 u.emu_start(start,STOP,count=10000)
 assert not errors and u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4,errors
 assert [u.reg_read(x)for x in NV]==SEEDS
 assert bytes(u.mem_read(ESP-0x2000-32,32))==bytes(32)
 assert bytes(u.mem_read(ESP+4,128))==bytes([0xa7])*128
 assert len(items)==4
 return dict(events=events,result=u.reg_read(UC_X86_REG_EAX),arena_sha256=hashlib.sha256(u.mem_read(DATA,0x10000)).hexdigest(),full_receiver_bytes=2156,synthetic_arena_bytes=len(arena),stack_nonvolatiles_guards=True)

def main():
 b=build(SOURCE,OUT);scenarios=[]
 for value in [0,1,2,0x80000000,0xffffffff]:
  for result in [0,1,0xffffffff]:scenarios.append(dict(name=f'base_{value:x}_api_{result:x}',base_result=value,set_result=result))
 scenarios.extend([dict(name='live_handles_and_item',base_result=0x80000000,base_changes_hwnd=True,mutate_each_item=True),dict(name='alternate_wrapper',alternate_wrapper=True,tab_handle=0xfedcba98,mutate_each_item=True)])
 cases=[]
 for s in scenarios:
  n,c=execute(True,b,s),execute(False,b,s);assert n==c,(s,n,c)
  assert n['result']==s.get('base_result',1)
  items=[x for x in n['events']if x[0]=='SendMessageA'and x[2]==0x1306]
  assert [x[3]for x in items]==[0,1,2,3]
  assert [x[4][3]for x in items]==['Keyboard','GamePad','Sound','Game Play']
  assert all(x[4][0]==1 for x in items)
  cases.append(dict(scenario=s,equal=True,projection=n))
 text=SOURCE.read_text();wrong={
  'wrong_literal':text.replace('"GamePad"','"Joystick"'),
  'cached_handle':text.replace('TC_ITEM item;','HWND cached = tab->m_hWnd;\n    TC_ITEM item;'),
  'normalized_base_result':text.replace('return result;','return TRUE;')}
 for i in range(4):wrong['cached_handle']=wrong['cached_handle'].replace(f'tab->SetItem({i},&item);',f'::SendMessage(cached,TCM_SETITEM,{i},(LPARAM)&item);')
 negatives=[]
 for name,txt in wrong.items():
  lane=OUT/name;lane.mkdir(exist_ok=True);p=lane/'wrong.cpp'
  txt=re.sub(r'#include "[^"]*host_options_classes.h"','#include "'+HEADER.as_posix()+'"',txt);p.write_bytes(txt.encode())
  bad=build(p,lane,False);s=next(x for x in scenarios if x['name']=='live_handles_and_item')
  n,c=execute(True,b,s),execute(False,bad,s);assert n!=c,name
  negatives.append(dict(name=name,detected=True,scenario=s,native=n,wrong=c,build=bad))
 assert b['inputs']=={p:sha(p)for p in b['inputs']}
 report=dict(scope='Original PC Options sheet callback on synthetic receivers. CPropertySheet initialization and CWnd::FromHandle are identified pinned MFC bodies but modeled;SendMessageA is modeled. No dialog/window/modal/DDX/game process execution.',build=b,case_count=len(cases),all_equal=True,cases=cases,negative_controls=negatives)
 path=OUT/'options_sheet_init_differential.json';path.write_bytes((json.dumps(report,indent=2)+'\n').encode())
 print(json.dumps(dict(cases=len(cases),all_equal=True,negative_controls=len(negatives),report=str(path))))

if __name__=='__main__':main()
