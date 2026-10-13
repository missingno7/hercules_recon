"""Finite CPU-only execution oracle for original HERCULES.EXE C700/C882 bytes."""
from __future__ import annotations
import hashlib, json, struct
from pathlib import Path
import sys

from oracle_common import parse_args, verify_runtime, write_receipt
ARGS = parse_args("Run finite original HERCULES.EXE C700/C882 pixel-loop fixtures", "c882_report.json")
from pe import PE
import unicorn
from unicorn import (Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_INVALID,
    UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MODE_32, UC_PROT_EXEC, UC_PROT_READ, UC_PROT_WRITE)
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX,
    UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EFLAGS, UC_X86_REG_EIP, UC_X86_REG_ESP)

BASE, START, STOP = 0x400000, 0xC7F6, 0xC89F
TEXTURE, DEST, STACK, STACK_SIZE = 0x10000000, 0x10002000, 0x20000000, 0x10000
FRAME = STACK + 0x8000
MAX_INSNS = 10000
EXE_SHA = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
SURFACE, STRIDE = 0xBA0210, 0xBA0214
TPAGE, COLOR, SOURCE_ROW, SOURCE_BASE = 0xA8E6F8, 0xA90A00, 0xA909D8, 0xA909FC
DEST_CELL, BLEND_CELL, COLOR_LUT_CELL, WIDTH = 0xA90944, 0xA90948, 0xA909AC, 0xA90A0C
COLOR_LUT, BLEND_FAMILY = 0x91C488, 0x92E3D0
PIXEL_COLOR, SOURCE_V, WIDTH_WORD = 0x23, 0, 2
SOURCE_BYTES, DEST_BYTES = bytes((0, 7)), bytes((0x35, 0x3C))
LUT_RESULT = 0xA6


def sha(b): return hashlib.sha256(b).hexdigest()
def fail(msg): raise AssertionError(msg)
def align(v): return (v + 0xFFF) & ~0xFFF
def perms(c):
    p=0
    if c & 0x40000000: p |= UC_PROT_READ
    if c & 0x80000000: p |= UC_PROT_WRITE
    if c & 0x20000000: p |= UC_PROT_EXEC
    return p

def map_pe(uc, pe):
    hs=pe.pe.OPTIONAL_HEADER.SizeOfHeaders
    uc.mem_map(BASE, align(hs), UC_PROT_READ); uc.mem_write(BASE, pe.data[:hs])
    for s in pe.sections:
        size=align(max(s['virtual_size'],s['raw_size'])); va=BASE+s['virtual_address']
        uc.mem_map(va,size,perms(s['characteristics']))
        if s['raw_size']: uc.mem_write(va,pe.read_rva(s['virtual_address'],s['raw_size']))
        if size>s['raw_size']: uc.mem_write(va+s['raw_size'],b'\0'*(size-s['raw_size']))

def data_range(pe):
    s=next(s for s in pe.sections if s['name']=='.data')
    return BASE+s['virtual_address'],align(max(s['virtual_size'],s['raw_size']))

