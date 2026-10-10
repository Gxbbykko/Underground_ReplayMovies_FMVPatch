# Engineering history

[← FMVPatch overview](../README.md) · [Architecture](ARCHITECTURE.md) · [Validation](VALIDATION.md)

The project evolved from Xbox `.mad` compatibility into an optional high-resolution picture integration **within FMVFix**. This page organizes the major discoveries; it is not a transcript of every private debugging experiment.

## Recorded evolution

| Milestone | Result |
| :--- | :--- |
| Native movie frame capture | Separated introduction and career movie capture paths; established reliable source frame counts and movie identity. |
| Offline data preparation | Validated sourcepack capture and prepared AI-upscaled video **outside** gameplay. |
| Renderer investigation | Found a native Direct3D9 presentation boundary suitable for substituting picture without replacing MAD clock/audio. |
| Exact serial mapping | Defined native movie identity + frame serial as the authority for every replacement. |
| 1080p / 4K HEVC integration | Decoded prepared MP4 alternatives as optional picture inputs. |
| Visible-aperture repair | Distinguished HEVC coded padding from the intended display rectangle. |
| Startup prewarming | Prepared predicted next-title startup frames to reduce first-frame misses. |
| Stage 2.5.3 validation | Measured 20 reported 1080p movies, 15 reported 4K movies, with documented initial fallback under out-of-order selection. |
| Quiet production source | Disabled routine reports and retained targeted errors for actual failures without changing the frozen Xbox compatibility functions. |

## Corrections that changed the design

**Pixel data needs a trustworthy observation boundary.** Early movie-class captures and later career post-upload captures were validated separately; treating any texture write as the authoritative movie frame risks miscounting or misidentifying images.

**MP4 does not own game time.** The native MAD serial, soundtrack and teardown remain authoritative; a separately timed external player would undermine synchronization.

**Coded HEVC size is not always visible picture size.** A corrected display aperture fixed framing while preserving frame mapping.

**Correct fallback beats wrong substitution.** When ReplayMovies skips the predicted movie, cached frames for the wrong title are ignored; initial serials fall back to native MAD until the real decoder catches up.

## Current evidence-based status

The Stage 2.5.3 runtime measurement is strong for the **reported** movie sets, and the working picture decoder/render path is considered stable. Zero-first-frame-fallback under arbitrary selection remains **unproven**. The later source-only and archived Windows build checkpoints must not be conflated with a new full-catalog gameplay test.

No source-derived videos, original game media or private code are included.
