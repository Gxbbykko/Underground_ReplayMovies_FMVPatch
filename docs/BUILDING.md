# Build and reproducibility notes

[← ReplayMovies overview](../README.md) · [Validation](VALIDATION.md)

## Publication status

The ReplayMovies **C++ implementation, project files and build configuration are not public** at this stage. This page documents what the historical record supports, not an executable step-by-step build tutorial for source files absent from the repository.

| Property | Documented evidence |
| :--- | :--- |
| Application baseline | Need for Speed: Underground (2003), **English PC v1.4 x86**. |
| Plugin architecture | Windows **32-bit ASI** (PE32 DLL compatible with the chosen loader). |
| Historical toolchain | Microsoft Visual Studio / MSVC, Win32 Release configuration in archived build material. |
| Archived build artifact | `ReplayMovies/Release/ReplayMovies.asi` observed as a valid x86 PE32 DLL in the private handoff. |
| Source availability | Withheld from GitHub pending explicit approval. |
| User package availability | Intended for Nexus Mods; not published by this repository. |

## What an eventual reproducible build should verify

1. Select the project's **Win32/x86** target; 64-bit builds do not satisfy the game's 32-bit injection environment.
2. Use the exact supported executable baseline for hook/signature validation, not merely any installation labeled "Underground".
3. Build from a **versioned, disclosed source snapshot** when source publication is authorized.
4. Record toolchain version, build logs, output fingerprint and deployment prerequisites.
5. Verify native menu construction, 17-film catalog access, profile persistence, skip/end behavior and representative menu generation stress paths in-game.
6. Never bundle EA assets, personal profile data or raw FMV captures with the plugin.

> [!CAUTION]
> An archived successful build is not proof of a newly reproduced build on another machine. This documentation pass did **not** compile or execute ReplayMovies.

## Release boundary

Once published, installable files and user support belong on the actual Nexus Mods page, not in a GitHub archive containing Visual Studio intermediates or game files. No speculative download URL is provided.
