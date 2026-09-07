# YDSP Bundle Format (`YDSP`)

`.ydsp` is editable source. `.ydsb` is the versioned, little-endian RIFF
container produced by `YdspCompiler::compileBundle` (or the `yup_dsp_compiler`
host tool) that makes a patch portable and inspectable.

A bundle stores the **editable source closure** - the root patch plus every
imported source file - together with the compiler metadata and diagnostics that
describe how it was built. It does **not** store generated machine code: at
runtime `YdspBundle::instantiate()` recompiles the root source with the
recorded options, so one bundle works across targets and can be recompiled with
a newer compiler. Version 1 bundles are therefore source-only; no native or
WebAssembly kernel artifacts are serialized.

Reference implementation: [`compiler/yup_YdspBundle.cpp`](../../modules/yup_dsp_jit/compiler/yup_YdspBundle.cpp)

---

## 1. Primitives & Conventions

### Byte order

All multi-byte integers are written **little-endian** (YUP `writeInt` /
`readInt`). This includes chunk sizes, enum values, and string lengths.

### FourCC

A FourCC is a 32-bit tag built from four ASCII characters, packed
**least-significant-byte first** so the bytes appear in reading order on disk:

```
makeFourCC('R','I','F','F')  →  bytes: 52 49 46 46  ('R' 'I' 'F' 'F')
```

### Chunk layout

Every chunk follows the classic RIFF triplet. The size counts **only** the data
bytes (not the header, not the padding). Chunks are padded to an **even byte
boundary**; the pad byte is `0x00` and is *not* included in `size`.

```
       0        4        8                          8 + size
       +--------+--------+---------------------------+-----------+
       | FourCC |  size  |          data ...         |  pad(0/1) |
       +--------+--------+---------------------------+-----------+
        4 bytes  4 bytes    'size' bytes (LE u32)      0 or 1 byte
                                                     (present when
                                                      size is odd)
```

### LIST chunk

A `LIST` chunk is a container whose data begins with a 4-byte **list type**
FourCC, followed by an arbitrary sequence of sub-chunks:

```
       +--------+--------+----------+-----------------------------+
       | 'LIST' |  size  | listType |   sub-chunk, sub-chunk, ... |
       +--------+--------+----------+-----------------------------+
                          4 bytes
```

### String encoding

Strings are **length-prefixed**: an `int32` UTF-8 byte length followed by the
raw UTF-8 bytes (no NUL terminator). A length `<= 0` decodes to an empty
string. `readString` rejects a declared length that exceeds the bytes remaining
in the record, and a non-zero length whose bytes do not decode to a non-empty
string.

---

## 2. Top-Level Structure

The whole file is a single `RIFF` chunk whose form type is `YDSP`. The writer
always emits four chunks, in this order:

```
+===========================================================================+
|  RIFF  |  size  |  'YDSP'                                                 |
+===========================================================================+
         |                                                                  |
         |   +--------+--------+------------------+                         |
         |   |  VERS  |   4    |  version (u32)   |   format version = 1    |
         |   +--------+--------+------------------+                         |
         |                                                                  |
         |   +--------+--------+-------------------------------------+      |
         |   |  META  |  size  |  language/ABI/codegen metadata ...   |      |
         |   +--------+--------+-------------------------------------+      |
         |                                                                  |
         |   +--------+--------+----------+                                |
         |   |  LIST  |  size  |  'SOUR'  |   <-- source records + imports  |
         |   +--------+--------+----------+                                |
         |        |                                                         |
         |        +---> SRCF (source record)                                |
         |        +---> IMAP (import edge)                                  |
         |        +---> ...                                                 |
         |                                                                  |
         |   +--------+--------+-------------------------------------+      |
         |   |  DIAG  |  size  |  compiler diagnostics (0..N)         |      |
         |   +--------+--------+-------------------------------------+      |
+===========================================================================+
```

| FourCC | Kind  | Meaning                                                    |
|--------|-------|------------------------------------------------------------|
| `RIFF` | chunk | Outer container. Form type is `YDSP`.                      |
| `YDSP` | form  | Bundle magic ("YUP DSP bundle").                            |
| `VERS` | chunk | Format version (currently `1`). **Required.**                |
| `META` | chunk | Language version, ABIs, codegen revision, flags, targets.   |
| `LIST` | list  | List type `SOUR`, holding `SRCF` and `IMAP` records.        |
| `DIAG` | chunk | Compiler diagnostics (severity, location, code, message).   |

