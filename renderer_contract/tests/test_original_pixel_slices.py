"""Finite original-x86 pixel oracles for CC50, C2E0, C8F0, and CAB0."""
from __future__ import annotations
import hashlib, json, struct
from pathlib import Path
import sys

SCRIPT_DIR=Path(__file__).resolve().parent
def find_repository_root(start):
    for path in (start,*start.parents):
        if (path/'renderer_contract').is_dir() and (path/'scripts'/'pe.py').is_file():
            return path
    raise SystemExit(f'cannot locate repository ancestor containing renderer_contract and scripts: {start}')

ROOT=find_repository_root(SCRIPT_DIR)
if (SCRIPT_DIR/'oracle_common.py').is_file():
    COMMON_DIR=SCRIPT_DIR
else:
    COMMON_DIR=ROOT/'renderer_contract'/'tests'
if not (COMMON_DIR/'oracle_common.py').is_file():
    raise SystemExit(f'cannot find oracle_common.py beside test or under repository: {COMMON_DIR}')
sys.path.insert(0,str(COMMON_DIR))
from oracle_common import parse_args, verify_runtime, write_receipt
ARGS=parse_args("Run finite original HERCULES.EXE palette, fill, blend, and copy slices","native_pixel_slices.json")
if not any(arg=='--receipt' or arg.startswith('--receipt=') for arg in sys.argv[1:]):
    raise SystemExit('--receipt is required; choose a receipt path inside <original-root>/work')
from pe import PE
import unicorn
from unicorn import (Uc,UcError,UC_ARCH_X86,UC_HOOK_CODE,UC_HOOK_MEM_INVALID,UC_HOOK_MEM_READ,
    UC_HOOK_MEM_WRITE,UC_MODE_32,UC_PROT_EXEC,UC_PROT_READ,UC_PROT_WRITE)
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBP,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_EDX,UC_X86_REG_EFLAGS,UC_X86_REG_EIP,UC_X86_REG_ESI,UC_X86_REG_ESP)

BASE=0x400000; EXE_SHA='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
SCRATCH=0x10000000; SCRATCH_SIZE=0x4000; TEXTURE=SCRATCH+0x100; DEST=SCRATCH+0x2100
STACK=0x20000000; STACK_SIZE=0x10000; FRAME=STACK+0x8000
MAX_INSNS=20000
SURFACE=0xBA0210; STRIDE=0xBA0214; TPage=0xA8E6F8; SOURCE_BASE=0xA909FC
DEST_CELL=0xA90944; BLEND_CELL=0xA90948; COLOR_BASE_CELL=0xA90954; COLOR_ROW_CELL=0xA90A04
WIDTH=0xA90A0C; HEIGHT=0xA90A10; FILL_COUNT=0xA9096C; FILL_COLOR=0xA90A08; FILL_DEST=0xA90940
PALETTE=0x92C390; BLEND=0x92E3D0
SOURCE=bytes((0,7)); OLD_DEST=bytes((0x35,0x3C)); COLOR=0x23; ROW=3; SHADING_RESULT=0xA6


def sha(data): return hashlib.sha256(data).hexdigest()
def fail(message): raise AssertionError(message)
def align(value): return (value+0xFFF)&~0xFFF
def perms(chars):
    p=0
    if chars&0x40000000:p|=UC_PROT_READ
    if chars&0x80000000:p|=UC_PROT_WRITE
    if chars&0x20000000:p|=UC_PROT_EXEC
    return p

def map_original(uc,pe):
    h=pe.pe.OPTIONAL_HEADER.SizeOfHeaders
    uc.mem_map(BASE,align(h),UC_PROT_READ); uc.mem_write(BASE,pe.data[:h])
    for s in pe.sections:
        size=align(max(s['virtual_size'],s['raw_size'])); va=BASE+s['virtual_address']
        uc.mem_map(va,size,perms(s['characteristics']))
        if s['raw_size']:uc.mem_write(va,pe.read_rva(s['virtual_address'],s['raw_size']))
        if size>s['raw_size']:uc.mem_write(va+s['raw_size'],b'\0'*(size-s['raw_size']))

