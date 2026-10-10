# ReplayMovies architecture

## Native frontend, not an overlay

The original `MU_Main` runtime presents six native candidates. ReplayMovies extends the native candidate system to seven entries and makes its own entry participate in the existing frontend layout, selector, and dispatch. The extra option's logical name is `ReplayMovies`; its visible caption is **REPLAY MOVIES**.

The finished implementation follows native **object ownership**. The game owns the seven-entry candidate allocation. ReplayMovies constructs its frontend objects within the current menu generation and publishes the seventh entry only after the required native nodes and presentation data are ready. A fail-closed fallback preserves a safe native candidate if a generation is not fully assembled.

## Generation-local lifetime

A Main Menu return or a frontend transition can create a fresh `MU_Main` generation. Allocator addresses may be reused across generations; a matching address therefore does not prove a matching object lifetime.

Consequently, the implementation reacquires the live runtime, native candidates, related frontend nodes, and presentation descriptors for each qualifying generation. It does not retain stale frontend pointers across menu reconstruction. The final record covers ordinary returns and an alternate Race → Quit Game path that bypassed the primary completed-generation observer until a guarded fallback was added.

## Presentation descriptor ownership

An early design inferred that selected and unselected presentation descriptors always lived at a fixed physical separation. This was **superseded**. Some native allocations exhibit that separation, but other valid layouts do not.

The corrected implementation follows the owning subordinate nodes/property storage and pairs descriptors **semantically**. That change closed the intermittent missing-option issue and passed 15 fresh-generation stress runs without reopening the native menu construction model.

## Navigation and movie activation

The added item behaves as a native row and routes to a dedicated career-film submenu. Navigation, selection, an empty or single-item catalogue, Back-to-Main-Menu return, and native FMV teardown were included in completion checks. The native movie subsystem continues to own actual playback; the ReplayMovies frontend is a controlled invocation and catalog surface, not a replacement video decoder.

## Catalog design

There is one canonical career catalog of **17 movies**. A Developer view exposes valid canonical entries. A User view projects only the active profile's encountered entries into a compact list, maintaining canonical identity through selection and return.

Normal career playback may record progression. Deliberate replay through ReplayMovies does not manufacture a new progression event. This separation is necessary to avoid changing the player's game history simply by reviewing an earlier cinematic.

## Evidence boundary

The published description is reconstructed from the final ReplayMovies completion and reconstruction/ownership checkpoints. Source listings, function-hook implementations, private debugger journals, and unpublished configuration examples are not included here.

Related: [Profile progression](PROFILE_PROGRESSION.md) · [Validation](VALIDATION.md)
