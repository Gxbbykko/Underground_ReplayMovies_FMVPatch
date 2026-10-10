# FMVPatch architecture

## What the runtime does—and does not do

FMVPatch presents **already-upscaled** video inside the PC game. The AI-upscaling work is an **offline asset-preparation stage**; at runtime the integrated FMVFix implementation performs HEVC/MP4 decoding, frame matching, and picture substitution. It does **not** synthesize new 1080p or 4K content using an AI model while playing.

The final implementation is a capability of **FMVFix**, not a third standalone plugin.

## Picture source and timing authority

There are two cooperating inputs:

1. **Native `.mad`:** movie identity, frame serial/timing authority, existing audio, skip, lifecycle transitions, and teardown.
2. **Prepared MP4:** optional replacement picture corresponding to the movie and selected 1080p/4K tier.

The replacement image is accepted only for the **matching movie and frame serial**. If the optional source is not ready, it is safer to leave the native MAD picture on screen than to present the wrong movie or an incorrectly timed frame.

## Startup and decode path

A start-of-movie prewarm path attempts to make initial replacement frames available before the native picture clock advances. Later frames are produced through an asynchronous decode path. Both must agree on movie identity, decoded-frame geometry, and serial mapping.

During the Stage 2.5.3 reporting run, some arbitrary ReplayMovies selections did not match the fixed predicted-next-movie order. Prewarming therefore contained another movie's frames and could not be reused. The fallback was deliberate; the correct decoder caught up after the initial one to three serials.

## HEVC picture geometry

The Stage 2.5.3 record describes handling of coded HEVC dimensions that include encoder padding. For example, reported 1080p-oriented coded frames may be **2176×1088**, while the valid visible picture is **2160×1080**. The correct display aperture/visible crop must be honored rather than guessed. The notebook reports the corresponding 2160×2160 square example for `E3_TITLE`.

The production-source design maintains validation of decoded frame dimensions and a shared visible-picture crop path for prewarm and normal decode. Invalid or unrecognized geometry fails safely to native MAD picture rather than showing padded or displaced output.

## Rendering, failure, and ownership

The integrated layer substitutes picture data through the existing rendering path while avoiding changes to native audio, movie completion, and return. The most recent production-source configuration retains quiet fallback for expected startup misses, while genuine decode, input, resource, and hook-installation faults receive category-deduplicated diagnostics. Detailed runtime report generation was disabled for that production-source variant.

A structural audit of this later variant does **not** by itself establish that it was newly compiled and played; the notebook explicitly notes the lack of native MSVC access during that audit.

Related: [Validation](VALIDATION.md) · [Prewarm optimization](PREWARM_OPTIMIZATION.md) · [Known limitations](KNOWN_LIMITATIONS.md)
