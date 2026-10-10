# Frame synchronization

[← FMVPatch overview](../README.md) · [MP4 playback](MP4_PLAYBACK.md) · [Validation](VALIDATION.md)

## Native movie serials are authoritative

FMVPatch does not calculate the visible movie position by starting a second video timer and hoping it stays aligned. The native `.mad` session supplies a **movie identity and frame serial**, and only the corresponding MP4 picture is eligible for substitution.

```text
Native MAD:  movie = 01_DAY, serial = N
Prepared MP4: movie = 01_DAY, decoded serial = N
                          ↓
                 Exact identity match
                          ↓
                 Replacement picture

Wrong movie, wrong serial, or not ready → native MAD picture
```

This is a tighter contract than nominal FPS or container duration: two movies can have similar nominal timing and still contain unequal frame counts, reorderings or visible-frame geometry.

## Source-frame accounting

The offline source workflow validated source frame counts against native capture checkpoints. For example, the notebook records `01_DAY` with **536** career source frames, and separate introductory captures for `NA_BOOT`, `PSA` and `E3_TITLE`. The runtime reports then count **distinct native serials** for which an exact MP4 frame reached the presentation boundary.

## Unique serials are not draw attempts

A native serial can be encountered by **more than one draw attempt**. Therefore `ExactFrameMissesFallbackToMAD`-style draw counters may be higher than the count of **unique frame serials** that never received an MP4 substitute. The Stage 2.5.3 table reports unique serials: 9 missed at 1080p and 27 at 4K across affected sessions.

## Startup and catch-up

The asynchronous decoder may not have serial zero ready if the selected movie differs from the prediction that was prewarmed. A frame from the predicted **wrong** movie must not be substituted. Native fallback is then used until the correct decoder catches up. In the affected reports:

- 1080p: missing substitution only for serial `0` of each of nine affected movie runs.
- 4K: missing substitution only for serials `0`, `1`, `2` of each of nine affected runs.

The remaining reported frames matched without decoder or texture upload/binding errors. See [Prewarm optimization](PREWARM_OPTIMIZATION.md).
