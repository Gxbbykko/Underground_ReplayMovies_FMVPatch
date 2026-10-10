# Xbox vs PC: comparison method

[← FMVFix overview](../README.md) · [MAD format](MAD_FORMAT.md) · [Parser and decoder](PARSER_AND_DECODER.md)

## Controlled comparison

The Xbox compatibility investigation compared the **same logical movie**, requested at the same native lifecycle boundary, while varying the platform-origin `.mad` asset. The aim was to observe *where* PC playback of an Xbox-origin file diverged from normal playback—not to assume that size, bitrate or extension alone determined acceptance.

| Checkpoint | PC movie baseline | Xbox-origin `.mad` under PC engine | Diagnostic question |
| :--- | :--- | :--- | :--- |
| Request | Native movie start | Same request path | Did the logical movie start get issued? |
| File/worker | Native buffered input | Platform-origin data enters same surroundings | Was data read, or was the file rejected first? |
| Container | MAD chunks routed | Xbox layout may differ | Which chunk/classification path was reached? |
| Decode | Existing PC expectations | Distinct platform stream assumptions may appear | Did compressed data reach a compatible consumer? |
| Presentation | Native movie texture/draw | Can reach black picture despite earlier progress | Did a decoded frame reach the display boundary? |
| Termination | Native end/skip/teardown | Must remain intact | Does a return restore the frontend correctly? |

## Why "black screen" was insufficient

A black-screen FMV state is consistent with **several different causes**: unsupported stream admission, parser routing, decoder-path differences, missing frame delivery or incorrect picture presentation. It is not by itself evidence that the entire `.mad` file was rejected. The debugger work therefore separated chunk handling and frame/presentation checkpoints.

## Compatibility outcome

The engineering record reports a completed Xbox-to-PC adaptation while retaining Underground's native FMV lifecycle. The optional MP4 picture layer was developed **around**, not in place of, that compatibility behavior. Six core compatibility functions were later compared against the frozen baseline and reported unchanged.

> [!IMPORTANT]
> "Xbox compatibility implemented" refers to the documented English PC v1.4 x86 engineering baseline. It is **not** a promise that every Xbox movie, PC executable variant, localization or combination of third-party fixes has been independently tested.

The proprietary platform streams and raw debugger captures are not shipped in this repository.
