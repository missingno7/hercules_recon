from __future__ import annotations
import argparse, hashlib, json, struct, sys
from pathlib import Path
DEFAULT_ROOT = Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description='Read-only pinned PE texture-bank ownership and alias audit')
parser.add_argument('--original-root',type=Path,default=DEFAULT_ROOT,
    help='repository root containing scripts/pe.py and renderer_contract/evidence')
parser.add_argument('--exe',type=Path,required=True,help='pinned original HERCULES.EXE')
parser.add_argument('--output',type=Path,required=True,
    help='private JSON output under checkout/work')
args=parser.parse_args()
ROOT=args.original_root.resolve()
EXE=args.exe.resolve()
OUT=args.output.resolve()
try:
    OUT.relative_to((ROOT/'work').resolve())
except ValueError as error:
    raise SystemExit('--output must stay under checkout/work') from error
if OUT==EXE:
    raise SystemExit('--output must not overwrite --exe')
sys.path.insert(0,str(ROOT/'scripts'))
import env  # adds the project's existing local analysis dependency directory
from pe import PE
import capstone
from capstone.x86 import X86_OP_IMM, X86_OP_MEM

EXE_SHA = '587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
ORD = 0x96E6E0
ALT = 0xA6E6E0
BANK = 0x100000
TARGETS = {
    0x96E3E0: 'D420 palette gate',
    0x96E424: 'nearby renderer global (unresolved here)',
    0x96E680: 'nearby renderer global (unresolved here)',
    0x96E698: 'optional color-update flag',
    0x4361B8: 'host callback source table',
    0x4365A0: 'pointer cell aliasing host callback source table',
    0xA8E6F4: 'current texture page value',
    0xA8E6F8: 'blend selector',
    0xA8E6FC: 'alternate-bank boolean',
    0xA8E780: 'BD80 notification callback pointer',
    0xA8E788: 'BD80 notification gate',
    0xA8E918: 'reciprocal table start',
    0xA909FC: 'current selected texture-page pointer / sprite U cursor',
}

