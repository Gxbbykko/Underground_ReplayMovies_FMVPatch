# Engineering history

[← ReplayMovies overview](../README.md) · [Architecture](ARCHITECTURE.md) · [Validation](VALIDATION.md)

This is a **curated chronology**, not a live defect backlog. The source notebooks preserve intermediate failures in detail; the final validated state takes precedence over older provisional conclusions.

## Major milestones

| Stage | Discovery and outcome |
| :--- | :--- |
| Native menu reconstruction | The original six-entry `MU_Main` candidate system can be extended using **game-owned** seven-entry storage. |
| Menu identity and presentation | The internal identity `ReplayMovies` is distinct from the native-style visible caption **REPLAY MOVIES**. |
| Frontend routing | The added row participates in native selection, highlights, submenu navigation and Back-to-Main-Menu restoration. |
| Career catalog | A canonical **17-film** movie list replaced the initial single-title proof of concept. |
| Per-profile projection | User / Developer visibility and profile-specific `SeenMask` persistence were implemented; natural and skip commits were validated. |
| Edge-case closure | Zero-row, one-row, Trailers/NASCAR leakage, orphan highlight and movie-return issues were resolved. |
| Generation observation | Alternate Race → Quit Game reconstruction path gained a guarded fallback observer. |
| Final descriptor fix | Incorrect universal physical `+0x118` spacing was replaced by **semantic native ownership**; **15/15 fresh generations** passed. |

## Lessons from failed approaches

**Native memory ownership is not optional.** Early candidate-vector publication through memory owned by the plugin conflicted with native destructor behavior. Later work kept candidate allocation and release native-owned.

**Address reuse is not lifetime continuity.** New `MU_Main` generations can occupy reused heap addresses. The runtime must rebuild its relationships per generation rather than trusting cached pointers.

**Physical adjacency was a misleading clue.** A real distance observed in some allocations (`0x118`) was mistakenly promoted to a universal rule. The final fix traces descriptor ownership instead of offset arithmetic.

**The catalog has two index spaces.** The row displayed to a User is not necessarily the canonical movie index. Compact projection, activation and post-FMV restoration all need to preserve canonical identity.

**Historical OPEN labels can be stale.** An early EOL summary was followed by further work on `01_DAY` return, NASCAR leakage and race-exit reconstruction. The **latest** notebook checks close those issues; describing them as still open would misrepresent the project.

## Final engineering policy

ReplayMovies is **feature complete and in maintenance mode**. Changes should be justified by a verified regression or compatibility need and should preserve the native frontend contracts rather than reopening solved architecture.

No private implementation sources, debugger journals or commercial movie assets are reproduced here.
