# ShaderBundle RIFF Format (`YSLB`)

The `ShaderBundle` class serialises a compiled + transpiled shader program into a
compact binary container based on the **RIFF** (Resource Interchange File Format)
chunk structure. This lets a shader be compiled once (through `glslang` /
`SPIRV-Cross`) and reused at runtime without paying the compilation cost again.

A bundle stores three kinds of payload:

- The **original source** used for compilation, typically GLSL v450.
- The **SPIR-V binary** for each pipeline stage.
- One **`ShaderInfo`** per `(stage x target language)` combination, each holding
  the transpiled source plus the full `ShaderReflection`.

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
makeFourCC('R','I','F','F')  ->  bytes: 52 49 46 46  ('R' 'I' 'F' 'F')
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
- **Raw / chunk-sized**: the string fills the entire chunk data (`SRCE`). Its
  length is implied by the chunk `size`.

---

## 2. Top-Level Structure

The whole file is a single `RIFF` chunk whose form type is `YSLB`.

```
+===========================================================================+
|  RIFF  |  size  |  'YSLB'                                                  |
+===========================================================================+
         |                                                                  |
         |   +--------+--------+------------------+                         |
         |   |  VERS  |   4    |  version (u32)   |    format version = 1   |
         |   +--------+--------+------------------+                         |
         |                                                                  |
         |   +--------+--------+------------------------------+             |
         |   |  SRCE  |  size  |  original source (UTF-8)     |             |
         |   +--------+--------+------------------------------+             |
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
| `VERS` | chunk | Format version (currently `1`). **Required.**       |
| `SRCE` | chunk | Original shader source, raw UTF-8. **Required.**    |
| `LIST` | list  | List type `SHAD`, containing one `SHDR` per stage.  |

> On load, the reader rejects the file if `VERS` is missing, if `SRCE` is
> missing, or if the version is greater than the supported version.

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

Each `VART` chunk holds one transpiled `(stage x language)` variant. The stage is
inherited from the enclosing `SHDR`; the variant only stores its target language,
entry point, transpiled source, and reflection.

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
|   +--------+--------+----------------------------+                       |
|   |  REFL  |  size  |  reflection (archive blob) |   binary chunk        |
|   +--------+--------+----------------------------+                       |
+--------------------------------------------------------------------------+
```

| Field        | Type          | Meaning                                       |
|--------------|---------------|-----------------------------------------------|
| `language`   | `int32`       | Target `ShaderLanguage` of this variant.      |
| `entryPoint` | prefixed str  | Entry-point function name (e.g. `"main"`).    |
| `source`     | prefixed str  | Transpiled source code in `language`.         |
| `REFL`       | chunk         | Serialised `ShaderReflection` (see below).    |

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
| `SRCE` | `kFourCC_SRCE`    | Original source chunk.                        |
| `SHAD` | `kFourCC_SHAD`    | List type for the stage list.                 |
| `SHDR` | `kFourCC_SHDR`    | Per-stage chunk.                              |
| `SPVB` | `kFourCC_SPVB`    | SPIR-V binary chunk.                          |
| `VARS` | `kFourCC_VARS`    | List type for the variant list.               |
| `VART` | `kFourCC_VART`    | Per-variant chunk.                            |
| `REFL` | `kFourCC_REFL`    | Reflection archive chunk.                     |

Current format version: **`1`** (`kCurrentVersion`).

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
├── VERS                       format version (u32)
├── SRCE                       original UTF-8 source
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
```

| Save                     | Load                                     |
|--------------------------|------------------------------------------|
| `saveToStream`           | `loadFromStream`                         |
| `saveToFile`             | `loadFromFile`                           |
| `saveToMemoryBlock`      | `loadFromMemoryBlock`                    |
|                          | `loadFromData` (raw pointer + size)      |

### Parser robustness

The loader is tolerant of unknown chunks: `iterateChunks` walks every
`[fourcc, size, data]` triplet and simply ignores tags it does not recognise,
consuming pad bytes between chunks. This keeps the format **forward-compatible** —
new chunk types can be added without breaking older readers, as long as the
`VERS` value is not bumped past what the reader supports.
