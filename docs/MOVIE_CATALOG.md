# Movie catalog

[← ReplayMovies overview](../README.md) · [Profile progression](PROFILE_PROGRESSION.md) · [Frontend integration](FRONTEND_INTEGRATION.md)

## Catalog in the game

![ReplayMovies submenu showing a list of available career films](../assets/screenshots/sub-menu.webp)

This screenshot documents the submenu presentation of the career-film list. It does not independently verify which entries would be unlocked for a new User profile; access remains governed by the progression rules described below.

## Catalog boundary

ReplayMovies presents **17 career cinematics**. The automatic introduction movies (`NA_BOOT`, `PSA`, `E3_TITLE`) belong to the wider native FMV system but **are not** ReplayMovies career entries. The separate FMVFix/FMVPatch playback catalog therefore counts **20 total films** (three intro + seventeen career) without contradicting ReplayMovies' seventeen-entry catalog.

## Career movie IDs and recorded presentation labels

The engineering notebook's 17-film catalog prototype records the following canonical ordering and titles. These labels are reproduced as **documented presentation names**; the private final catalog header remains the definitive source to check before localizing or changing public display text.

| Canonical position | Movie ID | Recorded title |
| ---: | :--- | :--- |
| 1 | `01_DAY` | Go Race! |
| 2 | `02_FAN_S` | Your Fantasy? |
| 3 | `02_FAN_E` | Get You Races |
| 4 | `04_GUM_S` | Get Respect |
| 5 | `05_ENG` | Engine of Darkness |
| 6 | `06_SAM` | Samantha's Uniques |
| 7 | `07_GOD` | Eddie: Race God |
| 8 | `11_PNK` | On the street, punk! |
| 9 | `13_KIT` | Goodbye, Samantha |
| 10 | `12_BUD` | Street Race Buddha |
| 11 | `09_GBE` | The Good, the Bad, the Eddie |
| 12 | `14_MUT` | Samantha's Cute Car |
| 13 | `16_RES` | This is Respect |
| 14 | `15_DIK` | Game Over |
| 15 | `08_EDD` | Wrath of Eddie |
| 16 | `17_MET` | Legend of Metal |
| 17 | `19_FIN` | Legend Ascendent |

> [!NOTE]
> The identifiers are logical movie IDs, **not** a promise that movies sort numerically by filename. The historical titles above preserve the notebook's spelling. Runtime display-name verification against the withheld final source should precede any translation or title corrections.

## Canonical identity vs visible row

The Developer catalog can expose all 17 canonical entries. User mode instead reads the active profile's persistent `SeenMask` and projects only encountered entries into a compact visible list. As a result, `visible row 1` does not necessarily mean `canonical movie 1`.

```text
Canonical films       Profile saw       Visible User catalog
01_DAY                yes               Row 1 -> 01_DAY
02_FAN_S              no                [hidden]
02_FAN_E              yes               Row 2 -> 02_FAN_E
04_GUM_S              no                [hidden]
05_ENG                yes               Row 3 -> 05_ENG
```

Catalog selection must keep that original identity through activation, native FMV playback, return and reconstruction. ReplayMovies-originated viewing does **not** set progression bits. Normal qualifying career viewing does.

## Historical frontend presentation

Early catalog prototypes experimented with the native ten-authored-row Trailers shell and multiple views; later work reconstructed safe fuller catalog selection and User compaction. Those prototypes explain the evolution, **not a current ten-movie cap**.

The records used to infer movie metadata and test entry ordering are retained privately; no original `.mad` or derived video content is published here.
