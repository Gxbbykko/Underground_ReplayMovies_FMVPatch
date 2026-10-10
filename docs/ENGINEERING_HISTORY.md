# Engineering history

[← FMVFix overview](../README.md) · [Xbox vs PC](XBOX_VS_PC.md) · [Validation](VALIDATION.md)

This page gives the resolved engineering trajectory. It does not reproduce the full private debugger notebook or mistake intermediate experiment states for current behavior.

## Investigation milestones

| Phase | Engineering conclusion |
| :--- | :--- |
| Controlled PC/Xbox comparison | Use identical movie-start conditions and change the platform-origin `.mad` rather than speculate from file size. |
| Input / parser tracing | Separate file admission from outer container classification and nested stream consumption. |
| Native decode/presentation | A black playback state can occur after file and stream progress; track texture/materialized frames separately. |
| Compatibility bridge | Xbox-to-PC adaptations retained inside Underground's native FMV lifecycle. |
| Source non-regression | Six compatibility routines frozen while optional high-resolution picture handling was added. |
| Integrated MP4 path | Stage 2.5.3 tested exact-frame 1080p/4K picture substitution over the retained MAD clock. |
| Quiet production handoff | Routine debug reports disabled; genuine fault categories retain targeted diagnostics, normal fallback remains silent. |

## Important corrections

- **Black screen ≠ automatic rejection.** It was necessary to trace acceptance, chunk parsing, decoding and presentation independently.
- **Nested stream data ≠ all top-level chunks.** Observed `MADe` usage refined an earlier dispatcher model.
- **MP4 presentation ≠ Xbox parser replacement.** The native MAD still supplies timing, soundtrack, skip and teardown even in high-resolution modes.
- **A source-only audit ≠ a new runtime test.** The notebook's no-MSVC note and later archived Windows release build refer to distinct milestones.

## What is complete, and what remains bounded

The Xbox compatibility system is treated as **completed for the documented baseline**. The optional picture layer has a recorded **startup prewarm limitation under arbitrary ReplayMovies selection order**; it does not require describing the frozen core compatibility framework as unfinished.

Development artifacts, original movie files and raw debugger histories remain private. Future public claims should include the exact executable, mode, movie set and evidence date.
