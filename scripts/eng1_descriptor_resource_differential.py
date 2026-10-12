"""Compare the descriptor scan and real cleanup providers with unpatched x86."""
import hashlib,json,os,re,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from match import DEFAULT_TOOLCHAIN,verify_toolchain
from refinery_forms import function_text
from title_behavioral import context_files
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def run():
    out=ROOT/'work/eng1_behavioral/differential/descriptor_resource'
    out.mkdir(parents=True,exist_ok=True)
    cleanup=ROOT/'src/eng1/behavioral/level_loading.c'
    body=function_text(cleanup.read_text(),'dispatch_local_034d0')
    provider=out/'cleanup_provider.c'
    provider.write_bytes(('#include "../../../../calibration/resource_record.h"\nextern ActorResource g_actor_resources[81];\nextern void __cdecl resource_destroy(int);\n'+body+'\n').encode())
    if function_text(provider.read_text(),'dispatch_local_034d0')!=body:
        raise ValueError('Fixture cleanup extraction differs from canonical source')
    sources=[ROOT/p for p in ['tests/eng1_descriptor_resource_differential.c','src/eng1/behavioral/descriptor_resource_scan.c','calibration/resource_data.c','src/eng1/behavioral/resource_data.c','calibration/resource_callbacks.c','calibration/resource_destroy.c','calibration/engine_interface_storage.c','calibration/engine_file_bridge.c','calibration/frame_lifecycle_bridges.c']]+[provider]
    oracle=ROOT/'assets/pc/HERCULES/ENG1.DLL'
    if sha(oracle)!='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757':
        raise ValueError('Original ENG1 identity changed')
    paths={p for source in sources for p in context_files(source)}|{oracle,cleanup,Path(__file__).resolve()}
    before={p.relative_to(ROOT).as_posix():sha(p) for p in paths}
    tool=verify_toolchain(DEFAULT_TOOLCHAIN);env=os.environ.copy()
    for key in ['CL','_CL_','LINK','_LINK_']:env.pop(key,None)
    env.update(PATH=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH',''),INCLUDE=str(DEFAULT_TOOLCHAIN/'include'),LIB=str(DEFAULT_TOOLCHAIN/'lib'))
    exe=out/'fixture.exe';exe.unlink(missing_ok=True)
    commands=[];objects=[];logs=[]
    for index,source in enumerate(sources):
        obj=out/('unit_'+str(index)+'.obj');obj.unlink(missing_ok=True)
        command=[str(DEFAULT_TOOLCHAIN/'bin/cl.exe'),'/nologo','/c','/O2','/Gy','/MT','/Fo'+str(obj),str(source)]
        commands.append(command)
        built=subprocess.run(command,cwd=out,env=env,capture_output=True,text=True,timeout=60)
        logs.append(built.stdout+built.stderr)
        if built.returncode:break
        objects.append(obj)
    if built.returncode==0:
        command=[str(DEFAULT_TOOLCHAIN/'bin/cl.exe'),'/nologo','/Fe'+str(exe),*map(str,objects),'/link','/INCREMENTAL:NO','/OPT:REF','kernel32.lib']
        commands.append(command)
        built=subprocess.run(command,cwd=out,env=env,capture_output=True,text=True,timeout=60)
        logs.append(built.stdout+built.stderr)
    (out/'compile.log').write_bytes(''.join(logs).encode())
    report=dict(scope='Four seeded unpatched3910 differential scenarios; real cancel/destroy/cleanup source effects,81full rows and context plus callback order/args. Host endpoints and valid-list guard counting are test spies; no gameplay or universal equality claim.',toolchain=tool,input_sha256=before,commands=commands,compile_exit_code=built.returncode,cleanup_extraction_exact=True,cleanup_body_sha256=hashlib.sha256(body.encode()).hexdigest())
    if built.returncode==0:
        tested=subprocess.run([str(exe),str(oracle)],cwd=out,env=env,capture_output=True,text=True,timeout=60)
        report.update(run_exit_code=tested.returncode,stdout=tested.stdout,stderr=tested.stderr,executable_sha256=sha(exe))
        match=re.search(r'checks=(\d+) failures=(\d+)',tested.stdout)
        if match:report.update(checks=int(match[1]),failures=int(match[2]))
    report['inputs_unchanged']=all(sha(ROOT/p)==value for p,value in before.items())
    report['result']='PASS' if report.get('run_exit_code')==0 and report.get('failures')==0 and report['inputs_unchanged'] else 'FAIL'
    (out/'report.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(report.get('stdout',built.stdout+built.stderr))
    return 0 if report['result']=='PASS' else 1
if __name__=='__main__':raise SystemExit(run())