def data_region(pe):
    s=next(s for s in pe.sections if s['name']=='.data')
    return BASE+s['virtual_address'],align(max(s['virtual_size'],s['raw_size']))

def put32(uc,address,value):uc.mem_write(address,struct.pack('<I',value&0xffffffff))
def put16(uc,address,value):uc.mem_write(address,struct.pack('<H',value&0xffff))

class Machine:
    def __init__(self,pe,name,windows,data_reads,data_writes,dest_ranges=()):
        self.pe=pe;self.name=name;self.uc=Uc(UC_ARCH_X86,UC_MODE_32)
        map_original(self.uc,pe)
        self.uc.mem_map(SCRATCH,SCRATCH_SIZE,UC_PROT_READ|UC_PROT_WRITE)
        self.uc.mem_map(STACK,STACK_SIZE,UC_PROT_READ|UC_PROT_WRITE)
        self.data_va,self.data_size=data_region(pe)
        self.original_data=bytes(self.uc.mem_read(self.data_va,self.data_size))
        self.events=[];self.trace=[];self.error={'kind':None,'detail':None};self.stopped=[]
        self.windows=windows;self.active_stop=None;self.allowed_data_reads=set(data_reads)
        self.allowed_data_writes=dict(data_writes);self.dest_ranges=tuple(dest_ranges)
        self.before_exec_data=None;self.code_snapshots={}
        for lo,hi in windows:self.code_snapshots[(lo,hi)]=bytes(self.uc.mem_read(BASE+lo,hi-lo))
        self.uc.hook_add(UC_HOOK_CODE,self._code)
        self.uc.hook_add(UC_HOOK_MEM_READ,self._read)
        self.uc.hook_add(UC_HOOK_MEM_WRITE,self._write)
        self.uc.hook_add(UC_HOOK_MEM_INVALID,self._invalid)
    def _code(self,m,address,size,_):
        rva=address-BASE
        if self.active_stop is not None and address==BASE+self.active_stop:
            self.stopped.append(rva);m.emu_stop();return
        if not any(lo<=rva<hi for lo,hi in self.windows):
            self.error.update(kind='unexpected_code',detail=hex(address));m.emu_stop();return
        self.trace.append(rva)
    def _read(self,m,access,address,size,value,_):
        pc=m.reg_read(UC_X86_REG_EIP)-BASE
        raw=bytes(m.mem_read(address,size));v=int.from_bytes(raw,'little')
        self.events.append(('read',pc,address,size,v))
        if self.data_va<=address and address+size<=self.data_va+self.data_size:
            if (pc,address,size) not in self.allowed_data_reads:
                self.error.update(kind='unexpected_data_read',detail=(hex(pc),hex(address),size));m.emu_stop()
        elif not (SCRATCH<=address and address+size<=SCRATCH+SCRATCH_SIZE) and not (STACK<=address and address+size<=STACK+STACK_SIZE):
            self.error.update(kind='unexpected_read',detail=(hex(pc),hex(address),size));m.emu_stop()
    def _write(self,m,access,address,size,value,_):
        pc=m.reg_read(UC_X86_REG_EIP)-BASE;v=value&((1<<(size*8))-1)
        self.events.append(('write',pc,address,size,v))
        if self.data_va<=address and address+size<=self.data_va+self.data_size:
            if (pc,address,size) not in self.allowed_data_writes:
                self.error.update(kind='unexpected_data_write',detail=(hex(pc),hex(address),size));m.emu_stop()
        elif (SCRATCH<=address and address+size<=SCRATCH+SCRATCH_SIZE) or (STACK<=address and address+size<=STACK+STACK_SIZE):
            pass
        elif any(lo<=address and address+size<=hi for lo,hi in self.dest_ranges):
            pass
        else:
            self.error.update(kind='unexpected_write',detail=(hex(pc),hex(address),size));m.emu_stop()
    def _invalid(self,m,access,address,size,value,_):
        self.error.update(kind='invalid_memory',detail=(hex(m.reg_read(UC_X86_REG_EIP)),access,hex(address),size));m.emu_stop();return False
    def phase(self,start,stop,registers=None):
        self.active_stop=stop
        if registers:
            for reg,value in registers.items():self.uc.reg_write(reg,value)
        self.uc.reg_write(UC_X86_REG_EIP,BASE+start)
        try:self.uc.emu_start(BASE+start,0,timeout=1_000_000,count=MAX_INSNS)
        except UcError as e:
            if self.error['kind'] is None:self.error.update(kind='unicorn_error',detail=str(e))
        if self.error['kind'] is not None:fail(f'{self.name}: {self.error}')
        if not self.stopped or self.stopped[-1]!=stop:fail(f'{self.name}: did not stop before {stop:#x}')
        self.active_stop=None
    def check(self,expected_data_writes,expected_all_writes,expected_destination=None):
        if self.error['kind'] is not None:fail(f'{self.name}: {self.error}')
        got_data=[(e[0],e[1],e[2],e[3],e[4]) for e in self.events if e[0]=='write' and self.data_va<=e[2]<self.data_va+self.data_size]
        if got_data!=expected_data_writes:fail(f'{self.name}: .data write stream {got_data!r} != {expected_data_writes!r}')
        got_writes=[(e[1],e[2],e[3],e[4]) for e in self.events if e[0]=='write']
        if got_writes!=expected_all_writes:fail(f'{self.name}: write stream {got_writes!r} != {expected_all_writes!r}')
        expected=bytearray(self.before_exec_data)
        for _,_,address,size,value in expected_data_writes:
            off=address-self.data_va;expected[off:off+size]=int(value).to_bytes(size,'little')
        if bytes(self.uc.mem_read(self.data_va,self.data_size))!=bytes(expected):fail(f'{self.name}: unexpected .data mutation')
        if expected_destination is not None:
            address,expected_bytes=expected_destination
            got=bytes(self.uc.mem_read(address,len(expected_bytes)))
            if got!=expected_bytes:fail(f'{self.name}: destination {got.hex()} != {expected_bytes.hex()}')
        for (lo,hi),before in self.code_snapshots.items():
            if bytes(self.uc.mem_read(BASE+lo,len(before)))!=before:fail(f'{self.name}: original code changed in {lo:#x}..{hi:#x}')
        self.uc.mem_write(self.data_va,self.original_data)
        if bytes(self.uc.mem_read(self.data_va,self.data_size))!=self.original_data:fail(f'{self.name}: original .data not restored')
    def read_events(self):return [{'pc_rva':hex(pc),'address':hex(a),'size':n,'value':hex(v)} for k,pc,a,n,v in self.events if k=='read']
    def write_events(self):return [{'pc_rva':hex(pc),'address':hex(a),'size':n,'value':hex(v)} for k,pc,a,n,v in self.events if k=='write']