> **Required on load**: the reader fails if `VERS` is missing (or not exactly
> the supported version) or if the bundle contains no source record at all.
> `META` and `DIAG` are optional on load - bundles written without them, or
> truncated in ways the reader tolerates, still load. The writer always emits
> all four chunks.

> Filesystem paths and timestamps are never serialized. Imported files are
> referenced by bundle-local source IDs (the root is `source-0`, imports are
> `source-1`, `source-2`, ... in deterministic DFS order), which keeps a bundle
> reproducible and relocatable.

---

## 3. Version Chunk (`VERS`)

A single `int32` holding the bundle format version:

```
+--------+--------+------------------+
| 'VERS' |   4    |  version (u32)   |
+--------+--------+------------------+
```

Current version is `1`. The reader requires the chunk size to be exactly 4 and
the value to be exactly `1`; anything else is rejected with
`YdspBundle: unsupported format version`. Both older and newer versions are
rejected, so existing `.ydsb` files must be regenerated whenever the format
changes.

---

## 4. Metadata Chunk (`META`)

The `META` chunk records the language and ABI the bundle was compiled against,
plus the codegen options used. Fields are written in this order:

```
+--------+--------+----------------------------------------------------------+
| 'META' |  size  |                                                          |
+--------+--------+                                                          |
|                                                                           |
|   +------------------+   languageVersion (int32)   = 2                    |
|   +------------------+   graphAbi        (int32)   = 1                    |
|   +------------------+   nativeAbi       (int32)   = 1                    |
|   +------------------+   codegenRevision (int32)   = 1                    |
|   +------------------+   fastMath        (byte)    0 or 1                 |
|   +------------------+   hasWasm         (byte)    0 or 1                 |
|                                                                           |
|   +------------------+   nativeTargetCount (int32)                        |
|   +------------------+------------------------+                           |
|   | len (int32) | target (UTF-8 bytes)        |   × nativeTargetCount     |
|   +------------------+------------------------+                           |
+---------------------------------------------------------------------------+
```

| Field            | Type    | Meaning                                                |
|------------------|---------|--------------------------------------------------------|
| `languageVersion`| `int32` | YDSP language version.                                 |
| `graphAbi`       | `int32` | Runtime/audio-graph ABI version.                       |
| `nativeAbi`      | `int32` | Native codegen ABI version.                            |
| `codegenRevision`| `int32` | Codegen revision.                                      |
| `fastMath`       | `byte`  | 1 when compiled with fast math, 0 otherwise.           |
| `hasWasm`        | `byte`  | 1 when a WebAssembly backend was requested, 0 otherwise.|
| `nativeTargets`  | list    | Target triples as `<os>-<arch>` strings, sorted.       |

Native target strings use the `<os>-<arch>` form, e.g. `macos-arm64`,
`linux-x64`, `windows-arm64`, `windows-x64`.

> On load the reader parses `META` only when at least 6 bytes are present
> (the four ints and the two flags). The native-target list is read only when
> at least 4 more bytes remain for the count; the count must be in `0..1024`
> and every target must be non-empty and unique.

---

## 5. Source List (`LIST SOUR`)

The `LIST` chunk with list type `SOUR` holds the source closure. The root
source (the text passed to `compileBundle`) is always record `source-0` with
the root flag set; each imported file is added in DFS order as `source-N`, with
an `IMAP` edge recording who imported it.

### `SRCF` - source record

```
+--------+--------+---------------------------------------------------------+
| 'SRCF' |  size  |                                                         |
+--------+--------+                                                         |
|                                                                           |
|   +------------------+------------------------+                           |
|   | len (int32) | id (UTF-8 bytes)            |   bundle-local source ID  |
|   +------------------+------------------------+                           |
|   +------------------+   isRoot (byte)         0 or 1                    |
|   +------------------+------------------------+                           |
|   | len (int32) | source (UTF-8 bytes)        |   editable source text    |
|   +------------------+------------------------+                           |
+---------------------------------------------------------------------------+
```

| Field    | Type          | Meaning                                            |
|----------|---------------|----------------------------------------------------|
| `id`     | prefixed str  | Bundle-local source ID (`source-0`, `source-1`, ...). |
| `isRoot` | `byte`        | 1 for the root source, 0 for imports.              |
| `source` | prefixed str  | The source text as UTF-8.                          |

Source IDs must be non-empty and **unique**; the reader rejects duplicates
with `YdspBundle: duplicate source ID`.

### `IMAP` - import edge record

