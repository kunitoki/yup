# YDSP optimization

A YDSP patch compiles to native code every time you load it, so the compiler
has to decide - on the machine it is running on - how wide to vectorize, which
transcendental implementations to call and whether to contract multiplies and
adds. `YdspCompileOptions` lets you steer those decisions, and
`YdspOptimizationReport` tells you what was decided.

The defaults are the right choice for an audio plugin: compile for the host
CPU, pick the fastest safe code, and keep the result bit-identical across
runs of the same binary on the same machine. Reach for this page when you need
stricter numerics, reproducible output across machines, or an explanation for
why a patch is slower than you expect.

## Compile options

```cpp
yup::YdspCompileOptions options;
options.optimizationTier = yup::YdspOptimizationTier::automatic;
options.fastMath = true;
options.emitOptimizationReport = true;

yup::YdspCompiler compiler;
auto graph = compiler.compile (source, options);

if (! graph)
    yup::Logger::writeToLog (compiler.getDiagnostics().toString());
```

| Option | Default | What it does |
| --- | --- | --- |
| `targetPolicy` | `YdspTargetPolicy::host` | `host` compiles for the CPU the code is running on. `baseline` compiles for `baselineTarget` instead, so the same instructions are emitted everywhere. |
| `baselineTarget` | `YdspNativeTarget::scalar` | The instruction set used when the policy is `baseline`: `scalar`, `sse2`, `avx2`, `avx512` or `asimd`. Ignored under the `host` policy. |
| `optimizationTier` | `YdspOptimizationTier::automatic` | `baseline` turns every loop transform off and keeps the kernels scalar. `automatic` enables vectorization, unrolling and reduction splitting. `aggressive` currently behaves like `automatic` and notes in the report what it would additionally need. |
| `fastMath` | `true` | See [Fast math](#fast-math) below. |
| `emitOptimizationReport` | `false` | Fills in the report described in [Reading the report](#reading-the-report). It costs a little compile time, nothing at runtime. |
| `enableTracing` | `false` | Keeps `trace` statements alive. When false, all tracing work is removed from the kernels - but trace strings and names are still checked, so a patch that compiles with tracing off compiles with it on. |
| `sourceOverrides` | empty | Unsaved editor text, keyed by absolute path, used instead of reading those files from disk. This is what a hot-reloading editor compiles from; see [Hot reload](command-line.md#hot-reload). |

```{note}
`optimizationTier` and `targetPolicy` are requests. The compiler only emits an
instruction set the CPU actually has, and only applies a transform it finds
profitable, so what you asked for and what you get can differ. The report says
which it was.
```

## Fast math

`fastMath` is on by default and changes three things:

- **Transcendentals.** A loop calling `sin`, `cos`, `tan`, `atan2`, `tanh`,
  `exp`, `log`, `pow` and friends is widened to [SLEEF](https://sleef.org)'s
  four-lane `u35` routines, accurate to 3.5 ULP. With `fastMath` off, those
  loops stay scalar on the platform's libm - the strict vector path measured
  slower than the scalar libm it would replace, so it is not taken. Scalar
  `float` values always go through libm either way.
- **Contraction.** A multiply followed by an add may be fused into a single
  rounded operation. With `fastMath` off, nothing is contracted and every
  operation rounds separately. An explicit `fma()` in the source stays one
  rounding in every tier.
- **Order of operations.** The additions of a contracted chain such as
  `a * b + c - d * e` may be reordered so the product that arrives last - the
  output of the previous stage, or a value fed back from the previous sample -
  is added last, and `x * k1 * k2` multiplies by `k1 * k2` once when both are
  fixed for the block. A sum built one term at a time over many statements -
  `acc = acc + c[i] * h[i]` across the taps of a FIR - may be added up in a
  few partial sums side by side. The result differs from the order as written
  by a rounding, as with contraction. With `fastMath` off, operations keep the
  order of the source.

Turn it off when a patch is a reference implementation, when you are comparing
output against a double-precision model, or when a resonant structure at the
edge of stability behaves differently from the offline version. Expect it to
cost throughput in anything built out of transcendentals; for everything else
the 3.5 ULP tier is far below the noise floor of 24-bit audio.

```cpp
options.fastMath = false; // strict 1-ULP transcendentals, no contraction or reordering
```

```{admonition} Determinism
:class: tip
Contraction and reordering are applied consistently, and a target without a fused
multiply-add instruction - WebAssembly included - expands it through an exact
double-precision sequence that rounds once. The same source therefore produces
the same samples on every supported target, with or without `fastMath`.
```

## Targets and portability

Under the default `host` policy the compiler reads the CPU's features and emits
the best instruction set it finds: SSE2 or AVX2 on x86-64, ASIMD on AArch64,
scalar code where there is nothing better. In the browser it emits four-lane
`f32x4` WebAssembly SIMD, as long as the application itself was built with
`-msimd128` - Emscripten's default. A scalar WASM build keeps every loop
transform off and records why in the report.

That is what you want in an application: the patch is compiled on the machine
that will run it. It is *not* what you want when you need two machines to
produce identical samples, or when you are measuring one instruction set on a
CPU that has a wider one. For those, pin the target:

```cpp
options.targetPolicy = yup::YdspTargetPolicy::baseline;
options.baselineTarget = yup::YdspNativeTarget::sse2;
```

Precompiled [bundles](bundle-format.md) take this further: their native kernels
always use baseline scalar instructions, so a `.ydsb` never depends on the CPU
features of the machine that built it, and instantiating one loads the stored
code rather than generating new code for the host. Compile from source with
`YdspCompiler::compile()` when you want the host's instruction set.

```{note}
AVX-512 is never selected today, even on a CPU that has it: on several
microarchitectures the wider registers cost more in clock throttling than they
return, and the compiler has no benchmark feedback to tell the two cases
apart. It uses AVX2 instead and says so in `rejectedTransforms`.
```

## Reading the report

Set `emitOptimizationReport` and read `YdspCompiler::getOptimizationReport()`
after a successful compile. It describes the code that was actually emitted:

```cpp
yup::YdspCompileOptions options;
options.emitOptimizationReport = true;

yup::YdspCompiler compiler;
auto graph = compiler.compile (source, options);

if (graph)
{
    const auto& report = compiler.getOptimizationReport();

    yup::Logger::writeToLog (yup::String ("isa: ") + report.selectedMicroarchitecture
                             + ", lanes: " + yup::String (report.vectorWidth)
                             + ", vectorized: " + yup::String (report.vectorizationEnabled ? "yes" : "no")
                             + ", code: " + yup::String (report.generatedCodeSize) + " bytes"
                             + ", compiled in " + yup::String (report.compileTimeMilliseconds, 2) + " ms");

    for (const auto& rejected : report.rejectedTransforms)
        yup::Logger::writeToLog ("rejected: " + rejected);
}
```

| Field | What it tells you |
| --- | --- |
| `optimizationTier`, `fastMath` | The settings the compile ran with, echoed back. |
| `selectedIsa` | The instruction set actually emitted, after capability and profitability checks. |
| `selectedMicroarchitecture` | The CPU the host policy identified, as a readable name. |
| `vectorWidth` | How many samples one widened operation processes. `1` means scalar code. |
| `vectorizationEnabled` | Whether the kernels were widened at all. |
| `unrollingEnabled` | Whether bounded loops were fully unrolled. |
| `reductionSplittingEnabled` | Whether accumulation chains were split so lanes accumulate independently. |
| `contractionEnabled` | Whether multiply-add contraction was applied. |
| `rejectedTransforms` | Every transform the compiler considered and declined, with its reason. This is the field to read when a patch is slower than expected. |
| `generatedCodeSize` | Size of the emitted machine code, in bytes. |
| `compileTimeMilliseconds` | How long the compile took - useful when a hot-reload cycle starts to feel slow. |
| `cacheHit`, `cacheDecision` | Whether the compiled code came from the cache, and why. |
| `benchmarkDecision` | The outcome of the cost measurements that choose between candidate targets. |

A report requested on one compile describes that compile only;
`getOptimizationReport()` always returns the most recent one. When the last
compile did not request a report, `generatedCodeSize` and
`compileTimeMilliseconds` are zero.

## What the compiler does on its own

You do not schedule any of this; it is listed so the report's fields mean
something. Between parsing and code generation the compiler folds constants,
simplifies arithmetic, turns small branches into selects, forwards stores to
loads, eliminates common subexpressions and dead code, hoists loop-invariant
work out of the sample loop, removes array bounds checks it can prove are never
taken, fuses and unrolls loops, widens them to the target's vector width,
splits reduction chains so lanes accumulate independently, keeps scalar state
and small constant-indexed state arrays in registers across the sample loop,
and contracts and reorders multiplies and adds where `fastMath` allows it.

A few consequences are worth knowing when you write a patch:

- **A `process block` keeps scalar state in registers too.** It is read once
  and written back once, so a recurrence written inside a
  `for n in 0..blockSize` loop does not go through memory on every sample.
- **Coefficients computed in the process body run once per block.** Anything
  that depends only on parameters, the sample rate and constants - a `tan`, an
  `exp`, a `?:` that picks between two such values - is moved ahead of the
  sample loop. You do not need to restructure the patch to get that.
- **A state array that only its `{ ... }` initialiser writes is a table of
  constants.** `m[3]` is then the literal you wrote, and is folded like one.
  One assignment to an element anywhere in the processor makes the array
  ordinary state again.
- **A bank loop may use its variable as a number.** `float (i)` and
  `float (i + 1)` - the harmonic number of an additive bank - no longer keep a
  constant-bound loop scalar.

- **A small state array indexed by constants is as cheap as scalar state.** The
  stages of a filter written as `z[0]`, `z[1]`, ... (up to 8 elements per
  array, 16 per processor) live in registers for the whole block. An index
  that is only known at run time keeps the array in memory.

- **Bounds checks disappear when they can be proven redundant.** Indexing a
  delay line with an expression the compiler can bound - a wrapped counter, a
  `%` by the array length, a mask - costs nothing. An index it cannot bound
  keeps its check. The same reasoning removes the overflow checks of integer
  arithmetic, so it pays to let the compiler see the range of a position: write
  it as `i = i + 1; if (i >= n) { i = 0; }` or as `i = (i + 1) & mask`, and
  `i - k` and `(i - k) & mask` are then plain instructions. A `for` variable
  with constant bounds has a known range too.
- **A kernel calling vectorized transcendentals may be capped to four lanes**,
  because the SLEEF routines come in four-lane form. A patch full of `sin` and
  `exp` will not reach AVX-512 width; one made of multiplies and adds will.

## Making a patch faster

1. Compile with `emitOptimizationReport` on and look at `vectorWidth` and
   `vectorizationEnabled` first. Scalar code is the single biggest thing to
   fix.
2. Read `rejectedTransforms`. It names what the compiler wanted to do and
   could not.
3. Keep the per-sample body branch-free where you can. Small branches become
   selects and stay vectorizable; branches around large blocks do not.
4. Bound your array indices visibly, so the checks fold away.
5. Prefer the plain arithmetic operators over transcendentals in the inner
   loop, and lift anything that does not change per sample into an `init`
   block.
6. Leave `fastMath` on unless you have a reason not to.

## Related pages

- [Language reference](language.md) - the realtime contract and the execution
  report that covers allocation and realtime safety, which are separate from
  the native-code report described here.
- [Compiler and player](command-line.md) - `yup_dsp_compiler --fast-math` and
  the rest of the command-line flags.
- [Bundle format](bundle-format.md) - what a precompiled `.ydsb` contains and
  how it is recompiled for the host.
