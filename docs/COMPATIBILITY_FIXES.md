# Compatibility fixes and invariants

[← FMVFix overview](../README.md) · [Architecture](ARCHITECTURE.md) · [Validation](VALIDATION.md)

## Objective

Adapt Xbox-origin `.mad` playback to the English PC v1.4 engine **without replacing its native movie lifecycle**. Compatibility work addressed platform assumptions discovered through file admission, chunk/stream processing, decoder delivery and the presentation path. The movie should continue to behave as an Underground FMV, not as a separately scheduled player window.

## Invariants retained by the completed integration

| Contract | Why it matters |
| :--- | :--- |
| Native movie identity | Existing frontend and game progression request the intended cinematic. |
| Native frame clock and serials | Skipping, duration, synchronized presentation and return remain controlled by the game. |
| Native audio | Optional high-resolution picture does not replace or desynchronize the original soundtrack. |
| Native skip/teardown | FMV termination returns through the original game lifecycle. |
| Frozen Xbox compatibility layer | Later MP4 experiments must not regress already validated platform adaptation. |
| MAD picture fallback | An optional replacement frame that is unavailable or late must not strand the movie. |

## Six frozen compatibility functions

The later source comparison names six stable internal routines: the movie capability/start hook, three other compatibility hooks, executable verification and bridge installation **as an audited set of six functions**. The source review reports that their bodies were retained while other integration features changed. This is useful **non-regression evidence** but does not replace a new runtime test of a differently compiled binary.

## Scoped third-party interaction

The project investigated Extra Options and Widescreen Fix codebases as **separate reference projects**. In particular, a `NA_BOOT` presentation adjustment in FMVFix assumes the recorded **ThirteenAG cropped-mode** geometry for that tested hook path. This does not justify claiming that every Widescreen Fix version or mode is an unconditional requirement.

## Change policy

Because this native compatibility framework is considered complete, improvements to MP4 decoding, reporting or prewarm should not casually rewrite the frozen Xbox/PC parsing/bridge behavior. Any proposed compatibility patch needs a reproducible before/after trace on the supported executable.

No private source code or proprietary `.mad` data is provided on this page.