def one_byte_events(m,addresses):
    return [(k,pc,a,n,v) for k,pc,a,n,v in m.events if a in addresses]


def run_cc50(pe,source,destination,palette_value,tag):
    palette_base=PALETTE+(ROW<<8);palette_entry=palette_base+source
    name=f'cc50_{tag}'
    read_allow={(0xCC71,COLOR_ROW_CELL,4),(0xCC76,0xA909C8,4),(0xCC7C,0xA909B8,4),
        (0xCC98,0xA909F8,4),
        (0xD2C7,0xA90990,4),(0xD2CF,FILL_COUNT,4),(0xD2D8,0xA90994,4),
        (0xD2E1,0xA90990,4),(0xD2EC,0xA9093C,4),(0xD2F5,0xA90938,4),
        (0xD300,0xA90994,4),(0xD30B,FILL_DEST,4),(0xD311,SOURCE_BASE,4),
        (0xD317,COLOR_BASE_CELL,4),(0xD324,0xA90998,4),(0xD331,0xA90978,1)}
    if source!=0:read_allow.add((0xD33D,palette_entry,1))
    dw={(0xCC8C,0xA90968,4):None,(0xCC9D,COLOR_BASE_CELL,4):None}
    m=Machine(pe,name,[(0xCC71,0xCCA3),(0xD2C6,0xD349)],read_allow,dw,[(DEST,DEST+0x100)])
    u=m.uc
    # Row setup runs on original CC71..CCA3 bytes; the pixel slice runs on its
    # original fixed-point setup D2C6..D348 with zero gradients and one pixel.
    put32(u,COLOR_ROW_CELL,ROW);put32(u,0xA909C8,0);put32(u,0xA909B8,0)
    put32(u,0xA90990,0);put32(u,FILL_COUNT,1);put32(u,0xA90994,0)
    put32(u,0xA9093C,0);put32(u,0xA90938,0);put32(u,FILL_DEST,DEST)
    put32(u,SOURCE_BASE,TEXTURE);put32(u,0xA90998,0);put32(u,0xA90978,0)
    u.mem_write(TEXTURE,bytes((source,)));u.mem_write(DEST,bytes((destination,)))
    u.mem_write(palette_entry,bytes((palette_value,)))
    m.before_exec_data=bytes(u.mem_read(m.data_va,m.data_size))
    m.phase(0xCC71,0xCCA3,{UC_X86_REG_EBP:FRAME,UC_X86_REG_ESP:FRAME-4,UC_X86_REG_EFLAGS:0x202})
    m.phase(0xD2C6,0xD349,{UC_X86_REG_EBP:FRAME,UC_X86_REG_ESP:FRAME-4,UC_X86_REG_EFLAGS:0x202})
    # Palette setup stores A90968=0 and A90954=palette_base.
    de=[('write',0xCC8C,0xA90968,4,0),('write',0xCC9D,COLOR_BASE_CELL,4,palette_base)]
    pixel=[e for e in m.events if e[2] in (TEXTURE,TEXTURE+1,DEST,DEST+1,palette_entry)]
    if source==0:expected_pixel=[('read',0xD32E,TEXTURE,1,0)]
    else:expected_pixel=[('read',0xD32E,TEXTURE,1,source),('read',0xD33D,palette_entry,1,palette_value),('write',0xD341,DEST,1,palette_value)]
    if pixel!=expected_pixel:fail(f'{name}: pixel event stream {pixel!r} != {expected_pixel!r}')
    # D2C6 PUSH EBP and D348 POP EBP are stack events; no other stack writes occur.
    expected_all=[(0xCC8C,0xA90968,4,0),(0xCC9D,COLOR_BASE_CELL,4,palette_base),
        (0xD2C6,FRAME-8,4,FRAME)]
    if source!=0:expected_all.append((0xD341,DEST,1,palette_value))
    m.check(de,expected_all,(DEST,bytes((destination if source==0 else palette_value,))))
    return {'case':name,'entry_rva':'0xcc71','setup_stop_before_rva':'0xcca3',
        'loop_entry_rva':'0xd2c6','loop_stop_before_rva':'0xd349',
        'row':ROW,'u':0,'v':0,'width':1,'palette_base':hex(palette_base),'source':hex(source),
        'palette_entry':hex(palette_entry),'palette_value':hex(palette_value),'destination_before':hex(destination),
        'destination_after':hex(u.mem_read(DEST,1)[0]),'zero_source_skipped':source==0,
        'nonzero_palette_zero_was_written':source!=0 and palette_value==0,
        'pixel_events':[{'kind':e[0],'pc_rva':hex(e[1]),'address':hex(e[2]),'size':e[3],'value':hex(e[4])} for e in expected_pixel],
        'reads':m.read_events(),'writes':m.write_events(),'code_unchanged':True,'data_restored':True}


