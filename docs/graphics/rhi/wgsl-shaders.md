# GLSL on the WebGPU target

WebGPU consumes WGSL. YUP produces it from the same GLSL (or ESSL) you write for
every other backend, using the `WgslTranspiler` that `ShaderTranspiler`,
`ShaderBundleCompiler` and `yup_shader_bundler` call for the WGSL target.

The transpiler has one rule: **a shader either becomes WGSL that behaves like the
GLSL, or the transpile fails** with a `line:column: message` diagnostic naming
the construct WGSL can't express. It never succeeds with different behavior.

## What is translated

Write GLSL 4.50 or ESSL 3.x as usual. The transpiler handles the places where
WGSL differs:

- **Types and conversions**: GLSL's implicit `int` / `uint` / `float`
  conversions become explicit, and constructors WGSL lacks are expanded
  (`mat3(m4)`, `mat2(s)`, `vec2(v4)`, `float(v)`, mixed matrix arguments).
- **Expressions with side effects**: `a[i++]`, assignments inside expressions,
  the comma operator, and `?:` / `&&` / `||` with side effects are rewritten into
  statements. Plain `?:` becomes `select()`.
- **Parameters**: `out` and `inout` parameters become pointers. Arguments that
  aren't plain locals (a swizzle, an array element, a global) go through a
  temporary that is written back after the call, as GLSL specifies. Parameters
  that the function writes are copied into locals.
- **Control flow**: `switch` fallthrough is expanded and a missing `default` is
  added. `do`/`while` and complex `for` loops use WGSL `loop` with a `continuing`
  block, so `continue` still evaluates the condition or the update.
- **Swizzle stores**: `v.xy = ...` and `v.rgb *= k` are split into per-component
  stores.
- **Builtin functions**: renamed where WGSL spells them differently. `mod`,
  `inverse`, `outerProduct`, `matrixCompMult`, `isnan`, `isinf`, `uaddCarry`,
  `usubBorrow` and `umulExtended` get small helper functions. `frexp` / `modf`
  and all texture functions are mapped too.
- **Names**: identifiers that are reserved in WGSL (`filter`, `target`, `ref`,
  `type`, ...) get a trailing underscore. Overloaded functions get distinct names.
- **Blocks and memory layout**: uniform blocks keep the std140 layout and storage
  blocks the std430 layout. Members get `@size` padding, std140 arrays of
  scalars or `vec2` and two-row matrices use 16-byte wrapper structs, `bool`
  members are stored as `u32`, and storage used with `atomic*()` functions
  becomes `atomic<T>`. Your CPU-side buffer layout stays exactly the GLSL one.
- **Stage IO**: locations are kept (missing ones are assigned in declaration
  order), interpolation qualifiers become `@interpolate`, integer varyings are
  always `flat`, matrix and array varyings use consecutive locations, and IO
  blocks are flattened. `invariant gl_Position` becomes `@invariant`.
- **Specialization constants**: `layout(constant_id = N) const` becomes an
  `@id(N) override`, and `local_size_x_id` makes the workgroup size overridable.
- **Dual-source blending**: `layout(location = 0, index = 1)` becomes
  `@blend_src(1)` and enables `dual_source_blending`.

## Bindings

Every resource keeps the `@group` / `@binding` of its GLSL `set` / `binding`.
ESSL samplers declared without a binding are numbered like glslang does: the
next free binding of their set, in declaration order. Only resources the entry
point uses are numbered; unused unbound ones are dropped from the WGSL.

A combined image sampler (`sampler2D` and friends) is split into a texture at
its own binding and a companion sampler named `<texture>_sampler`. **Companion
samplers take the bindings after the highest binding used in their group**, in
ascending order of their texture binding. `ShaderTranspiler::reflectFromSPIRV()`
with the WGSL target reports that binding as `backendSlotSecondary`.

```{note}
The RHI binding map does not describe combined samplers for WebGPU yet: in
shaders you ship through `GpuPipeline`, declare a separate `texture2D` and
`sampler` and combine them with `sampler2D(tex, samp)` at the call site, as the
bundled examples do.
```

## Not supported

These constructs have no WGSL equivalent. The transpile fails and says so:

- geometry and tessellation shaders, subpass inputs, transform feedback
- double precision types and literals
- `gl_PointSize` other than `1.0` (a `1.0` write is dropped with a warning),
  `gl_PointCoord`, `gl_ClipDistance`, `gl_CullDistance`, `gl_Layer`,
  `gl_PrimitiveID`, `gl_HelperInvocation` and the other builtins WGSL lacks
- `samplerBuffer`, `sampler1DArray`, rectangle textures, `sampler2DMSArray`,
  1D shadow samplers, arrays of textures or samplers, storage images other than
  1D, 2D, 2D array and 3D, and storage image formats WGSL doesn't have
- sampling integer textures (use `texelFetch`), `textureQueryLod`, biased or
  gradient shadow lookups and shadow lookups at a level other than 0
- push constants (use a uniform block), `row_major` matrices, `packed`,
  `shared` and `scalar` layouts, `layout(component)`
- storage buffers written from a vertex shader
- comparing structs, arrays or matrices with `==`
- `interpolateAt*()`, `imulExtended()`, image atomics, atomic counters

`layout(early_fragment_tests)` is accepted with a warning: WGSL has no way to
request it. Pass a `StringArray*` in `WgslTranspileOptions::warnings` to collect
warnings.

## Differences to keep in mind

- **Clip space**: like the MSL and HLSL outputs (without `flipVertY`), WGSL
  output keeps `gl_Position` as written. WebGPU's clip space has Y up and a
  `[0, 1]` depth range, the same conventions as Metal and Direct3D.
- **Barriers**: `memoryBarrierShared()` and `groupMemoryBarrier()` become
  `workgroupBarrier()`, which also synchronizes execution. WGSL requires barriers
  in uniform control flow.
- **`atomicCompSwap`** is emulated with a retry loop around
  `atomicCompareExchangeWeak`, so it keeps GLSL's strong semantics.
