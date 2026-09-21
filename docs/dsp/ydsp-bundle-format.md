# YDSP Bundle Format (`YDSP`)

`.ydsp` is editable source. `.ydsb` is a little-endian RIFF container produced
by `YdspCompiler::compileBundle()` or the `yup_dsp_compiler` host tool.

Version 2 stores the root source, its complete import closure, and compiled
native and/or WebAssembly kernels. The compiler can emit ARM64 and x64
for macOS, Linux and Windows regardless of its own architecture or OS.
Compilation does not install or execute the emitted kernels. WebAssembly
compiler hosts also include both native emitters, with AsmJit's executable
memory support disabled.

Native bundle kernels use baseline scalar instructions, independent of the
compiler host's CPU features. `fastMath` controls numerical transformations.
The normal `YdspCompiler::compile()` API still selects and optimises for the
running host.

## Creating and loading a bundle

```cpp
yup::YdspCompiler compiler;
yup::YdspBundleCompileOptions options;
options.nativeTargets = {
    { yup::YdspTargetOperatingSystem::macosTarget, yup::YdspTargetArchitecture::arm64 },
    { yup::YdspTargetOperatingSystem::windowsTarget, yup::YdspTargetArchitecture::x64 }
};
options.includeWasm = true;

auto compiled = compiler.compileBundle (patchSource, options, patchFile.getFullPathName());
if (compiled.wasOk())
    compiled.getReference().saveToFile (bundleFile);

auto loaded = yup::YdspBundle::loadFromFile (bundleFile);
if (loaded.wasOk())
{
    auto graph = loaded.getReference().instantiate();
    if (graph.wasOk())
        graph.getReference().prepare (48000.0, 512);
}
```

An empty `nativeTargets` list requests no native artifacts. With the default
`includeWasm = true`, this produces a WebAssembly-only bundle. Requesting no
targets at all is an error. Duplicate native targets are emitted once.

`instantiate()` is a control-thread operation. On a native host it selects the
exact OS/architecture pair; in a WebAssembly build it uses the stored wasm32
modules. A missing target is an error, with no source-code fallback.

Instantiation currently parses and analyses the packaged sources and rebuilds
IR to reconstruct graph wiring, state layout and kernel metadata. It loads
the stored code instead of running machine-code generation again. Imports
resolve exclusively through the bundle's source IDs and import map, so the
original source files need not exist. The frontend work still contributes to
load time; this is not a compiler-free graph loader.

Native artifacts contain executable code. Load native bundles only from
trusted sources. Structural validation is not machine-code verification.

## Container and primitives

All integer fields are little-endian. Strings are an `int32` byte length
followed by UTF-8 bytes without a terminating zero. Byte blobs are an
`int32` byte length followed by those bytes.

Each RIFF chunk has a FourCC, a `uint32` payload size, and the payload. An odd
payload size is followed by one padding byte, excluded from its size. The
outer `RIFF` payload begins with form type `YDSP`.

```text
RIFF 'YDSP'
  VERS
  META
  LIST 'SOUR'
    SRCF ...
    IMAP ...
  NATV ...       one per native target
  WASM           when WebAssembly kernels were requested
  DIAG
```

### VERS

One `int32`, currently **2**. The loader requires an exact match. Version 1
source-only bundles must be regenerated.

### META

| Field | Encoding | Current value |
|---|---|---|
| Language version | int32 | 4 |
| Graph/runtime ABI | int32 | 2 |
| Native ABI | int32 | 2 |
| Codegen revision | int32 | 14 |
| Fast math | byte | 0 or 1 |
| Tracing enabled | byte | 0 or 1 |
| Has WebAssembly | byte | 0 or 1 |
| Native target count | int32 | Number of target strings |
| Native targets | Length-prefixed strings | e.g. `macos-arm64` |

`META` is required. Language, ABI and codegen revisions must match the
loader. Target strings must match the actual native artifacts, and
`hasWasm` must match the presence of WebAssembly modules. Changes to graph
reconstruction or emitted-code contracts require a revision bump.

Codegen revision 14 keeps the source value of a float literal that adapts to a
`float64` context, instead of rounding the constant through `float32` on the way.
Widening an actual `float32` value is unchanged. Regenerate older bundles.

Language version 4 adds integer/boolean match statements. Codegen revision 13
preserves match arm scopes and folded boolean comparisons. Regenerate older bundles.

