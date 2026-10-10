# Performance and measurements

[← FMVPatch overview](../README.md) · [Validation](VALIDATION.md) · [Known limitations](KNOWN_LIMITATIONS.md)

## What the existing measurements actually quantify

The available Stage 2.5.3 reports measure **native movie serials matched by replacement MP4 picture frames**, together with decoder and texture error counters. They are excellent evidence for substitution coverage but **not** a general GPU benchmark, CPU load test, latency study or Windows compatibility matrix.

| Metric | 1080p | 4K |
| :--- | ---: | ---: |
| Movies in report | 20 | 15 |
| Native serials | 9,931 | 5,610 |
| Exact MP4 picture matches | 9,922 | 5,583 |
| Unique initial serials not replaced | 9 | 27 |
| Runs with no fallback serial | 11 | 6 |
| Decoder errors | 0 | 0 |
| Texture upload/bind failures | 0 | 0 |

The matching proportions in these **specific report sets** are approximately **99.91% at 1080p** and **99.52% at 4K**. These are *coverage ratios*, **not** frame rates or a measure of perceived visual smoothness.

## What the data says

- Every movie included in the reports advanced on its native frame clock; the MP4 layer produced exact replacements for nearly all reported serials.
- Nine non-predicted selections caused **startup-only** missing substitutions; the correctly identified native MAD picture filled those gaps.
- **Zero** decoder and GPU texture errors were recorded in those particular runs.
- The 4K results represent **15 movies**, so their statistics cannot be generalized to a 20-movie 4K full-catalog test.

## What the data cannot establish

No complete public dataset here measures GPU utilization, startup latency in milliseconds, average presentation FPS, memory pressure, Windows HEVC codec availability, visual quality, or every graphics wrapper. A numeric frame-hit percentage should **not** be labeled an FPS or "100% perfect playback" score.

## Future benchmark methodology

For a publishable wider hardware benchmark, record GPU/CPU, Windows build, decoder availability, game version, exact FMV order, tier, display/framing configuration, serial hit coverage, initial misses, decoder/upload failures, and any human-observed tearing/audio drift. Keep source movies and personal debugging material private.
