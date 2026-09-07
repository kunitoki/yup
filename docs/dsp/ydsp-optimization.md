# YDSP optimization

How the `yup_dsp_jit` compiler turns a patch into fast native code, and the
levers you have over the trade it makes between speed and strictness.

## fastMath and accuracy tiers

`YdspCompileOptions::fastMath` is **enabled by default on native targets**.
Native kernels lower with SLEEF's `u35` tier (about 3.5 ULP error) and allow
fused multiply-add contraction, which changes rounding. The WebAssembly
backend always stays strict: it does not link SLEEF and never contracts,
regardless of the flag.

Set `fastMath = false` for strict 1-ULP native behavior:

- scalar float32 transcendentals stay on the platform libm;
- a *widened* (vectorized) transcendental uses SLEEF's `u10` tier instead of
  `u35`, keeping ~1 ULP parity with libm;
- `a * b + c` is no longer contracted.

The scalar transcendental path stays on libm even under fastMath: the
platform libm scalar is already per-target tuned, and measured SLEEF scalar
calls regressed against it (for example `exp` on Apple silicon). fastMath's
scalar benefit is contraction alone.

## SLEEF-backed vector math

SLEEF (vendored as the `sleef_library` module, Boost license) supplies the
4-lane elementary functions the vectorizer widens into. When a loop's widened
value feeds a transcendental (`sin`, `cos`, `tan`, `asin`, `acos`, `atan`,
`sinh`, `cosh`, `tanh`, `asinh`, `acosh`, `atanh`, `exp`, `log`, `log10`,
`pow`, `atan2`, `fmod`), the loop is vectorized instead of being refused, and
the codegen lowers it to 4-lane SLEEF calls - at `u35` under fastMath and
`u10` otherwise.

- SSE2 / ASIMD targets widen at four lanes and call SLEEF directly.
- AVX2 widens at eight lanes; an 8-lane transcendental splits into two 4-lane
  SLEEF calls (`vextractf128` + `vinsertf128`). A dedicated 8-lane SLEEF
  build is future work.
- The WebAssembly backend does not link SLEEF: it keeps strict libm-only
  transcendentals and never widens a transcendental loop.
- Rounding ops (`round`/`floor`/`ceil`) are not widened, and `copysign`
  stays scalar libm (its branch-free lowering needs a float bitwise hook the
  backends do not expose yet).

Compile-time flags that land on every kernel and drive these choices:
`YdspIrFunction::fastMath` and `YdspIrFunction::vectorMathEnabled`, mirrored
from `YdspOptimizer::setFastMath()` / `setTargetHasVectorMath()`.

## Optimizer passes

Beyond the base pass pipeline, the JIT borrows three moves from the HISE/SNEX
reference JIT:

- `x / c -> x * (1 / c)` under fastMath (single-use constant divisors, e.g.
  the denominators in filters and oscillators);
- `x % 2^k -> x & (2^k - 1)` when the dividend is provably non-negative
  (masked values, non-negative constants) - semantics-preserving in every
  tier;
- fusion of adjacent, same-bound, memory-disjoint loops into one loop before
  the vectorizer (per-sample loop-overhead is halved); conservative
  preconditions keep it from changing what the second loop observes.

Constant math calls fold at compile time for the full intrinsic family
including `asinh`/`acosh`/`atanh`/`round`. The codegen also runs a redundant
move cleanup over the compiler's node list (the register allocator's own
moves are coalesced by asmjit itself; no post-allocation peephole is exposed
by asmjit's Compiler API).

## Register allocation

Native kernels are lowered through asmjit's linear-scan allocator with a few
hints:

- loop induction variables, widened (4/8-lane) values, and the hoisted
  per-stream base pointers carry maximum allocator weight, so they are never
  spilled;
- the allocator's verbose diagnostics (`kRAAnnotate | kRADebugAll`) run only
  when the `YUP_YDSP_RA_DEBUG` environment variable is set, not on every
  compile;
- AVX-512 stays disabled (it can downclock several CPU families) until an
  empirical microarchitecture cost model exists; the rejection is recorded in
  `YdspOptimizationReport::rejectedTransforms`.

## Reading the result

- `YdspCompileOptions::emitOptimizationReport` fills
  `YdspOptimizationReport` with the selected ISA, vector width, enabled
  transforms, rejected transforms and generated code size.
- Missed-vectorization diagnostics explain why each loop stayed scalar
  (`YdspKernelReport::loopVectorization`).
- `tests/yup_dsp_jit/yup_YdspBenchmarkTests.cpp` prints per-policy
  `ns/sample` against hand-written C++ references (baseline strict, baseline
  + fastMath, host strict, host + fastMath default) plus a listing summary
  that splits per-sample transcendental calls from other calls.

## Accuracy notes

`u35` results differ from libm by design (up to ~3.5 ULP). Anything that
compares JIT output bit-exactly against a strict reference must either compile
with `fastMath = false` or use the documented relative tolerance.