Language version 3 replaces `input value` / `output value` with
`input parameter` / `output parameter`, and makes `value` an ordinary identifier. Version 2
bundles are rejected with a migration diagnostic: update their source and
regenerate them. The container format is unchanged. Runtime and native ABI 2
append a trace queue pointer to kernel and event contexts. Codegen revision 12
adds trace recording helpers; regenerate older bundles. The tracing option is
preserved so site metadata rebuilt from source matches the stored kernels.
Codegen revision 2 preserves short-circuit evaluation of logical operators
and ternaries, and checks dynamic array/stream indices; `select()` remains eager.
Codegen revision 3 preserves signed 64-bit source literals without passing
through floating point, checks structural sizes before narrowing, and rejects
explicit nonfinite source constants. Regenerate older artifacts.
Codegen revision 4 saturates integer kernel negation, absolute value and
`INT_MIN / -1` to the maximum of the operand width. Division/remainder by zero
and the overflowing remainder produce zero.
Codegen revision 5 maps NaN to zero before float-to-int conversions used by
bounded ring-index proofs. Regenerate older artifacts.
Codegen revision 6 folds integer shifts using the operand width and masked
count, matching native and WebAssembly execution. Regenerate older artifacts.
Codegen revision 7 respects source storage and destination integer widths when
folding numeric conversions. Out-of-range float-to-int conversions remain
unfolded. Regenerate older artifacts.
Codegen revision 8 makes float-to-integer conversions saturate at the destination
limits and map NaN to zero in native code, WebAssembly and constant folding.
WebAssembly uses non-trapping float-to-integer conversion instructions.
Regenerate older artifacts.
Codegen revision 9 preserves integer storage width during constant folding,
including arithmetic results consumed by comparisons. Regenerate older artifacts.
Codegen revision 10 removes array checks for proven nonoverflowing integer
products, including strided indices. Regenerate older artifacts.
Codegen revision 11 saturates integer kernel addition, subtraction and multiplication
at the operand width, preserving direct instructions for proven-safe operations.
Regenerate older artifacts.

### LIST SOUR

The list payload starts with FourCC `SOUR` and contains source/import chunks.

Each `SRCF` contains a source ID string, a one-byte root flag, and a source
text string. Exactly one source is the root. The compiler assigns
`source-0` to the root and subsequent IDs in deterministic import traversal
order. IDs must be non-empty and unique.

Each `IMAP` contains three strings: importing source ID, import spelling, and
imported source ID. Both IDs must refer to packaged sources. Nested imports
retain the identity of their importing source rather than depending on a
filesystem path. Parsing and disk reads are shared with compilation instead
of repeated in a separate source-collection pass.

### NATV

| Field | Encoding |
|---|---|
| Operating system | int32: macOS = 0, Linux = 1, Windows = 2 |
| Architecture | int32: ARM64 = 0, x64 = 1 |
| Kernel count | int32 |
| Each kernel's code | Length-prefixed byte blob |
| Each kernel's symbol count | int32 |
| Each symbol | Name string, then uint64 offset into that kernel's code |

Kernels use compiler order: process/init kernels first, then event handlers.
Their entry point is byte zero. Code is position-independent; helper calls
load addresses from eight-byte slots within the code image. Serialized slots
contain zero, never pointers into the compiling process.

The loader resolves named helpers such as `sin.f32`, `pow.f64`, and `emit`,
patches a private copy, then installs it using the native JIT runtime.
Unknown symbols, out-of-range slots and incompatible execution targets fail.
Foreign code can be inspected and serialized without being executed.

### WASM

An `int32` module count followed by length-prefixed wasm32 module blobs, in
the same process/init/event-handler order. Each blob includes the standard
WebAssembly magic and version header. The runtime registers these stored
modules in the current realm; `prewarmKernels()` is still available for
preparing an audio-worklet realm.

Event payload offsets are translated from the compiler host's pointer width
to the wasm32 ABI, including emitted-event staging.

### DIAG

An `int32` diagnostic count followed by these fields per record:

| Field | Encoding |
|---|---|
| Severity | int32: error = 0, warning = 1, info = 2 |
| Source ID | String |
| Start line, start column, end line, end column | Four int32 values |
| Diagnostic code | String |
| Message | String |

Diagnostics are retained for inspection. This chunk is optional.

## Validation and compatibility

The loader checks chunk bounds, required metadata/version/source records,
source uniqueness, import references, native-target uniqueness, symbol-slot
bounds and WebAssembly headers. At least one target must be present.
Instantiation also checks that the selected target's kernel count matches
the graph reconstructed from the sources.

Unknown top-level chunks are skipped. Unknown records inside `LIST SOUR`
are rejected. Native code remains trusted executable input even after these
checks.

## Command-line and CMake integration

```sh
yup_dsp_compiler patch.ydsp --output patch.ydsb --target macos-arm64 --target windows-x64
yup_dsp_compiler --inspect patch.ydsb --list
```

`--target` selects actual emitted native code, not metadata alone.
WebAssembly is included by default; without `--target` the bundle is
WebAssembly-only. `--fast-math` enables relaxed numerical transformations.

For CMake projects, `yup_add_ydsp_bundle()` invokes the host compiler at build
time. Include the native targets your application will instantiate:

```cmake
yup_add_ydsp_bundle (MyPatch
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/patch.ydsp"
    TARGETS macos-arm64 windows-x64)
```
