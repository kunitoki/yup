# ShaderBundle RIFF Format (`YSLB`)

The `ShaderBundle` class serialises a compiled + transpiled shader program into a
compact binary container based on the **RIFF** (Resource Interchange File Format)
chunk structure. This lets a shader be compiled once (through `glslang` /
`SPIRV-Cross`) and reused at runtime without paying the compilation cost again.

A bundle stores two kinds of payload:

- The **SPIR-V binary** for each pipeline stage.
- One **`ShaderInfo`** per `(stage × target language)` combination, each holding
  the transpiled source, the original Vulkan GLSL input source, and full
  `ShaderReflection`.

Reference implementation: [`shading/yup_ShaderBundle.cpp`](shading/yup_ShaderBundle.cpp)

---

## 1. Primitives & Conventions

### Byte order

All multi-byte integers are written **little-endian** (YUP `writeInt` / `readInt`).
This includes chunk sizes, enum values, and string lengths.

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

Two string forms are used:

- **Length-prefixed** (`writeStringRaw`): `int32` UTF-8 byte length, then the
  raw UTF-8 bytes (no NUL terminator). A length `<= 0` decodes to an empty string.
- **Raw / chunk-sized**: the string fills the entire chunk data (e.g. `ISRC`).
  Its length is implied by the chunk `size`.

---

## 2. Top-Level Structure

The whole file is a single `RIFF` chunk whose form type is `YSLB`.

```
+===========================================================================+
|  RIFF  |  size  |  'YSLB'                                                  |
+===========================================================================+
         |                                                                  |
         |   +--------+--------+------------------+                         |
         |   |  VERS  |   4    |  version (u32)   |    format version = 2   |
         |   +--------+--------+------------------+                         |
         |                                                                  |
         |   +--------+--------+----------+                                 |
         |   |  LIST  |  size  |  'SHAD'  |   <-- list of shader stages     |
         |   +--------+--------+----------+                                 |
         |        |                                                         |
         |        +---> SHDR (stage 0)                                      |
         |        +---> SHDR (stage 1)                                      |
         |        +---> ...                                                 |
+===========================================================================+
```

| FourCC | Kind  | Meaning                                             |
|--------|-------|-----------------------------------------------------|
| `RIFF` | chunk | Outer container. Form type is `YSLB`.               |
| `YSLB` | form  | Bundle magic ("YUP Shader Language Bundle").        |
| `VERS` | chunk | Format version (currently `2`). **Required.**       |
| `LIST` | list  | List type `SHAD`, containing one `SHDR` per stage.  |

> On load, the reader rejects the file if `VERS` is missing or if the version
> does not exactly match the supported version (`kCurrentVersion = 2`).
> Both older and newer versions are rejected with an error message that
> identifies the mismatch.

---

## 3. Per-Stage Chunk (`SHDR`)

Each `SHDR` chunk inside the `SHAD` list describes one pipeline stage. It begins
with two `int32` header fields, followed by the SPIR-V blob and a `VARS` list of
transpiled language variants.

```
+--------+--------+---------------------------------------------------------+
| 'SHDR' |  size  |                                                         |
+--------+--------+                                                         |
|                                                                           |
|   +------------------+   stage    (int32, ShaderStage enum)               |
|   +------------------+   srcLang  (int32, ShaderLanguage enum)            |
|                                                                           |
|   +--------+--------+----------------------------+                        |
|   |  SPVB  |  size  |  SPIR-V binary (bytes)     |   binary chunk         |
|   +--------+--------+----------------------------+                        |
|                                                                           |
|   +--------+--------+----------+                                          |
|   |  LIST  |  size  |  'VARS'  |    <-- list of transpiled variants       |
|   +--------+--------+----------+                                          |
|        |                                                                  |
|        +---> VART (variant 0)                                             |
|        +---> VART (variant 1)                                             |
|        +---> ...                                                          |
+---------------------------------------------------------------------------+
```

| Field    | Type    | Meaning                                                  |
|----------|---------|----------------------------------------------------------|
| `stage`  | `int32` | `ShaderStage` (vertex, fragment, compute, ...).          |
| `srcLang`| `int32` | `ShaderLanguage` the SPIR-V was compiled from.           |
| `SPVB`   | chunk   | SPIR-V binary for this stage. May be empty (size `0`).   |
| `VARS`   | list    | List type `VARS`, containing one `VART` per variant.     |

> Stage ordering: stages that appear in `shaders` are written first (in insertion
> order), then any SPIR-V-only stages that have no transpiled variant.

