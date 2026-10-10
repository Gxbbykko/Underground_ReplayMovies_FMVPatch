# FMVPatch prewarm optimization

## Purpose

Initial movie frames may be requested before an asynchronous HEVC decoder has produced the matching MP4 picture. Startup prewarming is designed to make early frames ready so the native MAD clock can advance without a visible substitution gap.

The integrated runtime retains **native MAD fallback** if an exact replacement is not ready. It never treats a cached frame for a different movie as a valid substitute.

## Recorded Stage 2.5.3 policy

The observed prewarm strategy predicts a next movie in catalog order. That works when the actual next play matches the prediction, but an arbitrary ReplayMovies selection can violate it.

A recorded example is:

| Predictor expected | User selected | Consequence |
| --- | --- | --- |
| `07_GOD` → `08_EDD` | `07_GOD` → `11_PNK` | Startup cache for `08_EDD` could not be adopted for `11_PNK`; the correct asynchronous decoder took over. |

This is an **identity mismatch**, not evidence of a decoder failure. The MP4 format validation passed for the nine affected titles.

## Measured impact

In the reviewed archive, nine out-of-order titles had startup fallback:

- **1080p:** one initial native MAD serial per affected movie (**9 serials** total).
- **4K:** three initial serials per affected movie (**27 serials** total).
- After the startup window, exact replacement progressed normally without recorded decoder or texture-upload failures.

The 1080p all-catalog report lists 20 movies. The available 4K report lists 15 movies.

## Proposed future direction

The notebook recommends identifying an actual movie-selection event early enough to prewarm **the selected title** before the movie-start call, rather than assuming the following catalog position. This is an architectural recommendation only; the recorded Stage 2.5.3 runtime data does not demonstrate implementation or final validation of that alternative.

The existing decoder, renderer, clock mapping, and native fallback should be treated as stable while any such improvement is investigated.

Related: [Validation](VALIDATION.md) · [Known limitations](KNOWN_LIMITATIONS.md)