def run_c2e0(pe):
    name='c2e0_zero_fill_unaligned_9'
    start=DEST+1;count=9
    reads={(0xC63F,FILL_COUNT,4),(0xC645,FILL_COLOR,1),(0xC64E,FILL_DEST,4)}
    m=Machine(pe,name,[(0xC63F,0xC681)],reads,{},[(start,start+count)])
    u=m.uc;before=bytes((0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8))
    u.mem_write(start,before);put32(u,FILL_COUNT,count);u.mem_write(FILL_COLOR,b'\0');put32(u,FILL_DEST,start)
    m.before_exec_data=bytes(u.mem_read(m.data_va,m.data_size))
    m.phase(0xC63F,0xC681,{UC_X86_REG_EFLAGS:0x202,UC_X86_REG_ESP:FRAME-4,UC_X86_REG_EBP:FRAME})
    expected=[(0xC671,start+i,1,0) for i in range(3)]+[(0xC678,start+3,4,0)]+[(0xC67F,start+7+i,1,0) for i in range(2)]
    got=[(pc,a,n,v) for k,pc,a,n,v in m.events if k=='write']
    if got!=expected:fail(f'{name}: REP write stream {got!r} != {expected!r}')
    if bytes(u.mem_read(start,count))!=b'\0'*count:fail(f'{name}: zero fill output mismatch')
    m.check([],expected,(start,b'\0'*count))
    return {'case':name,'entry_rva':'0xc63f','stop_before_rva':'0xc681','fill_byte':'0x00',
        'destination':hex(start),'destination_alignment':start&3,'count':count,
        'expected_rep_runs':[{'pc_rva':'0xc671','bytes':3},{'pc_rva':'0xc678','dwords':1},{'pc_rva':'0xc67f','bytes':2}],
        'destination_after':'00'*count,'write_events':m.write_events(),'reads':m.read_events(),
        'code_unchanged':True,'data_restored':True}


