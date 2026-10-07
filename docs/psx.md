# Disc evidence and PlayStation comparison

The PC binaries remain the matching targets. The PSX discs give independent
evidence about module boundaries, development paths, level dispatch, and library
provenance. The compact, regenerable record is [`evidence/psx.json`](../evidence/psx.json).
Full file lists and extracted content stay under ignored `work/discs/`.

## The supplied PC CD contains a modified executable

The image is a later repack, not an untouched 1997 retail disc. Its ISO volume
`HERCULES_AG` has a creation field beginning `20201227120900`, and it includes
`NOCD/`, `WIN_X64/`, and `CORRECT/` directories. This does not invalidate all its
contents: the original-style InstallShield archive `DATA/DATA32.Z` provides a
distinct baseline that can be compared directly.

Archive `DATA32.Z` is 25,334,701 bytes, SHA-256
`fa9fbd3b258c51b172778a28562c730f9460cd12a7fc6c0001920bbd258071b2`.
It contains seven files. These were decompressed without executing the installer.

| File | Archive versus installed / WIN_X64 |
|---|---|
| `ENG1.DLL`, `ENG3.DLL`, `TITLE.DLL` | Byte-identical |
| `HERCULES.FS` | Byte-identical, 46,924,407 bytes |
| `HERCULES.EXE` | Exactly one byte differs |
| `HERCULES.HLP` | Different: 44,497 versus 46,876 bytes |
| `HERCULES.GID` | Archive-only in supplied installed directory |

The archive EXE hash is
`587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`.
The supplied installed, `WIN_X64`, and `NOCD` copies all hash to
`b3ab3ee9e5c973bb245d8eed795e8e7065d7dfc371e412e602b2059af894a551`.
Both are 275,968 bytes. At file offset `0x94b0`, VA `0x0040a0b0`, the archive
contains `0x53` (`push ebx`) and the other copies contain `0xc3` (`ret`). This is
direct evidence of an entry-point patch to one function. The directory name
suggests a CD-check bypass; that semantic purpose still requires examination of
the complete function. Use the archive version as the historical EXE baseline,
while retaining both hashes and the provenance qualification that the enclosing
disc is a repack.

## PSX identities and executable architecture

Both images are single-track `MODE2/2352` discs. The boot executable and three
additional files carry valid `PS-X EXE` headers. Sizes below include the 2,048-byte
header.

| Role / path | v1.0 size | v1.1 size | v1.0 entry PC | v1.1 entry PC |
|---|---:|---:|---|---|
| Boot: `SLUS_005.29` / `SLUS_010.29` | 129,024 | 129,024 | `0x800132e8` | `0x800135c8` |
| `EX/ENGINE1` | 249,856 | 305,152 | `0x800380a0` | `0x80038230` |
| `EX/ENGINE3` | 198,656 | 245,760 | `0x800380a0` | `0x80038230` |
| `EX/TITLE` | 288,768 | 290,816 | `0x800380a0` | `0x80038230` |

The boot images load at `0x80010000`. The three other images all load at
`0x80038098` in v1.0, and `0x80038228` in v1.1; each has entry PC = load + 8.
Their shared address range demonstrates mutually replacing executable modules,
consistent with the PC's separate `ENG1.DLL`, `ENG3.DLL`, and `TITLE.DLL`.
The boot image alone is therefore not the complete PSX game-code oracle.

The actual `SYSTEM.CNF` files select `SLUS_005.29` and `SLUS_010.29` respectively,
regardless of disc filenames. Both request `TCB=4`, `EVENT=16`, and
`STACK=801FFF00`; EXE headers instead contain initial SP `0x801ffff0`. Keep these
as distinct header/config fields. ISO creation fields begin `19970624015349`
and `19991116121052`; these are embedded dates, not independent release dating.
Full CUE, image, and executable SHA-256 values are in the evidence JSON.

## A concrete PC / PSX shared structure

The same twelve-record table exists in PC `TITLE.DLL` and both PSX `EX/TITLE`
images. Every record is three 32-bit pointers: level display name, development
path, engine basename. All decoded strings and their order agree.

| Image | File offset | Address |
|---|---|---|
| PC `TITLE.DLL` | `0x21ab0` | `0x100234b0` |
| PSX v1.0 `EX/TITLE` | `0x2de88` | `0x80065720` |
| PSX v1.1 `EX/TITLE` | `0x2e25c` | `0x80065c84` |

