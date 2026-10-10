# Prewarm optimization

[← FMVPatch overview](../README.md) · [Known limitations](KNOWN_LIMITATIONS.md) · [Validation](VALIDATION.md)

## Why prewarm exists

HEVC startup can take long enough that a movie's first native serial arrives before its replacement MP4 picture is decoded. The documented optimization prepares frames **ahead of** the expected next cinematic, so the native render path can substitute immediately when the prediction is correct.

## Recorded Stage 2.5.3 behavior

The implemented predictor follows a **fixed catalog order**. This works well for the predicted next movie but does not inherently know the title the user may choose next from ReplayMovies.

```text
Predicted order: 07_GOD → 08_EDD
Actual selection: 07_GOD → 11_PNK

Cached startup frames: 08_EDD
Active movie:          11_PNK
Result: refuse mismatched cache → native MAD picture → decoder catches up
```

This refusal is correct: a stale movie's frames must never be misrepresented as the current movie's frames.

## Measured impact

| Group | Total movies in report | Zero-fallback full runs | Affected out-of-order runs | Initial unique serials not substituted |
| :--- | ---: | ---: | ---: | ---: |
| 1080p | 20 | **11** | **9** | **9** |
| 4K | 15 | **6** | **9** | **27** |

In the nine affected sessions, **1080p fell back for serial 0**, while **4K fell back for serials 0–2**; later MP4 frames matched. Reported decoder and texture upload/binding errors were zero in both groups.

The affected titles were: `11_PNK`, `13_KIT`, `12_BUD`, `09_GBE`, `14_MUT`, `16_RES`, `15_DIK`, `08_EDD` and `17_MET`.

## Proposed improvement — not a shipped guarantee

The notebook proposes observing **the actual selected film before native `StartMovie`** and directing prewarm to that film rather than predicting a fixed next entry. That is a sensible **future optimization proposal**, not something the Stage 2.5.3 reports demonstrate as implemented or tested.

The working decoder/renderer should remain stable until any change has its own reproducible before/after evidence. No claim of universal zero-first-frame fallback is made.
