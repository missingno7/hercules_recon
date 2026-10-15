"""Audit the measured PC sound scratch prefix against native code and assets."""
import argparse
import hashlib
import json
import struct
import sys
from collections import Counter
from pathlib import Path

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python']
from match import compile_source,verify_toolchain,DEFAULT_TOOLCHAIN
from coff import COFF
import pefile
import capstone

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'work/host_sound_scratch_data')
args=parser.parse_args()
OUT=args.output.resolve()
assert OUT.is_relative_to((ROOT/'work').resolve())
OUT.mkdir(parents=True,exist_ok=True)
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
FS=Path('D:/Prog/hercules_recon/assets/pc/HERCULES/HERCULES.FS')
SOURCE=ROOT/'calibration/host_sound_scratch_storage.c'
HEADER=ROOT/'calibration/host_sound_bank.h'
BASE,WIDTH=0x699848,1048576
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
    assert sha(FS)=='affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4'
    tool=verify_toolchain(DEFAULT_TOOLCHAIN)
    watched=[ORACLE,FS,SOURCE,HEADER,Path(__file__),ROOT/'toolchains/manifest.json',
             *[ROOT/'scripts'/p for p in ['match.py','coff.py','env.py']]]
    pins={str(p):sha(p)for p in watched}
    archive=FS.read_bytes()
    rows=[]
    for index in range(1024):
        key,offset,length=struct.unpack_from('<III',archive,index*12)
        if not key:break
        assert offset+length<=len(archive)
        if archive[offset:offset+4]!=b'ESF\x03':continue
        magic,count,rate,flags=struct.unpack_from('<4I',archive,offset)
        compressed=count//4
        sample_count=count//2
        consumed=(sample_count+1)//2
        begin=WIDTH-1-compressed
        last_read=begin+consumed-1
        last_written=sample_count*2-1
        assert count>0 and count%2==0 and length==16+compressed
        assert 0<=begin and last_read<WIDTH and last_written<begin
        rows.append(dict(row=index,key=hex(key),offset=offset,member_bytes=length,
                         member_sha256=hashlib.sha256(archive[offset:offset+length]).hexdigest(),
                         sample_bytes=count,rate_low_word=rate&65535,flags=hex(flags),
                         compressed_bytes=compressed,decoder_packed_bytes=consumed,
                         compressed_begin=begin,last_decoder_read=last_read,
                         last_decoder_write=last_written,unread_tail_byte=consumed>compressed))
    assert index==821 and len(rows)==559
    tails=sum(row['unread_tail_byte']for row in rows)
    assert tails==250 and max(row['last_decoder_read']for row in rows)==WIDTH-1
    image=pefile.PE(str(ORACLE))
    section=next(s for s in image.sections if s.VirtualAddress<=BASE and
                 BASE+WIDTH<=s.VirtualAddress+s.Misc_VirtualSize)
    assert BASE>=section.VirtualAddress+section.SizeOfRawData
    md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    md.detail=True
    md.skipdata=True
    references,coverage=[],[]
    for sec in image.sections:
        if not sec.Characteristics&0x20000000:continue
        raw=sec.get_data()
        start=image.OPTIONAL_HEADER.ImageBase+sec.VirtualAddress
        decoded,skipped,end=0,0,start
        for ins in md.disasm(raw,start):
            assert ins.address==end
            end+=ins.size
            if not ins.id:
                skipped+=ins.size
                continue
            decoded+=ins.size
            literals=[]
            for operand in ins.operands:
                value=operand.imm if operand.type==capstone.x86.X86_OP_IMM else operand.mem.disp if operand.type==capstone.x86.X86_OP_MEM else None
                if value is not None and 0x400000+BASE<=value<0x400000+BASE+WIDTH:
                    literals.append(hex(value))
            if literals:
                references.append(dict(rva=hex(ins.address-0x400000),hex=ins.bytes.hex(),
                                       mnemonic=ins.mnemonic,operands=ins.op_str,literals=literals))
        assert decoded+skipped==len(raw)and end==start+len(raw)
        coverage.append(dict(section=sec.Name.rstrip(b'\0').decode(),bytes=len(raw),
                             decoded_bytes=decoded,skipped_bytes=skipped))
    assert {row['rva']for row in references}=={'0xf571','0xf5e4','0xf611','0xf650'}
    obj,command=compile_source(SOURCE,['/O2'])
    co=COFF(obj)
    assert not any(s['flags']&0x20 and s['size']for s in co.sections)
    definitions=[s for s in co.symbols.values()if s['storage']==2 and s['section']>0]
    assert len(definitions)==1 and definitions[0]['name']=='_g_host_sound_scratch_699848'
    symbol=definitions[0]
    sec=co.sections[symbol['section']-1]
    assert sec['flags']&0x80 and sec['size']==WIDTH and symbol['value']==0 and not sec['relocations']
    assert not sec['data']or sec['data']==bytes(WIDTH)
    assert pins=={p:sha(Path(p))for p in pins}
    report=dict(scope='Measured accessed-prefix backing only. Original total allocation,element type,TU and placement unproved; no new arbitrary-header bounds or real game execution.',
                inputs=pins,toolchain=tool,source=str(SOURCE),object=str(obj),object_sha256=sha(obj),command=command,
                prefix=dict(rva=hex(BASE),bytes=WIDTH,short_count=524288,source_bss=True,
                            code_bytes=0,relocations=0,native_virtual_zero=True),
                assets=dict(directory_rows=821,esf_members=559,sample_remainders=dict(Counter(str(x['sample_bytes']%4)for x in rows)),
                            tail_byte_members=tails,max_sample_bytes=max(x['sample_bytes']for x in rows),
                            all_decode_writes_before_packed_input=True,max_accessed_offset=hex(WIDTH-1),rows=rows),
                native_literal_references=references,linear_scan_coverage=coverage,
                limits=['Linear skip-data inventory is not a proof of all indirect references or original ownership.',
                        'Asset bounds concern actual decode and compressed-input accesses; arbitrary oversized COM lock outputs or forged headers remain outside this domain.',
                        'The final byte is not read from the archive on250members. Constructor/decoder witness tests separately verify that actual code consumes it.'])
    target=OUT/'sound_scratch_data.json'
    target.write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(dict(esf_members=559,tail_byte_members=tails,prefix_bytes=WIDTH,report=str(target))))


if __name__=='__main__':main()