def run_c8f0(pe,selector):
    name=f'c8f0_blend_selector_{selector}'
    blend_base=BLEND+selector*0x10000;blend_index=(SOURCE[1]<<8)|OLD_DEST[1];entry=blend_base+blend_index
    decoy=blend_base+(SHADING_RESULT<<8)+OLD_DEST[1]
    reads={(0xCA08,TPage,4),(0xCA2D,BLEND_CELL,4),(0xCA35,SOURCE_BASE,4),
        (0xCA3B,FRAME-4,4),(0xCA3E,WIDTH,2),(0xCA45,DEST_CELL,4)}
    reads.add((0xCA5B,entry,1))
    writes={(0xCA12,SOURCE_BASE,4):None,(0xCA23,BLEND_CELL,4):None}
    m=Machine(pe,name,[(0xCA08,0xCA64)],reads,writes,[(DEST,DEST+2)])
    u=m.uc;result=0xD0+selector
    put32(u,TPage,selector);put32(u,SOURCE_BASE,TEXTURE);put32(u,DEST_CELL,DEST);put16(u,WIDTH,2)
    u.mem_write(TEXTURE,SOURCE);u.mem_write(DEST,OLD_DEST)
    u.mem_write(entry,bytes((result,)));u.mem_write(decoy,b'\x19')
    put32(u,FRAME-4,0)
    m.before_exec_data=bytes(u.mem_read(m.data_va,m.data_size))
    m.phase(0xCA08,0xCA64,{UC_X86_REG_ESI:TEXTURE,UC_X86_REG_ECX:0,UC_X86_REG_EDX:0,
        UC_X86_REG_EBP:FRAME,UC_X86_REG_ESP:FRAME-4,UC_X86_REG_EFLAGS:0x202})
    expect_pixel=[('read',0xCA51,TEXTURE,1,0),('read',0xCA51,TEXTURE+1,1,7),
        ('read',0xCA58,DEST+1,1,OLD_DEST[1]),('read',0xCA5B,entry,1,result),
        ('write',0xCA5E,DEST+1,1,result)]
    pixel=[e for e in m.events if e[2] in (TEXTURE,TEXTURE+1,DEST,DEST+1,entry,decoy)]
    if pixel!=expect_pixel:fail(f'{name}: pixel stream {pixel!r} != {expect_pixel!r}')
    de=[('write',0xCA12,SOURCE_BASE,4,TEXTURE),('write',0xCA23,BLEND_CELL,4,blend_base)]
    all_w=[(event[1],event[2],event[3],event[4]) for event in de]
    all_w.extend([(0xCA28,FRAME-4,4,0),(0xCA5E,DEST+1,1,result)])
    m.check(de,all_w,(DEST,bytes((OLD_DEST[0],result))))
    return {'case':name,'entry_rva':'0xca08','stop_before_rva':'0xca64',
        'u':0,'v':0,'width':2,'selector':selector,'blend_base':hex(blend_base),'blend_index':hex(blend_index),
        'blend_entry_read':hex(entry),'source':[hex(x) for x in SOURCE],'destination_before':[hex(x) for x in OLD_DEST],
        'destination_after':[hex(x) for x in u.mem_read(DEST,2)],'zero_source_skipped':True,
        'pixel_events':[{'kind':e[0],'pc_rva':hex(e[1]),'address':hex(e[2]),'size':e[3],'value':hex(e[4])} for e in expect_pixel],
        'reads':m.read_events(),'writes':m.write_events(),'code_unchanged':True,'data_restored':True}


