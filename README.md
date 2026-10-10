# FMVFix

**Engineering status:** Xbox `.mad` compatibility framework completed; integrated FMV runtime preserved.

FMVFix adapts Xbox-origin cinematic `.mad` playback to Need for Speed: Underground (2003)'s **native PC movie subsystem**. The investigation traced differences through movie activation, buffered container parsing, stream/decode handling, picture presentation, and teardown. The result is an engine-compatibility layer, not an external player.

## Responsibilities

- Preserve the game's own **movie start, frame clock, audio, skip, and return lifecycle**.
- Accommodate Xbox-origin `.mad` data where the original PC path diverged.
- Keep the validated Xbox-to-PC compatibility functions stable while integrating optional picture substitution.
- Fail safely back to native cinematic presentation when an optional MP4 replacement is unavailable or late.

## Relationship with FMVPatch

[FMVPatch](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/FMVPatch) documents the **optional 1080p/4K MP4 replacement-video path** inside the final FMVFix implementation. It is not a third standalone plugin. Offline AI-upscaled movie assets are supplied separately by the user; the engine does **not** perform AI upscaling during playback.

The engineering record distinguishes three runtime selections in `FMVFix.ini`: `Mode=Xbox` uses the native compatibility path without MP4 substitution, while `Mode=1080p` and `Mode=4K` select the respective pre-prepared MP4 tiers. The real INI remains private pending publication review.

## Evidence and limits

The validated native lifecycle retains the original movie timing and normal teardown. In the later **Stage 2.5.3** movie-substitution audit, the decoder/renderer successfully processed a 20-movie 1080p set and a **15-movie reported 4K subset**. The last reported limitation is out-of-order startup prewarming, **not** a blanket assertion that every MP4 frame always replaces the native `.mad` frame.

The subsequent quiet production-source configuration disabled routine diagnostic reports and introduced category-deduplicated error dialogs. Its **source-level audit is recorded**, but the notebook explicitly states that a new native Windows/MSVC build was not performed in that audit environment. Do not interpret that source audit alone as a fresh production-binary runtime test.

## Documentation

| Subject | Page |
| --- | --- |
| Ownership and lifecycle | [Architecture](docs/ARCHITECTURE.md) |
| Platform comparisons | [Xbox vs PC](docs/XBOX_VS_PC.md) |
| Movie format | [MAD findings](docs/MAD_FORMAT.md) |
| Parser and decoder | [Parser and decoder](docs/PARSER_AND_DECODER.md) |
| Engine return path | [Native lifecycle](docs/NATIVE_LIFECYCLE.md) |
| Compatibility fixes | [Compatibility fixes](docs/COMPATIBILITY_FIXES.md) |
| Validation scope | [Validation](docs/VALIDATION.md) |
| Development record | [Engineering history](docs/ENGINEERING_HISTORY.md) |
| Build-publication status | [Build notes](docs/BUILDING.md) |

Some specialized linked pages remain editorial outlines pending targeted evidence review; this pass details the architecture and validation pages.

## Availability and compatibility

The public testing baseline is the **English PC v1.4 x86 executable**. FMVFix source, Visual Studio projects, real INIs, movie assets, and compiled `.asi` are **not** published on GitHub. Compiled user releases are planned for **Nexus Mods**.

[Main repository hub](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/main) · [FMVPatch](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/FMVPatch)
