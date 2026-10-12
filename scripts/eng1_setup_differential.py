import sys,json,os,subprocess,re,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'scripts'))
from match import DEFAULT_TOOLCHAIN,verify_toolchain
from title_behavioral import context_files
from pe import PE
sha=lambda b:hashlib.sha256(b).hexdigest()
o=root/'work/eng1_behavioral/differential/setup_dispatch';o.mkdir(parents=True,exist_ok=True)
sources=[root/'tests/eng1_setup_differential.c',root/'src/shared/empty_entries.c']
oracle=root/'assets/pc/HERCULES/ENG1.DLL';pe=PE(oracle)
before={str(p.relative_to(root)):sha(p.read_bytes()) for s in sources for p in context_files(s)};before['assets/pc/HERCULES/ENG1.DLL']=sha(oracle.read_bytes())
before['scripts/eng1_setup_differential.py']=sha(Path(__file__).read_bytes())
tool=verify_toolchain(DEFAULT_TOOLCHAIN);env=os.environ.copy()
for k in ['CL','_CL_','LINK','_LINK_']:env.pop(k,None)
env.update(PATH=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH',''),INCLUDE=str(DEFAULT_TOOLCHAIN/'include'),LIB=str(DEFAULT_TOOLCHAIN/'lib'))
exe=o/'fixture.exe';exe.unlink(missing_ok=True)
cmd=[str(DEFAULT_TOOLCHAIN/'bin/cl.exe'),'/nologo','/O2','/Gy','/MT','/Fo'+str(o)+os.sep,'/Fe'+str(exe),*map(str,sources),'/link','/INCREMENTAL:NO','/OPT:REF','kernel32.lib']
build=subprocess.run(cmd,cwd=o,env=env,capture_output=True,text=True,timeout=60);(o/'compile.log').write_bytes((build.stdout+build.stderr).encode())
report={'scope':'Exhaustive256 signed-byte source dispatch cases; unpatched native253inactive-or-accepted-empty cases. No instruction patching or native dispatch in replacement.','toolchain_id':tool,'command':cmd,'compile_exit_code':build.returncode,'input_sha256_before':before,'native_span_sha256':sha(pe.read_rva(0x1fdf0,80)),'native_instruction_prefix_sha256':sha(pe.read_rva(0x1fdf0,42)),'limits':['Active5/6/9 tail bodies source-spied only; no nested native effects verified.','FOREST3 setup is an actual return branch; other forest initialization remains separate.','No function-byte/source/TU/layout or whole-module match claim.']}
if build.returncode==0:
 run=subprocess.run([str(exe),str(oracle)],cwd=o,env=env,capture_output=True,text=True,timeout=60);(o/'run.log').write_bytes((run.stdout+run.stderr).encode());report.update(run_exit_code=run.returncode,stdout=run.stdout,stderr=run.stderr,executable_sha256=sha(exe.read_bytes()),object_sha256={p.name:sha(p.read_bytes()) for p in o.glob('*.obj')})
 m=re.search(r'setup_cases=(\d+) native_cases=(\d+) checks=(\d+) failures=(\d+)',run.stdout)
 if m:report.update(cases=int(m[1]),native_cases=int(m[2]),checks=int(m[3]),failures=int(m[4]))
 report['result']='PASS' if run.returncode==0 and m and int(m[4])==0 else 'FAIL'
else:report.update(result='COMPILE_FAILURE',compile_stdout=build.stdout,compile_stderr=build.stderr)
after={p:sha((root/p).read_bytes()) for p in before};report.update(input_sha256_after=after,inputs_unchanged=before==after)
(o/'report.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(build.stdout+build.stderr if build.returncode else report.get('stdout',''));sys.exit(0 if report['result']=='PASS' and before==after else 1)
