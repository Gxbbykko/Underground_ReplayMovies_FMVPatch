# Build and reproducibility notes

[← FMVFix overview](../README.md) · [Validation](VALIDATION.md)

## Source and toolchain boundary

The private FMVFix project targets a **Win32/x86 ASI** for Need for Speed: Underground (2003), English PC v1.4. Source files, Visual Studio project definitions, real runtime INIs, game assets and generated binaries are **not available in this GitHub branch**, so this is not an actionable public build tutorial.

## Recorded build provenance

| Checkpoint | Evidence |
| :--- | :--- |
| Stage 2.5.3 playback | Runtime report-based picture substitution and frame matching are documented. |
| Quiet production source audit | Structural checks and frozen-function comparison performed; native MSVC unavailable **in that audit environment**. |
| Later project archive reviewed 2026-10-09 | Contains `FMVFix.asi` classified as x86 PE32, and an MSVC Windows Release build log reporting success. |
| New compilation for these docs | **Not performed**. |

The later archived build does not retroactively turn a source-only checkpoint into a live runtime test; similarly the earlier source-only caveat does not negate the separate successful build artifact.

## Eventual clean-build checklist

1. Confirm executable identity and the **32-bit** target; do not assume any `speed.exe` is compatible.
2. Reproduce the Win32 Release build from an explicitly approved, versioned source snapshot once publication is authorized.
3. Record Visual Studio toolset, library/runtime dependencies, relevant warnings and artifact checksums.
4. Verify Xbox mode **without** MP4 substitution, then each selected 1080p/4K picture tier and native fallback.
5. Exercise natural completion, skip, movie restarts, frontend return and integration with ReplayMovies.
6. Document any required/verified graphics wrapper settings; the particular `NA_BOOT` framing adjustment was tested against ThirteenAG's cropped-mode geometry.
7. Never include copyrighted `.mad`/MP4 movie files or private profile data in a public release.

## Distribution

The installer/download destination is intended to be **Nexus Mods**, once an actual listing is published. No speculative release URL, compiled artifact or source snapshot is linked here.
