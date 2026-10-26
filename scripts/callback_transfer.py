"""Explicit rooted-callback call proof, separate from ordinary BSS references.

Version1 covers only low9-bit, stride8/12, slot-zero, one-DWORD cdecl calls.
It proves the instruction-level dispatch expression, not a dynamic table extent,
callee set or callback behavior. Every root must already pass the complete
scalar BSS/relocation gate, and a separate native witness must have the same
shape and root. No implicit or unreviewed indirect transfer is admitted.
"""
import hashlib


def _integer(value,label,minimum=0):
    if type(value)is not int or value<minimum:raise ValueError('Invalid callback '+label)
    return value


def identity_span(record):
    if not isinstance(record,dict):raise ValueError('Invalid callback proof record')
    identity=record.get('identity')
    if not isinstance(identity,dict) or set(identity)!={'rva','size','sha256'}:raise ValueError('Callback needs one independent native window identity')
    at=_integer(identity['rva'],'identity RVA');size=_integer(identity['size'],'identity size',1)
    if size>32 or not isinstance(identity['sha256'],str) or len(identity['sha256'])!=64 or any(c not in '0123456789abcdef' for c in identity['sha256']):raise ValueError('Invalid callback identity extent/hash')
    return at,size


def _shape(code,address,mask,stride):
    import capstone as cs
    from capstone.x86 import X86_OP_REG,X86_OP_IMM,X86_OP_MEM
    decoder=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_32);decoder.detail=True;ins=list(decoder.disasm(code,address))
    if not code or sum(i.size for i in ins)!=len(code):raise ValueError('Incomplete callback window decode')
    # One root load, mask, argument push, optional x3 LEA, indirect CALL,
    # optional count reload, and caller cleanup. No other instructions/edges.
    prefix=5 if stride==12 else 4
    if len(ins) not in (prefix+1,prefix+2):raise ValueError('Unsupported callback window instruction inventory')
    root,masked,push=ins[:3];call=ins[prefix-1];cleanup=ins[-1]
    reg=lambda i,n:i.operands[n].type==X86_OP_REG and i.operands[n].size==4 and i.reg_name(i.operands[n].reg) in {'eax','ebx','ecx','edx','esi','edi','ebp'}
    if root.mnemonic!='mov' or len(root.operands)!=2 or not reg(root,0) or root.operands[1].type!=X86_OP_MEM or root.operands[1].size!=4:raise ValueError('Callback root must be a DWORD register load')
    mem=root.operands[1].mem
    if mem.base or mem.index or mem.segment or root.disp_size!=4:raise ValueError('Callback root must be an absolute scalar load')
    if masked.mnemonic!='and' or len(masked.operands)!=2 or not reg(masked,0) or masked.operands[1].type!=X86_OP_IMM or masked.operands[1].imm!=mask:raise ValueError('Callback index mask differs')
    if push.mnemonic!='push' or len(push.operands)!=1 or not reg(push,0):raise ValueError('Callback needs exactly one DWORD register argument')
    base=root.operands[0].reg;index=masked.operands[0].reg;argument=push.operands[0].reg
    if len({base,index,argument})!=3:raise ValueError('Callback root/index/argument registers must be disjoint')
    if stride==12:
        lea=ins[3]
        if lea.mnemonic!='lea' or len(lea.operands)!=2 or not reg(lea,0) or lea.operands[1].type!=X86_OP_MEM:raise ValueError('Callback stride12 needs its exact x3 LEA')
        m=lea.operands[1].mem
        if m.base!=index or m.index!=index or m.scale!=2 or m.disp or m.segment or lea.operands[0].reg in {base,index,argument}:raise ValueError('Callback stride LEA changes register provenance')
        index=lea.operands[0].reg
    if call.mnemonic!='call' or len(call.operands)!=1 or call.operands[0].type!=X86_OP_MEM or call.operands[0].size!=4:raise ValueError('Callback must be one near memory-indirect CALL')
    m=call.operands[0].mem
    if m.base!=base or m.index!=index or m.scale!=(4 if stride==12 else 8) or m.disp or m.segment:raise ValueError('Callback root/index/stride/slot provenance differs')
    if cleanup.mnemonic!='add' or len(cleanup.operands)!=2 or cleanup.operands[0].type!=X86_OP_REG or cleanup.reg_name(cleanup.operands[0].reg)!='esp' or cleanup.operands[1].type!=X86_OP_IMM or cleanup.operands[1].imm!=4:raise ValueError('Callback must clean exactly one DWORD argument')
    if len(ins)==prefix+2:
        reload=ins[-2]
        if reload.mnemonic!='mov' or len(reload.operands)!=2 or not reg(reload,0) or reload.operands[1].type!=X86_OP_MEM or reload.operands[1].size!=4:raise ValueError('Unsupported instruction before callback cleanup')
        m=reload.operands[1].mem
        if not m.base or m.index or m.disp or m.segment:raise ValueError('Unsupported callback count reload')
    return dict(root_address=mem.disp,root_field=root.address+root.disp_offset,call_address=call.address,argument_register=push.reg_name(argument),root_register=root.reg_name(base),mask=mask,stride=stride,slot_offset=0,argument_bytes=4,cleanup_bytes=4,window_size=len(code))


