# YDSP bundle format

`.ydsp` is editable source. `.ydsb` is the versioned, little-endian RIFF
container produced by `yup_dsp_compiler`.

The form type is `YDSP`. Required chunks are `VERS`, `META`, `LIST SOUR`, and
`DIAG`. `LIST SOUR` contains `SRCF` records with a bundle-local source ID, root
flag, and UTF-8 source text. `META` contains ABI and code-generation
metadata. `DIAG` contains severity, source ID, range, code, and message.

Chunks use `[FourCC][u32 byte size][payload]`, followed by one zero byte when
the payload has odd size. Readers reject truncation, unsupported versions,
duplicate source IDs, invalid UTF-8, and out-of-bounds sizes. Unknown chunks
are ignored. Filesystem paths and timestamps are never serialized.