`TRAINING GAUNTLET`, `CYCLOPS CHASE`, and `PASSAGEWAYS TORMENT` select
`\SRC3\ENGINE\` / `ENGINE3`. The other nine records select `\SRC1\ENGINE\` /
`ENGINE1`: `PLAYROOM`, `TRAINING GROUND`, `FOREST OF CENTAURS`, `NESSUS BATTLE`,
`VISIT TO THEBES`, `HYDRA BATTLE`, `MEDUSA BATTLE`, `BATTLE OF TITANS`, and
`VORTEX OF SOULS`.

This is shared data structure and module-selection evidence, stronger than a
similar game title or a few reused strings. It does not yet establish any
one-to-one MIPS/x86 function mapping. The engine images also share console UI
phrases with the PC DLLs, including controller/CD prompts. Development paths
are preserved table values; they do not by themselves prove that either retail
runtime reads files from those host paths.

Both PSX boot images contain identical library-style RCS identifiers:

```text
$Id: bios.c,v 1.81 1996/12/16 06:24:14 makoto Exp $
$Id: intr.c,v 1.74 1996/12/04 07:30:16 makoto Exp $
$Id: sys.c,v 1.129 1996/12/25 03:36:20 noda Exp $
```

Nearby `CdInit`, `Cdl*`, `ResetGraph`, and interrupt diagnostics support a
PlayStation support-library origin. These identifiers are library fingerprints,
not Hercules source filenames or proof of a precise compiler release. No MAP,
PDB, COFF symbol table, or game-source symbol file was identified in this scan.

## Revision changes

v1.0 contains 309 ISO files and v1.1 contains 307. Of 305 common paths, 273 have
identical complete file payloads and 32 differ. The full XA subheaders and user
bytes were compared for XA files, not only their truncated ISO logical views.
v1.0-only paths are `SLUS_005.29`, `CR/C30.0`, `CR/C31.0`, `SD4/S4.18`;
v1.1-only paths are `SLUS_010.29`, `TI/SNL.0`.

All four executable images changed. Engine load/entry addresses move by
`0x190`; ENGINE1 grows by 55,296 bytes and ENGINE3 by 47,104 bytes, while TITLE
grows by 2,048. Several overlay files (`OV/*`), level files, sounds, a movie,
and credits also differ. Thus v1.1 is useful revision evidence but cannot be
treated as a tiny fixed-address patch to v1.0.

The comparison tool records unique, word-aligned 64-byte anchors and their
largest joined unchanged spans. These expose displaced identical regions:
for example ENGINE3 has a 576-byte span at v1.0 file `0x16014` versus v1.1
`0x16038`. Such spans may be code, data, or padding. They are anchors for later
disassembly, not claimed function boundaries or matching-decompilation results.

## Reproduce extraction and comparison

`scripts/disc.py` reads sectors directly with Python's standard library. No
mounting or installer execution is needed. Use an available Python 3.10+ as
`$pythonRuntime` (the bundled runtime was used for this milestone).

```powershell
& $pythonRuntime scripts/disc.py assets/pc/CD/HERCULES_ENG.cue --extract work/discs/pc_cd --report work/discs/pc_cd.json
& $pythonRuntime scripts/disc.py "assets/psx/usa_original/Disney's Hercules Action Game (USA) (v1.0).cue" --extract work/discs/psx_usa_original --report work/discs/psx_usa_original.json
& $pythonRuntime scripts/disc.py "assets/psx/usa_rerelease/Disney's Hercules Action Game (USA) (v1.1).cue" --extract work/discs/psx_usa_rerelease --report work/discs/psx_usa_rerelease.json
& $pythonRuntime scripts/disc.py --compare-psx work/discs/psx_usa_original work/discs/psx_usa_rerelease --report evidence/psx.json
```

The comparison additionally uses the project's PE adapter/dependencies to
verify the PC level table. Extraction preserves CUE/BIN evidence and rejects
outputs under `assets/`. The PC data track uses `MODE1/2352`; its second audio
track begins at sector 151,979 (`33:46:29`) and is excluded from ISO extraction.

Each PSX disc has 33 files containing XA Form 2 sectors. Their 2,048-byte logical
views are labelled as such; these views alone are **not faithful media files**.
The companion `.xa-raw` files retain the complete 2,352-byte sectors. The report
also hashes the XA subheaders and full 2,324-byte Form 2 payloads, excluding
physical sector addresses and EDC/ECC. Neither EDC/ECC validation nor external
disc-database validation has been performed.

The InstallShield 3 archive is not a Unix `.Z` file; local 7-Zip 23.01 rejected
it. [Idecomp](https://github.com/lephilousophe/idecomp) was installed externally
at `C:\tools\idecomp`, pinned to commit
`bd2b77624b96bb2a4f347518d087126759296a03`. It is GPLv3; no extractor source was
copied into this repository.

```powershell
git clone https://github.com/lephilousophe/idecomp.git C:\tools\idecomp
git -C C:\tools\idecomp checkout bd2b77624b96bb2a4f347518d087126759296a03
& $pythonRuntime C:\tools\idecomp\idecomp.py -l work/discs/pc_cd/DATA/DATA32.Z
& $pythonRuntime C:\tools\idecomp\idecomp.py -C work/discs/pc_install work/discs/pc_cd/DATA/DATA32.Z
```

Audited tool hashes: `idecomp.py`
`345323b9b6d4604b71d9038408d0e3996007f62bd9c5bb9a0374291b10b7c025`;
`pwexplode.py`
`8fc500030e6fd8aae91e99c8a85f3781eebeed25cdd08a32a5cc9194012a030c`.
The archive table was listed and inspected before extraction; the decoder
checked each decompressed file's expected size. A second independent decoder
has not been used.