---

## 4. Per-Variant Chunk (`VART`)

Each `VART` chunk holds one transpiled `(stage × language)` variant. The stage is
inherited from the enclosing `SHDR`; the variant only stores its target language,
entry point, transpiled source, optional original input source, and reflection.

```
+--------+--------+--------------------------------------------------------+
| 'VART' |  size  |                                                        |
+--------+--------+                                                        |
|                                                                          |
|   +------------------+   language  (int32, ShaderLanguage enum)          |
|                                                                          |
|   +------------------+------------------------+                          |
|   | len (int32) | entryPoint (UTF-8 bytes)    |   length-prefixed str    |
|   +------------------+------------------------+                          |
|                                                                          |
|   +------------------+------------------------+                          |
|   | len (int32) | source (UTF-8 bytes)        |   length-prefixed str    |
|   +------------------+------------------------+                          |
|                                                                          |
|   +--------+--------+----------------------------+   (optional)          |
|   |  ISRC  |  size  |  input source (UTF-8)      |   raw chunk-sized str |
|   +--------+--------+----------------------------+                       |
|                                                                          |
|   +--------+--------+----------------------------+                       |
|   |  REFL  |  size  |  reflection (archive blob) |   binary chunk        |
|   +--------+--------+----------------------------+                       |
+--------------------------------------------------------------------------+
```

| Field        | Type          | Meaning                                                |
|--------------|---------------|--------------------------------------------------------|
| `language`   | `int32`       | Target `ShaderLanguage` of this variant.               |
| `entryPoint` | prefixed str  | Entry-point function name (e.g. `"main"`).             |
| `source`     | prefixed str  | Transpiled source code in `language`.                  |
| `ISRC`       | chunk         | Original Vulkan GLSL input source, raw UTF-8. Optional — omitted when empty. |
| `REFL`       | chunk         | Serialised `ShaderReflection` (see below).             |

### `ISRC` — Original input source

The `ISRC` sub-chunk stores the original Vulkan GLSL (`#version 450`) source that
was compiled to SPIR-V for this stage. It is populated by `ShaderBundleCompiler`
from `ShaderBundleCompileRequest::source` and maps to `ShaderInfo::inputSource`.

**Why this matters for GL/GLES recompilation.** The `source` field contains
SPIRV-Cross decompiled output (already-combined `sampler2D` uniforms). Feeding
that back into glslang as Vulkan GLSL produces SPIR-V with no separate
image/sampler resources, so `build_combined_image_samplers()` returns nothing,
`glCombinedSamplers` is empty, and the GL fixup blob is empty — causing samplers
to default to texture unit 0 (black texture). `ISRC` preserves the original source
so `compileFromGlsl()` round-trips correctly.

When `ISRC` is absent (bundles without an input source), `ShaderInfo::inputSource`
is empty and callers should fall back to `ShaderInfo::source`.

---

## 5. Reflection Blob (`REFL`)

The `REFL` chunk data is **not** RIFF; it is a `BinaryOutputArchive` blob produced
by YUP's serialisation layer (`detail::doSave` / `detail::doLoad`) over the
`ShaderReflection` struct. Its internal layout is owned by the archive format and
the `SerialisationTraits<ShaderReflection>` specialisation in
[`shading/yup_ShaderBundle.h`](shading/yup_ShaderBundle.h).

Serialised reflection fields include:

- `entryPoints`
- Resource-binding arrays: `uniformBuffers`, `storageBuffers`, `stageInputs`,
  `stageOutputs`, `subpassInputs`, `storageImages`, `sampledImages`,
  `atomicCounters`, `accelerationStructures`, `glPlainUniforms`, `tensors`,
  `pushConstantBuffers`, `shaderRecordBuffers`, `separateImages`,
  `separateSamplers`
- `builtinInputs`, `builtinOutputs`
- `specConstants`
- `workgroupSize`
- `positionInvariant`
- `capabilities`, `extensions`
- `glCombinedSamplers` (folded combined `sampler2D` entries with texture units,
  used by `makeGLFixupBlob` to bind samplers by name after program linking)

Because reflection uses the versioned archive layer, its schema can evolve
independently of the RIFF envelope version.

---

## 6. FourCC Reference

