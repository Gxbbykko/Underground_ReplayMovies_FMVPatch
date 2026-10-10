# Parser and decoder investigation

[← FMVFix overview](../README.md) · [MAD format](MAD_FORMAT.md) · [Compatibility fixes](COMPATIBILITY_FIXES.md)

## Separate acceptance from picture delivery

One of the most important lessons was that **entering the FMV state is not equivalent to successful movie presentation**. The diagnostic chain separated the following boundaries:

1. **Start request:** native `StartMovie` receives the logical movie identity.
2. **Input admission:** movie file/worker accepts and buffers data.
3. **Chunk handling:** stream markers and nested headers are classified.
4. **Decoder route:** appropriate video/audio stream consumers receive data.
5. **Frame materialization:** decoded picture reaches the PC movie-texture path.
6. **Presentation:** the game binds/displays the texture and later tears down the movie.

A failure at one layer must not be retrospectively attributed to an earlier layer without evidence. The historical black-picture tests were therefore worked through with targeted debugger checkpoints and PC/Xbox comparisons.

## What the runtime evidence contributes

Native `SCHl` processing and nested `MADe` reads were observed in the Xbox-path investigation. Later movie-frame capture experiments distinguished native decoder output from **presentation texture** output: some intro-class movies were captured at a proven native frame-clock hook, while career films used a separate post-upload observation point. This distinction was essential for one-to-one synchronization and for avoiding accidental recapture of the wrong buffer.

The eventual MP4 substitute is **not** a substitute for parser acceptance: the original native `.mad` still supplies the movie clock, soundtrack, serials and teardown. The optional high-resolution path uses separately prepared video only at the presentation boundary.

## Proven contract and uncertainty

The final handoff describes **six source-frozen compatibility functions** retained in the integrated build. The public documents do not enumerate all low-level instruction edits as if they formed a separately audited open-source patch set. The strongest explicit numerical reports concern the later Stage 2.5.3 picture-substitution sessions, **not a formal parser pass/fail sweep** of every Xbox file.

> [!NOTE]
> Historical hypotheses and failed probes are preserved in the private notebook. They should not be mistaken for an unfinished decoder in the completed engineering framework.
