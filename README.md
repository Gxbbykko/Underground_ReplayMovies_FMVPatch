# FMVPatch

**Status:** Completed upscale/playback integration milestone; documentation chapter.

FMVPatch documents integration of **previously AI-upscaled** cinematic videos with Underground's native FMV lifecycle. The playback layer uses 1080p/4K MP4 alternatives; it does **not** perform AI upscaling live during gameplay. The engineering record covers video decode, render substitution, synchronization, prewarming, fallback behavior, and final validation.

**Implementation relationship:** FMVPatch functionality is integrated in the final FMVFix implementation. This chapter is a technical division, **not a claim that a third standalone `.asi` exists**.

## Documentation index

- [Architecture](docs/ARCHITECTURE.md)
- [Upscaling pipeline](docs/UPSCALING_PIPELINE.md)
- [MP4 playback](docs/MP4_PLAYBACK.md)
- [Video modes](docs/VIDEO_MODES.md)
- [Renderer integration](docs/RENDERER_INTEGRATION.md)
- [Frame synchronization](docs/FRAME_SYNCHRONIZATION.md)
- [Prewarm optimization](docs/PREWARM_OPTIMIZATION.md)
- [Performance](docs/PERFORMANCE.md)
- [Known limitations](docs/KNOWN_LIMITATIONS.md)
- [Engineering history](docs/ENGINEERING_HISTORY.md)
- [Validation](docs/VALIDATION.md)

## Source and distribution

The integrated FMVFix/FMVPatch implementation source is **not public yet**. No compiled plugins or movie assets are hosted on GitHub. Public installable releases are reserved for Nexus Mods when published.

[← Main project hub](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/main) · [FMVFix](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/FMVFix)