| FourCC | Constant          | Role                                          |
|--------|-------------------|-----------------------------------------------|
| `RIFF` | `kFourCC_RIFF`    | Outer RIFF container.                         |
| `LIST` | `kFourCC_LIST`    | Generic list container.                       |
| `YSLB` | `kFourCC_YSLB`    | Bundle form type (magic).                     |
| `VERS` | `kFourCC_VERS`    | Format version chunk.                         |
| `SHAD` | `kFourCC_SHAD`    | List type for the stage list.                 |
| `SHDR` | `kFourCC_SHDR`    | Per-stage chunk.                              |
| `SPVB` | `kFourCC_SPVB`    | SPIR-V binary chunk.                          |
| `VARS` | `kFourCC_VARS`    | List type for the variant list.               |
| `VART` | `kFourCC_VART`    | Per-variant chunk.                            |
| `ISRC` | `kFourCC_ISRC`    | Original input source chunk (per-variant, optional). |
| `REFL` | `kFourCC_REFL`    | Reflection archive chunk.                     |

Current format version: **`2`** (`kCurrentVersion`).

### Version history

| Version | Change                                                                 |
|---------|------------------------------------------------------------------------|
| 1       | Initial format. Top-level `SRCE` chunk held a single per-bundle source. |
| 2       | Removed `SRCE`. Added per-variant `ISRC` sub-chunk inside `VART`.     |

---

## 7. Enum Encodings

Enums are serialised as their underlying `int32` ordinal (declaration order in
[`shading/yup_ShaderTypes.h`](shading/yup_ShaderTypes.h)).

**`ShaderStage`**

| Value | Name          |
|-------|---------------|
| 0     | `vertex`      |
| 1     | `fragment`    |
| 2     | `compute`     |
| 3     | `geometry`    |
| 4     | `tessControl` |
| 5     | `tessEval`    |

**`ShaderLanguage`**

| Value | Name    |
|-------|---------|
| 0     | `glsl`  |
| 1     | `essl`  |
| 2     | `hlsl`  |
| 3     | `msl`   |
| 4     | `spirv` |
| 5     | `wgsl`  |

---

## 8. Complete Nesting Overview

```
RIFF 'YSLB'
├── VERS                       format version (u32) = 2
└── LIST 'SHAD'                stage list
    ├── SHDR                   stage #0
    │   ├── stage   (i32)
    │   ├── srcLang (i32)
    │   ├── SPVB               SPIR-V binary
    │   └── LIST 'VARS'        variant list
    │       ├── VART           variant #0
    │       │   ├── language   (i32)
    │       │   ├── entryPoint (len-prefixed str)
    │       │   ├── source     (len-prefixed str)
    │       │   ├── ISRC       original input source (optional, raw UTF-8)
    │       │   └── REFL       reflection archive blob
    │       └── VART ...
    └── SHDR ...
```

---

## 9. Reading & Writing

The API mirrors save/load across streams, files, raw data, and `MemoryBlock`:

```cpp
// Compile once and persist.
yup::ShaderBundleCompiler compiler;

auto result = compiler.compile (request);
if (result)
    result.getReference().saveToFile (yup::File ("myShader.ysl"));

// Load at runtime and look up a variant.
auto loaded = yup::ShaderBundle::loadFromFile (yup::File ("myShader.ysl"));
if (loaded)
    if (auto* info = loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::msl))
        useSource (info->source);

// For live recompilation on GL/GLES, prefer inputSource over source:
if (auto* info = loaded.getReference().findShader (ShaderStage::vertex, ShaderLanguage::glsl))
{
    const auto& src = info->inputSource.isNotEmpty() ? info->inputSource : info->source;
    auto recompiled = GpuPipeline::compileFromGlsl (ctx, src, fragSrc, options);
}
```

| Save                     | Load                                     |
|--------------------------|------------------------------------------|
| `saveToStream`           | `loadFromStream`                         |
| `saveToFile`             | `loadFromFile`                           |
| `saveToMemoryBlock`      | `loadFromMemoryBlock`                    |
|                          | `loadFromData` (raw pointer + size)      |

### Version rejection

The loader performs an **exact** version check: if `VERS != kCurrentVersion` (2),
loading fails immediately with an error message of the form:

```
ShaderBundle: bundle version N is not supported (expected 2)
```

This applies to both older bundles (version 1, missing `ISRC`) and future bundles.
Existing `.ysl` files must be regenerated with the current `yup_shader_bundler`
tool whenever the format version changes.

### Parser robustness

Within a given version, the loader is tolerant of unknown chunks inside `VART`:
`iterateChunks` walks every `[fourcc, size, data]` triplet and ignores tags it
does not recognise, consuming pad bytes between chunks. Adding new optional
sub-chunks to `VART` in a future version is safe as long as the `VERS` value is
bumped and old bundles are regenerated.
