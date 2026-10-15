"""Fresh VC5 initialized music-record audit; no raw/function/layout acceptance."""
import argparse,hashlib,json,re,struct,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python']
from match import compile_source,verify_toolchain,DEFAULT_TOOLCHAIN
from coff import COFF
from pe import PE
from title_behavioral import context_files
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()

def asset_lookup(definition,oracle,archive):
 """Read directory keys only; this does not execute the archive opener."""
 data=archive.read_bytes();assert hashlib.sha256(data).hexdigest()=='affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4'
 native=PE(oracle);assert native.read_rva(0x365a8,18)==b'M:\\LANGUAGE\\SONIX\0'
 spec=json.loads(definition.read_bytes());literals={x['rva']:x['literal']for x in spec['literals']};directory={}
 for i in range(1024):
  key,offset,length=struct.unpack_from('<III',data,i*12)
  if not key:break
  assert offset+length<=len(data)and key not in directory;directory[key]=(i,offset,length)
 assert len(directory)==821
 filenames=[native.read_rva(rv,64).split(b'\0')[0].decode('ascii')for rv in [0x35c0c,0x35d14]];rows=[]
 for record in spec['records']:
  path=literals[record['path_rva']];files=[]
  for filename in filenames:
   name='M:\\LANGUAGE\\SONIX'+path[2:]+filename;raw=name.upper().encode('ascii');key=(sum(v<<((i%4)*8)for i,v in enumerate(raw))+len(raw))&0xffffffff;found=directory.get(key)
   files.append(dict(filename=filename,normalized=name,key=hex(key),found=found is not None,directory_record=found,unknown_matches_length=found is not None and record['unknown_004']==found[2]))
  rows.append(dict(index=record['index'],path=path,files=files))
 return dict(archive=str(archive),archive_sha256=hashlib.sha256(data).hexdigest(),directory_records=821,rows=rows,scope='Read-only key lookup under the actual Q-to-SONIX prefix and accepted hash algorithm. No archive opener or game process executes. A matching key is not a unique unhashed filename proof;the second DWORD is not assigned file-length semantics.')

