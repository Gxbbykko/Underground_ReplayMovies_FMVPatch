# Chapter II — FMVFix

**Status:** Completed Xbox `.mad` compatibility engineering milestone; documentation chapter.

FMVFix addresses Xbox movie compatibility within Need for Speed: Underground (2003)'s existing PC FMV lifecycle. The engineering record covers container/parser admission, decode-path divergence, audio handling, presentation, native teardown, and final integration. The current integrated FMVFix implementation also contains the separate FMVPatch video-substitution functionality described in Chapter III.

## Documentation index

- [Architecture](docs/ARCHITECTURE.md)
- [Xbox vs PC](docs/XBOX_VS_PC.md)
- [MAD format findings](docs/MAD_FORMAT.md)
- [Parser and decoder](docs/PARSER_AND_DECODER.md)
- [Native lifecycle](docs/NATIVE_LIFECYCLE.md)
- [Compatibility fixes](docs/COMPATIBILITY_FIXES.md)
- [Engineering history](docs/ENGINEERING_HISTORY.md)
- [Validation](docs/VALIDATION.md)
- [Build notes](docs/BUILDING.md)

## Source and distribution

The C++/Visual Studio source is **not public yet** while the repository is being finalized. No installable `.asi` is hosted on GitHub. Ready-to-install releases are reserved for Nexus Mods when published. Configuration examples remain under review and are not yet published.

[← Main project hub](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/main) · [Chapter III — FMVPatch](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/FMVPatch)
