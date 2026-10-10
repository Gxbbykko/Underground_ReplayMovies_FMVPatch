# FMVPatch

**Engineering status:** Integrated 1080p/4K cinematic-substitution path validated; documented startup-prewarm limitation.

FMVPatch is the documentation area for playback of **previously AI-upscaled** Need for Speed: Underground (2003) cinematic video through the final FMVFix runtime. The implementation decodes MP4 picture frames and substitutes them at the native movie's presentation boundary. It does **not** perform AI upscaling live and is **not** a separate third `.asi`.

## Playback design

The original `.mad` movie remains responsible for the game's **frame clock, movie identity, audio, skip, and teardown lifecycle**. The optional HEVC MP4 pipeline prepares replacement image frames for 1080p or 4K display; only frames matching the current native movie serial are substituted. Missing, late, or unavailable replacements leave the original MAD picture active.

The work includes offline preparation, source-to-target frame accounting, decoded-frame availability, renderer integration, frame matching, and startup prewarming. MP4 files and their source-derived captures are **not hosted on GitHub**.

## Last measured validation

| Measurement | 1080p | 4K |
| --- | ---: | ---: |
| Movies represented by reports | **20** | **15** |
| Native MAD frame serials | **9,931** | **5,610** |
| Exact MP4 frame matches | **9,922** | **5,583** |
| Initial serials not substituted | **9** | **27** |
| Decoder failures | **0** | **0** |
| Texture upload/binding failures | **0** | **0** |

The 1080p report covers all **20 movies in the supported catalog**: three intro titles and 17 career cinematics. The available 4K report covers **15 movies**; it is not evidence of a full 20-movie 4K runtime pass.

**Known limitation:** Nine movies were selected outside the fixed predicted prewarm order. In those runs, the first native serial (`0`) fell back in 1080p; 4K fell back for serials `0`–`2`. Afterward the asynchronous decoder caught up, with no reported decoder or texture errors. This limitation is recorded, not concealed.

## Documentation

| Subject | Page |
| --- | --- |
| End-to-end runtime | [Architecture](docs/ARCHITECTURE.md) |
| Video preparation | [Upscaling pipeline](docs/UPSCALING_PIPELINE.md) |
| Playback pipeline | [MP4 playback](docs/MP4_PLAYBACK.md) |
| Runtime tiers | [Video modes](docs/VIDEO_MODES.md) |
| Rendering boundary | [Renderer integration](docs/RENDERER_INTEGRATION.md) |
| Frame matching | [Frame synchronization](docs/FRAME_SYNCHRONIZATION.md) |
| Startup handling | [Prewarm optimization](docs/PREWARM_OPTIMIZATION.md) |
| Validation | [Validation](docs/VALIDATION.md) |
| Documented limits | [Known limitations](docs/KNOWN_LIMITATIONS.md) |
| Performance notes | [Performance](docs/PERFORMANCE.md) |
| History | [Engineering history](docs/ENGINEERING_HISTORY.md) |

Specialized pages not expanded in this initial detailing pass remain editorial outlines rather than invented technical claims.

## Distribution

The MP4 integration belongs to **FMVFix**. Its C++ implementation, the unreleased INI, compiled plugins, captured video assets, and upscaled MP4 files remain private. User-ready plugins are intended for **Nexus Mods**, not GitHub.

[Main repository hub](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/main) · [FMVFix](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/FMVFix)
