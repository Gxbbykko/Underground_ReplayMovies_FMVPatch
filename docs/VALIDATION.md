# FMVPatch validation

## Stage 2.5.3 reported results

The final consolidated runtime-report review covers **20 films at 1080p** and **15 reported films at 4K**. Both groups track the original native MAD movie frame serials and the exact MP4 serials substituted at the presentation boundary.

| Measurement | 1080p | 4K |
| --- | ---: | ---: |
| Movies in reviewed reports | 20 | 15 |
| Native MAD frame serials | 9,931 | 5,610 |
| Exact MP4 frame hits | 9,922 | 5,583 |
| Initial serials not substituted | 9 | 27 |
| Decoder failures | 0 | 0 |
| Texture upload/binding failures | 0 | 0 |

**Interpretation:** In the reported runs, 9,922 out of 9,931 1080p native serials and 5,583 out of 5,610 4K native serials received their exact MP4 picture substitute. The remaining initial serials used native fallback. The 4K column is not a complete 20-movie 4K validation claim.

The report review also identifies **11/20** 1080p movies and **6/15** 4K movies with full-session zero-fallback results. The other nine movies had startup misses only and then caught up.

## Why nine movies had startup fallback

The fixed-order predictor prewarmed the next expected catalog item, while the test operator sometimes selected another movie in ReplayMovies. For example, the prewarm path anticipated `07_GOD → 08_EDD`, but the actual next selection was `11_PNK`. The prepared frames belonged to the wrong movie, and the runtime appropriately rejected them and started the correct asynchronous decoder.

The nine affected movie IDs in the report are:

`11_PNK`, `13_KIT`, `12_BUD`, `09_GBE`, `14_MUT`, `16_RES`, `15_DIK`, `08_EDD`, `17_MET`.

For those nine runs:

- **1080p:** serial `0` used the native picture; subsequent MP4 frames matched.
- **4K:** serials `0`, `1`, and `2` used native picture; subsequent MP4 frames matched.
- **Decoder and texture error counts:** remained zero in the reported groups.

A separate draw-attempt fallback counter can exceed the count of unmatched **unique frame serials**, because one serial may be requested by more than one draw attempt. These measurements must not be conflated.

## Important geometry verification

Earlier 1080p-stage work encountered HEVC coded-frame padding. The later report confirms a visible-display-aperture correction and acceptance of validated crop dimensions (including a 2176×1088 coded / 2160×1080 visible example). Do not classify the nine startup misses as a recurrence of that older geometry problem; in the affected runs the MP4 format validation passed.

## What has not been demonstrated here

- A **20-of-20** 4K movie-report set is not present in this result.
- **Zero initial fallback** for arbitrary ReplayMovies selection order was **not** shown.
- The later quiet-production source edit was checked structurally, but was **not** newly compiled and run in the notebook's audit environment.
- These logs do not establish universal performance characteristics for every GPU, Windows configuration, game executable, or HEVC installation.

## Evidence scope

This page transcribes measured results and recorded interpretation from the final `Reports(6)` / Stage 2.5.3 notebook review. It adds **no new runtime tests** and distributes **no source-derived frames or movie files**.

Related: [Known limitations](KNOWN_LIMITATIONS.md) · [Prewarm optimization](PREWARM_OPTIMIZATION.md)