def run_cab0(pe):
    name='cab0_direct_copy_zero_and_nonzero'
    reads={(0xCBD8,SOURCE_BASE,4),(0xCBDE,FRAME-4,4),(0xCBE1,WIDTH,2),(0xCBE8,DEST_CELL,4)}
    m=Machine(pe,name,[(0xCBD6,0xCC01)],reads,{},[(DEST,DEST+2)])
    u=m.uc;put32(u,SOURCE_BASE,TEXTURE);put32(u,DEST_CELL,DEST);put16(u,WIDTH,2);put32(u,FRAME-4,0)
    u.mem_write(TEXTURE,SOURCE);u.mem_write(DEST,OLD_DEST)
    m.before_exec_data=bytes(u.mem_read(m.data_va,m.data_size))
    m.phase(0xCBD6,0xCC01,{UC_X86_REG_EBP:FRAME,UC_X86_REG_ESP:FRAME-4,UC_X86_REG_EFLAGS:0x202})
    expected_pixel=[('read',0xCBF4,TEXTURE,1,0),('read',0xCBF4,TEXTURE+1,1,7),('write',0xCBFB,DEST+1,1,7)]
    pixel=[e for e in m.events if e[2] in (TEXTURE,TEXTURE+1,DEST,DEST+1)]
    if pixel!=expected_pixel:fail(f'{name}: pixel stream {pixel!r} != {expected_pixel!r}')
    m.check([],[(0xCBFB,DEST+1,1,7)],(DEST,bytes((OLD_DEST[0],7))))
    return {'case':name,'entry_rva':'0xcbd6','stop_before_rva':'0xcc01','u':0,'v':0,'width':2,'source':[hex(x) for x in SOURCE],
        'destination_before':[hex(x) for x in OLD_DEST],'destination_after':[hex(x) for x in u.mem_read(DEST,2)],
        'zero_source_skipped':True,'pixel_events':[{'kind':e[0],'pc_rva':hex(e[1]),'address':hex(e[2]),'size':e[3],'value':hex(e[4])} for e in expected_pixel],
        'reads':m.read_events(),'writes':m.write_events(),'code_unchanged':True,'data_restored':True}


