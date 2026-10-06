# NF Processor Effects -- notes for Claude

Dual reverb / effects processor (16 reverbs, 6 delays, 8 modulation effects, 2 dbx-style dynamics, 4 routings, 99 factory + 99 user programs)
for NF Audio Tools, built from the owner's request and the manufacturer's manual of a 1U dual-reverb rack unit. Name: **NF Processor Effects** (owner's choice,
no third-party brand in the product). Bundle `com.nfaudiotools.nfprocessoreffects`, code `Nfpe`, manufacturer `Nfat`, licence product code `NF_PROCESSOR_EFFECTS`.

- Licence OFF by default (CMake `NFPE_LICENSE`), owner's rule for new plug-ins.
- DSP is JUCE-free in `Source/DSP/` (Common, Catalog, Reverbs, Delays, Modulation, Dynamics, Engine, Audition, Programs); `Tests/DspTests.cpp` checks it.
- The algorithms are our own: the manual describes controls and ranges only. Factory programs use the manual's list (effects + routing) with each effect's default knobs (no knob values published).
- UI: 1200x400 base (3:1), default window 810x270. The real unit is 1U (11:1) so the panel is re-flowed in two rows. All vector, no baked controls.
- Build (Intel Mac): `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64 -DFETCHCONTENT_SOURCE_DIR_JUCE=$HOME/JUCE -DNFPE_BUILD_SNAPSHOT=ON`
- Window: always opens at 990 x 330 (owner's request 2026-10-06: a bit larger than the family default 810 x 270; the size is NOT restored); a click on the NF logo returns to it after resizing.
