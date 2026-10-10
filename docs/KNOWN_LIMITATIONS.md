# FMVPatch known limitations

## Out-of-order startup prewarming

**Status: documented in the last consolidated runtime report.** The Stage 2.5.3 prewarm system predicts the next movie using a fixed catalog order. If ReplayMovies selects a different movie, the cached startup frames can belong to the wrong title; they must not be substituted.

The nine affected movies in the measured archive lost only their initial MP4 substitutions: frame serial `0` at 1080p, or serials `0`–`2` at 4K. The **native MAD picture remained the fallback** and the correct asynchronous MP4 decoder caught up afterward. No decoder or texture upload/binding failures were reported for these runs.

This is **not** equivalent to a full movie failing, missing audio, or a corrupted HEVC stream. It does mean arbitrary-order playback did **not** demonstrate zero-fallback replacement from the first frame.

## Report coverage

- The reported **1080p** sample covers all 20 catalog movies.
- The **4K** sample covers 15 reported movies. The same result cannot be extended to an untested five-movie subset.
- Counts of draw attempts that used fallback are not interchangeable with counts of distinct native frame serials without an exact replacement.

## Runtime and asset dependencies

The project assumes appropriate pre-prepared video files for the selected tier and a compatible 32-bit PC game environment. Missing or invalid external MP4 files do not become assets of this GitHub repository. The integrated runtime retains fallback to native movie frames for unready replacements.

The original executable and third-party modifications outside the established English PC v1.4 test baseline need independent checking.

## Production-source build qualification

The subsequent quiet-production edit disabled routine reports and configured non-blocking, deduplicated error dialogs. The notebook states that a native Windows/MSVC compilation was **not** available at the time of that source audit. Do not present that audit as a new production-binary playback pass.

## Candidate improvement, not an implemented guarantee

The notebook proposes prewarming the actual selected title before the native movie-start boundary rather than predicting the next fixed-order catalog entry. It is a documented **possible improvement**, not proof that such a change has already shipped or passed validation.

Related: [Validation](VALIDATION.md) · [Prewarm optimization](PREWARM_OPTIMIZATION.md)
