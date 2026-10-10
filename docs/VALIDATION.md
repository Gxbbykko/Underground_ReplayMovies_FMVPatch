# ReplayMovies validation

## Final status

The final chronological engineering record classifies ReplayMovies as **feature complete / maintenance only**. That status follows debugger-level cause analysis, source-level corrections, and runtime checks; it does not erase historical failures documented earlier in the investigation.

## Validated areas

| Area | Recorded result |
| --- | --- |
| Main Menu integration | Seventh selectable native row, highlight/presentation, and submenu routing closed. |
| Repeated menu construction | **15/15** fresh `MU_Main` reconstruction stress passes for the semantic descriptor-ownership fix. |
| Geometry/ownership issue | Intermittent missing ReplayMovies option corrected; universal physical descriptor-offset assumption rejected. |
| Alternate lifecycle path | Race → Quit Game menu reconstruction bypass diagnosed and guarded by a secondary observer; later record marks it closed. |
| Movie return | Returning from `01_DAY` preserves/rebuilds ReplayMovies. |
| Fresh User profile | True empty catalog confirmed; native Trailers/NASCAR contamination resolved. |
| Unlock projection | `01_DAY` and `02_FAN_S` appear after their respective career encounters. |
| Persistence | Profile seen state records on natural movie completion and skip/early termination. |
| Developer catalog | All **17 career entries** accessible without falsely unlocking the User profile. |
| One-item catalog | One-row selector behavior verified independently of full-catalog behavior. |
| Replay-origin playback | Replaying a movie does not generate a new career unlock. |

## Specific ownership regression: 15/15

The failed earlier rule assumed that selected and unselected descriptors were always adjacent at a particular offset. Debugging demonstrated different valid allocation layouts. Resolving descriptors by their owning nodes corrected the missing-entry behavior on the failing invocation and then passed **15 fresh `MU_Main` generations** over the recorded reconstruction paths.

This figure measures **those stress cases**, not 15 distinct movie files or a universal compatibility benchmark.

## Historical states vs current conclusions

The notebooks retain failed attempts involving candidate memory ownership, geometry matching, zero-entry catalogs, alternate menu construction, and movie-return routes. They are valuable causal evidence, but their older OPEN/FAILED labels are superseded by the late closure entries described here.

## Qualification

The tested baseline is the English PC v1.4 32-bit executable. This public summary does not claim exhaustive coverage of every localization, game build, third-party frontend replacement, or external injector. No new tests were run solely to prepare this Markdown page.

Related: [Architecture](ARCHITECTURE.md) · [Profile progression](PROFILE_PROGRESSION.md)
