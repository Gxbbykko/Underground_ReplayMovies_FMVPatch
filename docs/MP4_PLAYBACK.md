# MP4 playback

[← FMVPatch overview](../README.md) · [Frame synchronization](FRAME_SYNCHRONIZATION.md) · [Renderer integration](RENDERER_INTEGRATION.md)

## Playback model

The optional runtime decodes **pre-prepared HEVC video** and exposes its frames as potential replacement pictures at Underground's **native Direct3D9 movie presentation boundary**. It does not replace movie-start requests, native soundtrack delivery, native frame counting, or FMV teardown.

1. The native movie subsystem starts the requested `.mad` film.
2. The selected video mode (`Xbox`, `1080p` or `4K`) determines whether any optional MP4 decoder should participate.
3. In MP4 modes, the runtime associates the **actual movie identity** with the prepared tier input and starts or reuses the corresponding decode state.
4. On a native draw opportunity, it looks for the **same frame serial** in the decoded video.
5. An available, valid exact match substitutes the MP4 picture; an unavailable or late match leaves the **native MAD picture** in place.
6. Natural completion, skip or restart remains governed by native movie lifecycle management.

## Why asynchronous decoding is useful

Modern HEVC decoding and source-video I/O need not finish synchronously at every native draw. Prepared startup frames can reduce first-frame waits, while a bounded catch-up path continues decoding after a movie starts. The critical safety rule is **never to show a decoded frame from the wrong movie or serial** just to hide a temporary miss.

## Expected fallbacks versus faults

| Event | Meaning |
| :--- | :--- |
| Selected MP4 frame not decoded yet | Expected fallback to native MAD for that serial. |
| Prewarm cache belongs to a different predicted movie | Discard/ignore mismatched cached picture, start correct decoder. |
| Movie skipped or restarted | Preserve native skip/teardown lifecycle; do not mistake expected interruption for failure. |
| Genuine input, decoder, texture, allocation or hook failure | Production source includes category-limited diagnostics while preserving fallback. |

Routine diagnostic reports were disabled in the later quiet-production configuration, without implying that the earlier Stage 2.5.3 measurements never occurred.

## Limits of the public claim

The consolidated tests show **20 movies represented at 1080p** and **15 at 4K**, not universal HEVC hardware compatibility or error-free playback on every Windows/GPU combination. See [Validation](VALIDATION.md).