```
+--------+--------+---------------------------------------------------------+
| 'IMAP' |  size  |                                                         |
+--------+--------+                                                         |
|                                                                           |
|   +------------------+------------------------+                           |
|   | len (int32) | importingSourceId (UTF-8)  |   importing source         |
|   +------------------+------------------------+                           |
|   +------------------+------------------------+                           |
|   | len (int32) | spelling (UTF-8)           |   import spelling as written|
|   +------------------+------------------------+                           |
|   +------------------+------------------------+                           |
|   | len (int32) | importedSourceId (UTF-8)   |   imported source          |
|   +------------------+------------------------+                           |
+---------------------------------------------------------------------------+
```

| Field              | Type          | Meaning                                    |
|--------------------|---------------|--------------------------------------------|
| `importingSourceId`| prefixed str  | Source that contains the `import` statement.|
| `spelling`         | prefixed str  | The import path exactly as written.        |
| `importedSourceId` | prefixed str  | The imported source's bundle-local ID.     |

> Records inside the list are padded to an even byte boundary just like
> top-level chunks. The reader rejects any record type other than `SRCF` or
> `IMAP` inside `SOUR` (`YdspBundle: invalid source record`), and rejects
> import edges whose endpoints do not both exist
> (`YdspBundle: dangling import map entry`).

---

## 6. Diagnostics Chunk (`DIAG`)

The `DIAG` chunk stores the compiler diagnostics produced while building the
bundle, so tools can render them without recompiling:

```
+--------+--------+---------------------------------------------------------+
| 'DIAG' |  size  |                                                         |
+--------+--------+                                                         |
|                                                                           |
|   +------------------+   count (int32)                                    |
|                                                                           |
|   +------------------+   severity (int32, YdspSeverity)                   |
|   +------------------+------------------------+                           |
|   | len (int32) | sourceId (UTF-8 bytes)      |   bundle-local source ID  |
|   +------------------+------------------------+                           |
|   +------------------+   startLine   (int32)                             |
|   +------------------+   startColumn (int32)                             |
|   +------------------+   endLine     (int32)                             |
|   +------------------+   endColumn   (int32)                             |
|   +------------------+------------------------+                           |
|   | len (int32) | code (UTF-8 bytes)          |   diagnostic code         |
|   +------------------+------------------------+                           |
|   +------------------+------------------------+                           |
|   | len (int32) | message (UTF-8 bytes)       |   human-readable message  |
|   +------------------+------------------------+                           |
+---------------------------------------------------------------------------+
```

| Field     | Type          | Meaning                                        |
|-----------|---------------|------------------------------------------------|
| `count`   | `int32`       | Number of diagnostics (`0..100000`).           |
| `severity`| `int32`       | `YdspSeverity` (error/warning/info), see below.|
| `sourceId`| prefixed str  | Source the diagnostic refers to.               |
| `range`   | 4 × `int32`   | `startLine`, `startColumn`, `endLine`, `endColumn` (1-based). |
| `code`    | prefixed str  | Diagnostic code (may be empty).                |
| `message` | prefixed str  | Human-readable message.                        |

> On load the severity must be one of the three known `YdspSeverity` values,
> and every field must parse; otherwise the bundle is rejected
> (`YdspBundle: malformed diagnostic`).

---

## 7. FourCC Reference

| FourCC | Role                                                  |
|--------|-------------------------------------------------------|
| `RIFF` | Outer RIFF container.                                 |
| `YDSP` | Bundle form type (magic).                             |
| `VERS` | Format version chunk.                                 |
| `META` | Language/ABI/codegen metadata chunk.                  |
| `LIST` | Generic list container.                               |
| `SOUR` | List type for the source closure.                     |
| `SRCF` | Source record (id, root flag, source text).           |
| `IMAP` | Import edge record (importing source, spelling, imported source). |
| `DIAG` | Compiler diagnostics chunk.                           |

Current format version: **`1`**.

### Version history

| Version | Change                          |
|---------|---------------------------------|
| 1       | Initial format.                 |

---

## 8. Enum Encodings

Enums are serialised as their underlying `int32` ordinal (declaration order in
[`compiler/yup_YdspDiagnostics.h`](../../modules/yup_dsp_jit/compiler/yup_YdspDiagnostics.h)).

**`YdspSeverity`**

| Value | Name      |
|-------|-----------|
| 0     | `error`   |
| 1     | `warning` |
| 2     | `info`    |

---

## 9. Complete Nesting Overview

