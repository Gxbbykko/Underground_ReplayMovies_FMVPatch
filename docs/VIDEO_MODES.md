# Video modes

[← FMVPatch overview](../README.md) · [Architecture](ARCHITECTURE.md) · [Known limitations](KNOWN_LIMITATIONS.md)

The integrated **FMVFix** runtime recognizes three documented values of the private `[FMVFix] Mode` setting. These are runtime *picture policies*, not separate standalone ASI plugins.

| Mode | Optional MP4 picture | Clock / audio / skip | If the selected tier cannot supply an exact frame |
| :--- | :--- | :--- | :--- |
| `Xbox` | **Disabled** | Native MAD | Native MAD picture; no MP4 substitution. |
| `1080p` | Prepared **1080p** HEVC MP4 | Native MAD | Native MAD picture for that missing/late serial. |
| `4K` | Prepared **4K** HEVC MP4 | Native MAD | Native MAD picture for that missing/late serial. |

## What selecting a mode does not do

- It does **not** perform AI upscaling or encode video in real time.
- It does **not** switch to an independently scheduled MP4 player.
- It does **not** replace original `.mad` audio, game clock or FMV teardown.
- It does **not** guarantee the next movie's startup frames have been decoded, especially with arbitrary ReplayMovies selection order.
- It does **not** prove a fallback from **4K to 1080p**; the documented safe fallback is the **native MAD picture**. Do not claim undocumented automatic cross-tier behavior.

## Default and private configuration

The reviewed source archive documented `Mode=1080p` in its release-oriented INI and `Xbox` as a source fallback if no valid mode was read. This distinction should be checked for the exact version users ultimately install; the real configuration file is **not published here**.

## Choosing a tier

Higher-resolution MP4 decoding has different asset, codec and GPU requirements than native MAD picture playback. This repository does not provide a benchmark covering arbitrary hardware or graphics wrappers. The 4K report contains **15 movie runs**, so selecting 4K must not be described as a completed 20-film 4K compatibility certification.

Consult [Performance](PERFORMANCE.md) for the observed measurements and [Validation](VALIDATION.md) for the movie-set distinction.
