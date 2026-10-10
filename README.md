# Need for Speed: Underground (2003) — ReplayMovies × FMVFix

Public technical documentation hub for three connected engineering chapters developed for **Need for Speed: Underground (2003), PC**.

| Engineering chapter | Focus | Navigate |
| --- | --- | --- |
| **I — ReplayMovies** | Native seventh Main Menu option, career cinematic replay, and frontend lifecycle | [ReplayMovies branch](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/ReplayMovies) |
| **II — FMVFix** | Xbox `.mad` compatibility with Underground's PC movie subsystem | [FMVFix branch](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/FMVFix) |
| **III — FMVPatch** | Previously AI-upscaled 1080p/4K MP4 playback integration and frame synchronization | [FMVPatch branch](https://github.com/Gxbbykko/Underground_ReplayMovies_FMVPatch/tree/FMVPatch) |

## Publication status

The engineering notebooks record completed implementation and validation milestones. These GitHub branches are being prepared as a **documentation-first archive**. Their detailed documentation remains under editorial preparation.

**Source-code publication is deferred.** ReplayMovies and FMVFix C++ sources and Visual Studio projects are intentionally **not included** at this stage. FMVPatch's runtime implementation is integrated with FMVFix; the chapter names do not imply three separately distributed plugins.

**Compiled, ready-to-install plugins will be distributed through Nexus Mods**, not GitHub. Download links will be added when the relevant Nexus Mods pages have been published.

## Compatibility and asset policy

The reverse-engineering baseline is the **English PC v1.4 32-bit executable**. Other executable variants must not be assumed compatible without separate validation.

No original PC/Xbox `.mad` files, game assets, upscaled movie videos, or compiled `.asi` plugins are distributed here. Game and product names belong to their respective owners. Third-party acknowledgments will be expanded during documentation review.

## Repository layout

`main` is the publication/navigation hub. The three named branches are **engineering chapters**, not automatic merge candidates. Technical corrections and documentation contributions can be discussed using GitHub Issues and pull requests once the corresponding templates have been reviewed and enabled.