def verify_data(source,definition,oracle,out):
 out.mkdir(parents=True,exist_ok=True);verify_toolchain(DEFAULT_TOOLCHAIN)
 assert sha(oracle)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
 spec=json.loads(definition.read_bytes());native=PE(oracle)
 watched={source,definition,oracle,Path(__file__),ROOT/'toolchains/manifest.json',*[ROOT/'scripts'/p for p in ['match.py','coff.py','pe.py','env.py','title_behavioral.py']]};watched.update(context_files(source));pins={str(p):sha(p)for p in watched}
 obj,cmd=compile_source(source,['/O2']);c=COFF(obj)
 (out/'compile.json').write_bytes((json.dumps(dict(object=str(obj),command=cmd,source_sha256=sha(source)),indent=2)+'\n').encode())
 if any(s['size']and s['flags']&0x20 for s in c.sections):raise ValueError('Unexpected executable contribution')
 symbols={}
 for name in ['_g_host_music_paths_rva_35178',*[x['symbol']for x in spec['literals']]]:
  ss=[s for s in c.symbols.values()if re.fullmatch(re.escape(name)+r'(?:\$S[0-9]+)?',s['name'])and s['section']>0]
  if len(ss)!=1:raise ValueError('Missing or duplicate data symbol: '+name+';defined='+repr([s['name']for s in c.symbols.values()if s['section']>0]))
  symbols[name]=ss[0]
 segments=[];literal_rows=[]
 for x in spec['literals']:
  s=symbols[x['symbol']];sec=c.sections[s['section']-1];start=s['value'];n=x['bytes'];raw=sec['data'][start:start+n];rv=int(x['rva'],0)
  if len(raw)!=n or raw!=x['literal'].encode('ascii')+b'\0' or raw!=native.read_rva(rv,n) or hashlib.sha256(raw).hexdigest()!=x['sha256']:raise ValueError('Literal differs: '+x['symbol'])
  if not sec['flags']&0x80000000:raise ValueError('Expected writable literal representation')
  if any(start<=r['offset']<start+n for r in sec['relocations']):raise ValueError('Literal relocation')
  segments.append(dict(rva=hex(rv),data=raw.hex()));literal_rows.append(dict(symbol=x['symbol'],coff_symbol=s['name'],rva=hex(rv),bytes=n,sha256=x['sha256']))
 table=symbols['_g_host_music_paths_rva_35178'];sec=c.sections[table['section']-1];start=table['value'];n=spec['bytes'];raw=bytearray(sec['data'][start:start+n]);rv=int(spec['rva'],0)
 if n!=len(spec['records'])*8 or len(raw)!=n:raise ValueError('Wrong observed prefix extent')
 rels=[r for r in sec['relocations']if start<=r['offset']<start+n]
 if len(rels)!=len(spec['records'])or {r['offset']-start for r in rels}!={i*8 for i in range(len(spec['records']))}:raise ValueError('Wrong pointer relocation positions')
 rows=[]
 for i,x in enumerate(spec['records']):
  if x['index']!=i:raise ValueError('Wrong record order')
  rel=next(r for r in rels if r['offset']==start+i*8)
  if rel['type']!=6:raise ValueError('Unexpected relocation kind')
  targets=[s for s in c.symbols.values()if s['name']==rel['symbol']and s['section']>0]
  if len(targets)!=1:raise ValueError('Ambiguous pointer target')
  target=targets[0];addend=struct.unpack_from('<I',raw,i*8)[0];literal=symbols[x['path_symbol']]
  literal_spec=next(v for v in spec['literals']if v['symbol']==x['path_symbol'])
  if int(x['path_rva'],0)!=int(literal_spec['rva'],0):raise ValueError('Inconsistent native literal binding: record '+str(i))
  if target['section']!=literal['section']or (target['value']+addend)&0xffffffff!=literal['value']:raise ValueError('Pointer alias/target differs: record '+str(i))
  value=struct.unpack_from('<I',raw,i*8+4)[0]
  if value!=x['unknown_004']:raise ValueError('Unknown DWORD differs: record '+str(i))
  struct.pack_into('<I',raw,i*8,native.image_base+int(x['path_rva'],0));rows.append(dict(index=i,path_rva=x['path_rva'],unknown_004=value,relocation=rel))
 if bytes(raw)!=native.read_rva(rv,n)or hashlib.sha256(raw).hexdigest()!=spec['sha256']:raise ValueError('Resolved native fields differ')
 segments.append(dict(rva=hex(rv),data=raw.hex()))
 assert pins=={p:sha(p)for p in pins}
 return dict(inputs=pins,source=str(source),source_sha256=sha(source),object=str(obj),object_sha256=sha(obj),command=cmd,records=len(rows),pointer_fields_verified=len(rels),literal_bytes=sum(x['bytes']for x in literal_rows),literals=literal_rows,rows=rows,fixture_segments=segments,relocated_fields_equal=True,raw_pointer_table_equal=False,limits='Observed initialized prefix and literal/alias identities only. Native base relocations are stripped;no original total capacity,unique declarations,TU,section layout or linked-image equality.')

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,default=ROOT/'calibration/host_music_paths.c');p.add_argument('--definition',type=Path,default=ROOT/'evidence/host_music_paths_data.json');p.add_argument('--original-root',type=Path,default=Path('D:/Prog/hercules_recon'));p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();assert out.is_relative_to(ROOT/'work');out.mkdir(parents=True,exist_ok=True)
 source=a.source.resolve();definition=a.definition.resolve();oracle=a.original_root/'work/discs/pc_install/HERCULES.EXE';good=verify_data(source,definition,oracle,out/'build');text=source.read_text();negatives=[]
 wrong={'wrong_literal':text.replace('q:\\\\TRAINING','r:\\\\TRAINING'),'wrong_unknown_DWORD':text.replace('0x3390bUL','0x3390aUL',1),'broken_pointer_alias':text.replace('{ host_music_path_training,','{ host_music_path_playroom,',1)}
 for name,code in wrong.items():
  assert code!=text;lane=out/name;lane.mkdir(exist_ok=True);bad=lane/'wrong.c';code=re.sub(r'#include "([^"]+)"',lambda m:'#include "'+(source.parent/m.group(1)).resolve().as_posix()+'"',code);bad.write_bytes(code.encode())
  try:verify_data(bad,definition,oracle,lane/'build')
  except ValueError as error:negatives.append(dict(name=name,detected=True,reason=str(error),source_sha256=sha(bad)))
  else:raise AssertionError('Wrong provider accepted: '+name)
 # A jointly wrong source/manifest must not manufacture a native pointer binding.
 lane=out/'inconsistent_native_binding';lane.mkdir(exist_ok=True);bad=lane/'wrong.c';code=re.sub(r'#include "([^"]+)"',lambda m:'#include "'+(source.parent/m.group(1)).resolve().as_posix()+'"',wrong['broken_pointer_alias']);bad.write_bytes(code.encode())
 spec=json.loads(definition.read_bytes());spec['records'][1]['path_symbol']=spec['records'][0]['path_symbol'];bad_definition=lane/'definition.json';bad_definition.write_bytes((json.dumps(spec,indent=2)+'\n').encode())
 try:verify_data(bad,bad_definition,oracle,lane/'build')
 except ValueError as error:negatives.append(dict(name='inconsistent_native_binding',detected=True,reason=str(error),source_sha256=sha(bad),definition_sha256=sha(bad_definition)))
 else:raise AssertionError('Inconsistent native pointer binding accepted')
 archive=a.original_root/'assets/pc/HERCULES/HERCULES.FS';assets=asset_lookup(definition,oracle,archive)
 report=dict(scope=__doc__,build=good,negative_controls=negatives,asset_lookup=assets)
 dest=out/'host_music_paths_data.json';dest.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(records=good['records'],literal_bytes=good['literal_bytes'],negative_controls=len(negatives),report=str(dest))))
if __name__=='__main__':main()