def run_case(pe, selector):
    color_entry=COLOR_LUT+PIXEL_COLOR*256+SOURCE_BYTES[1]
    blend_index=(LUT_RESULT<<8)|DEST_BYTES[1]
    blend_entry=BLEND_FAMILY+selector*0x10000+blend_index
    wrong_index=(SOURCE_BYTES[1]<<8)|DEST_BYTES[1]
    wrong_entry=BLEND_FAMILY+selector*0x10000+wrong_index
    if wrong_entry==blend_entry: fail('blend-index discriminator aliases expected entry')
    uc=Uc(UC_ARCH_X86,UC_MODE_32); map_pe(uc,pe)
    uc.mem_map(TEXTURE,0x4000,UC_PROT_READ|UC_PROT_WRITE)
    uc.mem_map(STACK,STACK_SIZE,UC_PROT_READ|UC_PROT_WRITE)
    data_va,data_size=data_range(pe); original_data=bytes(uc.mem_read(data_va,data_size))
    # Synthetic native inputs and LUT entries are overlaid only in this emulator.
    for address,value in ((SURFACE,DEST),(STRIDE,8),(TPAGE,selector),(COLOR,PIXEL_COLOR),
                          (SOURCE_ROW,SOURCE_V),(SOURCE_BASE,TEXTURE)):
        uc.mem_write(address,struct.pack('<I',value))
    uc.mem_write(WIDTH,struct.pack('<H',WIDTH_WORD))
    uc.mem_write(TEXTURE,SOURCE_BYTES); uc.mem_write(DEST,DEST_BYTES)
    uc.mem_write(color_entry,bytes((LUT_RESULT,)))
    blend_entries=[]
    for s in range(4):
        a=BLEND_FAMILY+s*0x10000+blend_index
        uc.mem_write(a,bytes((0xD0+s,))); blend_entries.append(a)
    uc.mem_write(wrong_entry,b'\x19')
    if wrong_entry not in blend_entries: blend_entries.append(wrong_entry)
    before=bytes(uc.mem_read(data_va,data_size))
    code=bytes(uc.mem_read(BASE+START,STOP-START))
    trace=[]; events=[]; err={'kind':None,'detail':None}; stop={'pc':None}
    data_end=data_va+data_size
    scratch=((TEXTURE,TEXTURE+0x4000),(STACK,STACK+STACK_SIZE))
    write_cells={DEST_CELL:BASE+0xC80D,BLEND_CELL:BASE+0xC831,
                 COLOR_LUT_CELL:BASE+0xC83B,SOURCE_BASE:BASE+0xC84C}
    read_cells={BASE+0xC7F6:(STRIDE,4),BASE+0xC7FD:(SURFACE,4),
        BASE+0xC805:(COLOR,4),BASE+0xC812:(TPAGE,4),BASE+0xC836:(SOURCE_ROW,4),
        BASE+0xC841:(SOURCE_BASE,4),BASE+0xC858:(BLEND_CELL,4),
        BASE+0xC860:(SOURCE_BASE,4),BASE+0xC869:(WIDTH,2),
        BASE+0xC870:(DEST_CELL,4),BASE+0xC87A:(COLOR_LUT_CELL,4)}
    def in_scratch(a,n): return any(lo<=a and a+n<=hi for lo,hi in scratch)
    def on_code(m,a,size,_):
        if a==BASE+STOP: stop['pc']=a; m.emu_stop(); return
        if not BASE+START<=a<BASE+STOP:
            err.update(kind='unexpected_code',detail=hex(a)); m.emu_stop(); return
        trace.append(a-BASE)
    def on_read(m,access,a,n,value,_):
        pc=m.reg_read(UC_X86_REG_EIP); raw=bytes(m.mem_read(a,n))
        events.append(('read',pc-BASE,a,n,int.from_bytes(raw,'little')))
        if data_va<=a and a+n<=data_end:
            expected=read_cells.get(pc)
            table_read=(pc,a,n) in ((BASE+0xC88E,color_entry,1),(BASE+0xC895,blend_entry,1))
            if expected!=(a,n) and not table_read: err.update(kind='unexpected_data_read',detail=(hex(pc),hex(a),n)); m.emu_stop()
        elif not in_scratch(a,n): err.update(kind='unexpected_read',detail=(hex(pc),hex(a),n)); m.emu_stop()
    def on_write(m,access,a,n,value,_):
        pc=m.reg_read(UC_X86_REG_EIP); v=value&((1<<(n*8))-1)
        events.append(('write',pc-BASE,a,n,v))
        if data_va<=a and a+n<=data_end:
            if write_cells.get(a)!=pc: err.update(kind='unexpected_data_write',detail=(hex(pc),hex(a),n)); m.emu_stop()
        elif not in_scratch(a,n): err.update(kind='unexpected_write',detail=(hex(pc),hex(a),n)); m.emu_stop()
    def on_invalid(m,access,a,n,value,_):
        err.update(kind='invalid_memory',detail=(hex(m.reg_read(UC_X86_REG_EIP)),access,hex(a),n)); m.emu_stop(); return False
    uc.hook_add(UC_HOOK_CODE,on_code); uc.hook_add(UC_HOOK_MEM_READ,on_read)
    uc.hook_add(UC_HOOK_MEM_WRITE,on_write); uc.hook_add(UC_HOOK_MEM_INVALID,on_invalid)
    uc.mem_write(FRAME-4,b'\0'*4)
    for reg,val in ((UC_X86_REG_EAX,0),(UC_X86_REG_EBX,0),(UC_X86_REG_ECX,0),
                    (UC_X86_REG_EDX,0),(UC_X86_REG_EBP,FRAME),(UC_X86_REG_ESP,FRAME-4),
                    (UC_X86_REG_EFLAGS,0x202),(UC_X86_REG_EIP,BASE+START)):
        uc.reg_write(reg,val)
    try: uc.emu_start(BASE+START,0,timeout=1_000_000,count=MAX_INSNS)
    except UcError as e:
        if err['kind'] is None: err.update(kind='unicorn_error',detail=str(e))
    if err['kind'] is not None: fail(f'selector {selector}: {err}')
    if stop['pc']!=BASE+STOP: fail(f'selector {selector}: missed C89F boundary')
    if bytes(uc.mem_read(BASE+START,len(code)))!=code: fail('original instructions changed')
    blend_base=BLEND_FAMILY+selector*0x10000
    color_base=COLOR_LUT+PIXEL_COLOR*256
    expected_reads=[(0xC7F6,STRIDE,4,8),(0xC7FD,SURFACE,4,DEST),(0xC805,COLOR,4,PIXEL_COLOR),
        (0xC812,TPAGE,4,selector),(0xC836,SOURCE_ROW,4,SOURCE_V),(0xC841,SOURCE_BASE,4,TEXTURE),
        (0xC858,BLEND_CELL,4,blend_base),(0xC860,SOURCE_BASE,4,TEXTURE),
        (0xC866,FRAME-4,4,SOURCE_V<<8),(0xC869,WIDTH,2,WIDTH_WORD),
        (0xC870,DEST_CELL,4,DEST),(0xC87A,COLOR_LUT_CELL,4,color_base),
        (0xC882,TEXTURE,1,SOURCE_BYTES[0]),(0xC882,TEXTURE+1,1,SOURCE_BYTES[1]),
        (0xC88E,color_entry,1,LUT_RESULT),(0xC892,DEST+1,1,DEST_BYTES[1]),
        (0xC895,blend_entry,1,0xD0+selector),(0xC89E,FRAME-8,4,FRAME)]
    actual_reads=[(e[1],e[2],e[3],e[4]) for e in events if e[0]=='read']
    if actual_reads!=expected_reads: fail(f'selector {selector}: original read stream {actual_reads!r} != {expected_reads!r}')
    exp_pixel=[('read',0xC882,TEXTURE,1,0),('read',0xC882,TEXTURE+1,1,7),
        ('read',0xC88E,color_entry,1,LUT_RESULT),('read',0xC892,DEST+1,1,DEST_BYTES[1]),
        ('read',0xC895,blend_entry,1,0xD0+selector),
        ('write',0xC898,DEST+1,1,0xD0+selector)]
    pixel_addresses={TEXTURE,TEXTURE+1,DEST,DEST+1,color_entry,blend_entry,wrong_entry}
    actual_pixel=[e for e in events if e[2] in pixel_addresses]
    if actual_pixel!=exp_pixel: fail(f'selector {selector}: pixel access stream {actual_pixel!r} != {exp_pixel!r}')
    if bytes(uc.mem_read(DEST,2))!=bytes((DEST_BYTES[0],0xD0+selector)):
        fail(f'selector {selector}: zero texel/destination result mismatch')
    expected_writes=[(0xC80D,DEST_CELL,4,DEST),(0xC831,BLEND_CELL,4,blend_base),
        (0xC83B,COLOR_LUT_CELL,4,color_base),(0xC84C,SOURCE_BASE,4,TEXTURE),
        (0xC852,FRAME-4,4,0),(0xC855,FRAME-8,4,FRAME),(0xC898,DEST+1,1,0xD0+selector)]
    actual_writes=[(e[1],e[2],e[3],e[4]) for e in events if e[0]=='write']
    if actual_writes!=expected_writes: fail(f'selector {selector}: write stream {actual_writes!r} != {expected_writes!r}')
    expected_data=bytearray(before)
    for a,v in ((DEST_CELL,DEST),(BLEND_CELL,blend_base),(COLOR_LUT_CELL,color_base),(SOURCE_BASE,TEXTURE)):
        expected_data[a-data_va:a-data_va+4]=struct.pack('<I',v)
    after=bytes(uc.mem_read(data_va,data_size))
    if after!=bytes(expected_data): fail(f'selector {selector}: unexplained .data mutation')
    reads=[{'pc_rva':hex(e[1]),'address':hex(e[2]),'size':e[3],'value':hex(e[4])} for e in events if e[0]=='read']
    writes=[{'pc_rva':hex(e[0]),'address':hex(e[1]),'size':e[2],'value':hex(e[3])} for e in actual_writes]
    uc.mem_write(data_va,original_data)
    if bytes(uc.mem_read(data_va,data_size))!=original_data: fail('failed to restore original .data')
    return {'selector':selector,'blend_base':hex(blend_base),'blend_entry_index':hex(blend_index),
        'blend_entry_read':hex(blend_entry),'color':hex(PIXEL_COLOR),'color_lut_entry_read':hex(color_entry),
        'color_lut_result':hex(LUT_RESULT),'source':[hex(x) for x in SOURCE_BYTES],
        'destination_before':[hex(x) for x in DEST_BYTES],
        'destination_after':[hex(x) for x in uc.mem_read(DEST,2)],'zero_texel_skipped':True,
        'pixel_access_events':[{'kind':e[0],'pc_rva':hex(e[1]),'address':hex(e[2]),'size':e[3],'value':hex(e[4])} for e in exp_pixel],
        'all_original_writes_accounted_for':True,'original_data_restored':True,
        'code_unchanged':True,'instruction_count':len(trace),'reads':reads,'writes':writes}

