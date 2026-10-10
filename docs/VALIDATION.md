# FMVFix validation

## Compatibility and implementation status

The final project record treats Xbox-to-PC `.mad` compatibility as implemented within the native Underground movie lifecycle. The later MP4 source audit reports all **six frozen compatibility functions unchanged** from the established baseline.

These are separate forms of evidence:

| Evidence | What it supports | What it does not prove |
| --- | --- | --- |
| PC/Xbox debugger comparisons | Identification of movie activation, stream parsing, presentation, and teardown boundaries. | Every Xbox cinematic works on every PC game executable. |
| Subsequent runtime movie reports | Frame-clock progression and successful integrated picture substitution for the sampled movie set. | Pixel-perfect visual assessment on all displays and all tiers. |
| Frozen-function source audit | The six compatibility functions were retained during later production configuration changes. | A new native Windows/MSVC binary was compiled and played in that audit environment. |

## Last reported integrated test scope

The Stage 2.5.3 integrated movie-validation record reports:

- **1080p:** 20 movies, 9,931 native MAD frame serials, 9,922 exact MP4 frame hits.
- **4K:** 15 **reported** movies, 5,610 native MAD frame serials, 5,583 exact MP4 frame hits.
- **Decoder errors:** zero in the reported tests.
- **Texture upload/binding failures:** zero in the reported tests.

Those figures measure the optional **FMVPatch** picture layer running over the retained native compatibility system. They must not be described as a standalone Xbox `.mad` parser acceptance count. See the [FMVPatch validation](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/blob/FMVPatch/docs/VALIDATION.md) for the precise frame-hit interpretation and startup misses.

## Production-source qualification

The subsequent quiet-production configuration disabled detailed routine reports while retaining real-error messages and native fallback. The notebook records a structural/source-level audit and states that a **native MSVC build was unavailable** in that audit environment. That limitation applies to the later production-source audit, not retroactively to earlier Stage 2.5.3 playback reports.

## Supported baseline

English Need for Speed: Underground (2003) PC v1.4 **32-bit**. Other executable variants require independent validation.

No new runtime tests were conducted for this documentation revision.

Related: [Architecture](ARCHITECTURE.md)
