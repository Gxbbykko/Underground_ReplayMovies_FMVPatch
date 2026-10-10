# Need for Speed: Underground (2003) — ReplayMovies × FMVFix

Technical documentation for three connected reverse-engineering and cinematic-playback projects targeting **Need for Speed: Underground (2003), PC**.

This repository separates project documentation into named Git branches. The branches are permanent areas of documentation, **not feature branches intended for automatic merging**.

## Explore the projects

| Branch | What it does | Engineering status |
| --- | --- | --- |
| [ReplayMovies](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/ReplayMovies) | Adds **REPLAY MOVIES** to the native Main Menu, with career-cinematic selection and profile-aware unlocks. | Feature-complete; maintenance only. |
| [FMVFix](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/FMVFix) | Adapts Xbox-origin `.mad` playback to the PC game's existing cinematic engine. | Compatibility implementation completed; integrated runtime baseline preserved. |
| [FMVPatch](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/FMVPatch) | Documents playback substitution using **previously AI-upscaled** 1080p/4K HEVC MP4 movies. | Integrated runtime validated, with a known selection-order prewarm limitation. |

**There are two primary plugin implementations, not three:** ReplayMovies is its own system, while FMVPatch's video-substitution functionality belongs to the final FMVFix implementation. FMVPatch is a separate documentation area, **not a third standalone `.asi`**.

## How the systems relate

**ReplayMovies** extends the game's frontend using native menu structures. It exposes 17 career cinematics through a catalog, with separate User and Developer modes. User mode reflects movies encountered by the current profile; Developer mode allows catalog testing without manufacturing career progress. The automatic startup-intro sequence is outside the ReplayMovies career catalog.

**FMVFix** addresses the gap between Xbox movie assets and the PC movie subsystem while retaining the game's native cinematic start, timing, audio, skip, and teardown behavior.

**FMVPatch** supplements that foundation with an optional video layer. Its MP4 frames are matched to the native movie clock and presented through the existing rendering path. If replacement data is unavailable or late, the native `.mad` picture remains the fallback. AI processing is performed **offline** when creating the video assets; no live AI upscaling runs in-game.

## Recorded validation highlights

| Area | Evidence from the retained engineering record |
| --- | --- |
| ReplayMovies menu reconstruction | **15/15** fresh `MU_Main` reconstruction stress passes for the resolved descriptor-ownership issue. |
| ReplayMovies progression | Fresh-profile empty catalog, one-row behavior, profile persistence, natural/skip unlock commits, and native Trailers isolation recorded as closed. |
| FMVPatch 1080p | **20 movies** reported; **9,922 / 9,931** native movie-frame serials replaced by exact MP4 matches. |
| FMVPatch 4K | **15 movies** reported; **5,583 / 5,610** native movie-frame serials replaced by exact MP4 matches. |
| Playback errors in those reports | **0** decoder failures and **0** texture upload/binding failures in both reported groups. |

These figures are **test-run measurements**, not promises of universal performance, full 4K catalog coverage, or zero fallback under arbitrary selection order. Nine out-of-order selections exposed an initial-frame prewarm limitation; playback continued using native fallback until the asynchronous MP4 decoder caught up. See [FMVPatch validation](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/blob/FMVPatch/docs/VALIDATION.md) and [known limitations](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/blob/FMVPatch/docs/KNOWN_LIMITATIONS.md).

## Compatibility scope

The documented reverse-engineering baseline is the **English PC v1.4 32-bit (`x86`) executable**. Other executables, languages, distributions, graphics wrappers, and combinations of third-party fixes are not presumed compatible unless separately tested.

## Publication and downloads

- **GitHub:** technical documentation and project history. C++ source files, Visual Studio projects, project-specific configuration examples, and debug archives remain unpublished until expressly approved.
- **Nexus Mods:** intended home for ready-to-install compiled plugins and their support information. Links will be added when actual mod pages are live.
- **Game and movie assets:** no original Xbox/PC `.mad` assets, game executables, proprietary data, or upscaled MP4 movie files are distributed in this repository.

The project builds on investigation of the original game and references work by the **Extra Options** and **Widescreen Fix** communities. Those are separate third-party projects; their authorship should not be confused with the code maintained here. Precise compatibility and acknowledgments will be expanded once individually reviewed.

## Reading the engineering documentation

These pages separate **runtime-confirmed**, **statically established**, **inferred**, and **superseded/failed** findings. Historical debugger experiments may reflect intermediate states that were later resolved. The latest validated checkpoints, not older troubleshooting notes, determine current status.

Start with the README on each named branch, then follow its technical-documentation index. `main` remains a public navigation and publication hub.

## License and rights

See [LICENSE](LICENSE) for the repository's published license text. Third-party trademarks, game executable code, and copyrighted video assets remain the property of their respective rights holders; a repository license does not grant rights to distribute those assets.
