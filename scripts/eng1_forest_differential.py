import sys,json,os,subprocess,re,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'scripts'))
from match import DEFAULT_TOOLCHAIN,verify_toolchain
from title_behavioral import context_files
sha=lambda b:hashlib.sha256(b).hexdigest()
o=root/'work/eng1_behavioral/differential/forest_actor_init';o.mkdir(parents=True,exist_ok=True)
sources=[root/'tests/eng1_forest_actor_differential.c',root/'src/eng1/initial_spatial_roots.c',root/'src/eng1/spatial_read_guard.c',root/'src/eng1/engine_file_bridge.c']
oracle=root/'assets/pc/HERCULES/ENG1.DLL'
archive=root/'assets/pc/HERCULES/HERCULES.FS'
evidence_path=root/'evidence/eng1_forest_actor_init.json'
evidence=json.loads(evidence_path.read_bytes())
bound=evidence['forest_producer']
assert sha(archive.read_bytes())==evidence['archive_sha256']
with archive.open('rb') as f:f.seek(bound['archive_offset']);map_bytes=f.read(bound['archive_length'])
assert sha(map_bytes)==bound['sha256']
block=bound['blocks'][0];placement=int(block['placement_start'],16);blockoff=int(block['block'],16)
assert block['count']==283 and blockoff-placement==2266
forest_bytes=map_bytes[placement:blockoff+20];assert len(forest_bytes)==2286
forest=o/'forest_block.bin';forest.write_bytes(forest_bytes)
before={str(p.relative_to(root)):sha(p.read_bytes()) for s in sources for p in context_files(s)}
before['assets/pc/HERCULES/ENG1.DLL']=sha(oracle.read_bytes())
before['assets/pc/HERCULES/HERCULES.FS']=sha(archive.read_bytes())
before['evidence/eng1_forest_actor_init.json']=sha(evidence_path.read_bytes())
before['scripts/eng1_forest_differential.py']=sha(Path(__file__).read_bytes())
tool=verify_toolchain(DEFAULT_TOOLCHAIN)
env=os.environ.copy()
for k in ['CL','_CL_','LINK','_LINK_']:env.pop(k,None)
env.update(PATH=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH',''),INCLUDE=str(DEFAULT_TOOLCHAIN/'include'),LIB=str(DEFAULT_TOOLCHAIN/'lib'))
exe=o/'fixture.exe';exe.unlink(missing_ok=True)
cmd=[str(DEFAULT_TOOLCHAIN/'bin/cl.exe'),'/nologo','/O2','/Gy','/MT','/Fo'+str(o)+os.sep,'/Fe'+str(exe),*map(str,sources),'/link','/INCREMENTAL:NO','/OPT:REF','kernel32.lib']
build=subprocess.run(cmd,cwd=o,env=env,capture_output=True,text=True,timeout=60)
(o/'compile.log').write_bytes((build.stdout+build.stderr).encode())
report={'scope':'Native unpatched26040 cold/warm worklist differential including actual FOREST283 placements; source26480 factory/action spies test explicit windows and reentrant count changes. Historical VC5, finite semantic acceptance only.','toolchain_id':tool,'command':cmd,'compile_exit_code':build.returncode,'input_sha256_before':before,'forest_producer':bound,'forest_fixture_sha256':sha(forest_bytes),'limitations':['No function-byte match, source spelling, translation-unit or layout claim.','26480 dispatch bodies270c0/27720/27c40/28290 are observational test spies, not recovered implementations.','General root-group/category capacity remains one guarded measured prefix; loaded maps with unsupported groups fail visibly.']}
if build.returncode==0:
 run=subprocess.run([str(exe),str(oracle),str(forest)],cwd=o,env=env,capture_output=True,text=True,timeout=60)
 (o/'run.log').write_bytes((run.stdout+run.stderr).encode());report.update(run_exit_code=run.returncode,stdout=run.stdout,stderr=run.stderr,executable_sha256=sha(exe.read_bytes()),object_sha256={p.name:sha(p.read_bytes()) for p in o.glob('*.obj')})
 m=re.search(r'forest_cases=(\d+) checks=(\d+) failures=(\d+)',run.stdout)
 if m:report.update(cases=int(m[1]),checks=int(m[2]),failures=int(m[3]))
 report['result']='PASS' if run.returncode==0 and m and int(m[3])==0 else 'FAIL'
else:report.update(result='COMPILE_FAILURE',compile_stdout=build.stdout,compile_stderr=build.stderr)
after={p:sha((root/p).read_bytes()) for p in before};report.update(input_sha256_after=after,inputs_unchanged=after==before)
(o/'report.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
print(build.stdout+build.stderr if build.returncode else report.get('stdout',''))
sys.exit(0 if report['result']=='PASS' and before==after else 1)
