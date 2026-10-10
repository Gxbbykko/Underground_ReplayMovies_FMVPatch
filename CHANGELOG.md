# FMVPatch documentation history

[← FMVPatch overview](README.md) · [Engineering history](docs/ENGINEERING_HISTORY.md)

FMVPatch is a documentation area for the **FMVFix-integrated** picture-substitution system, not a separate binary release line.

## Unreleased — documentation refinement

- Expand offline source preparation, HEVC decode, visible-aperture geometry, exact frame synchronization, renderer integration and performance measurements.
- Keep the known fixed-order startup prewarm limitation explicit.

## 2026-10-10 — Detailing Stage 1

- Updated the README, architecture, prewarm, validation and known-limitation pages.
- Recorded **20-film 1080p** and **15-film 4K** Stage 2.5.3 test scopes.
- Documented **9** unique 1080p and **27** unique 4K initial fallback serials, with zero reported decoder/texture failures.

## 2026-10-10 — Public documentation foundation

- Added the `FMVPatch` documentation tree for integrated high-resolution playback.
- Explicitly distinguished **offline** AI-upscaled MP4 assets from any in-game AI operation.

Read [Validation](docs/VALIDATION.md) before interpreting these measurements as a universal compatibility or zero-fallback guarantee.