def main():
    pe=PE(ARGS.exe)
    if sha(pe.data)!=EXE_SHA or pe.image_base!=BASE: fail('pinned original identity/base mismatch')
    if getattr(pe.pe,'DIRECTORY_ENTRY_BASERELOC',None): fail('unexpected relocation table')
    cases=[run_case(pe,s) for s in range(4)]
    r={'schema_version':1,'status':'BOUNDED_EQUIVALENT',
       'scope':'Finite one-row, two-pixel C700/C882 execution from original RVA C7F6 through the C89F inner-loop boundary. Source bytes 0 and 7; zero bypasses all destination/LUT access, nonzero uses frame_color_lut[0x23*256+7], then blend[(mapped_color<<8)|old_destination]. This is not full rasterization, renderer integration, gameplay, FUNCTION_MATCH, or whole-module equality.',
       'oracle':{'path':str(pe.path),'sha256':sha(pe.data),'image_base':hex(pe.image_base),'entry_rva':hex(START),'stop_before_rva':hex(STOP),'span_size':STOP-START,'span_sha256':sha(pe.read_rva(START,STOP-START)),'base_relocations':0},
       'execution':{'engine':'Unicorn CPU emulator','unicorn_version':unicorn.__version__,'architecture':'x86 32-bit','original_sections_mapped_at_preferred_base':True,'instruction_budget_per_case':MAX_INSNS,'entry_registers':{'eax_clipped_y':0,'ecx_clipped_x':0,'edx_source_v':0,'ebp':hex(FRAME),'esp':hex(FRAME-4)},'synthetic_texture':{'address':hex(TEXTURE),'bytes':[hex(x) for x in SOURCE_BYTES]},'synthetic_destination':{'address':hex(DEST),'stride':8,'bytes':[hex(x) for x in DEST_BYTES]},'synthetic_color_lut_entry':hex(COLOR_LUT+PIXEL_COLOR*256+SOURCE_BYTES[1]),'blend_index_equation':'(color_lut_result << 8) | old_destination_byte','blend_family_base':hex(BLEND_FAMILY),'all_original_sections_true_base':True,'no_code_bytes_written':True,'unknown_pc_or_memory_fails':True,'fixture_data_restored':True},
       'cases':cases,'limitations':['Only finite two-pixel rows were executed; clipping and complete raster paths were not exercised.','This covers the C700 color-LUT plus blend path only; D420/CC50 palette CLUT and other copy/blend helpers remain separate.','No GPU, DirectDraw, gameplay, source identity, FUNCTION_MATCH, or whole-module result is claimed.'],
       'invocation':{'exe':str(ARGS.exe),'original_root':str(ARGS.original_root),'unicorn_deps':str(ARGS.deps),'analysis_deps':str(ARGS.analysis_deps),'receipt':str(ARGS.receipt)}}
    r['execution'].update(verify_runtime(unicorn)); write_receipt(ARGS.receipt,r)
    print(json.dumps({'status':r['status'],'exe_sha256':r['oracle']['sha256'],'span_sha256':r['oracle']['span_sha256'],'cases':[{'selector':c['selector'],'blend_base':c['blend_base'],'blend_entry':c['blend_entry_read'],'destination_after':c['destination_after'],'zero_skipped':c['zero_texel_skipped']} for c in cases],'receipt':str(ARGS.receipt)},indent=2))
if __name__=='__main__': main()