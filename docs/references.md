# External references and research

Reviewed 2026-10-07. External material guides hypotheses; the supplied PC binaries
remain the matching oracle. No external game source, symbols, or machine code has
been imported into reconstructed source.

## Methodology references

- [Icy Tower rereconstruction](https://github.com/missingno7/icytower_rerecon),
  observed revision `70ece2aaf6ff79f52165e38fa3c346160a7e298d`.
  Its useful principles are immutable input identities, small function context,
  isolated source experiments, fresh compilation, and strict promotion that
  protects existing matches. Its DWARF-rich GCC situation is different from
  Hercules's stripped VC-era binaries; its implementation is not copied.
- [Blood II reconstruction](https://github.com/missingno7/blood2_recon), explicitly
  supplied as an additional reference; observed revision
  `791fd765378b1c950bf73cf6bc1a558f2121f34a`.
  Its [toolchain research](https://github.com/missingno7/blood2_recon/blob/791fd765378b1c950bf73cf6bc1a558f2121f34a/docs/toolchain.md)
  led directly to the pinned VC5 archives acquired here. Its separation of
  measured evidence, hypotheses, and unresolved issues is useful. Its independent
  compiler/CRT/linker analysis also cautions against identifying a compiler from
  the PE linker field alone. Reported VC5 dependence on declaration and PCH
  context is a useful future experiment, not yet a measured Hercules fact.

Do not transfer Blood II's compiler selection, SDK versions, source availability,
acceptance categories, or porting scope to Hercules. Hercules's current pilot
rules remain stricter than diagnostic relocation similarity.

## Verified primary provenance

[Rob Watkins's own commercial-games portfolio](https://robwatkinsgamedeveloper.weebly.com/commercial-games.html)
lists Hercules for PS1 and PC and credits his role as audio and front-end
programmer. This is a relevant provenance lead for the front end and audio, but
does not identify source files, a compiler, or any particular DLL. The neighboring
discussion of an assembly-to-C++ conversion concerns **Super Street Fighter II
Turbo**, not Hercules; it must not be used to claim Hercules was such a conversion.

The compiler archive maintainer's trees are the direct distribution sources for
this checkout:

- [VC5 RTM](https://github.com/archaic-msvc/msvc500/tree/8abf95ce980161ad87b0b02402269cce76988953)
- [VC5 SP2](https://github.com/archaic-msvc/msvc500sp2/tree/4ebf02022705b4c9e9108d3ed3f286ed80ba2ed9)

These are third-party preserved Microsoft tool distributions, not a claim of
an official Microsoft download or a redistributable licence. The repository
stores only identities and setup code. File resources and output banners were
measured locally; full extracted trees are hash-locked.

## Bounded search results and remaining leads

The initial web pass searched Hercules/Eurocom with source, decompilation, PDB,
MAP, symbols, debug/prototype, demo, developer portfolio, and compiler terms.
GitHub repository searches included `hercules disney`, `hercules eurocom`, and
`eurocom engine`. No verified Hercules game-source release, symbol file,
matching-decompilation project, or debug PC build was found in this pass. This is
a search result, not evidence that none exists.

- [`maearon/hercules_remake`](https://github.com/maearon/hercules_remake) describes
  a Godot remake. It is not historical source or matching evidence.
- [`BrinchEbsen/EngineXFilelistUtil`](https://github.com/BrinchEbsen/EngineXFilelistUtil)
  describes a Eurocom EngineX archive tool. No link to Hercules's 1997 formats
  was established, so it is not used for extraction or source inference.
- Demo-disc and alternate PC release listings are leads for later binary
  acquisition, not verified additional oracles. No external demo/prototype
  payload was downloaded during this pass.
- Search hits for **Hercules: The Legendary Journeys**, Hercules mainframe
  emulators, and Hercules MMORPG servers are unrelated and were excluded.

Next research should start with newly recovered source paths and diagnostic
names, compare them against other Eurocom builds, and inspect a provenance-backed
PC demo or PSX demo for surviving debug records. This avoids repeating broad
title searches while the supplied binaries already provide stronger evidence.