```
RIFF 'YDSP'
├── VERS                       format version (u32) = 1
├── META                       language/ABI/codegen metadata
│   ├── languageVersion (i32) = 2
│   ├── graphAbi        (i32) = 1
│   ├── nativeAbi       (i32) = 1
│   ├── codegenRevision (i32) = 1
│   ├── fastMath        (byte)
│   ├── hasWasm         (byte)
│   ├── nativeTargetCount (i32)
│   └── target strings × N
├── LIST 'SOUR'                source closure
│   ├── SRCF                   source #0 (root)
│   │   ├── id       (len-prefixed str)
│   │   ├── isRoot   (byte)
│   │   └── source   (len-prefixed str)
│   ├── IMAP                   import edge
│   │   ├── importingSourceId (len-prefixed str)
│   │   ├── spelling          (len-prefixed str)
│   │   └── importedSourceId  (len-prefixed str)
│   ├── SRCF                   source #1 (import)
│   ├── IMAP ...
│   └── ...
└── DIAG                       compiler diagnostics
    ├── count (i32)
    └── per diagnostic: severity (i32), sourceId, range (4 × i32),
        code, message
```

---

## 10. Reading & Writing

The API mirrors save/load across streams, files, raw data, and `MemoryBlock`:

```cpp
// Compile once and persist.
yup::YdspCompiler compiler;
yup::YdspBundleCompileOptions options;
options.nativeTargets.push_back ({ yup::YdspTargetOperatingSystem::macosTarget,
                                   yup::YdspTargetArchitecture::arm64 });
options.fastMath = true;

auto result = compiler.compileBundle (patchSource, options, importBasePath);
if (result)
    result.getReference().saveToFile (yup::File ("myPatch.ydsb"));

// Load at runtime and instantiate (recompiles the root source).
auto loaded = yup::YdspBundle::loadFromFile (yup::File ("myPatch.ydsb"));
if (loaded)
{
    auto graph = loaded.getReference().instantiate();
    if (graph)
        useGraph (std::move (graph.getReference()));
}
```

| Save                     | Load                                     |
|--------------------------|------------------------------------------|
| `saveToStream`           | `loadFromStream`                         |
| `saveToFile`             | `loadFromFile`                           |
| `saveToMemoryBlock`      | `loadFromMemoryBlock`                    |
|                          | `loadFromData` (raw pointer + size)      |

### Offline compilation

Bundles are produced from the command line with the `yup_dsp_compiler` host
tool:

```
yup_dsp_compiler patch.ydsp --output patch.ydsb [--target <os>-<arch>]... [--fast-math]
yup_dsp_compiler --inspect <bundle.ydsb> [--list]
```

Use `--fast-math` only when changed floating-point evaluation is acceptable.
Failed source compilation does not create an output bundle.

### Embedding bundles in CMake

For CMake projects, `yup_add_ydsp_bundle()` runs the host compiler at
configuration time and embeds the result:

```cmake
yup_add_ydsp_bundle (MyPatch
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/patch.ydsp
    RESOURCE_NAME MyPatchFile)
```

The source extension remains `.ydsp`; compiled artifacts use `.ydsb`.

### Instantiating at runtime

Applications load bundles with `YdspBundle::loadFromFile()`,
`loadFromStream()`, or `loadFromData()`, then call `instantiate()` on the
control thread. `instantiate()` recompiles the stored root source with the
recorded options, so a bundle never depends on prebuilt code being present.

### Version rejection

The loader performs an **exact** version check: a `VERS` chunk whose size is
not 4 or whose value is not `1` fails with `YdspBundle: unsupported format
version`. A bundle with no `VERS` chunk at all (or no source records) fails
with `YdspBundle: missing required chunks`. Both older and future versions are
rejected; existing `.ydsb` files must be regenerated whenever the format
version changes.

### Structural validation

Beyond the version, a bundle is accepted only when:

- The RIFF header and payload bounds are consistent (rejects truncation and
  out-of-bounds sizes).
- `VERS` is present and at least one `SRCF` record exists.
- Exactly one source is marked as the root (`YdspBundle: invalid root source
  count`).
- Source IDs are non-empty and unique; import edges reference existing sources.

### Parser robustness

Top-level chunks the reader does not recognise are ignored (including `LIST`
chunks with a list type other than `SOUR`), so adding new optional chunks in a
future version is safe as long as `VERS` is bumped. Inside `LIST SOUR`, an
unknown record type is rejected rather than skipped, and malformed or truncated
records fail with a descriptive error.
