# ReplayMovies

**Engineering status:** Feature complete; frozen for maintenance and compatibility fixes.

ReplayMovies integrates a native-style **REPLAY MOVIES** item as the seventh entry in the Need for Speed: Underground (2003) PC Main Menu. The work covers menu construction, selection and highlight navigation, a career-movie submenu, profile-aware catalog projection, and native FMV launch and return.

## What is implemented

- **Native Main Menu integration:** six original entries retained; a seventh option is created in the current frontend generation rather than drawn as a detached overlay.
- **Native selection and return:** the option participates in the game's selector, navigation, menu presentation, and normal movie teardown.
- **17 career cinematics:** the ReplayMovies catalog is separate from automatic startup-intro playback and from the native Trailers catalog.
- **User mode:** only movies actually encountered by the active profile appear; the catalog is compacted from the canonical entries.
- **Developer mode:** the full valid career catalog is available for inspection without falsely unlocking profile progression.
- **Safe edge cases:** empty fresh-profile catalogs, one-row menus, skip/end transitions, Back navigation, and repeated main-menu reconstruction are covered by the final validation record.

## Runtime model

The menu is built from the **current** `MU_Main` generation. The game owns the expanded seven-entry candidate allocation; ReplayMovies attaches native frontend objects and publishes the new entry only after construction succeeds. Previously observed pointer reuse between menu generations is **not** a basis for caching native object pointers.

The movie launch remains on the native Underground FMV path. ReplayMovies-originated playback does **not** award career unlocks. Separately, eligible normal career FMV endings (including skips) update the per-profile seen state.

## Final recorded validation

**15/15 fresh `MU_Main` reconstruction passes** confirmed the correction to an intermittent missing-option issue. Its cause was a false assumption about the physical offset between selected and unselected frontend property descriptors; the final implementation resolves their **semantic native ownership** instead. Later notebook checkpoints also close the alternate Race → Quit Game reconstruction route, `01_DAY` return, empty-catalog Trailers/NASCAR contamination, and User-mode persistence checks.

See [Validation](docs/VALIDATION.md) for the precise scope, and [Architecture](docs/ARCHITECTURE.md) for the ownership and generation rules. This is an engineering completion statement, **not** a claim that every possible game build or external mod combination has been tested.

## Documentation

| Topic | Page |
| --- | --- |
| Design and ownership | [Architecture](docs/ARCHITECTURE.md) |
| Native Main Menu UI | [Frontend integration](docs/FRONTEND_INTEGRATION.md) |
| Movie entries | [Movie catalog](docs/MOVIE_CATALOG.md) |
| Per-profile unlocks | [Profile progression](docs/PROFILE_PROGRESSION.md) |
| Playback return | [Lifecycle and hooks](docs/LIFECYCLE_AND_HOOKS.md) |
| Verification | [Validation](docs/VALIDATION.md) |
| Development history | [Engineering history](docs/ENGINEERING_HISTORY.md) |
| Build-publication status | [Build notes](docs/BUILDING.md) |

Several linked specialized pages remain editorial outlines pending a separate evidence review. The architecture, progression, and validation pages in this documentation pass contain sourced detail.

## Availability

The ReplayMovies C++ source, project files, and configuration examples are intentionally private for now. Compiled releases are reserved for **Nexus Mods** and are not distributed from GitHub.

[Main repository hub](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/main)
