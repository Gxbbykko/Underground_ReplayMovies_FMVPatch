# Frontend integration

[← ReplayMovies overview](../README.md) · [Architecture](ARCHITECTURE.md) · [Lifecycle](LIFECYCLE_AND_HOOKS.md)

## Screenshots of the completed frontend

| Native menu integration | Career-film selection |
| :---: | :---: |
| ![REPLAY MOVIES added to the native Main Menu](../assets/screenshots/main-menu.webp) | ![The ReplayMovies submenu displaying movie choices](../assets/screenshots/sub-menu.webp) |

The left capture shows the existing Main Menu with **REPLAY MOVIES** as a native selectable option. The right capture shows its dedicated film-selection page. These are UI examples; empty-profile and one-row behavior are separately established by the [validation record](VALIDATION.md).

## A native Main Menu entry

The sixth-to-seventh candidate expansion is the foundation of ReplayMovies. The stock entries remain intact, and **REPLAY MOVIES** occupies a real native selectable position in `MU_Main`, with the ordinary highlight, directional navigation and selection machinery. It is **not** a floating text element that merely imitates a menu row.

| Identity | Meaning |
| :--- | :--- |
| `ReplayMovies` | Logical name used to recognize and dispatch the new option. |
| `REPLAY MOVIES` | Frontend-facing label styled like existing uppercase items. |
| Candidate 7 | Native selection slot after the six original menu entries. |
| Fresh menu generation | The only lifetime within which new frontend pointers remain valid. |

The plugin constructs the native-style frontend objects and only publishes the candidate once their ownership and display data are valid. Incomplete construction keeps a safe native fallback instead of exposing an invalid candidate.

## Movie browser

The ReplayMovies entry opens a movie-selection interface using Underground's own frontend patterns and authored menu infrastructure. A historical prototype used the native Trailers/Options shell as a controlled way to reproduce selector and Back behavior. The final system also handles expanded catalog presentation and the **17 career films**, not merely the early first-movie test case.

User and Developer catalogs share canonical movie identities but differ in visibility. In User mode, unlocked films are compacted into visible rows while retaining the mapping to their original canonical IDs. Thus the selected *display row* must not be treated as the selected *movie identity*.

## Navigation and edge cases

- **Selection:** activating a visible catalog row resolves the correct canonical film and enters the native movie path.
- **Back:** returns to a fresh, correctly highlighted Main Menu without leaving detached or orphaned presentation state.
- **Zero entries:** a fresh profile displays no replayable career movies; Up/Down are inert and Back remains safe.
- **One entry:** navigation works without assuming a second item exists.
- **Trailers isolation:** the game's pre-authored Trailers/NASCAR presentation does not leak into an empty ReplayMovies catalog.

## Why geometry fixes mattered

Several earlier display defects were tied to incomplete frontend data, descriptor lifetime, or prematurely restored Main Menu state. The final descriptor fix uses semantic native ownership rather than an assumed `+0x118` memory distance. The post-FMV generation must reconstruct its own frontend state instead of retaining stale menu structures.

## Completion criteria

The notebook records native selection/return behavior, the repaired orphan highlight and Trailers isolation, the `01_DAY` return path, alternate Race → Quit Game construction, and **15/15** fresh menu-generation passes for the ownership defect. These are the verified targets of the finished implementation, not proposed UI work.
