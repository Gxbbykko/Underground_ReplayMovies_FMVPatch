# ReplayMovies profile progression

## Two access modes

**User mode** reflects the cinematics encountered by the **active profile**, not the full career catalog. A new profile with no eligible movie encounters should therefore display an **empty** ReplayMovies list.

**Developer mode** exposes all valid entries in the 17-movie canonical catalog for testing. Developer access does not write artificial unlocks to the profile's real progression record.

Mode selection is part of the ReplayMovies configuration rather than an extra in-game User/Developer menu option. The actual configuration INI is intentionally withheld from this public repository pending review.

## Encounter-to-catalog pipeline

The final engineering record documents this high-level flow:

1. A qualifying career/native FMV is encountered by the active profile.
2. Its canonical catalog identity is recorded in the profile's persistent seen-state mask (`SeenMask`).
3. The User catalog is rebuilt as a compact projection of **seen** canonical movie entries.
4. Selecting one of those visible entries launches the corresponding native FMV.
5. Returning from playback preserves the correct canonical identity and prior unlock history.

The persisted record is called `ReplayMoviesProgress.ini` in the engineering log. This runtime progress file is **not** a distributable configuration example.

## When progress is committed

The late validation record confirms persistence at FMV termination after **both natural completion and skipping/early termination**. The same record confirms that ReplayMovies-originated movie launches are excluded from these unlock commits. Viewing a movie again does not unlock an otherwise unseen career movie.

The final regression checks cite `01_DAY` and `02_FAN_S`: each entered the visible User catalog only after its corresponding career movie had been played.

## Empty, single-entry, and native Trailers cases

- **Zero entries:** a fresh profile retains a genuine empty catalog, with no accidental native Trailers row or NASCAR movie imported into ReplayMovies.
- **One entry:** safe navigation and selection were established without requiring a placeholder second entry.
- **Multiple entries:** the compact visible order remains linked to canonical movie identities.
- **Back navigation:** the frontend returns to Main Menu without manufacturing movie state or progression.

Earlier notebook passages describe bugs in these states. The late confirmed validation closes them; they are historical failures, not current defects.

## Scope

This document describes observed project behavior. It does not publish personal profile files, the project's private implementation, exact hook code, or the unreleased configuration INI.

Related: [Architecture](ARCHITECTURE.md) · [Validation](VALIDATION.md)