def verify_calls(pe,bindings,records,reviewed,instructions,rva,size,relocation_receipts):
    """Called after complete root reference/owner verification, before equality."""
    import capstone as cs
    from capstone.x86 import X86_OP_IMM
    calls=bindings.get('indirect_calls',[])
    if not isinstance(calls,list):raise ValueError('Invalid explicit callback inventory')
    verified={};windows=[];starts={i.address-rva:n for n,i in enumerate(instructions)}
    for record in calls:
        if isinstance(record,dict) and record.get('profile')=='fixed-slot-v1':
            from fixed_slot import verify_call
            receipt=verify_call(pe,record,records,reviewed,instructions,rva,size)
            off=record['offset']
            if off in verified:raise ValueError('Invalid/duplicate fixed-slot offset')
            verified[off]=receipt
            continue
        if not isinstance(record,dict) or set(record)!={'kind','offset','root_offset','root','mask','stride','identity','evidence'} or record['kind']!='rooted-callback-v1':raise ValueError('Invalid explicit rooted callback proof')
        refs=record['evidence']
        if not isinstance(refs,list) or not refs or any(not isinstance(s,str) or not s.strip() for s in refs):raise ValueError('Callback proof needs independent evidence')
        if not isinstance(record['root'],str) or not record['root']:raise ValueError('Invalid callback root symbol')
        off=_integer(record['offset'],'call offset');root_off=_integer(record['root_offset'],'root offset')
        if off in verified or root_off not in starts or off not in starts or root_off>=off or record['mask']!=0x1ff or type(record['mask'])is not int or record['stride'] not in (8,12) or type(record['stride'])is not int:raise ValueError('Invalid/duplicate callback offsets or profile')
        at=starts[root_off];last=starts[off]+1
        # Include the observed post-call cleanup, with at most one count reload.
        if last>=len(instructions):raise ValueError('Missing callback cleanup')
        if instructions[last].mnemonic=='mov':last+=1
        if last>=len(instructions):raise ValueError('Missing callback cleanup')
        window=instructions[at:last+1]
        if any(a.address+a.size!=b.address for a,b in zip(window,window[1:])):raise ValueError('Callback window crosses undecoded bytes')
        end=window[-1].address+window[-1].size-rva
        if any(root_off<b and a<end for a,b in windows):raise ValueError('Overlapping callback proof windows')
        source=_shape(b''.join(bytes(i.bytes) for i in window),rva+root_off,record['mask'],record['stride'])
        if source['call_address']!=rva+off:raise ValueError('Callback proof selects another call')
        field=source['root_field']-rva;use=reviewed.get(field);owner=records.get(record['root'],{})
        if not use or use!=(record['root'],6,0,False) or owner.get('kind')!='bss' or owner.get('size')!=4:raise ValueError('Callback needs an already verified complete scalar root relocation')
        native=_shape(pe.read_rva(rva+root_off,end-root_off),rva+root_off,record['mask'],record['stride'])
        if native['call_address']!=rva+off or native['root_address']!=pe.image_base+owner['rva']:raise ValueError('Native callback root/call differs')
        # No local direct/table edge may enter after the root load, including
        # the cleanup. Otherwise the straight-line provenance is insufficient.
        for ins in instructions:
            if (ins.group(cs.CS_GRP_CALL) or ins.group(cs.CS_GRP_JUMP)) and ins.operands and ins.operands[0].type==X86_OP_IMM and rva+root_off<ins.operands[0].imm<rva+end:raise ValueError('Control flow enters callback provenance window')
        if any(x['kind']=='local' and rva+root_off<x['destination_rva']<rva+end for x in relocation_receipts):raise ValueError('Local reference enters callback provenance window')
        witness,length=identity_span(record)
        if witness<rva+size and rva<witness+length:raise ValueError('Callback witness must be independent of the compared caller')
        sections=[s for s in pe.sections if s['characteristics']&0x20000000 and s['virtual_address']<=witness and witness+length<=s['virtual_address']+min(s['virtual_size'],s['raw_size'])]
        if len(sections)!=1:raise ValueError('Callback witness must be file-backed executable code')
        raw=pe.read_rva(witness,length)
        if hashlib.sha256(raw).hexdigest()!=record['identity']['sha256']:raise ValueError('Callback witness hash changed')
        independent=_shape(raw,witness,record['mask'],record['stride'])
        if independent['root_address']!=pe.image_base+owner['rva']:raise ValueError('Independent callback witness identifies another root')
        field=independent['root_field'];relocs=[(e.rva,e.type) for block in getattr(pe.pe,'DIRECTORY_ENTRY_BASERELOC',[]) for e in block.entries if e.type and e.rva<witness+length and witness<e.rva+4]
        if relocs!=[(field,3)]:raise ValueError('Callback witness needs exactly one native HIGHLOW root field')
        verified[off]=dict(kind=record['kind'],offset=off,root=record['root'],root_rva=owner['rva'],root_offset=root_off,window_size=end-root_off,mask=record['mask'],stride=record['stride'],slot_offset=0,argument_bytes=4,cleanup_bytes=4,independent_witness_rva=witness,evidence=refs)
        windows.append((root_off,end))
    return verified
