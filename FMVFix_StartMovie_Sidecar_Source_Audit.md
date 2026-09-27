# FMVFix — StartMovie / AI Sidecar Source Audit

## Relevant implementation chain

The current source identifies the build as:

```text
FMVFix CLEAN CXX20 STARTMOVIE-BRIDGE + AICACHE0D-STARTMOVIE-NAME-BINDING 2026-09-27
```

The functional chain is:

```text
FMVFix::Initialize
  -> InstallBridge
       -> StartMovie-local capability hook
            -> movie generation increment
            -> FMVFix_OnStartMoviePath(resolvedMoviePath)
                 -> PrepareAIC0ForStartMovie(resolvedMoviePath)
                      -> extract basename
                      -> accept controlled target PSA
                      -> resolve MovieSR\PSA.srpack
                      -> validate/load SRPACK
                      -> latch current movie generation
  -> InstallRendererEnhancement
       -> qualified FMV DrawPrimitiveUP interception
            -> target MovieTexture private UnlockRect clock
            -> decodedFrameSerial
            -> sidecar frame decodedFrameSerial
            -> temporary enhanced texture bind
            -> original texture restored after draw
```

## Exact source functions that implement the mechanism

- `FMVFix_HookStartMovieCapability()` — stable StartMovie bridge; increments the movie-session serial and forwards native argument 1 (`resolvedMoviePath`) to the AI-cache selector while preserving the proven compatibility publication.
- `FMVFix_OnStartMoviePath()` — thin callback into the AI-cache preparation path.
- `PrepareAIC0ForStartMovie()` — copies the native path, extracts the basename, accepts the controlled `PSA` target, resolves `MovieSR\PSA.srpack`, preloads/validates the pack and latches the current StartMovie generation.
- `LoadAIC0Pack()` — validates the diagnostic SRPACK header and exact file size before exposing frame data.
- `InstallAIC0FrameClock()` — installs a private shadow vtable on the exact target `IDirect3DTexture9` object.
- `AIC0HookUnlockRect()` — calls the original `UnlockRect` first, then increments the decoded-frame serial on successful level-0 updates of the exact target MovieTexture.
- `FMVFix_HookDrawPrimitiveUP()` — qualifies the native FMV draw, maps decoded serial directly to sidecar frame index, uploads/binds the enhanced texture, draws, and restores the original movie texture.
- `EnsureAIC0Texture()` — creates/updates the 1024×512 `D3DPOOL_MANAGED` presentation texture and avoids redundant re-upload of the same cached frame.
- `BuildAIC0AdjustedVertices()` — remaps the native 512×256 FMV UV half-texel convention to the 1024×512 sidecar texture.
- `ReleaseAIC0Texture()` — releases FMVFix-owned presentation resources when the controlled target session ends.

## Current diagnostic constants enforced by source

```text
Target movie             PSA
Sidecar                  MovieSR\PSA.srpack
Sidecar magic            NFSUSR1
Version                  1
Resolution               1024x512
Frame count              16
Header                    64 bytes
Frame bytes              1024 * 512 * 4
Native canonical texture 512x256
Runtime inference         none
Per-draw file I/O         none
```

## What this proves about the intended FMV AI-upscale architecture

The current FMVFix AI path is **offline-AI / runtime-sidecar presentation**. It is not currently an in-game neural inference path.

The `.mad` stream remains responsible for:

```text
decoding
frame progression
audio
timing
movie lifetime
```

The sidecar is responsible only for supplying an already-prepared higher-resolution visual frame for the same decoded-frame index.

## Important controlled-probe limitations

The source is intentionally not yet a generalized final player:

- only `PSA` is targetable;
- only one sidecar filename is resolved;
- only 16 frames are accepted;
- the full small pack is loaded synchronously at StartMovie;
- the target generation is armed once;
- the private MovieTexture frame-clock hook is installed once;
- cached presentation ends after decoded serial 15 and falls back to vanilla.

A final all-movie implementation needs movie-name-derived sidecar discovery, bounded prefetch/streaming, clean per-session vtable restoration/retargeting, and repeatable sidecar arming for every compatible FMV.

## Validation boundary from supplied artifacts

The source archive includes a successful Release build log. The supplied debugger notebook records 0C synchronization/indexing as runtime-confirmed, then defines the 0D StartMovie-name test and expected log fields. No `FMVFix_AICache0D.txt` runtime log is present in the supplied source archive.

Therefore:

```text
AI-Cache 0D implementation = present
AI-Cache 0D build          = confirmed
AI-Cache 0D runtime pass   = not evidenced by the supplied artifacts
```