def main():
    pe=PE(ARGS.exe)
    if sha(pe.data)!=EXE_SHA or pe.image_base!=BASE:fail('pinned EXE identity/base mismatch')
    if getattr(pe.pe,'DIRECTORY_ENTRY_BASERELOC',None):fail('unexpected base relocation table')
    runtime=verify_runtime(unicorn,ARGS.deps)
    cases=[]
    cases.append(run_cc50(pe,0,0x2B,0xE4,'zero_source'))
    cases.append(run_cc50(pe,7,0xA5,0,'nonzero_palette_zero'))
    cases.append(run_c2e0(pe))
    cases.extend(run_c8f0(pe,s) for s in range(4))
    cases.append(run_cab0(pe))
    r={'schema_version':1,'status':'BOUNDED_EQUIVALENT',
       'scope':'Finite CPU-only fixtures execute original HERCULES.EXE bytes for the active CC50 palette setup/loop, C2E0 raw fill, C8F0 blend setup/loop, and CAB0 copy setup/loop. No full polygon/sprite rasterization, clipping, GPU, DirectDraw, gameplay, source identity, FUNCTION_MATCH, or whole-module equality is claimed.',
       'oracle':{'path':str(pe.path),'sha256':sha(pe.data),'image_base':hex(pe.image_base),
         'base_relocations':0,'spans':[{'start_rva':hex(lo),'stop_before_rva':hex(hi),'size':hi-lo,
           'sha256':sha(pe.read_rva(lo,hi-lo))} for lo,hi in [(0xCC71,0xCCA3),(0xD2C6,0xD349),(0xC63F,0xC681),(0xCA08,0xCA64),(0xCBD6,0xCC01)]]},
       'execution':{'engine':'Unicorn CPU emulator','unicorn_version':unicorn.__version__,'architecture':'x86 32-bit',
         'python_executable':sys.executable,'python_version':sys.version.split()[0],
         'test_script_sha256':sha(Path(__file__).read_bytes()),
         'all_original_sections_mapped_at_preferred_base':True,'instruction_budget_per_phase':MAX_INSNS,
         'no_original_code_written':True,'unknown_pc_memory_or_os_endpoints_fail':True,
         'table_entries_and_surface_bytes_are_synthetic_in_emulator_only':True,
         'data_section_mutations_checked_and_restored_each_case':True},
       'cases':cases,'limits':['CC50 tests one source sample at zero U/V and row 3 after actual CC71 palette-base setup; it does not exercise geometry/clipping or raster interpolation across pixels.','C2E0 executes the raw byte-fill loop for one unaligned nine-byte zero fill to cover REP byte/dword/tail paths; it does not test caller-side D420 selection.','C8F0 covers four finite selectors for a two-byte row; it does not execute sprite clipping or entry setup.','CAB0 covers the two-byte direct-copy loop only; it does not execute sprite clipping or entry setup.','No GPU, DirectDraw, gameplay, source identity, FUNCTION_MATCH, or whole-module result is claimed.'],
       'invocation':{'exe':str(ARGS.exe),'original_root':str(ARGS.original_root),'deps':str(ARGS.deps),'analysis_deps':str(ARGS.analysis_deps),'receipt':str(ARGS.receipt)}}
    r['execution'].update(runtime);write_receipt(ARGS.receipt,r)
    print(json.dumps({'status':r['status'],'exe_sha256':r['oracle']['sha256'],
       'cases':[{'case':c['case'],'destination_after':c.get('destination_after'),'zero_source_skipped':c.get('zero_source_skipped'),'blend_base':c.get('blend_base'),'palette_base':c.get('palette_base')} for c in cases],
       'receipt':str(ARGS.receipt)},indent=2))
if __name__=='__main__':main()
