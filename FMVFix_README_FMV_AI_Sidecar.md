## FMV AI Upscale — StartMovie-Bound Sidecar

FMVFix keeps **Need for Speed: Underground's native `.mad` playback pipeline authoritative**. It does not replace the movie decoder, audio clock, or FMV timing. Instead, the current AI-sidecar path binds an already-upscaled frame package to the exact movie session at `StartMovie`, then presents the matching enhanced frame in sync with the game's real decoded `MovieTexture` updates.

### What the hook does

```text
StartMovie("MOVIES\\<movie>.mad")
        │
        ├─ identify the exact movie session
        ├─ resolve its pre-upscaled MovieSR sidecar
        └─ latch the sidecar to this StartMovie generation
                    │
                    ▼
Native MAD decoder updates MovieTexture
                    │
                    └─ successful MovieTexture upload = next decoded-frame index
                                        │
                                        ▼
                              sidecar frame N
                                        │
                                        ▼
                           Underground FMV draw
```

The important synchronization rule is:

> **Decoded movie frame N → AI sidecar frame N.** Repeated renderer draws of the same decoded frame do not advance the sidecar.

### `mini_source_code_hook`

```cpp
// Presentation-only sketch of the real FMVFix architecture.
// Deliberately omits game addresses, patch bytes and low-level hook details.

void OnStartMovie(const char* moviePath)
{
    const auto session = BeginNewMovieSession();
    const auto movie   = GetMovieBasename(moviePath);

    if (auto sidecar = LoadPreparedSidecar(movie))
        BindSidecarToSession(session, std::move(sidecar));
}

void OnDecodedMovieTextureUpdated()
{
    if (IsBoundMovieSessionActive())
        AdvanceDecodedFrameIndex();
}

HRESULT OnFMVDraw()
{
    if (auto frame = GetSidecarFrameForCurrentDecodedIndex())
        return DrawFMVWithEnhancedFrame(*frame);

    return DrawOriginalFMV(); // fail-open / vanilla fallback
}
```

### Why this approach

- **No `.mad` decoder replacement** — Xbox/PC compatibility work remains isolated from enhancement presentation.
- **No second playback clock** — synchronization follows the game's actual decoded `MovieTexture` uploads.
- **No DrawPrimitiveUP frame counting** — the same decoded frame may be rendered multiple times.
- **No runtime AI requirement** — sidecar frames can be generated offline with an AI super-resolution model and played back deterministically.
- **Fail-open rendering** — if the target session, sidecar, frame index or enhanced texture is unavailable, FMVFix uses the original movie frame.
- **Audio and timing stay native** — only the visual presentation texture is substituted.

### Current AI-Cache 0D prototype

The current source is a controlled proof using **`PSA.mad`** and `MovieSR\PSA.srpack`. The included pack is a **16-frame 1024×512 diagnostic sidecar**, used to validate exact movie-name binding and decoded-frame synchronization. It is not yet the final full-length AI-upscaled movie transport.

The production direction is movie-named sidecars plus bounded streaming/prefetch, so a complete raw multi-gigabyte movie is never synchronously loaded into memory.

> **Architecture:** native MAD playback underneath, offline AI quality on top, exact StartMovie/session binding in between.
