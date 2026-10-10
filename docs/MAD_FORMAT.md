# MAD format observations

[← FMVFix overview](../README.md) · [Xbox vs PC](XBOX_VS_PC.md) · [Parser and decoder](PARSER_AND_DECODER.md)

## Format-level reasoning

The project investigated **native movie chunk classification and nested stream handling**, rather than treating `.mad` as a generic MP4. The debugger record refers to native chunk tags including `MADk`, `SCHl`, `SCDl` and a nested Xbox stream marker `MADe`.

| Observed marker | Investigation role |
| :--- | :--- |
| `MADk` | Outer movie/container recognition encountered in historical tracing. |
| `SCHl` | Stream header area used to follow the native stream handler. |
| `SCDl` | Stream/data-family marker involved in native chunk routing. |
| `MADe` | Nested Xbox stream marker observed in a stream iterator rather than assumed to pass through every outer dispatcher. |

These terms summarize **observed identifiers**, not a complete open-file-format specification. Their semantic roles were inferred and refined through runtime control flow; not every marker should be modeled as a top-level sibling chunk.

## A useful corrected interpretation

At one checkpoint, the Xbox `MADe` marker was reached through a nested stream object's current-chunk pointer. An earlier expectation that it would be dispatched through the same outer `SCHl`/`SCDl` path was superseded. This distinction mattered when deciding where platform-specific admission and decode behavior must be investigated.

The notebook also records a nested header with word-sized values interpretable as **512 × 256**, but that observation alone does **not** prove a universal movie resolution or an exact on-disk format grammar. It must be interpreted alongside the separate PC CPU movie-texture evidence.

## Why this is not a codec recipe

The public documentation intentionally does not reproduce private parser implementations, proprietary payload bytes or a full reverse-engineered decoding specification. It documents the meaningful **classification boundaries** needed to understand compatibility work.

For end-to-end responsibilities, continue with [Architecture](ARCHITECTURE.md) and [Native lifecycle](NATIVE_LIFECYCLE.md).