def sha(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()

def hx(n): return f'0x{n:08x}'

def section_for_va(pe, va):
    rva=va-pe.image_base
    for s in pe.sections:
        if s['virtual_address'] <= rva < s['virtual_address'] + s['virtual_size']:
            offset=rva-s['virtual_address']
            return {'name':s['name'],'rva':hx(rva),'section_offset':hx(offset),
                    'file_backed':offset < s['raw_size'],
                    'file_offset':hx(s['raw_offset']+offset) if offset < s['raw_size'] else None,
                    'zero_fill':offset >= s['raw_size']}
    return None

pe=PE(EXE)
if sha(pe.data)!=EXE_SHA: raise SystemExit('unexpected EXE hash')
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
md.detail=True; md.skipdata=True
textsec=next(s for s in pe.sections if s['name']=='.text')
insns=list(md.disasm(pe.read_rva(textsec['virtual_address'],textsec['raw_size']),pe.image_base+textsec['virtual_address']))

base_imm=[]; bank_imm=[]; direct_bank_mem=[]; global_refs={hx(a):[] for a in TARGETS}
callback_table_code_refs=[]
for ins in insns:
    if ins.id == 0:
        continue
    row={'rva':hx(ins.address-pe.image_base),'instruction':f'{ins.mnemonic} {ins.op_str}'}
    for oi,op in enumerate(ins.operands):
        if op.type==X86_OP_IMM:
            v=op.imm & 0xffffffff
            if 0x4361B8 <= v <= 0x4365A0:
                callback_table_code_refs.append({**row,'value':hx(v),
                    'kind':'source_table_or_pointer_cell_address'})
            if v in (ORD,ALT):
                base_imm.append({**row,'value':hx(v)})
            if ORD <= v < ORD+BANK or ALT <= v < ALT+BANK:
                bank_imm.append({**row,'value':hx(v)})
        elif op.type==X86_OP_MEM:
            d=op.mem.disp & 0xffffffff
            if 0x4361B8 <= d <= 0x4365A0:
                callback_table_code_refs.append({**row,'operand_index':oi,
                    'value':hx(d),'kind':'source_table_or_pointer_cell_memory_displacement',
                    'base_reg':ins.reg_name(op.mem.base) if op.mem.base else None,
                    'index_reg':ins.reg_name(op.mem.index) if op.mem.index else None,
                    'access':op.access})
            if ORD <= d < ORD+BANK or ALT <= d < ALT+BANK:
                direct_bank_mem.append({**row,'operand_index':oi,'memory_disp':hx(d),
                    'base_reg':ins.reg_name(op.mem.base) if op.mem.base else None,
                    'index_reg':ins.reg_name(op.mem.index) if op.mem.index else None,
                    'scale':op.mem.scale,'access':op.access})
            for addr in TARGETS:
                if d==addr:
                    global_refs[hx(addr)].append({**row,'operand_index':oi,
                        'base_reg':ins.reg_name(op.mem.base) if op.mem.base else None,
                        'index_reg':ins.reg_name(op.mem.index) if op.mem.index else None,
                        'access':op.access})

# Search all file-backed non-.text bytes for DWORD values numerically in either
# candidate page interval, or equal to a bank base. Numeric range membership alone
# does not make a value a pointer. Scan every byte offset to include unaligned values.
data_pointer_literals=[]
for s in pe.sections:
    if s['name']=='.text': continue
    raw=pe.read_rva(s['virtual_address'],s['raw_size'])
    for off in range(0,max(0,len(raw)-3)):
        val=struct.unpack_from('<I',raw,off)[0]
        if ORD <= val < ORD+BANK or ALT <= val < ALT+BANK:
            data_pointer_literals.append({'section':s['name'],'rva':hx(s['virtual_address']+off),
                                          'file_offset':hx(s['raw_offset']+off),'value':hx(val),
                                          'words16':[hx(val&0xffff),hx(val>>16)]})

# Search the same raw bytes for exact pointer-valued candidates into the callback
# source table or its adjacent pointer cell. These are candidates, not proven aliases.
callback_table_data_candidates=[]
for s in pe.sections:
    if s['name']=='.text': continue
    raw=pe.read_rva(s['virtual_address'],s['raw_size'])
    for off in range(0,max(0,len(raw)-3)):
        val=struct.unpack_from('<I',raw,off)[0]
        if 0x4361B8 <= val <= 0x4365A0:
            callback_table_data_candidates.append({'section':s['name'],
                'rva':hx(s['virtual_address']+off),'file_offset':hx(s['raw_offset']+off),
                'value':hx(val),'aligned_to_dword':off%4==0})

callback_ref_groups={}
for x in callback_table_code_refs:
    key=(x['kind'],x['value'])
    dst=callback_ref_groups.setdefault(key,{'kind':x['kind'],'value':x['value'],
        'count':0,'sites':[],'instruction_forms':{}})
    dst['count']+=1; dst['sites'].append(x['rva'])
    dst['instruction_forms'][x['instruction']]=dst['instruction_forms'].get(x['instruction'],0)+1
callback_ref_summary=[]
for key in sorted(callback_ref_groups,key=lambda x:(x[0],int(x[1],16))):
    x=callback_ref_groups[key]
    callback_ref_summary.append({'kind':x['kind'],'value':x['value'],'count':x['count'],
        'example_site_rvas':x['sites'][:4],'instruction_forms':[
            {'instruction':ins,'count':count} for ins,count in sorted(x['instruction_forms'].items())]})

data_candidate_by_section={}
for x in data_pointer_literals:
    dst=data_candidate_by_section.setdefault(x['section'],{'count':0,'examples':[]})
    dst['count']+=1
    if len(dst['examples'])<3: dst['examples'].append(x)

def group_by_value(rows, value_key):
    grouped={}
    for row in rows:
        v=row[value_key]
        dst=grouped.setdefault(v,{'value':v,'count':0,'sites':[]})
        dst['count']+=1
        dst['sites'].append({'rva':row['rva'],'instruction':row['instruction']})
    return [grouped[k] for k in sorted(grouped,key=lambda x:int(x,16))]

mem_by_disp={}
for row in direct_bank_mem:
    dst=mem_by_disp.setdefault(row['memory_disp'],{
        'address_or_base':row['memory_disp'],'count':0,'reads':0,'writes':0,
        'forms':set(),'sites':[]})
    dst['count']+=1
    if row['access'] & 1: dst['reads']+=1
    if row['access'] & 2: dst['writes']+=1
    dst['forms'].add((row['base_reg'],row['index_reg']))
    dst['sites'].append({'rva':row['rva'],'instruction':row['instruction']})
mem_groups=[]
for k in sorted(mem_by_disp,key=lambda x:int(x,16)):
    v=mem_by_disp[k]
    v['forms']=[{'base':a,'index':b} for a,b in sorted(v['forms'],key=lambda x:(str(x[0]),str(x[1])))]
    mem_groups.append(v)
mem_page_summary={}
for row in direct_bank_mem:
    d=int(row['memory_disp'],16)
    if ORD<=d<ORD+BANK:
        bank='ordinary'; page=(d-ORD)//0x10000
    else:
        bank='alternate_index_interval'; page=(d-ALT)//0x10000
    key=f'{bank}_page_{page}'
    dst=mem_page_summary.setdefault(key,{'memory_operand_count':0,'reads':0,'writes':0})
    dst['memory_operand_count']+=1
    if row['access']&1: dst['reads']+=1
    if row['access']&2: dst['writes']+=1

selected_global_refs={}
for k in ('0x0096e3e0','0x0096e424','0x0096e680','0x0096e698',
          '0x004361b8','0x004365a0'):
    rows=global_refs.get(k,[])
    selected_global_refs[k]={'label':TARGETS[int(k,16)],'count':len(rows),
        'sites':[{'rva':x['rva'],'instruction':x['instruction']} for x in rows]}

# Exact source-table/callback pointer map from the original bytes.
table_rva=0x361B8; table_size=1000; table=pe.read_rva(table_rva,table_size)
pointer_rva=0x365A0
callback_slots=[]
for off in (0x204,0x208,0x20c,0x23c):
    v=struct.unpack_from('<I',table,off)[0]
    callback_slots.append({'offset':hx(off),'slot':off//4,'cell_va':hx(pe.image_base+table_rva+off),
                           'target_va':hx(v),'target_rva':hx(v-pe.image_base),
                           'entry_sha256':sha(table[off:off+4])})
pointer_value=struct.unpack('<I',pe.read_rva(pointer_rva,4))[0]
table_values=[struct.unpack_from('<I',table,off)[0] for off in range(0,table_size,4)]
table_value_classes={'null':0,'ordinary_candidate_bank':0,'alternate_candidate_bank':0,
    'inside_text':0,'inside_other_image_section':0,'outside_image':0}
for value in table_values:
    if value==0: cls='null'
    elif ORD<=value<ORD+BANK: cls='ordinary_candidate_bank'
    elif ALT<=value<ALT+BANK: cls='alternate_candidate_bank'
    else:
        loc=section_for_va(pe,value)
        cls='outside_image' if loc is None else ('inside_text' if loc['name']=='.text' else 'inside_other_image_section')
    table_value_classes[cls]+=1

function_spans=[]
for name,lo,hi in (
    ('A710_upload',0xA710,0xA8E5),
    ('A8F0_same_map_copy',0xA8F0,0xABF3),
    ('AC60_packet_consumer_body',0xAC60,0xB2EC),
    ('reciprocal_table_initializer',0x72A0,0x72CC),
):
    function_spans.append({'name':name,'rva':hx(lo),'end_exclusive':hx(hi),
        'size':hi-lo,'sha256':sha(pe.read_rva(lo,hi-lo))})

callers_72a0=[]
for ins in insns:
    if ins.id and ins.mnemonic=='call' and any(
        op.type==X86_OP_IMM and (op.imm & 0xffffffff)==pe.image_base+0x72A0
        for op in ins.operands):
        callers_72a0.append(hx(ins.address-pe.image_base))

evidence_inputs=[]
for rel in (
    'renderer_contract/evidence/textures.json',
    'renderer_contract/evidence/native_upload.json',
    'renderer_contract/evidence/native_vram_move.json',
    'renderer_contract/evidence/native_page_masks.json',
    'renderer_contract/evidence/native_polygon_edges.json',
    'renderer_contract/evidence/sprite_geometry.json',
    'renderer_contract/evidence/closure.json',
    'renderer_contract/evidence/compatibility.json',
):
    f=ROOT/rel
    evidence_inputs.append({'path':rel,'sha256':sha(f.read_bytes())})

sec=[]
for s in pe.sections:
    sec.append({'name':s['name'],'rva':hx(s['virtual_address']),
        'va':hx(pe.image_base+s['virtual_address']),'virtual_size':hx(s['virtual_size']),
        'raw_size':hx(s['raw_size']),'virtual_end_exclusive':hx(pe.image_base+s['virtual_address']+s['virtual_size']),
        'raw_file_offset':hx(s['raw_offset']),
        'raw_end_exclusive_rva':hx(s['virtual_address']+s['raw_size']),
        'characteristics':hx(s['characteristics'])})

ranges={
    'ordinary_pageable_bank':{'start':hx(ORD),'end_exclusive':hx(ORD+BANK),'size':hx(BANK),
    'page_size':hx(0x10000),'pages':16,'section_start':section_for_va(pe,ORD),
    'section_last_byte':section_for_va(pe,ORD+BANK-1)},
 'alternate_fixed_page0_uv_window':{'start':hx(ALT),'end_exclusive':hx(ALT+0x10000),'size':hx(0x10000),
    'page_size':hx(0x10000),'selected_page_index':0,
    'section_start':section_for_va(pe,ALT),'section_last_byte':section_for_va(pe,ALT+0xffff)},
 'alternate_index_derived_interval_not_fully_selected':{'start':hx(ALT),'end_exclusive':hx(ALT+BANK),'size':hx(BANK),
    'distance_from_ordinary':hx(ALT-ORD),'page_size':hx(0x10000),'pages':16,
    'selection_note':'AC60 alternate branch writes the fixed base 0xA6E6E0 after computing ordinary page offset; it does not add the page index in that branch.',
    'section_start':section_for_va(pe,ALT),'section_last_byte':section_for_va(pe,ALT+BANK-1)},
 'alternate_high_y_a710_a8f0_write_envelope':{'start':hx(ALT),'end_exclusive':hx(ALT+0x90000),'size':hx(0x90000),
    'covers_alternate_page_indices':[0,8],'proof_level':'STATIC_EXPRESSION; y>=512 excluded from bounded upload/move oracle'},
 'alternate_page2_address_interval_aliases_host_state':{'start':hx(ALT+0x20000),'end_exclusive':hx(ALT+0x30000),'size':hx(0x10000),
    'page_index':2,'reciprocal_table':{'start':'0x00a8e918','end_exclusive':'0x00a90918','size':'0x2000',
        'offset_in_page':'0x238','entirely_inside_page':True},
    'texture_state_cells':['0x00a8e6f4','0x00a8e6f8','0x00a8e6fc'],
    'rasterizer_scratch_observed':'0x00a9091c..0x00a90a1a and other explicit host globals also lie in this page'},
 'reciprocal_table':{'start':hx(0xA8E918),'end_exclusive':hx(0xA8E918+0x2000),'size':hx(0x2000),
    'section_start':section_for_va(pe,0xA8E918),'section_last_byte':section_for_va(pe,0xA8E918+0x1fff)},
 'texture_state':{hx(a):{'label':label,'section':section_for_va(pe,a)} for a,label in TARGETS.items()},
}
report={
 'schema_version':1,
 'scope':'Read-only PE layout and full .text explicit-reference scan for texture address candidates; no allocation or game-runtime ownership claimed.',
 'run':{'original_root':str(ROOT),'exe_argument':str(EXE),'output':str(OUT),
        'capstone_version':capstone.__version__,'pe_helper':'scripts/pe.py',
        'pe_helper_sha256':sha((ROOT/'scripts'/'pe.py').read_bytes()),
        'audit_script_sha256':sha(Path(__file__).read_bytes())},
 'proof_levels':{'pe_layout':'STATIC_LAYOUT from pinned PE section metadata and file-backed ranges',
    'host_explicit_references':'STATIC_SCAN across decoded x86-32 .text operands; raw numeric candidate scan is not pointer proof',
    'upload_move_addresses':'BOUNDED_EQUIVALENT receipts for finite A710/A8F0 cases, limited to tested ordinary-domain inputs',
    'alternate_page_selection':'STATIC_CODE plus bounded AC60 page-mask fixtures for FT4/GT4/E1 discriminators',
    'clipped_sprite_uv':'BOUNDED_EQUIVALENT geometry fixtures; wider-than-byte U/V means page0 is not a read bound'},
 'oracle':{'path':str(pe.path),'sha256':sha(pe.data),
           'image_base':hx(pe.image_base),'size_of_image':hx(pe.pe.OPTIONAL_HEADER.SizeOfImage),
           'relocation_directory_size':pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size},
 'sections':sec,'candidate_ranges':ranges,
 'normal_upload_formula':{'rva':'0xA710','span_sha256':'30bd84ea8724152e3bfb0e2c521650399913bcec115f1aef489768085981d0fe',
    'domain':'0<=x and x+width_words<=1024; 0<=y and y+height_rows<=512; fully contained rectangle',
    'row':'y if y<256 else 0x800+(y&0xff)',
    'byte_address':'0x96e6e0 + row*0x100 + ((byte_x&0x700)<<8) + (byte_x&0xff), byte_x=2*x+byte_in_word',
    'derived_full_domain_span':[hx(ORD),hx(ORD+BANK)],
    'base_access_sites':{'A710_normal_low_y':['0xA867','0xA8A9'],
        'A710_high_y':['0xA781','0xA7C5'],
        'A8F0_normal_low_y':['0xAB1C','0xAB36','0xAB96','0xABA1'],
        'A8F0_high_y':['0xA997','0xA9B6','0xAA07','0xAA13']},
    'normal_domain_reaches_alternate_base':False,
    'high_y_static_branch':'for signed y>=512: row=0x1000+(y&0x1ff), so for x<1024 its address envelope is [0xA6E6E0,0xAFE6E0); this lies in alternate sampler pages 0..8 and overlaps page-2 host state. This branch is outside the bounded normal-input proof and does not establish intended alternate-page upload semantics.',
    'high_y_collision_witness':{'rect_x':284,'rect_y':514,'width_words':1,'height_rows':1,
        'first_byte_address':hx(ORD+0x120238),'overlaps':'reciprocal table R[0] at 0xA8E918',
        'proof':'static address expression only; no fixture executed this input'}},
 'alternate_page_selection':{'rva':'0xAC60','state':{'page':'A8E6F4=(E1_DWORD&0x1f)','blend':'A8E6F8=(E1_DWORD>>5)&3','alternate':'A8E6FC=((E1_DWORD&0x180)==0x180)'},
    'selected_base_cell':'A909FC','ordinary_page_base':'0x96E6E0+(((A8E6F4&0x1f)>>1)<<16)',
    'alternate_page_base':'0xA6E6E0; alternate branch overwrites the ordinary page result, so tpage page index is ignored in alternate mode',
    'fixture_limit':'Existing FT4/GT4/E1 page-mask fixtures test tpage 0x80/0x100/0x180, all with page selector zero; nonzero ordinary page selection is established statically by the indexed add sites only.',
    'page_selection_code_sites':{'ordinary_base_add':[hx(x) for x in (0xADCB,0xAEDB,0xAFEC,0xB070)],
        'alternate_base_override':[hx(x) for x in (0xADD9,0xAEE9,0xAFFA,0xB086)]},
    'alternate_base_distance':hx(ALT-ORD),'ordinary_page_read_span':[hx(ORD),hx(ORD+BANK)],
    'alternate_byte_uv_read_window':[hx(ALT),hx(ALT+0x10000)],
    'wide_sprite_uv_escape_note':'Sprite clipping can add negative-x/y offsets to U/V without byte masking. First source row is selected_page+U+(V<<8); later rows add 0x100 and mask only the row offset to 16 bits. The byte-UV page0 window is therefore not a proven bound for clipped-sprite accesses; see pinned sprite_geometry evidence.',
    'content_writer_proven':{'A710_A8F0_normal_domain':False,
        'A710_A8F0_high_y_static_address_overlap_only':True}},
 'host_callback_table':{'source_table_rva':hx(table_rva),'source_table_va':hx(pe.image_base+table_rva),'size':table_size,
    'sha256':sha(table),'end_va_exclusive':hx(pe.image_base+table_rva+table_size),
    'pointer_cell_rva':hx(pointer_rva),'pointer_cell_va':hx(pe.image_base+pointer_rva),
    'pointer_cell_initial_value':hx(pointer_value),'pointer_aliases_table':pointer_value==pe.image_base+table_rva,
    'pointer_cell_sha256':sha(pe.read_rva(pointer_rva,4)),
    'pointer_cell_direct_code_ref_sites':selected_global_refs.get('0x004365a0',{}).get('sites',[]),
    'static_direct_code_writes_observed':sum(1 for x in global_refs.get('0x004365a0',[]) if x['access']&2),
    'entry_count':len(table_values),'entry_target_classes':table_value_classes,
    'static_direct_table_operand_reads':sum(1 for x in callback_table_code_refs if x.get('access',0)&1),
    'static_direct_table_operand_writes':sum(1 for x in callback_table_code_refs if x.get('access',0)&2),
    'callbacks':callback_slots,
    'callback_targets_outside_candidate_spans':all(
        not (lo<=int(x['target_va'],16)<hi)
        for x in callback_slots for lo,hi in ((ORD,ORD+BANK),(ALT,ALT+BANK))),
    'pointer_cell_outside_candidate_spans':not any(
        lo<=pe.image_base+pointer_rva<hi for lo,hi in ((ORD,ORD+BANK),(ALT,ALT+BANK)))},
 'reciprocal_table_initializer':{'span':next(x for x in function_spans if x['name']=='reciprocal_table_initializer'),
    'direct_caller_sites':callers_72a0,
    'writes':'R[0]=0 at 0xA8E918; R[1..2047]=0x40000000/i as DWORDs, ending at 0xA90914; exclusive end 0xA90918',
    'storage_range':['0xA8E918','0xA90918'],'size_bytes':0x2000,
    'inside_alternate_page':2,'page_offset':'0x238','inside_page2':True,
    'startup_callsite_static_only':True},
 'selected_page_pointer_alias':{'cell':'0xA909FC','section':section_for_va(pe,0xA909FC),
    'read_write_site_count':len(global_refs.get('0x00a909fc',[])),
    'example_read_write_sites':[{'rva':x['rva'],'instruction':x['instruction']}
        for x in global_refs.get('0x00a909fc',[])[:5]],
    'meaning':'AC60 loads the selected page base here; sprite raster paths can change it to selected page + adjusted U. This is mutable pointer/state in alternate page 2, not texture storage.'},
 'source_function_spans':function_spans,
 'evidence_inputs':evidence_inputs,
 'explicit_refs':{'instruction_count':len(insns),
    'decoder':'Capstone x86-32 linear sweep over raw .text with skipdata enabled; only decoded non-skip instructions contribute operand refs.',
    'direct_candidate_page_memory_operand_summary':{'total':len(direct_bank_mem),'by_bank_page':mem_page_summary},
    'bank_base_immediate_groups':[{'value':x['value'],'count':x['count'],
        'sites':x['sites']} for x in group_by_value(base_imm,'value')],
    'all_immediate_values_in_page_spans':group_by_value(bank_imm,'value'),
    'memory_operand_groups_in_page_spans':[{
        'address_or_base':x['address_or_base'],'count':x['count'],'reads':x['reads'],'writes':x['writes'],
        'forms':x['forms'],'site_count':len(x['sites']),
        'example_site_rvas':[s['rva'] for s in x['sites'][:4]]} for x in mem_groups],
    'selected_global_refs_outside_page_spans':selected_global_refs,
    'nontext_dword_value_candidates_in_page_spans_all_byte_offsets':{
        'total':len(data_pointer_literals),'by_section':data_candidate_by_section,
        'interpretation':'Not classified as pointers from value-range membership alone. Hits can occur inside adjacent 16-bit data/PE metadata.'},
    'callback_table_address_code_references_grouped':callback_ref_summary,
    'callback_table_pointer_value_data_candidates_all_byte_offsets':callback_table_data_candidates,
    'callback_table_candidate_interpretation':'Raw-value matches are candidates only; machine-code operand context and callback table initialization must be considered separately.'},
 'proof_limits':['The full .data virtual tail is BSS-backed PE image memory, not a dedicated texture allocation record.',
    'Address spans do not prove pages were initialized/populated or that the whole bank is used.',
    'Static explicit-reference scan cannot bound indirect/dynamic pointers or external DLL writes.',
    'A710 high-y and negative signed-WORD paths are outside the normal domain, and alternate-bank population is not proven.']
}
OUT.parent.mkdir(parents=True,exist_ok=True)
out=OUT
out.write_text(json.dumps(report,indent=1)+'\n',encoding='utf-8')
print(json.dumps({'wrote':str(out),'oracle_sha256':report['oracle']['sha256'],'text_instructions':len(insns),
    'ordinary_range':ranges['ordinary_pageable_bank'],'alternate_page0':ranges['alternate_fixed_page0_uv_window'],
    'base_immediate_groups':report['explicit_refs']['bank_base_immediate_groups'],
    'direct_page_mem_operands':report['explicit_refs']['direct_candidate_page_memory_operand_summary'],
    'callback_table':{'source_table_va':report['host_callback_table']['source_table_va'],
        'size':table_size,'entry_count':len(table_values),'direct_reads':report['host_callback_table']['static_direct_table_operand_reads'],
        'direct_writes':report['host_callback_table']['static_direct_table_operand_writes'],
        'bank_target_entries':table_value_classes['ordinary_candidate_bank']+table_value_classes['alternate_candidate_bank']},
    'nontext_numeric_candidates':len(data_pointer_literals)},indent=2))
