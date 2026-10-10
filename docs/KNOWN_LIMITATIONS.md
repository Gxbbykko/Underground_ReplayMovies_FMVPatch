# Known limitations

[← FMVPatch overview](../README.md) · [Prewarm optimization](PREWARM_OPTIMIZATION.md) · [Validation](VALIDATION.md)

## Current verified limitations

| Area | What is known | Consequence |
| :--- | :--- | :--- |
| **Predicted next movie** | Stage 2.5.3 warms a fixed next-catalog title, not every possible arbitrary selection. | Nine unexpected selections temporarily used native MAD picture at startup. |
| **1080p startup frames** | Each affected title missed exact substitute serial **0**. | First picture can be native MAD; MP4 catches up afterward. |
| **4K startup frames** | Each affected title missed serials **0–2**. | First three pictures can be native MAD; MP4 catches up afterward. |
| **4K validation coverage** | Consolidated report contains **15 movies**. | Cannot describe this archive as a complete 20-movie 4K pass. |
| **Hardware envelope** | Reports do not certify all GPUs, HEVC decoders, OS versions, or graphics wrappers. | Test other configurations independently. |
| **Visual crop** | HEVC coded dimensions can include padding distinct from the visible aperture. | Correct crop metadata and tested presentation geometry matter. |

## Important interpretation

The nine initial fallback cases are **not** decoder crashes, soundtrack loss, a complete movie failure, or the earlier fixed HEVC display-aperture problem. In the reported sessions, decoder and texture-upload/binding error counts were **zero**, and the asynchronous decoder caught up.

Likewise, a fallback **draw attempt** count is not necessarily the number of unique movie serials without a substitute; one serial may be drawn more than once.

## Third-party framing scope

The `NA_BOOT` correction was made against the recorded **ThirteenAG cropped-mode** presentation geometry. Do not claim automatic support for every Widescreen Fix mode, release or other renderer wrapper from that specific observation.

## Build and evidence scope

The final production-configured source disabled verbose routine reporting and retained once-per-category diagnostics for genuine faults. The notebook's local source audit did not itself perform a fresh Windows compile; a separately supplied archive included a later Windows build artifact. No fresh gameplay pass was performed for these Markdown files.

## Proposed optimization — not completed validation

The logical next improvement is **selection-aware prewarm before movie start**, so the prewarm cache follows the actually chosen film instead of a fixed predicted next title. This remains a proposed change requiring implementation and regression tests, **not an already verified universal zero-miss feature**.
