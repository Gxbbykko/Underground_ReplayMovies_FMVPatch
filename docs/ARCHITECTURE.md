# FMVFix architecture

## Engine compatibility boundary

The PC game already has a native cinematic lifecycle. FMVFix retains that lifecycle and adapts the platform-specific assumptions encountered when Xbox-origin `.mad` movie assets are used. It does not replace the game with an external movie player.

The investigation began with **matched PC and Xbox cinematic requests**. The same game, logical movie, frontend state, and native start operation were used to isolate the first meaningful divergence, rather than guessing that file size or bitrate was necessarily the cause.

## Native pipeline

At a high level, the documented native execution path is:

1. Frontend or startup sequence submits a movie request to the native movie-start function.
2. The engine allocates/activates a movie object and starts its file/worker stream.
3. Buffered `.mad` data enters native chunk classification and stream handling.
4. Decoder and presentation resources are used for frames and soundtrack behavior.
5. End, skip, or return invokes native teardown and restores the frontend.

The project followed this path using both debugger control comparisons and later runtime checks. An Xbox movie entering a black-screen FMV state was **not** treated as proof that the file had been wholly rejected: file acceptance, parser progress, decode, and presentation were tested as distinct stages.

## Stable integration contract

The final source record retains **six frozen Xbox-to-PC compatibility functions** while the optional MP4 functionality evolves around them. Its intent is to keep the proven compatibility layer independent of later image-quality and prewarming changes.

The native `.mad` remains the source of cinematic **timing, frame serials, audio, skip behavior, and teardown**. MP4 video substitution, when requested, changes the displayed image but does not redefine the game's movie clock. A missing/late substitute frame does not break the movie: the original native picture remains the fallback.

## Runtime modes

The engineering record describes these `FMVFix.ini` selections:

| Mode | Role |
| --- | --- |
| `Xbox` | Native Xbox `.mad` compatibility, **no MP4 replacement**. |
| `1080p` | Use pre-prepared 1080p MP4 picture substitution when an exact matching frame is available. |
| `4K` | Use pre-prepared 4K MP4 picture substitution when an exact matching frame is available. |

These mode names are documented, but the actual INI and private C++ implementation are not published in this repository.

## Evidence categories

Older crashes, failed parser probes, and provisional decoder hypotheses appear in the private investigation chronology. This public architecture summary describes the final validated model; it does not treat those historical failures as current runtime defects.

Related: [Validation](VALIDATION.md) · [FMVPatch architecture](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/blob/FMVPatch/docs/ARCHITECTURE.md)
