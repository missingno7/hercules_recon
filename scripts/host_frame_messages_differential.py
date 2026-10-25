"""Compare six original PC frame handlers with their actual recovered platform chains.

Identified MFC methods, archive/CRT/OS/COM endpoints are modeled explicitly.
No original process, message loop or device is started. All output stays in work/.
"""
import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'scripts/match.py').is_file())
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'work/host_frame_messages_replay')
parser.add_argument('--native-exe',type=Path,default=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE'))
args=parser.parse_args()
OUT=args.output.resolve()
assert OUT.is_relative_to((ROOT/'work').resolve())
OUT.mkdir(parents=True,exist_ok=True)
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,verify_toolchain,DEFAULT_TOOLCHAIN
from masked import masked_compare
from coff import COFF
from title_behavioral import context_files
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ORACLE=args.native_exe.resolve()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
pack=lambda v:struct.pack('<I',v&0xffffffff)
STACK,ESP,STOP,API=0x62000000,0x62008000,0x6200f000,0x65000000
HEAP,HEAP_SIZE=0x66000000,0x30000
DEVICE,DEVICE_VT,BUFFER_VT=HEAP,HEAP+0x1000,HEAP+0x2000
BUFFER_BASE,BLOCK1,BLOCK2=HEAP+0x10000,HEAP+0x20000,HEAP+0x21000
FRAME=0x67000000
FRAME_VT=FRAME+0x1000
FRAME_SPECS={
 'activate':('?OnRva_00009820@CMainFrame@@QAEXHPAUHTASK__@@@Z',0x9820,32,8),
 'size':('?OnRva_00009870@CMainFrame@@QAEXIHH@Z',0x9870,144,12),
 'syscommand':('?OnRva_00009910@CMainFrame@@QAEXIJ@Z',0x9910,64,8),
 'ncmouse':('?OnRva_00009950@CMainFrame@@QAEXIVCPoint@@@Z',0x9950,32,12),
 'initmenu':('?OnRva_00009970@CMainFrame@@QAEXPAVCMenu@@@Z',0x9970,32,4),
 'help':('?OnRva_00009840@CMainFrame@@QAEXXZ',0x9840,48,0)}
MFC={
 'WinHelp':('?WinHelpA@CWnd@@UAEXKI@Z',0x23412,224,148,8),
 'Default':('?Default@CWnd@@IAEJXZ',0x225ec,39,148,0),
 'BaseSize':('?OnSize@CFrameWnd@@IAEXIHH@Z',0x2b778,31,147,12),
 'BaseSysCommand':('?OnSysCommand@CFrameWnd@@IAEXIJ@Z',0x2a74c,151,147,8)}
FIELDS={
 'width':('_g_exe_ba0220',0x7a0220,4),
 'height':('_g_exe_ba0224',0x7a0224,4),
 'maximized':('_g_host_frame_maximized_7a022c',0x7a022c,1),
 'header':('_g_host_sound_header_699838',0x699838,16),
 'counter':('_g_host_sound_bank_counters_698ed8',0x698ed8,2400),
 'scratch':('_g_host_sound_scratch_699848',0x699848,1048576),
 'loops':('_g_host_sound_loop_flags_698a18',0x698a18,1200),
 'bank':('_g_exe_799908',0x799908,19200),
 'device':('_g_exe_79e440',0x79e440,4),
 'release_flag':('_g_audio_cleanup_iteration_flag',0x79e450,1),
 'ids':('_g_host_audio_channel_ids_799848',0x799848,96),
 'cursors':('_g_host_audio_channel_cursors_7998a8',0x7998a8,96),
 'sound_paused':('_g_host_sound_paused_79e44c',0x79e44c,4),
 'registry':('_g_registry_config',0x79e858,72),
 'window':('_g_fatal_owner_window',0x7a0200,4),
 'mode':('_g_exe_ba0238',0x7a0238,1),
 'type':('_g_exe_ba0228',0x7a0228,1),
 'depth':('_g_exe_ba0230',0x7a0230,4),
 'pending':('_g_host_input_pending_rva_7a0234',0x7a0234,4),
 'gate':('_g_host_input_gate_rva_03a594',0x3a594,1),
 'sound_dirty':('_g_exe_91c458',0x51c458,1),
 'music_dirty':('_g_exe_91c454',0x51c454,1),
 'reset':('_g_exe_91c46c',0x51c46c,4),
 'requested_display':('_g_exe_96e3d0',0x56e3d0,1),
 'fullscreen':('_g_fatal_suppress_dialog',0x7a020c,1),
 'primary_buffer':('_g_exe_615b9c',0x215b9c,4),
 'suspend_count':('_g_exe_615ba8',0x215ba8,4),
 'priority_enabled':('_g_exe_615bd8',0x215bd8,4),
 'saved_class':('_g_exe_615bd0',0x215bd0,4),
 'saved_thread_priority':('_g_exe_615bd4',0x215bd4,4),
 'thread_handle':('_g_exe_615bbc',0x215bbc,4),
}
for key,address in [('start',0x56e61c),('end',0x56e620),('previous',0x56e624),
                    ('paused_tick',0x56e628),('playing',0x56e62c),('opened',0x56e630)]:
    FIELDS[key]=('_g_exe_%x'%(address+0x400000),address,4)
CACHE=['mode','type','depth','gate','sound','music','extra']
for i,key in enumerate(CACHE):
    FIELDS['cache_'+key]=('_g_exe_%x'%(0x436638+i*4),0x36638+i*4,4)
METHODS={'Create':(0x0c,12),'Duplicate':(0x14,8),'Stop':(0x48,0),'Release':(8,0),
         'Position':(0x34,4),'Frequency':(0x44,4),'Pan':(0x40,4),'Volume':(0x3c,4),
         'Lock':(0x2c,28),'Unlock':(0x4c,16),'Status':(0x24,4),
         'CurrentPosition':(0x10,8),'Play':(0x30,12),'Restore':(0x50,0)}
APIS={'SetWindowTextA':8,'mciSendStringA':16,'timeGetTime':0,
      'GetCurrentProcess':0,'GetCurrentThread':0,'GetPriorityClass':4,
      'GetThreadPriority':4,'SetPriorityClass':8,'SetThreadPriority':8,'GetClientRect':8}
SYMBOLS={
 'sync':'_graphics_native_05a50','set_mode':'_cleanup_native_6ec0',
 'suspend':'_graphics_native_03770','priority':'_audio_native_035d0',
 'sound_on':'_host_sound_resume_10030','sound_pause':'_host_sound_pause_ffb0',
 'sound_off':'_audio_native_10010','stop':'_audio_native_fa10',
 'restore':'_host_sound_bank_restore_f760','poll':'_host_audio_poll_f820',
 'constructor':'_host_sound_bank_initialize_f3d0','decoder':'_host_sound_decode_2410',
 'release':'_cleanup_native_f710','music_on':'_host_cd_resume_9f60',
 'music_off':'_host_cd_unpause_9db0','music_pause':'_cleanup_native_9d60',
 'ready':'_cleanup_native_9c30'}


def build(source=None,out=None,frame_source=None):
    source=source or ROOT/'src/pc/host_platform_sync.cpp'
    out=out or OUT
    out.mkdir(parents=True,exist_ok=True)
    assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
    verify_toolchain(DEFAULT_TOOLCHAIN)
    files=[source,ROOT/'src/pc/host_platform_state_storage.c',
           frame_source or ROOT/'src/pc/host_frame_messages.cpp',ROOT/'src/pc/host_frame_state_storage.c',
           *[ROOT/p for p in ['src/pc/host_sound_channels.cpp','src/pc/host_sound_bank.cpp',
            'src/pc/host_sound_decoder.c','src/pc/host_sound_decoder_tables.c',
            'src/pc/host_sound_bank_storage.c','src/pc/host_sound_scratch_storage.c',
            'src/pc/audio_bank_release.cpp','src/pc/host_sound_channel_storage.cpp',
            'src/pc/sound_bank_storage.cpp','src/pc/archive_storage.c',
            'src/pc/host_warm_delivery.cpp','src/pc/host_warm_delivery_storage.cpp',
            'src/pc/host_callback_storage.c','src/shared/registry_storage.c',
            'src/pc/audio_small_helpers.c','src/pc/audio_gain_storage.c',
            'src/pc/host_cd_resume.c','src/pc/mci_pause.c','src/pc/mci_readiness.cpp',
            'src/pc/mci_storage.c','src/pc/display_mode_setter.c',
            'src/pc/graphics_storage.cpp','src/pc/host_system_key_storage.c',
            'src/pc/host_startup_storage.c','src/pc/audio_suspend.cpp',
            'src/pc/audio_priority.cpp','src/pc/audio_storage.cpp',
            'src/pc/fatal_storage.c']]]
    # Existing pinned historical inputs, in natural MFC-first order.
    library_inputs=[
        ('mfc/lib/nafxcw.lib', '5d9c3585552412a9b94a9a0b7c3a4208061a76d48a6c3283518bfe9f674c9e22'),
        ('lib/libcmt.lib', '7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b'),
        ('lib/kernel32.lib', '095f231e13cb3e6a6c4913d33ca8048506011034c89774b6ba0857466abba3cf'),
        ('lib/user32.lib', '5badb752dbcd6355a666d5b88aa9fd5e27291c16edc76f1b5258468d994f1809'),
        ('lib/gdi32.lib', '3c202569b76ec990be85393d484261fdb9b67614b35d7abd169eecf845d070f8'),
        ('lib/comdlg32.lib', 'cf4320d34c6fa1cfa0738973bdf3e984dddaeadf5050ddf6a5054969e7a4d827'),
        ('lib/winspool.lib', 'a27d801745ad55b08c4bc47fb21d2a9f374f38cb0821501858c1f41623641477'),
        ('lib/advapi32.lib', '2d7da3304b3f0b21fd5a3ddcec1aa91c7d64b270087793eb8e1dfd5be21e6ae5'),
        ('lib/shell32.lib', '2c717c106e8831b7b8dbc37ef93774f1978535b1122dbbcea2e29bd2bbe969c4'),
        ('lib/comctl32.lib', 'c6ee0e9711255dad6cbf371221dfe7b47d38bea641cb31d462906f624e4a2a41'),
        ('lib/winmm.lib', 'a996cf8e569b6f2dd0a478119a57c325733af545ffee2e6c6fa7dba40bfba260'),
        ('lib/ddraw.lib', 'd2150aeb8c09eed04a566751c98ccd1738d186d304f0268f22d5f6f655e80a97'),
        ('lib/dsound.lib', '4d6e8e1e0af841da16eb621f52ef40cab455b9bd31d4b9407a549e827d257773'),
    ]
    libraries=[DEFAULT_TOOLCHAIN/path for path,expected in library_inputs]
    assert all(sha(DEFAULT_TOOLCHAIN/path)==expected for path,expected in library_inputs)
    watched={ORACLE,ROOT/'recovery.json',Path(__file__),ROOT/'toolchains/manifest.json',*libraries}
    for source in files:watched.update(context_files(source))
    watched.update(ROOT/'scripts'/p for p in ['match.py','coff.py','masked.py','pe.py','env.py','title_behavioral.py'])
    pins={str(p):sha(p)for p in watched}
    fixture=out/'fixture.c'
    fixture.write_bytes(b'''#include <stddef.h>
int __cdecl archive_open(const char*a,const char*b) { for (;;) {} }
size_t __cdecl archive_read(void*a,size_t b,size_t c,int d) { for (;;) {} }
int __cdecl archive_close(int a) { for (;;) {} }
void __cdecl archive_fatal(const char*a) { for (;;) {} }
unsigned long __cdecl host_input_collect_8040(void) { for (;;) {} }
void __cdecl host_invoke_callback_one(void) { for (;;) {} }
int __cdecl main(void) { for (;;) {} }
extern void __cdecl graphics_native_05a50(void);
__declspec(dllexport) void anchor(void) { graphics_native_05a50(); }
''')
    frame_anchor=out/'frame_anchor.cpp'
    frame_anchor.write_bytes(('#include "'+(ROOT/'src/pc/host_system_keys.h').as_posix()+'"\n'+
        'extern "C" __declspec(dllexport) void frame_anchor(CMainFrame *p) { p->OnRva_00009820(0,0); p->OnRva_00009870(0,1,1); p->OnRva_00009910(0,0); p->OnRva_00009950(0,CPoint(1,2)); p->OnRva_00009970(0); p->OnRva_00009840(); }\n').encode())
    files.append(frame_anchor)
    objects,compiles,definitions,owners=[],[],{},{}
    for source in [*files,fixture]:
        flags=['/O2','/MT','/I'+str(DEFAULT_TOOLCHAIN/'mfc/include')]if source==files[2] or source==frame_anchor else['/O2']
        obj,command=compile_source(source,flags);objects.append(obj)
        compiles.append(dict(source=str(source),source_sha256=sha(source),object=str(obj),object_sha256=sha(obj),command=command))
        co=COFF(obj)
        for symbol in co.symbols.values():
            if symbol['storage']==2 and (symbol['section']>0 or symbol['section']==0 and symbol['value']>0):
                owners.setdefault(symbol['name'],[]).append((co,symbol,source))
                if symbol['type']&0x20:
                    assert symbol['name']not in definitions
                    definitions[symbol['name']]=(obj,co.function(symbol['name'])['size'])
    dll,mp=out/'fixture.dll',out/'fixture.map'
    command=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY',
             '/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF',
             '/OUT:'+str(dll),'/MAP:'+str(mp),*map(str,objects),*map(str,libraries)]
    env=os.environ.copy()
    for key in ['CL','_CL_','LINK','_LINK_']:env.pop(key,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    process=subprocess.run(command,env=env,capture_output=True,text=True)
    (out/'link.log').write_bytes((process.stdout+process.stderr).encode())
    assert process.returncode==0,process.stdout+process.stderr
    regex=re.compile(r'^\s*[\da-fA-F]{4}:[\da-fA-F]{8}\s+(\S+)\s+([\da-fA-F]{8})')
    symbols={m[1]:int(m[2],16)for line in mp.read_text().splitlines()if(m:=regex.match(line))}
    state=json.loads((ROOT/'recovery.json').read_bytes())
    rows={r['symbol']:r for r in state['functions']if Path(r['target']).name=='HERCULES.EXE'}
    profile,measurements={},[]
    for role,symbol in SYMBOLS.items():
        rva,width=(0x5a50,464)if role=='sync'else(int(rows[symbol]['rva'],0),rows[symbol]['size'])
        obj,extent=definitions[symbol]
        profile[role]=(symbol,rva,width)
        measurements.append(dict(role=role,symbol=symbol,source_size=extent,
            raw=compare(obj,symbol,ORACLE,rva,width),diagnostic=masked_compare(obj,symbol,ORACLE,rva,width)))
    for role,(symbol,rva,width,arguments)in FRAME_SPECS.items():
        obj,extent=definitions[symbol];profile[role]=(symbol,rva,width)
        measurements.append(dict(role=role,symbol=symbol,source_size=extent,
            raw=compare(obj,symbol,ORACLE,rva,width),diagnostic=masked_compare(obj,symbol,ORACLE,rva,width)))
    mfc_library=DEFAULT_TOOLCHAIN/'mfc/lib/nafxcw.lib'
    archive=mfc_library.read_bytes();position,index=8,0;members={}
    while position<len(archive):
        h=archive[position:position+60];assert h[58:]==b'`\n';length=int(h[48:58]);payload=archive[position+60:position+60+length]
        if index in[147,148]:members[index]=payload
        position+=60+length+(length&1);index+=1
    mfc_proofs=[]
    for role,(symbol,rva,width,index,args)in MFC.items():
        member=out/('mfc_member_'+str(index)+'.obj');member.write_bytes(members[index])
        result=masked_compare(member,symbol,ORACLE,rva,width)
        assert result['masked_equal'],(role,result)
        mfc_proofs.append(dict(role=role,symbol=symbol,rva=hex(rva),bytes=width,member=index,member_sha256=sha(member),complete_code_diagnostic=result))
    original=pefile.PE(str(ORACLE));data=[]
    assert struct.unpack('<I',original.get_data(0x2e870+0x74,4))[0]==0x400000+MFC['WinHelp'][1]
    for role,(name,rva,width)in FIELDS.items():
        assert len(owners[name])==1,name
        co,symbol,provider=owners[name][0]
        if symbol['section']==0:
            assert symbol['value']==width
            value=bytes(width)
        else:
            section=co.sections[symbol['section']-1]
            assert not section['relocations']and symbol['value']+width<=section['size']
            value=section['data'][symbol['value']:symbol['value']+width]if section['data']else bytes(width)
        target=next(t for t in original.sections if t.VirtualAddress<=rva and rva+width<=t.VirtualAddress+t.Misc_VirtualSize)
        zero=rva>=target.VirtualAddress+target.SizeOfRawData
        assert value==(bytes(width)if zero else original.get_data(rva,width)),role
        data.append(dict(role=role,symbol=name,rva=hex(rva),bytes=width,provider=str(provider),backing='virtual_zero'if zero else'initialized'))
    for name,rva in [('_g_exe_436810',0x36810),('_g_exe_43682c',0x3682c),('_g_exe_43684c',0x3684c)]:
        value=original.get_data(rva,256).split(b'\0')[0]+b'\0'
        co,symbol,provider=owners[name][0];sec=co.sections[symbol['section']-1]
        assert sec['data'][symbol['value']:symbol['value']+len(value)]==value
    release=next(x for x in measurements if x['role']=='release')
    assert int(release['diagnostic']['symbols']['_g_audio_cleanup_iteration_flag'],0)==FIELDS['release_flag'][1]
    assert pins=={p:sha(Path(p))for p in pins}
    return dict(inputs=pins,compiles=compiles,measurements=measurements,profile=profile,
                symbols=symbols,dll=str(dll),dll_sha256=sha(dll),map_sha256=sha(mp),
                link_command=command,data=data,mfc_proofs=mfc_proofs)


def execute(kind,b,s):
    native=kind=='native';image=pefile.PE(str(ORACLE if native else b['dll']))
    base,size=image.OPTIONAL_HEADER.ImageBase,image.OPTIONAL_HEADER.SizeOfImage
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095)
    u.mem_write(base,image.get_memory_mapped_image().ljust(size,b'\0'))
    u.mem_map(STACK,0x10000);u.mem_map(API,0x1000);u.mem_map(HEAP,HEAP_SIZE);u.mem_map(FRAME,0x2000)
    fields={k:(0x400000+rva if native else b['symbols'][sym],width)for k,(sym,rva,width)in FIELDS.items()}
    get32=lambda a:int.from_bytes(u.mem_read(a,4),'little')
    def string(a):return bytes(u.mem_read(a,256)).split(b'\0')[0].decode('latin1')
    views={k:(k,0,1 if k in ['mode','type','gate','music_dirty','sound_dirty','requested_display','fullscreen','release_flag','maximized']else 4)
           for k in fields if k not in ['registry','header','counter','scratch','loops','bank','ids','cursors']}
    views.update(sound=('registry',14,1),music=('registry',13,1),extra=('registry',16,4))
    for key in CACHE:views['cache_'+key]=('cache_'+key,0,4 if key in ['depth','extra']else 1)
    def put(key,value):
        field,offset,width=views[key];u.mem_write(fields[field][0]+offset,pack(value)[:width])
    def get(key):
        field,offset,width=views[key];return int.from_bytes(u.mem_read(fields[field][0]+offset,width),'little')
    for key,(a,width)in fields.items():
        u.mem_write(a,bytes((i*29+(i>>8)*17+43)&255 for i in range(width))if key in ['header','counter','scratch','loops','registry']else bytes(width))
    values=dict(window=0x12345678,mode=3,type=0,depth=0,pending=0xaabbccdd,gate=1,
                sound=1,music=1,extra=0x12345678,sound_dirty=0,music_dirty=0,reset=0xabcdef,
                requested_display=7,fullscreen=0,primary_buffer=0,suspend_count=4,
                priority_enabled=0,saved_class=0x20,saved_thread_priority=1,thread_handle=0,
                start=0x030101,end=0x070202,previous=500,paused_tick=1500,playing=0,opened=1,
                sound_paused=1,release_flag=0,width=321,height=241,maximized=0x80)
    values.update(s.get('values',{}))
    for key,value in values.items():put(key,value)
    for key in CACHE:
        a=fields['cache_'+key][0];u.mem_write(a,bytes([0xa5])*4)
        put('cache_'+key,get(key)if s.get('same_cache')else 0xff)
    for key,value in s.get('cache',{}).items():put('cache_'+key,value)
    u.mem_write(HEAP,bytes([0x91])*HEAP_SIZE);u.mem_write(DEVICE,pack(DEVICE_VT))
    for token in range(32):u.mem_write(BUFFER_BASE+token*16,pack(BUFFER_VT)+pack(1)+pack(100+token)+pack(0))
    for bank in [0,1,2]:
        for lane in range(4):u.mem_write(fields['bank'][0]+(bank*4+lane)*4,pack(BUFFER_BASE+(bank*4+lane)*16))
    for slot in range(24):
        u.mem_write(fields['ids'][0]+slot*4,pack(0xffffffff));u.mem_write(fields['cursors'][0]+slot*4,pack(1000+slot))
    for slot,idvalue in {0:0,3:0x10002,23:0x20003}.items():u.mem_write(fields['ids'][0]+slot*4,pack(idvalue))
    for token,value in s.get('status',{}).items():u.mem_write(BUFFER_BASE+int(token)*16+4,pack(value))
    for bank,value in {0:0,1:1,2:128}.items():u.mem_write(fields['loops'][0]+bank,bytes([value]))
    u.mem_write(fields['device'][0],pack(DEVICE))
    if s.get('primary_buffer'):put('primary_buffer',BUFFER_BASE+12*16)
    frame_data=bytearray((i*11+7)&255 for i in range(188));frame_data[:4]=pack(FRAME_VT);frame_data[0x1c:0x20]=pack(0x11223344)
    u.mem_write(FRAME,bytes(frame_data));u.mem_write(FRAME_VT+0x74,pack(API+80*16))
    endpoints={API+80*16:'WinHelp'}
    for role,(symbol,rva,width,index,arguments)in MFC.items():
        endpoints[0x400000+rva if native else b['symbols'][symbol]]=role
    for i,(method,(offset,extra))in enumerate(METHODS.items()):
        a=API+i*16;endpoints[a]=method
        u.mem_write((DEVICE_VT if method in ['Create','Duplicate']else BUFFER_VT)+offset,pack(a))
    for i,(name,extra)in enumerate(APIS.items(),32):
        a=API+i*16;endpoints[a]=name
        imports=[x for group in image.DIRECTORY_ENTRY_IMPORT for x in group.imports if x.name==name.encode()]
        assert len(imports)==1,name;u.mem_write(imports[0].address,pack(a))
    for role,rv,symbol in [('format',0x10f40,'_sprintf'),('stricmp',0x1c270,'__stricmp'),
        ('open',0x5130,'_archive_open'),('read',0x5210,'_archive_read'),
        ('close',0x51f0,'_archive_close'),('fatal',0x10460,'_archive_fatal')]:
        endpoints[0x400000+rv if native else b['symbols'][symbol]]=role
    addresses={r:0x400000+rv if native else b['symbols'][sym]for r,(sym,rv,width)in b['profile'].items()}
    source_widths={x['role']:x['source_size']for x in b['measurements']}
    ranges=[(addresses[r],addresses[r]+(width if native else source_widths[r]))for r,(sym,rv,width)in b['profile'].items()]
    events,entries,errors,counts=[],[],[],{}
    u.mem_write(ESP-0x1000,bytes([0xa7])*0x1100);arguments=s.get('arguments',[])
    u.mem_write(ESP,pack(STOP)+b''.join(pack(x)for x in arguments));u.reg_write(UC_X86_REG_ESP,ESP)
    u.reg_write(UC_X86_REG_ECX,FRAME)
    nv=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];seeds=[0x11223344,0x22334455,0x33445566,0x44556677]
    for reg,value in zip(nv,seeds):u.reg_write(reg,value)
    def action(role):
        i=counts.get(role,0);counts[role]=i+1
        choices=s.get('actions',{}).get(role,[]);a=choices[i]if i<len(choices)else{}
        for key,value in a.get('mutate',{}).items():put(key,value)
        return a
    def ret(sp,extra,value):
        u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xdeadbeef);u.reg_write(UC_X86_REG_EDX,0xdeadbeef)
        u.reg_write(UC_X86_REG_ESP,sp+4+extra);u.reg_write(UC_X86_REG_EIP,get32(sp))
    receiver_changed=False
    def on_code(machine,a,width,_):
        nonlocal receiver_changed
        if a==STOP:machine.emu_stop();return
        if a in addresses.values():entries.append(next(r for r,v in addresses.items()if v==a))
        role=endpoints.get(a)
        if not role:
            if not any(lo<=a<hi for lo,hi in ranges):errors.append(['execution',hex(a)]);machine.emu_stop()
            return
        sp=u.reg_read(UC_X86_REG_ESP);arg=lambda n:get32(sp+n*4)
        ac=action(role)
        if role in MFC or role=='WinHelp':
            assert u.reg_read(UC_X86_REG_ECX)==FRAME,(role,hex(u.reg_read(UC_X86_REG_ECX)))
            extra=MFC[role][4]if role in MFC else 8
            arguments=[arg(n+1)for n in range(extra//4)]
            if role=='WinHelp':assert arguments==[0,3]
            events.append([role,arguments])
            if ac.get('retarget_hwnd'):
                u.mem_write(FRAME+0x1c,pack(ac['retarget_hwnd']));receiver_changed=True
            ret(sp,extra,ac.get('result',0x24681357));return
        if role=='GetClientRect':
            assert arg(1)==get32(FRAME+0x1c)
            target=arg(2);assert ESP-0x1000<=target and target+16<=ESP
            rect=s.get('rect',[10,20,330,260]);u.mem_write(target,b''.join(pack(v)for v in rect))
            events.append([role,arg(1),rect,ac.get('result',1)]);ret(sp,8,ac.get('result',1));return
        if role=='SetWindowTextA':
            events.append([role,arg(1),string(arg(2))]);ret(sp,8,ac.get('result',1));return
        if role=='mciSendStringA':
            text,target,length=string(arg(1)),arg(2),arg(3);assert arg(4)==0 and length in [0,256]
            response=ac.get('response','true');code=ac.get('result',0)
            if length:
                assert ESP-0x1000<=target and target+length<=ESP
                data=response.encode()+b'\0';assert len(data)<=length;u.mem_write(target,data)
            else:assert target==0
            events.append([role,text,length,response if length else None,code]);ret(sp,16,code);return
        if role=='timeGetTime':
            value=ac.get('result',0x10203040+counts[role]*100)
            events.append([role,value]);ret(sp,0,value);return
        if role in APIS:
            arguments=[arg(i+1)for i in range(APIS[role]//4)]
            value=ac.get('result',0xfffffffe if role=='GetCurrentProcess'else 0xfffffffd if role=='GetCurrentThread'else 1)
            events.append([role,*arguments,value]);ret(sp,APIS[role],value);return
        if role=='stricmp':
            first,second=string(arg(1)),string(arg(2));value=int(first.lower()!=second.lower())
            events.append([role,first,second,value]);ret(sp,0,value);return
        if role=='format':
            fmt=string(arg(2))
            if '%ld'in fmt:arguments=[arg(3)];text=fmt.replace('%ld',str(arg(3)))
            else:arguments=[arg(3),arg(4)];text=fmt%tuple(arguments)
            target=arg(1);assert ESP-0x1000<=target and target+len(text)+1<=ESP
            u.mem_write(target,text.encode()+b'\0');events.append([role,fmt,arguments,text]);ret(sp,0,len(text));return
        if role=='open':events.append([role,string(arg(1)),string(arg(2))]);ret(sp,0,3);return
        if role=='read':
            target,size,count,handle=arg(1),arg(2),arg(3),arg(4);assert count==1 and handle==3
            if target==fields['header'][0]:assert size==16;payload=struct.pack('<4I',0x03465345,32,22050,1);where='header'
            else:
                where=target-fields['scratch'][0];assert 0<=where and where+size<=1048576 and size==8
                payload=bytes.fromhex('0123456789abcdef')
            u.mem_write(target,payload);events.append([role,where,size,handle]);ret(sp,0,1);return
        if role=='close':events.append([role,arg(1)]);ret(sp,0,0);return
        if role=='fatal':raise AssertionError(('unexpected fatal',string(arg(1))))
        receiver=arg(1);token=(receiver-BUFFER_BASE)//16
        if role in ['Create','Duplicate']:assert receiver==DEVICE
        else:assert 0<=token<32 and receiver==BUFFER_BASE+token*16
        arguments=[]
        if role=='Create':
            desc,target,outer=arg(2),arg(3),arg(4);assert outer==0
            assert fields['bank'][0]<=target<fields['bank'][0]+19200
            assert ESP-0x1000<=desc and desc+20<=ESP
            raw=bytearray(u.mem_read(desc,20));pcm=get32(desc+16);assert ESP-0x1000<=pcm and pcm+16<=ESP
            raw[16:20]=bytes(4);bank=(target-fields['bank'][0])//16
            arguments=[bank,raw.hex(),bytes(u.mem_read(pcm,16)).hex()]
            u.mem_write(target,pack(BUFFER_BASE+bank*64))
        elif role=='Duplicate':
            source,target=arg(2),arg(3);slot=(target-fields['bank'][0])//4
            arguments=[(source-BUFFER_BASE)//16,slot];u.mem_write(target,pack(BUFFER_BASE+slot*16))
        elif role=='Lock':
            assert arg(2)==0 and arg(8)==0
            for p in [arg(4),arg(5),arg(6),arg(7)]:assert ESP-0x1000<=p and p+4<=ESP
            u.mem_write(arg(4),pack(BLOCK1));u.mem_write(arg(5),pack(arg(3)));u.mem_write(arg(6),pack(0));u.mem_write(arg(7),pack(0))
            arguments=[arg(3)]
        elif role=='Unlock':assert arg(2)==BLOCK1 and arg(4)==0;arguments=[arg(3),arg(5)]
        elif role=='Status':
            target=arg(2);assert ESP-0x1000<=target and target+4<=ESP
            value=ac.get('status',get32(receiver+4));u.mem_write(target,pack(value));arguments=[value]
        elif role=='CurrentPosition':
            first,second=arg(2),arg(3)
            assert fields['cursors'][0]<=first<fields['cursors'][0]+96 and ESP-0x1000<=second and second+4<=ESP
            value=get32(receiver+8);u.mem_write(first,pack(value));u.mem_write(second,pack(value+17));arguments=[value]
        elif role=='Position':value=arg(2);u.mem_write(receiver+8,pack(value));arguments=[value]
        elif role in ['Frequency','Pan','Volume']:arguments=[arg(2)]
        elif role=='Play':
            arguments=[arg(2),arg(3),arg(4)];assert arguments[:2]==[0,0]and arguments[2]in[0,1]
            u.mem_write(receiver+4,pack(get32(receiver+4)|1))
        elif role=='Stop':u.mem_write(receiver+4,pack(get32(receiver+4)&~1))
        elif role=='Release':u.mem_write(receiver+12,pack(1))
        elif role=='Restore':u.mem_write(receiver+4,pack(get32(receiver+4)&~2))
        events.append([role,'device'if role in ['Create','Duplicate']else token,*arguments])
        ret(sp,METHODS[role][1]+4,ac.get('result',0))
    def on_write(machine,access,a,width,value,_):
        allowed=[(ESP-0x1000,ESP+4+len(s.get('arguments',[]))*4),(FRAME,FRAME+188),(BLOCK1,BLOCK1+4096),*[(a,a+n)for a,n in fields.values()]]
        if not any(lo<=a and a+width<=hi for lo,hi in allowed):errors.append(['write',hex(a),width]);machine.emu_stop()
    u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write)
    u.emu_start(addresses[s.get('handler','sync')],STOP,count=2000000)
    checks=dict(returned=u.reg_read(UC_X86_REG_EIP)==STOP,stack=u.reg_read(UC_X86_REG_ESP)==ESP+4+(FRAME_SPECS[s['handler']][3]if s.get('handler')in FRAME_SPECS else 0),
                nonvolatiles=all(u.reg_read(reg)==value for reg,value in zip(nv,seeds)),
                guards=bytes(u.mem_read(ESP-0x1000,32))==bytes([0xa7])*32 and bytes(u.mem_read(ESP+4+len(s.get('arguments',[]))*4,32))==bytes([0xa7])*32,
                whitelist=not errors)
    assert all(checks.values()),(kind,s,checks,errors)
    if s.get('no_callbacks'):assert entries==['sync']and not events,(kind,s,entries,events)
    for required in s.get('requires',[]):assert required in entries,(kind,s,required,entries)
    expected_receiver=bytearray(frame_data)
    if receiver_changed:expected_receiver[0x1c:0x20]=u.mem_read(FRAME+0x1c,4)
    assert bytes(u.mem_read(FRAME,188))==bytes(expected_receiver),(kind,s,'receiver')
    result=dict(receiver_sha256=hashlib.sha256(bytes(u.mem_read(FRAME,188))).hexdigest(),checks=checks,state={k:hashlib.sha256(bytes(u.mem_read(a,n))).hexdigest()for k,(a,n)in fields.items()},
                views={k:get(k)for k in views},heap_sha256=hashlib.sha256(bytes(u.mem_read(HEAP,HEAP_SIZE))).hexdigest(),
                events=events,entries=entries)
    return result


def platform_scenarios():
    result=[dict(name='active',requires=['sound_on','restore','music_on','ready']),
            dict(name='null_window',values=dict(window=0),no_callbacks=True),
            dict(name='same_cache',same_cache=True,no_callbacks=True),
            dict(name='dirty_but_same_cache',same_cache=True,values=dict(sound_dirty=1,music_dirty=1),no_callbacks=True),
            dict(name='sound_off',values=dict(sound=0),requires=['sound_off','stop']),
            dict(name='music_off',values=dict(music=0,playing=1),requires=['music_off','ready']),
            dict(name='wait_depth',values=dict(depth=1,sound_paused=0),requires=['set_mode','suspend','priority','sound_pause','poll']),
            dict(name='wait_both',values=dict(mode=1,depth=1,sound_paused=0,playing=1,paused_tick=0),requires=['sound_pause','music_pause','ready']),
            dict(name='suspend_primary_priority',values=dict(mode=1,priority_enabled=1),primary_buffer=True,requires=['suspend','priority','music_pause']),
            dict(name='restore_constructor',status={0:2},requires=['restore','constructor','decoder','release']),
            dict(name='restore_mixed_status',status={0:6},requires=['restore']),
            dict(name='title_reloads',actions={'SetWindowTextA':[dict(mutate=dict(mode=0,type=1,depth=1,gate=0))]}),
            dict(name='suspend_reloads',values=dict(mode=0,depth=0,sound_paused=0),primary_buffer=True,
                 actions={'Stop':[dict(mutate=dict(mode=3,type=0,depth=1))]},requires=['sound_pause']),
            dict(name='sound_changes_music',actions={'Position':[dict(mutate=dict(music=0,gate=0,extra=0xaabbccdd))]},requires=['music_off']),
            dict(name='clock_changes_cache',actions={'timeGetTime':[dict(result=90000,mutate=dict(mode=1,type=2,depth=3))]}),
            dict(name='restore_failure_ignored',status={0:2},actions={'Restore':[dict(result=0x80004005)]},requires=['constructor']),
            dict(name='ready_false',actions={'mciSendStringA':[dict(response='false')]}),
            dict(name='format_failure_ignored',actions={'mciSendStringA':[{}, {},dict(result=7)]})]
    result.extend([
        dict(name='high_depth',values=dict(depth=0x80000000,sound_paused=0),requires=['sound_pause']),
        dict(name='window_text_failure_ignored',actions={'SetWindowTextA':[dict(result=0)]},requires=['sound_on','music_on']),
        dict(name='priority_high_bit',values=dict(mode=1,priority_enabled=0x80000000),requires=['priority']),
        dict(name='dirty_forced_by_extra',same_cache=True,cache=dict(extra=0x87654321),values=dict(sound_dirty=128,music_dirty=128),requires=['sound_on','music_on']),
        dict(name='wide_registry_extra',same_cache=True,cache=dict(extra=0),values=dict(extra=0x80000000)),
    ])
    for mode in [0,1,2,127,128,255]:result.append(dict(name='mode_'+str(mode),values=dict(mode=mode)))
    for typ in [1,128,255]:result.append(dict(name='type_'+str(typ),values=dict(type=typ)))
    for value in [0,1,2,128,255]:result.append(dict(name='music_gate_'+str(value),values=dict(music=value)))
    return result


def scenarios():
    result=platform_scenarios()
    for active in [0,1,0xffffffff,0x80000000]:result.append(dict(name='activate_'+str(active),handler='activate',arguments=[active,0x76543210],requires=['sync']))
    for command in [0,0xf140,0xf14f,0x1234f141,0xf170,0xffffffff]:result.append(dict(name='sys_'+hex(command),handler='syscommand',arguments=[command,0x89abcdef]))
    for handler,args in [('ncmouse',[0xffffffff,0xfffffff6,0x12345678]),('initmenu',[0]),('help',[])]:
        result.append(dict(name=handler,handler=handler,arguments=args))
    result.extend([dict(name='depth_wrap_menu',handler='initmenu',arguments=[0x12345678],values=dict(depth=0xffffffff)),dict(name='default_mutates_before_activate',handler='activate',arguments=[0,3],actions={'Default':[dict(mutate=dict(mode=127))]})])
    for typ in [0,1,2,3,0x80000002]:result.append(dict(name='size_type_'+str(typ),handler='size',arguments=[typ,999,555]))
    for rect in [[0,0,0,240],[0,0,320,0],[20,30,10,50],[0,20,3,19],[100,100,101,101]]:result.append(dict(name='size_rect_'+str(rect),handler='size',arguments=[2,1,1],rect=rect))
    result.extend([dict(name='size_fullscreen',handler='size',arguments=[2,1,1],values=dict(fullscreen=128)),dict(name='size_base_exits_fullscreen',handler='size',arguments=[2,1,1],values=dict(fullscreen=1),actions={'BaseSize':[dict(mutate=dict(fullscreen=0),retarget_hwnd=0x87654321)]}),dict(name='size_base_enters_fullscreen',handler='size',arguments=[2,1,1],actions={'BaseSize':[dict(mutate=dict(fullscreen=1))]}),dict(name='size_api_false_with_rect',handler='size',arguments=[2,1,1],actions={'GetClientRect':[dict(result=0)]}),dict(name='size_max_mutation_after_store',handler='size',arguments=[2,1,1],actions={'GetClientRect':[dict(mutate=dict(maximized=128))]})])
    return result

def main():
    b,cases=build(),[]
    for s in scenarios():
        native=execute('native',b,s);source=execute('source',b,s)
        assert native==source,(s,native,source)
        cases.append(dict(scenario=s,native=native,source=source,equal=True))
    negative_source=OUT/'negative_mask.cpp'
    original=(ROOT/'src/pc/host_frame_messages.cpp').read_text()
    assert original.count('command & 0xfff0')==1
    # Absolute include preserves the real receiver context in the private negative.
    negative_source.write_bytes(original.replace('../../src/pc/host_system_keys.h',(ROOT/'src/pc/host_system_keys.h').as_posix()).replace('command & 0xfff0','command & ~0xf').encode())
    bad=build(out=OUT/'negative_mask',frame_source=negative_source)
    discriminator=next(s for s in scenarios() if s['name']=='sys_0x1234f141')
    native=execute('native',b,discriminator);wrong=execute('source',bad,discriminator)
    assert native!=wrong and native['entries']==['syscommand'] and 'sync' in wrong['entries']
    assert b['inputs']=={p:sha(Path(p))for p in b['inputs']}
    report=dict(scope='Actual frame methods and recovered platform descendants;MFC library methods modeled at identified entry boundaries. Only explicit archive,fatal,CRT,OS and COM boundaries are modeled;no original process or real device.',
                build=b,cases=cases,case_count=len(cases),all_equal=True,
                negative_control=dict(description="Incorrect ~0xf mask preserves high bits; original 0xfff0 clears them",scenario=discriminator,detected=True,native=native,wrong=wrong,build=bad),
                executed_roles=sorted({r for c in cases for r in c['native']['entries']}))
    p=OUT/'frame_messages_differential.json';p.write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(dict(case_count=len(cases),all_equal=True,executed_roles=report['executed_roles'],report=str(p))))


if __name__=='__main__':main()
