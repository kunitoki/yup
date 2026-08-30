# YDSP — A Realtime JIT-Compiled Audio DSP Language

YDSP is a small programming language for audio signal processing, embedded in
the `yup_dsp_jit` module. A YDSP program is compiled ahead of the audio
callback into machine code that runs under hard-realtime constraints: the
generated code performs **zero heap allocation, takes no locks and can never
block or throw**. Two backends are provided: an [AsmJit](https://asmjit.com)
backend that emits native x86-64/AArch64 code on desktop targets, and a
WebAssembly backend (used on `YUP_WASM`/emscripten targets) that emits a
portable wasm module instantiated and run by the browser's native
`WebAssembly` engine.

YDSP borrows two ideas:

- From **Faust**: a block-diagram *composition algebra* (`:` sequential,
  `,` parallel, `<:` split, `:>` merge, `~` recursion, `_` passthrough), a
  `process = ...;` entry definition, one-sample delay primitives (`mem`,
  `'`, `@`) and UI-style parameter annotations.
- From **Cmajor**: a readable `processor` surface with typed endpoints
  (`input stream`, `output stream`, `input value`, `output value`), `state`
  history variables, `graph`/`node`/`connection` declarative wiring, and the
  deterministic-realtime ethos (no pointers, no dynamic allocation, no
  recursion, statically bounded loops).

## 1. Program structure

A YDSP program is a list of statements:

```ebnf
program        = { import_statement | declare_statement | constant_declaration
                 | function_declaration | processor_definition | graph_definition } ;
import_statement = "import" identifier { "." identifier } [ "as" identifier ] ";" ;
constant_declaration = "let" identifier "=" expression ";" ;
function_declaration = "func" identifier "(" param_list ")" [ ":" type ]
    "{" { statement } "}" ;
```

A valid program contains exactly one `graph` definition (the entry point) and
any number of `processor` definitions it references. Metadata is optional. A
file that declares only top-level `func`s is a **library**: it has no graph of
its own and is pulled into other programs with `import` (see [1.3](#13-imports)).

### 1.0 Program constants

A program-scope `let` declares a **compile-time constant**. It is const-folded
before name resolution, so it is usable in three places: as a `state` array
size, as a `for` bound, and inside any expression. The purely structural
integers of the graph — a node's voice count `Processor[N]`, an oversampling
factor `* N`, a connection's inline delay `[N]` — still require an integer
literal.

```ydsp
let harmonics = 32;
let twoPi     = 2.0 * pi;

processor P {
    state float amp[harmonics];
    process { for i in 0..harmonics { /* ... */ } }
}
```

The initialiser must fold at compile time: literals, the numeric builtin
constants (`pi`, `e`, `inf`), unary `- ~ !`, the arithmetic operators
`+ - * / %`, and the bitwise/shift operators, over previously declared
constants. `sampleRate` and `blockSize` are runtime values and are rejected.

An integer-valued constant stays an integer (so it can size an array); anything
else becomes a `float32`. Constants cost nothing at runtime: no IR is emitted
for them. Redeclaring a constant's name as an endpoint, state, local, loop
variable, function, node or event parameter is an error.

Imported constants are namespaced like imported processors — `import x as fx;`
exposes `fx.someConstant`.

### 1.1 Metadata

```ydsp
declare name        "MyPatch";
declare author      "Jane Doe";
declare description "A saturating delay";
```

Allowed keys: `name`, `author`, `version`, `license`, `description`. Metadata
is surfaced through `YdspDiagnostics`/the compiler result and is otherwise
ignored by code generation.

### 1.2 Comments and literals

- Comments: `// line` and `/* block */`.
- Integer literals: `0`, `42`, `-7` (decimal).
- Float literals: `1.0`, `0.5`, `1e-3`, `2.5e+2`.
- Boolean literals: `true`, `false`.
- Strings: `"..."` (metadata and annotations only).

Identifiers are `[A-Za-z_][A-Za-z0-9_]*`. The identifiers `in` and `out` are
reserved and may only be used as stream names in endpoint declarations.

### 1.3 Imports

A program may pull processor definitions from other `.ydsp` files:

```ydsp
import fx.Delay as fx;
```

- `import` is a top-level statement, legal anywhere among the other top-level
  statements. The dotted path `X.Y.Z` maps to the file `X/Y/Z.ydsp`; the
  optional `as alias` namespaces the imported names (without an alias, the
  last path segment is used, so `import fx.Delay` exposes `Delay.*`).
  Imported processors are then referenced as `alias.ProcessorName` in `node`
  declarations, e.g. `node dly = fx.Delay;`.
- An imported file is lexed, parsed and recursively import-resolved; its
  `constant`, `func`, `processor` and `graph` definitions are merged into the
  program under the namespace prefix, so an imported graph is used as
  `node bus = fx.MasterBus;` (see [3.3](#33-subgraphs)) and an imported
  library function as `fx.helper (x)` inside any processor body. An imported
  graph is always a subgraph: any `[[ main ]]` it carries applies to its own
  file, never to the importing program.
- Paths resolve relative to the importing file: nested imports inside an
  imported file resolve against that file's folder, and the top-level
  program's imports resolve against the `importBasePath` argument of
  `YdspCompiler::compile()` (falling back to the process's current working
  directory when it is empty — see [6](#6-public-api-c)). Circular imports are
  an error, as is importing two different files that would share a namespace
  (use `as` to disambiguate); importing the same file twice is fine. The same
  file may also be pulled in by several importers (a diamond): each importer
  gets its own merged copy under its own namespace, so two files importing the
  same library under different aliases both work.

## 2. Processors (kernels)

A `processor` is a reusable signal-processing kernel. It declares typed
endpoints, persistent `state`, and a `process` body that runs either **per
sample** or **per block**.

```ydsp
processor Saturator {
    input  stream in;
    input  stream side;                      // sidechain input
    output stream out;

    input  value float drive = 0.5 [[ name: "Drive", min: 0, max: 2 ]];
    output value float level;                // meter, written by the kernel

    state  float z;                          // scalar history
    state  float buf[256];                   // history array (e.g. delay memory)
    state  int   wp;                         // integer state

    process {                                // per-sample body
        out = tanh (in * drive) * (1 + 0.5 * side);
        z   = 0.999 * z + in;
        level = abs (z);
    }
}
```

### 2.1 Endpoints

| Kind | Syntax | Semantics |
|------|--------|-----------|
| Audio input | `input stream name;` | A block of `blockSize` samples. Element type is `float32` by default (declared with a type keyword for `float64`/`int32`/`int64` streams). |
| Audio output | `output stream name;` | Written by the kernel. |
| Sidechain | an extra `input stream` | Just another input stream. |
| Parameter (read) | `input value float name = default;` | Host-automatable value, **sampled once per block**. Element type from the type keyword (`float32` default; `float64`/`int32`/`int64` supported). |
| Meter (write) | `output value float name;` | Written by the kernel, read by the host. |
| Event input | `input event <name>;` | A named event input channel (processor and graph scope). A processor may declare several, each carrying all seven shapes; a handler selects the shape it processes (see [2.11](#211-events)). A graph's own `input event` reaches a node only through an explicit `connection { }` entry, exactly like a stream. |

Endpoint annotations `[[ key: value, ... ]]` carry UI metadata (`name`,
`min`, `max`, `step`, `unit`, `style`, `mid`, `bipolar`) and are exposed
through the public API.

`[[ init: <value> ]]` is accepted as an alias for the endpoint's default value,
so an annotation block pastes in unchanged. An explicit `= <value>` always
wins; the annotation only applies when there is none:

```ydsp
input value float rate [[ name: "Rate", min: 0.5, max: 12.0, init: 4 ]];
```

`[[ smoothing: <seconds> ]]` de-zippers a `float32` parameter: the per-sample
body reads a one-pole ramp towards the parameter instead of the stepped value
the host writes. It is pure sugar for a `smooth()` local (see
[2.9](#29-delay-and-smoothing-primitives)) and is legal only on an `input value
float` of a processor with a per-sample `process { }` body:

```ydsp
input value float cutoff = 1500.0 [[ name: "Cutoff", min: 60.0, max: 12000.0, smoothing: 0.02 ]];
```

`[[ values: { "a", "b", ... } ]]` marks a parameter as **discrete**: the list's
labels are exposed through the public API (`YdspParameterInfo::discreteValues`)
and sit evenly spaced over `[min, max]`, so a UI snaps the control to those
steps and shows the matching label instead of the raw number. It is legal on any
`input value` parameter — of a processor or of a graph — and needs at least two
labels:

```ydsp
input value float wave = 0.0 [[ name: "Waveform", min: 0.0, max: 3.0,
                               values: { "Saw", "Square", "Triangle", "Pulse" } ]];
```

Unlike `[[ smoothing ]]`, it carries no codegen — it is host-facing metadata, like
the `name`/`min`/`max` a graph endpoint already publishes — so graph scope is
allowed, and is where a selector that drives *several* nodes has to declare it. A
graph parameter aliased onto node parameters replaces them in the host's
enumeration (see [3.1](#31-graph-declarations)), so labels on the node's own
parameter would never be read; one vowel control moving three formant filters has
to name its five vowels on the graph endpoint.

`[[ mid: <value> ]]` names the value that should sit at the middle of a host
slider's travel, so a UI can derive a logarithmic skew for the control
(`YdspParameterInfo::midValue`, absent unless annotated). Combined with
`[[ min: ... ]]` / `[[ max: ... ]]` it lets a frequency knob spend half its
travel below the midpoint and the rest above it:

```ydsp
input value float cutoff = 700.0 [[ name: "Cutoff", min: 30.0, max: 12000.0, mid: 632.46 ]];
```

`[[ bipolar: true ]]` (default `false`) marks a parameter whose range is centered
on zero, e.g. a pan or pitch-offset control (`YdspParameterInfo::bipolar`). Like
`mid`, it carries no codegen - it is host-facing metadata.


At **graph scope** event inputs are named channels too, declared as
`input event <name>;`. They carry no state and produce no IR; the runtime
dispatches each one's `MidiBuffer` only to the nodes it is explicitly
connected to (`<name> -> node.event;` in a `connection { }` block - see
[2.11](#211-events) and the span-based `process()` overload in
[6](#6-public-api-c)).

### 2.2 State (history)

`state` variables persist across blocks. They may be scalars (`float`, `int`)
or arrays of compile-time constant size (`float buf[256]`, or `float buf[N]`
for a program constant `N` — see [1.0](#10-program-constants)). State memory is
allocated once in `YdspAudioGraph::prepare()` and lives for the lifetime of the
graph, and is zeroed by `prepare()` and `reset()`.

A `state` declaration may carry an initialiser:

```ydsp
state float feedback = 0.5;                 // scalar
state float table[4]  = { 1.0, 0.5, 0.25 }; // fewer values than N: the rest stay zero
```

Initialisers are lowered into the processor's `init` kernel (synthesising one if
the processor has no explicit `init { }` block) and therefore run once per voice
slice before audio, and again on `reset()`. They are prepended, so an explicit
`init { }` body observes the initialised values. Supplying more values than the
array holds is an error, and struct-typed state cannot be initialised this way.

#### Struct state

A processor may declare `struct` types and use them as state, which is how a
patch groups the per-instance state of a repeated algorithm (a voice, a comb
filter, a biquad section) without four parallel arrays:

```ydsp
processor CombBank {
    struct Comb {
        float buf[1024];
        int   writePos;
        float feedback;
    }

    state Comb combs[4];      // an array of instances
    state Comb single;        // a single instance

    process {
        for i in 0..4 {
            let delayed = combs[i].buf[combs[i].writePos];
            combs[i].buf[combs[i].writePos] = in + delayed * combs[i].feedback;
        }
    }
}
```

Structs are declaration-only sugar over the existing state layout: the IR
builder flattens each field into the scalar or array segment (a scalar field of
a scalar instance becomes a scalar slot; every other combination becomes an
array region of `fields * instances` elements). Access forms are
`instance.field`, `instances[i].field`, `instance.arrayField[j]` and
`instances[i].arrayField[j]`. Structs are not a value type: they cannot be
declared as locals, passed to functions, returned, or used as an endpoint type,
and struct-typed state cannot carry an initialiser list.

Because each field is laid out separately, an array field is *not* interleaved
per instance — `instances[i].buf[j]` addresses `base + i * fieldSize + j`. Four
parallel `state float` arrays remain the better choice when a loop needs to walk
one field of every instance with unit stride.

### 2.3 Process bodies

**Per-sample** (`process { ... }`): the body is run once per input sample,
inside an implicit loop over the block. Stream endpoints are scalars: `in`
and `out` name a single sample. Reading a scalar `state` variable loads its
previous value; assigning it persists the new value for the next sample. Local
variables are re-initialised every sample.

**Per-block** (`process block { ... }`): the body is run once per block.
Stream endpoints are arrays: `in[i]`/`out[i]` index samples (`i` in
`0..blockSize`). State reads/writes are ordinary memory accesses (the
optimiser may hoist or reg-promote them). This mode is required for block-wise
algorithms (delay-line index updates, meters, FFT-style processing).

A processor has exactly one `process` body, in exactly one mode.

### 2.4 Local stack variables

```ydsp
process {
    let t = in * drive;          // immutable local
    float smoothed = 0.9 * t;    // mutable local
    int   i = 0;                 // typed local
    ...
}
```

Locals are stack slots (AsmJit virtual registers), never heap. `let` bindings
are immutable; typed locals are mutable. Typing is **strict**: implicit
conversions are not allowed — only the *contextual literal adaptation* rule
(an integer literal may take any integer type, a float literal any float
type) and explicit casts (`int32(x)`, `int64(x)`, `float32(x)`, `float64(x)`;
`int(x)`/`float(x)` are aliases of the 32-bit casts).

### 2.5 Statements

```ebnf
statement = assignment | if_statement | for_statement
          | local_declaration | block ;
```

- `target = expression ;` where target is a local, a `state` variable
  (`z = ...`, `buf[i] = ...`), a stream output (`out = ...`, `out[i] = ...`)
  or an endpoint value (see below).
- `if (condition) statement [else statement]`.
- `for ident in start .. end statement` — see [2.6](#26-loops).
- Local declarations: `let ident = expr;` or `type ident [= expr];`.
- `{ statement* }` blocks introduce a scope.

**Parameter writing**: an `output value` endpoint may be assigned in either
mode. An `input value` endpoint (parameter) may be *read* in both modes but
only *written* in block mode (per-block automation, e.g. an envelope driving a
filter cutoff). Sample-mode writes to parameters are rejected.

### 2.6 Loops

Loops are the only iteration construct — there is **no `while`, `do`, `break`
or `continue`**, and no recursion. A loop bound must be *statically provable*:

```ydsp
for i in 0..blockSize { out[i] = in[i] * drive; }   // block-driven bound
for j in 0..16     { sum += buf[j]; }               // constant bound
```

`0..N` iterates `i = 0, 1, ..., N-1` (upper bound exclusive). `N` must be a
non-negative integer literal, a program constant (which folds to one — see
[1.0](#10-program-constants)), or `blockSize` (or `blockSize` plus/minus an
integer literal, e.g. `blockSize - 1`). Inside an event handler the bound must
be constant. Nested loops are allowed; the product of all loop bounds is the
loop's worst-case iteration count, reported by the optimiser (see
[7](#7-realtime-safety-and-the-execution-report)).

The loop variable is scoped to the body: it is not visible after the loop, and
sibling loops may reuse the same name.

#### Automatic vectorisation

On the native (asmjit) backends the optimiser widens a constant-bound loop over
parallel `state` arrays - and, in a block-mode processor, the per-sample loop
over `in[i]` / `out[i]` - to the selected float32 width: four lanes for the
portable SSE2/ASIMD targets, eight lanes for AVX2, and four f32x4 lanes on the
WebAssembly backend when the module is compiled with `-msimd128` (the emscripten
default). This is the shape an oscillator bank, a modal filter bank, a partial
bank, or a per-sample gain/mix has:

```ydsp
for i in 0..16 {
    z[i] = z[i] * coeff + in;   // one packed operation per target lane group
    sum = sum + z[i];           // one packed add, folded once after the loop
}
```

This is an optimisation, not a language feature: nothing in the patch selects
it, and a loop that does not qualify simply stays scalar. A loop is widened only
when **all** of the following hold:

- the trip count is a compile-time constant with a constant start, or it is
  `blockSize` and the body reads or writes a stream at the loop variable. When
  the constant span is not a whole multiple of the vector width, the leading
  scalar remainder is peeled as straight-line copies before the vector loop; a
  `blockSize` loop gets a rolled scalar tail loop after the vector loop instead
  (a runtime remainder cannot be peeled). A constant-bound `state`-array bank
  whose span is shorter than one vector stays scalar;
- the body is a single block — no `if`, no nested loop;
- every array or stream element is reached through the loop variable itself
  (`z[i]`, `in[i]`, `out[i]`, not `z[j]` or `z[wp]`), and the loop variable is
  used for nothing else (`float (i)` disqualifies the loop, because the widened
  counter advances by the selected vector width);
- the only value carried between iterations is an accumulation (`sum = sum + x`),
  one per accumulator;
- the body touches no scalar `state`, and no `'`, `@` or `smooth` slot. A
  stream store anywhere but through the loop variable disqualifies the loop
  (it would write the same element every iteration);
- the body applies only `+ - * /`, `min`, `max`, `abs`, `sqrt`, `clamp` and
  `lerp` to widened values. A transcendental, a comparison or a `select` on a
  widened value leaves the loop scalar — but work that is *invariant* across the
  loop (a shared `exp`-derived coefficient, say) has already been hoisted into
  the loop's preheader by then, so it stays scalar and is broadcast once.

**An accumulation is not bit-exact.** Lane *j* accumulates elements *j*,
*j+W*, *j+2W* … and the lanes are then added in a target-specific tree. That
reassociates the sum and also removes the serial dependency between
iterations, and so most of the speedup. Element-wise work (a loop with no
accumulator) is bit-exact, and the peeled scalar remainder is bit-exact too.

**Why a loop stayed scalar is reported, per loop.** `YdspKernelReport` gained
`loopVectorization`, one `YdspVectorizationResult` per original loop: `widened`
with the lane count, or the exact rejection reason (`shortTripCount`,
`unsupportedWidenedOp` - a select, comparison, transcendental or rounding on a
widened value - `indirectAccess`, `loopCarriedValue`, `nonConstantStart`, and
so on). `YdspVectorizationReport::rejectionReasons()` deduplicates the human
text, and `YdspVectorizationResult::describe()` renders one line ("loop 1 was
not vectorized: a select, comparison, transcendental or rounding consumes a
widened value"). When `YdspCompileOptions::emitOptimizationReport` is set,
the same lines are also emitted as *info* diagnostics, the LLVM `-Rpass`
equivalent; without it, compilation stays silent and the structured report is
still available from the execution report.

The WebAssembly backend lowers the same widened IR to f32x4 when the module is
compiled with `-msimd128` (the emscripten default, which defines
`__wasm_simd128__`); without it, wasm stays scalar and a widened kernel is
rejected rather than silently miscompiled. wasm SIMD is 128-bit, so the wasm
vector width is always four lanes. It is deliberately separate from the x86 and
AArch64 host target selection. An accumulating bank loop can therefore differ in
the last bits between a desktop build and a browser build.
`YdspAudioGraph::getExecutionReport()` reports which kernels were widened and at
what lane count (see [7](#7-realtime-safety-and-the-execution-report)).

#### Automatic loop unrolling

On the native backends and the wasm SIMD backend (compiled with `-msimd128`),
a small loop whose trip count is a compile-time constant is then written out in
full, so the per-iteration bound compare and back edge disappear. This runs
*after* widening, and so unrolls at the widened trip count - `for i in 0..16`
over four lanes is four copies, not sixteen - and it declines a loop that is
nested, runtime-bound, or large enough that the copies would cost more in
instruction cache than the branches cost in issue slots. A scalar wasm build
(no `-msimd128`) stays rolled: a module is downloaded and parsed before it
runs, and the browser's own engine re-optimises the loop regardless.

It is bit-exact by construction: the body is copied verbatim, in order,
including the loop variable's own increment, so the unrolled code is the
identical instruction sequence the loop executed. Nothing about it is visible to
a patch, and the execution report still states the worst-case iteration count
rather than 1 — `YdspKernelReport::unrolled` is what tells the two forms
apart.

Once the copies exist, a **widened accumulator** is halved: the odd copies add
into a second accumulator and the two are added at the end, which halves the
serial depth of the reduction for one extra instruction. A long enough chain is
halved more than once — a 32-partial bank widened to four lanes has eight adds
in a row, and ends with four.

This re-associates the sum a second time, and is applied only where the
vectoriser has already taken that licence — a scalar accumulator, in a loop that
was not widened, is left exactly as written.

### 2.7 Expressions

```ebnf
expression = ternary ;
ternary    = or_expr [ "?" expression ":" expression ] ;
or_expr    = and_expr { "||" and_expr } ;
and_expr   = bit_or { "&&" bit_or } ;
bit_or     = bit_xor { "|" bit_xor } ;
bit_xor    = bit_and { "^" bit_and } ;
bit_and    = equality { "&" equality } ;
equality   = relational { ("==" | "!=") relational } ;
relational = shift { ("<" | "<=" | ">" | ">=") shift } ;
shift      = delay { ("<<" | ">>") delay } ;
delay      = additive { "@" additive } ;
additive   = multiplicative { ("+" | "-") multiplicative } ;
multiplicative = unary { ("*" | "/" | "%") unary } ;
unary      = [ "-" | "!" | "~" ] postfix ;
postfix    = primary { "'" | "[" expression "]" } ;
primary    = literal | identifier | call | "(" expression ")" ;
```

- Arithmetic: `+ - * / %` (float and int), unary `-`, logical `&& || !`
  (short-circuit), comparisons, ternary `cond ? a : b`.
- Bitwise (int only): `& | ^ ~ << >>` and the compound forms `&= |= ^= <<= >>=`,
  at the precedence shown above - notably *tighter* than comparisons, unlike C.
  Unary `~` (bitwise not) is unrelated to the graph algebra's binary `~`
  (recursion composition, [3.2](#32-the-faust-style-algebra)): different
  grammars, different arities, no ambiguity between them.
- Calls: intrinsic functions (see [2.8](#28-intrinsics)) and the delay
  primitives.
- Indexing: `state` arrays (`buf[i]`) and, in block mode, stream arrays
  (`in[i]`). Indices are `int`.
- The postfix `'` (one-sample delay) — see [2.9](#29-delay-and-smoothing-primitives).

Built-in constants:

| Name | Type | Value |
|------|------|-------|
| `sampleRate` | `float32` | sample rate in Hz |
| `samplePeriod` | `float32` | `1 / sampleRate` — seconds per sample |
| `blockSize` | `int32` | samples per block (the `numSamples` argument) |
| `pi`, `e` | `float32` | mathematical constants |
| `inf`, `nan` | `float32` | IEEE special values |
| `true`, `false` | `bool` | booleans |

`samplePeriod` is loop-invariant, so its division is hoisted out of the sample
loop by the optimiser and paid once per block. `blockSize` is unavailable inside
an event handler (there is no sample loop to measure); every other builtin is
available everywhere. Program constants declared with `let` join this table for
the whole program (see [1.0](#10-program-constants)).

### 2.8 Intrinsics

`abs sqrt min max fmod floor ceil rint` · `sin cos tan asin acos atan atan2`
· `sinh cosh tanh asinh acosh atanh` · `exp log log10 pow round copysign(x,y)`
· `clamp(x,lo,hi) sign(x) lerp(a,b,t) fma(a,b,c) select(cond,a,b)` ·
`smooth(x,tau)` (stateful, see [2.9](#29-delay-and-smoothing-primitives)) ·
conversions `int(x)`/`int32(x)`, `int64(x)`, `float(x)`/`float32(x)`,
`float64(x)`.

Float intrinsics require float operands of a single width and preserve that
width (f32 lowers to `sinf`/`powf`/..., f64 to `sin`/`pow`/...).

`min`, `max`, `clamp` and `abs`/`sign` also have an integer overload: if every
non-literal argument is `int32` or `int64`, the result stays integral instead
of promoting to float (a bare int literal alone still adapts to float32, same
as everywhere else). Mixing an int and a float argument is a compile error -
cast explicitly instead. The integer `abs` is the branchless two's-complement
idiom, so `abs(INT_MIN)` returns `INT_MIN` unchanged: the true magnitude does
not fit in the same width, and this is the one input for which the intrinsic
does not return the mathematical absolute value.

`fma(a,b,c)` is `a * b + c` with a **single** rounding rather than two. The
fast-math contraction pass also recognises `c - a * b` and uses the equivalent
fused subtract-multiply operation.

You rarely need to write it. When `YdspCompileOptions::fastMath` is enabled,
the compiler contracts `a * b + c` and `c - a * b` into the same operation where the multiply
feeds nothing but the add and can be moved down to it unchanged. Strict mode
keeps the two operations separate. Spelling `fma` out is for the cases the pass
declines: a multiply with a second reader, or one defined in a different block.

Contraction is normally a *precision* liberty: it changes rounding, and YDSP does not take those,
because a patch has to produce identical samples on every backend it can be
compiled for. This one is exempt because it is not per-target: the fused
operation has **one defined value**, and a target without the instruction
(WebAssembly, or x86-64 without FMA3) reaches that same value by computing in
float64 and rounding once on the way back down. Native and wasm agree. What a
contracted patch differs from is the *unfused* arithmetic — one rounding where
the source reads like two.

Two consequences worth knowing:

- It is **float32 only**. The portable fallback needs a format one step wider
  than the operands, which exists for float32 and does not for float64, so `fma`
  on float64 operands is a compile error rather than a silent per-target
  difference. `a * b + c` on float64 operands is simply not contracted.
- On a target with no fused instruction, each contracted site costs six
  operations instead of two (three widenings, a multiply, an add, a narrowing).
  That is the price of the guarantee above, and it is paid on WebAssembly.

Where it pays is a per-sample recurrence, in which the multiply and the add are
consecutive links of the loop-carried chain and fusing them removes one - a
one-pole `y = y * a + x * b`, or a ladder stage `z = z + g * (x - z)`. A widened
bank loop can contract only on a target with packed FMA support (AArch64 ASIMD
or AVX2 with FMA). A target without it retains the packed multiply and add,
because the scalar exact fallback cannot preserve the vector loop shape.

In v1 the arithmetic intrinsics (`sqrt`, `abs`, `min`, `max`, `floor`,
`ceil`) lower to native instructions; `fmod` and the transcendentals (`sin`,
`tanh`, `pow`, ...) lower to calls of their `libm` equivalents. Polynomial
approximations are planned future work.

### 2.9 Delay and smoothing primitives

The stateful primitives are available in **per-sample bodies** only (outside
loops, never in an event handler), require **float32 operands**, and are lowered
to hidden `state` slots — one per call site, reserved by the IR builder:

- `x'` and `mem(x)`: the value `x` had one sample ago.
- `x @ n`: `x` delayed by `n` samples; `n` must be a non-negative integer
  literal. `x @ 0` is `x`. Compiles to a ring buffer of `n + 1` state slots.
- `smooth(x, tau)`: a one-pole ramp towards `x` reaching ~63% of a step in
  `tau` seconds. Compiles to one float slot (the running value) plus one int
  slot (a primed flag).

Their hidden slots do not cost memory traffic per sample: like the declared
scalar `state`, each is loaded into a register once when the kernel is entered
and written back once when it returns, so the per-sample body only touches
registers. The `@` ring's write pointer is advanced with a branchless wrap
rather than an integer modulo — on x86-64 the modulo was a helper call, once
per tap per sample.

```ydsp
processor OnePole {
    input stream in; output stream out;
    input value float a = 0.5;
    process { out = (1 - a) * in + a * out'; }
}
```

#### Smoothing parameters

Parameter automation is **stepped** (see [2.12](#212-sample-accurate-dispatch-and-automation)), so
a `setParameter()` or an automation event changes a value at an exact sample and
clicks. `smooth()` de-zippers it in one token:

```ydsp
process { out = in * smooth (gain, 0.02); }
```

Per sample it is one `lerp` plus a compare that snaps to the target on the first
sample where the ramp cannot advance, so the value arrives *exactly* and leaves
no denormal tail. The compare tests the step, not the remaining distance: a
float32 lerp stalls once its increment falls below half an ulp, while still
short of the target by roughly `ulp / coeff`, so any fixed epsilon would either
be unreachable (a permanent offset) or loose enough to truncate a fast ramp.
Testing the step is scale-free — while the ramp advances it advances by at least
one ulp, so it is strictly monotone, and the sample it cannot advance is the
sample it arrives. The coefficient `1 - exp (-samplePeriod / tau)` is
loop-invariant for a constant `tau`, so the optimiser hoists the `exp` out of
the sample loop and it is paid once per kernel invocation.

The primed flag makes the *first* sample snap rather than ramp: `prepare()` and
`reset()` zero all state, so without it a patch would audibly sweep from 0 on
the first block after either.

`[[ smoothing: t ]]` on an `input value float` (see [2.1](#21-endpoints)) is
sugar for exactly this. It prepends one synthetic local to the top of the
per-sample body and rewrites that body's references to the endpoint, so these
two patches compile to identical code:

```ydsp
input value float cutoff = 1500.0 [[ smoothing: 0.02 ]];
process { out = in * cutoff; }
```

```ydsp
input value float cutoff = 1500.0;
process {
    float cutoffSmoothed = smooth (cutoff, 0.02);
    out = in * cutoffSmoothed;
}
```

Points to watch:

- **Event handlers and `func` bodies keep reading the raw target.** That is the
  right semantics for a handler — `envCoeff = pow (0.001, 1.0 / (decay *
  sampleRate))` on `noteOn` wants the value the user dialled in, not a value
  one third of the way through a ramp — and it is a consequence of the rewrite
  being scoped to the per-sample body. A `func` that reads a smoothed parameter
  directly (rather than taking it as an argument) sees the stepped value; pass
  the smoothed local in as an argument instead. `getParameter()` likewise keeps
  returning the target.
- **The sugar hoists; the intrinsic does not.** A `smooth()` call written inside
  an `if` only ticks on the samples where that branch is taken. `[[ smoothing ]]`
  always puts its smoother at the top of the body, so it ticks every sample.
- **`tau` may be any float expression, but keep it out of the sample loop.** A
  literal, a program constant or a parameter all leave the smoother's own
  coefficient loop-invariant, so its `exp` is paid once per kernel invocation. A
  `tau` computed from `state` or from a stream is genuinely per-sample and drags
  that `exp` into the loop.
- **Smoothing a parameter does have a cost, and it is not the `lerp`.** A value
  derived only from parameters — `float k = 1 - exp (-6.283185307 * cutoff /
  sampleRate)` — is loop-invariant and hoisted out of the sample loop entirely,
  because a parameter cannot change within one kernel invocation. Smoothing
  `cutoff` makes `k` depend on the smoother's state, so the whole expression
  drops back into the loop. Smooth the *coefficient* instead — `float k = smooth
  (1 - exp (...), 0.02)` — to keep the `exp` hoisted and pay only the `lerp`,
  or gate the recompute with the `'` idiom below.
- **A `'` on a smoothed local gates the work downstream of it.** The snap makes
  a settled smoother compare bit-exactly equal to its own previous sample, so a
  per-sample coefficient can be recomputed only while the parameter is actually
  moving — and, because the snap guarantees exact arrival, the branch really
  does close rather than staying alive on a residual offset:

  ```ydsp
  state float k;
  float c = smooth (cutoff, 0.02);
  if (c != c') { k = 1.0 - exp (-6.283185307 * c / sampleRate); }
  ```

  The deferred `'` write happens at the loop tail, so `c'` still ticks on every
  sample even though it is only read inside the `if`. This trades a uniform
  per-sample cost for a data-dependent branch: the worst case (a parameter under
  continuous automation) is unchanged, only the settled case gets cheaper. Bind
  the smoother to a local first — `smooth (...)'` re-lowers its operand at flush
  time and would allocate a *second* smoother.
- **The ramp is linear in the smoothed quantity.** For a log-domain sweep, wrap
  it: `float c = exp (smooth (log (cutoff), 0.02));`. Gain should ramp in linear
  amplitude, never in dB (dB goes to -inf at silence).
- **Not every parameter should be smoothed.** Mode, enum and bool parameters
  need a hard switch or a crossfade, so the feature is opt-in per endpoint.
- **Think in slew rate before smoothing a delay-line read position.** The index
  is an integer sample count either way, so what matters is how fast smoothing
  makes it move. A short span (a chorus depth of ~880 samples ramped over 20 ms)
  slews at about one sample per sample, which is what an LFO does anyway. A long
  span (a delay time of ~88000 samples over the same 20 ms) slews ~100 samples
  per sample and scrubs audibly through the buffer - worse than the single jump
  it replaced. Click-free modulation of a long delay time needs a
  fractional-delay read, which the language does not provide yet.

### 2.10 Processor grammar

```ebnf
processor_definition = "processor" identifier "{"
    { endpoint_declaration | struct_declaration | state_declaration
    | function_declaration | event_handler | process_declaration
    | init_declaration } "}" ;

endpoint_declaration = ("input" | "output") ("stream" | "value")
    type identifier [ "[" integer "]" ] [ "=" expression ] [ annotations ] ";"
    | ("input" | "output") "event" identifier ";" ;

struct_declaration = "struct" identifier "{"
    { type identifier [ "[" integer "]" ] ";" } "}" ;

state_declaration = "state" ( type | struct_name ) identifier
    [ "[" ( integer | constant_name ) "]" ] [ "=" initialiser ]
    [ annotations ] ";" ;

initialiser = expression | "{" [ expression { "," expression } [ "," ] ] "}" ;

function_declaration = "func" identifier "(" param_list ")" [ ":" type ]
    "{" { statement } "}" ;

param_list = identifier ":" type { "," identifier ":" type } ;

event_handler = "event" identifier "(" identifier ":" identifier ")"
    "{" { statement } "}" ;

process_declaration = "process" [ "block" ] "{"
    { statement } "}" ;

init_declaration = "init" "{" { statement } "}" ;
```

### 2.11 Events

YDSP is a complete MIDI and MPE instrument host: seven fixed event shapes, a
named `input event <name>;` per processor (several are allowed, like streams)
with one `event` handler per shape, a voice-bank suffix on node
instantiations, and voice behaviour declared as node annotations. MIDI byte
decoding and voice allocation happen in the C++ runtime (`YdspAudioGraph`), not
in the language.

```ydsp
processor Voice {
    output stream out;

    input value float decay = 0.25;

    input event midi;

    state float freq;
    state float env;
    state float phase;
    state float bend;
    state float press;
    state float modWheel;

    event midi (e: noteOn) {
        freq = e.pitch;
        if (! e.isLegato) { env = e.velocity; }
    }

    event midi (e: noteOff)       { env = 0.0; }
    event midi (e: pitchBend)     { bend = e.bendSemitones; }
    event midi (e: pressure)      { press = e.pressure; }
    event midi (e: controlChange) { if (e.control == 1) { modWheel = e.value; } }

    process {
        phase = phase + 440.0 * pow (2.0, (freq + bend - 69.0) / 12.0) / sampleRate;
        if (phase >= 1.0) { phase = phase - 1.0; }
        out = sin (phase * 6.283185307) * env * press * modWheel;
    }
}
```

#### Shapes and payload fields

The seven shapes a handler may select are fixed:

| Shape | Fields | Source |
|-------|--------|--------|
| `noteOn` | `.pitch`, `.velocity`, `.bendSemitones`, `.isLegato` | note start |
| `noteOff` | `.pitch`, `.velocity` | note end |
| `pitchBend` | `.bendSemitones` | per-note or channel bend |
| `pressure` | `.pressure` | channel aftertouch, poly aftertouch, MPE Z |
| `slide` | `.slide` | CC74 / MPE Y |
| `controlChange` | `.control` (`int`), `.value` | any CC |
| `programChange` | `.program` (`int`) | program change |

All floats are normalised to `[0, 1]` except `.pitch` (MIDI note scale) and
`.bendSemitones` (signed semitones). `.isLegato` is a `bool`, true only for a
mono-mode legato continuation. Reading a field that does not belong to the
handler's shape is a compile error naming the legal fields. There are no
user-defined event types or structs.

A `noteOn` handler can read `.bendSemitones` to start a freshly triggered
voice at the bend already in effect: for a host note it is the note's total
pitch-bend at trigger time (in legacy mode the last wheel position on the
channel, in MPE mode the per-note bend), so a key pressed while the wheel is
held up starts bent instead of resetting to center. Mono legato continuations
carry the bend of the note that is now sounding; a note `emit`ted from another
node carries `.bendSemitones == 0` unless the emitter wrote one.

Polyphonic aftertouch is *not* a separate shape: it folds into the affected
note's `pressure`, so a patch handles per-note and channel aftertouch with
one handler.

#### Handlers

`event midi (<param>: <shape>) { ... }` fires once per dequeued event of
`<shape>` on the processor's `midi` input. The handler shares the processor's
`state` and reads its parameters; streams, meters, `blockSize` and the delay
primitives (`'`, `@`, `mem`) are not accessible inside a handler, and loop
bounds must be compile-time constants. A shape the processor declares no
handler for costs nothing at runtime — it is never queued and forces no
sub-block split. Two handlers for the same shape on the same input, or a
handler for an undeclared input, are compile errors.

#### Emitting events

A processor generates events with `output event <name>;` and an `emit`
statement, legal in `process` (sample mode only) and in event handlers:

```ydsp
processor Arp {
    input event midi;
    output event midiOut;

    event midi (e: noteOn) { ... }

    process {
        ...
        emit noteOn (pitch: nextPitch, velocity: nextVelocity) -> midiOut;
    }
}
```

`<name>` is a channel identifier, not a shape: it need not equal any of the
seven shape names, and a single `output event` channel may carry several
different shapes over its lifetime (one `emit <shape> (...) -> <name>;` per
shape it emits). Every declared `output event` endpoint must be connected at
least once. In a `connection { }` block, a node's `output event` wires to
another node's `input event` (any name on either side, including the
polymorphic `midi` input) or to the graph's own `output event` boundary,
which the host reads back as MIDI:

```ydsp
graph G {
    input event midi;
    output stream out;

    node arp = Arp;
    node voice = Voice[8];

    connection {
        midi -> arp.midi;

        arp.midiOut -> voice.midi;
        voice.out -> out;
    }
}
```

A cycle through an event edge is rejected with the same error as an audio
feedback cycle. An inline delay (`[N]`) is not supported on an event
connection; latency compensation instead aligns an emitted event with the
already-compensated audio its source node produces in the same block.

#### Voice banks and voice modes

`node v = Voice[8];` runs eight independent copies of the processor
(independent per-voice state, shared parameters) and sums their outputs.
`Voice[1]` (or a bare `Voice`) is a degenerate one-voice bank and still
receives MIDI. A voice bank is only legal on processors that declare event
handlers, and event-driven nodes cannot use oversampling.

Voice behaviour is declared with node annotations:

```ydsp
node lead = Voice[8] [[ mode: poly, stealing: oldest ]];
node bass = Bass     [[ mode: mono, priority: last ]];
```

| Key | Values | Default |
|-----|--------|---------|
| `mode` | `poly`, `mono` | `poly` |
| `stealing` | `oldest`, `newest`, `none` | `oldest` |
| `priority` | `last`, `low`, `high` (mono only) | `last` |

The defaults reproduce the behaviour of an unannotated node exactly.

- **`mode: poly`** — each note takes its own voice slot: a free slot
  (oldest-released first) or, when every slot is held, the slot chosen by
  `stealing`. A steal runs the stolen voice's `noteOff` before the new
  `noteOn` at the same instant, so state is never left inconsistent.
  `stealing: none` ignores the note-on instead.
- **`mode: mono`** — requires exactly one voice (a compile error otherwise)
  and keeps a fixed-capacity stack of the notes whose keys are still down (16
  entries, no allocation). `priority` selects which one sounds. When a note
  displaces another, the new `noteOn` carries `.isLegato == true` and *no*
  `noteOff` is dispatched, so the patch decides whether to retrigger its
  envelope. Releasing the sounding note falls back to whatever is still held,
  again as a legato `noteOn`.

The mono idiom is to branch the whole note-start on `.isLegato` — a fresh press
re-attacks from silence, a continuation keeps the envelope and slides the pitch:

```ydsp
event midi (e: noteOn) {
    freqTarget = noteToFreq (e.pitch);
    envTarget  = e.velocity;

    if (e.isLegato) {
        glideCoeff = 1.0 - exp (-1.0 / (glide * sampleRate));  // portamento
    } else {
        env        = 0.0;                                      // re-attack
        glideCoeff = 1.0;                                      // take the pitch at once
    }
}
```

`examples/graphics/data/synths/PulseBass.ydsp` is a complete worked example.

#### Skipping idle voices

By default every voice of a bank runs every block, whether it is sounding or
not. A processor opts out of that by nominating one `state int` scalar as its
voice-activity flag:

```ydsp
state int active [[ role: voiceActivity ]];
```

The runtime reads that scalar out of each voice's state slice at the top of
every block. `0` means "this voice will produce nothing until its next event"
and the kernel is skipped entirely — no call, no output accumulation. Non-zero
means run it.

The contract is the patch's to keep: **clearing the flag asserts that the voice
cannot revive on its own.** A sleeping voice does not run, so it cannot set its
own flag back to 1 — only an incoming event can wake it. Delay lines, high-Q
filters and release curves are the patch's own business, so test what the voice
actually *emits*, not the envelope behind it:

```ydsp
event midi (e: noteOn) { active = 1; }  // re-arm on every note

process {
    ...
    float gain = smooth (env, 0.003);   // what the output is scaled by

    active = select (gain < 0.000001, 0, 1);

    out = osc * gain;
}
```

Never test the output *sample* — an oscillator crosses zero every cycle, and the
flag is only read at block boundaries, so a zero crossing landing on the last
sample of a block would sleep a loudly sounding voice. Test the amplitude.

The rules:

- **A released voice only.** The runtime predicate is `flag == 0 && ! held`, so
  a voice whose key is still down always runs regardless of its flag. That makes
  clearing the flag too early cost some wasted CPU on a held voice rather than
  producing a stuck silent one, and the main win — most of a bank idle while a
  few notes play — lives entirely in released voices anyway. The patch therefore
  never has to reason about held state: it only answers "am I audible?".
- **Whole-kernel, per block.** The flag is read once per voice per block, and
  re-checked only after an event handler runs (so a mid-block note-on wakes the
  voice for the remainder of the block). A voice is never put back to sleep
  mid-block: it keeps rendering to the block boundary and sleeps from the next
  block.
- **Opt-in, with no runtime switch.** A processor that declares no flag never
  sleeps a voice, so every patch keeps byte-identical behaviour until it opts
  in. The declaration *is* the switch.
- **The flag must be a scalar `int` with no initialiser** ("starts at 0,
  therefore starts asleep" is load-bearing), and at most one per processor. An
  unknown `role:` value, a non-`int` or array-typed flag, a second flag, an
  initialiser, or the annotation on a processor with no event handlers are all
  compile errors — a mis-declared flag would silently never take effect. Writing
  the flag from an `init { }` block is not rejected, but do not: `prepare()` and
  `reset()` zero the state, which is exactly the right starting point.
- **Sleeping freezes the voice's state**, so anything free-running inside it (a
  per-voice LFO, a filter driven by a still-running oscillator) resumes from
  where it left off rather than from where it would have been. That is inherent
  to skipping the kernel.

`YdspAudioGraph::getActiveVoiceCount ("nodeName")` reports how many voices of a
node will run on the next block, using the scheduler's own predicate (`held`
term included) so the count can never disagree with what actually ran. It
returns the full voice count for a processor that declares no flag, and 0 for
an unknown node name.

`examples/graphics/data/synths/ElectricPiano.ydsp` is the worked example: 16
voices of a 32-partial oscillator bank, with the activity test folded into the
existing per-chunk envelope loop so it costs one compare per partial every 64
samples rather than per sample.

#### MIDI, MPE and routing

Ingestion goes through `yup::MPEInstrument`, so plain MIDI and MPE share one
path and note expression is always addressed by a stable note identity rather
than by pitch. The host picks the mode; the patch is unaware of the
difference:

```cpp
graph.setLegacyMidiMode (2);      // plain MIDI (the default), ±2 semitone bend
graph.setMpeZoneLayout (layout);  // MPE
```

Under plain MIDI, channel bend, channel pressure and CC74 are broadcast to
every note on the channel. Under MPE they are resolved per member channel.

A shape with no explicit routing (every host-originated shape, and any
emitted shape whose channel is left unconnected past its required connection)
keeps this structural broadcast behaviour:

- **Note-scoped** (`noteOn`, `noteOff`, `pitchBend`, `pressure`, `slide`) —
  delivered to the single voice holding that note. Expression for a note with
  no allocated voice is discarded and counted in
  `YdspAudioGraph::getDroppedEventCount()`.
- **Graph-scoped** (`controlChange`, `programChange`) — broadcast to every voice of
  every event-driven node that declares a handler for the shape.

By default all voices run every block, even idle ones (fixed worst-case
cost). A processor that declares a `[[ role: voiceActivity ]]` state opts into
having its released, finished voices skipped — see
[Skipping idle voices](#skipping-idle-voices).

#### Channel-mode messages

Sustain (CC64), sostenuto (CC66), reset-all-controllers (CC121) and
all-notes-off (CC123) are honoured as ordinary note lifecycle: a note held by
a pedal simply does not receive its `noteOff` until the pedal lifts, so there
is no separate sustain state in the patch. All-sound-off (CC120) additionally
silences every event-driven node at its exact sample offset: voice state is
zeroed and the processor's `init` block re-runs, with no release tail. Every
CC — including these — is also delivered as a `controlChange` event.

### 2.12 Sample-accurate dispatch and automation

Events and parameter automation land at their **exact sample offset** within
a block, not at the block boundary. The runtime splits the block into
sub-blocks at each event/automation offset and calls the *same* compiled
kernel once per sub-block with `ctx.numSamples` and the buffer pointers
advanced to that offset — no codegen or IR changes are involved, and state
persists across the sub-block calls exactly as across blocks.

A node that is **not** event-driven and has no automation in a block takes the
single-call fast path at zero added overhead. Event-driven nodes always take
the sub-block path, even a single-voice one with nothing pending: that is where
per-voice work such as [idle-voice skipping](#skipping-idle-voices) lives, and
keeping it in one place is worth one extra call per block.

A voice bank's parameters are shared by every voice, so an automation point
inside a block is applied once per voice and every voice observes the same
pre/post timeline — the value changes at the same sample in all of them.

Automation is **stepped**, not interpolated: the host resolves a parameter
slot once on the control thread (`YdspAudioGraph::getParameterSlot("node.param")`),
then delivers `YdspAutomationEvent { slot, sampleOffset, value }` triples
to the audio-thread `process()`. De-zippering is the patch's job: use
`smooth (param, tau)` or `[[ smoothing: tau ]]` on the endpoint (see
[2.9](#29-delay-and-smoothing-primitives)).

## 3. Graphs (signal flow)

A `graph` declares audio inputs/outputs and parameters, instantiates
processors as nodes, and wires them together. The body must be written in
**one of two equivalent forms** — a `connection` block, or a Faust-style
`process = ...` algebra expression. Both compile to the same internal
connection graph.

A program may declare several graphs. One of them is the **entry point**: the
one annotated `graph Name [[ main ]] { ... }`, or, when the program declares
just one graph of its own, that graph. Every other graph is a **subgraph**,
usable as a node inside another graph (see [3.3](#33-subgraphs)).

### 3.1 Graph declarations

```ydsp
graph MyPatch {
    input  stream dry;                  // patch audio input
    input  stream side;                 // sidechain input
    output stream wet;                  // patch audio output
    input  value float master = 0.8;    // patch-level parameter

    node sat = Saturator (drive = 1.5); // instantiate with overrides
    node dly = Delay   (time = 0.25);

    connection {
        dry -> sat.in;                  // audio edge
        side -> sat.side;               // sidechain edge
        sat.out -> dly.in;
        dly.out -> wet;
        master -> sat.drive;            // parameter aliasing
    }
}
```

- `node name = Processor (param = value, ...);` instantiates a kernel. The
  override list sets initial parameter values (optional).
- A `connection` block wires endpoints: `src -> dst;` where `src`/`dst` are
  `node.endpoint`, a graph input stream, a graph output stream, or a graph
  parameter. Audio edges must connect stream-to-stream; parameter edges must
  connect value-to-value. A parameter edge *aliases* the parameter memory, so
  writing the graph parameter updates the node parameter directly (no copies,
  no per-block smoothing needed).
- Every graph input/output stream and every node stream endpoint must be
  connected **at least once**. Beyond that the graph is an arbitrary DAG:
  - a **source** may fan out to any number of destinations
    (`x -> a.in; x -> b.in;`), which costs nothing — the consumers read the
    same buffer, and generated code never writes through its inputs;
  - a **destination** fed by more than one source **sums** them
    (`a.out -> y; b.out -> y;`), with no mixer node needed. Implicit summing
    requires a `float32` or `float64` stream; fan-out has no type restriction.

  Summation order is the order the edges appear after analysis. It is
  deterministic for a given patch — and nothing more: subgraph inlining and
  kernel fusion both rebuild the edge list, so it is not "the order you wrote
  them in". Since float addition does not associate, do not depend on it.
- A graph parameter may drive any number of node parameters, but a node
  parameter takes at most one writer. Unconnected node parameters keep their
  defaults.
- A connection may carry an inline delay, written `src -> [N] -> dst;`. The
  delay line is float32-only, and its samples go to the destination alone: the
  source buffer is never modified.
- An optional `input event <name>;` (several are allowed, each a separate
  host-facing MIDI port) declares that the patch consumes MIDI. Like a stream
  or value graph input, it must be wired explicitly - `<name> -> node.event;`
  in a `connection { }` block - to reach a node's own `input event`; there is
  no broadcast by matching endpoint names (see [2.11](#211-events)). An
  optional `output event <name>;` is the reverse: a node's `output event`
  connected to it delivers emitted events back to the host as MIDI.
- A graph is classified purely by which endpoint kinds it declares, not by a
  keyword: one with only stream endpoints is audio-only, one with only event
  endpoints is MIDI-only (`inputs`/`outputs` are empty spans at the host call
  site), and a graph mixing both is a hybrid — all three are the same `graph`
  construct.

### 3.2 The Faust-style algebra

```ydsp
graph Chain {
    input stream dry; output stream wet;
    node sat = Saturator (drive = 1.5);
    node dly = Delay (time = 0.25);
    process = dry : sat : dly : wet;     // sequential composition
}
```

Every graph expression has an arity `(inArity, outArity)`:

| Operator | Meaning | Arity rule |
|----------|---------|------------|
| `a : b` | sequential | `a.outArity == b.inArity` → `(a.in, b.out)` |
| `a , b` | parallel | → `(a.in + b.in, a.out + b.out)` |
| `a <: b` | split | `a.outArity` **divides** `b.inArity` → `(a.in, b.out)` |
| `a :> b` | merge | `b.inArity` **divides** `a.outArity` → `(a.in, b.out)` |
| `a ~ b` | recursion | `b.inArity == b.outArity + 1` → feedback |
| `_` | identity | `(n, n)` for any `n` |
| graph input/output idents | leaf | `(1, 1)`, with a port on one side only |
| node name / instance | leaf | the node's arity |

`<:` **widens** and `:>` **narrows**: a split assigns `b`'s input *j* to `a`'s
output *j % a.outArity*, so each source channel is reused cyclically; a merge
sends `a`'s output *i* to `b`'s input *i % b.inArity*, and the outputs that
collide on one input are **summed**. Sequential composition is the case where
the two arities are equal, so all three share one rule.

```ydsp
process = dry <: (Distort , Chorus) :> wet;   // parallel dry/wet, summed
```

Neither operator inserts a relay node. `_` carries no ports of its own, so
`_ <: (a , b)` and `(a , b) :> _` emit no wires at all — they regroup which
channel goes where, and the fan becomes real edges when the value meets an
actual leaf. An unconstrained `_` takes arity **1** on the side the operator
governs (left of `<:`, right of `:>`, i.e. the maximum fan) and the known side's
arity otherwise.

Nothing can be sequenced *past* a graph output identifier or *into* a graph
input one: those leaves have arity `(1, 1)` but only one real side, so
`dry : wet : Gain` and `Gain : dry` are compile errors.

The `process = expr;` form must match the graph's declared inputs/outputs:
`expr.inArity == number of graph input streams` and
`expr.outArity == number of graph output streams`. The example above is
exactly equivalent to the `connection` version in [3.1](#31-graph-declarations)
(without the sidechain), and produces identical generated code.

The recursion operator `~` is parsed and arity-validated
(`b.inArity == b.outArity + 1`) but is **not supported in v1**: it requires
per-sample feedback across nodes, which needs fused code generation. Using it
produces a clear compile error.

This `~` is unrelated to the unary `~` (bitwise not) inside an expression
(see [2.7](#27-expressions)): the two are parsed by separate grammars - this
one only inside graph algebra, the other only inside a processor's expression
syntax - with different arities, so there is no ambiguity between them despite
the shared symbol.

### 3.3 Subgraphs

A `node` may instantiate a **graph** instead of a processor, which is how a
reusable chunk of signal flow — an effects chain, a voice architecture — gets a
name and a parameter surface of its own:

```ydsp
import fx.Compressor as comp;
import fx.Reverb     as verb;

graph MasterBus {                            // a subgraph: no [[ main ]]
    input  stream in;
    output stream out;
    input  value float threshold = -18.0;
    input  value float reverbMix = 0.25;

    node compressor = comp.Compressor (attack = 0.005);
    node reverb     = verb.Reverb;

    connection {
        in -> compressor.in;
        compressor.out -> reverb.in;
        reverb.out -> out;

        threshold -> compressor.threshold;   // forwarded parameters
        reverbMix -> reverb.mix;
    }
}

graph Patch [[ main ]] {
    input event midi;
    output stream y;

    node voices = Voice[8];
    node master = MasterBus (threshold = -24.0);   // a subgraph node

    connection { midi -> voices.midi; voices.out -> master.in; master.out -> y; }
}
```

A subgraph is **compiled away**: the semantic analyzer splices its contents
into the graph that uses it, so the optimiser, all three backends and the
runtime only ever see one flat graph of processors. That has a few visible
consequences:

- Inner node names are prefixed with the instance path, so the compressor above
  is addressable as `master.compressor` and its parameter as
  `master.compressor.threshold` (see [6](#6-public-api-c)).
- A subgraph's `input value` forwards to every node parameter it drives: the
  node's override list (`MasterBus (threshold = -24.0)`) sets those parameters'
  initial values, and a parameter edge onto the subgraph node
  (`master -> bus.threshold`) aliases straight through to them. An
  `output value` forwards outwards the same way.
- Inline delays add up across the boundary: `x -> [2] -> sub.in` outside plus
  `in -> [3] -> a.in` inside is one edge with a 5-sample delay.
- A boundary port may be fanned on either side, or on both. Two sources feeding
  `sub.in` are summed inside it; `sub.out` read from two places feeds both. When
  both sides fan, each internal edge becomes the cross product — a pass-through
  subgraph with two feeders and two consumers splices into four wires — and each
  resulting wire carries its own sum of the delays on the path it stands for.
- A subgraph may instantiate further subgraphs. A graph that reaches itself,
  directly or transitively, is a compile error naming the loop.

Because the mechanism is inlining rather than runtime nesting, three things
that *are* runtime mechanisms are rejected on a subgraph node, each with an
explicit diagnostic:

| Rejected | Why |
|----------|-----|
| `node v = Sub[16];` | a voice bank needs one processor with one `float32` output stream for the runtime's per-voice summing path |
| `node g = Sub * 4;` | over/undersampling is a per-processor rate change |
| `node g = Sub;` where `Sub` declares `input event midi` | graph-scope MIDI belongs to the entry point, which is the only graph with a runtime identity |

A subgraph parameter that drives no node inside it produces a warning: setting
it from outside would silently do nothing.

### 3.4 Graph grammar

```ebnf
graph_definition = "graph" identifier [ annotations ] "{"
    { endpoint_declaration | node_declaration }
    ( connection_block | process_definition ) "}" ;

endpoint_declaration = ("input" | "output") ("stream" | "value")
    type identifier [ "[" integer "]" ] [ "=" expression ] [ annotations ] ";"
    | "input" "event" "midi" ";" ;

node_declaration = "node" identifier "=" target_name
    [ "[" integer "]" ] [ "(" assignment_list ")" ]
    [ ("*" | "/") integer ] [ annotations ] ";" ;

target_name      = identifier { "." identifier } ;   (* a processor or a graph *)

connection_block = "connection" "{" { connection } "}" ;

connection      = identifier ("." identifier)?
    "->" [ "[" integer "]" "->" ]                  (* optional inline delay *)
    identifier ("." identifier)? ";" ;

process_definition = "process" "=" algebra_expression ";" ;

algebra_expression = algebra_term { ( ":" | "," | "<:" | ":>" | "~" ) algebra_term } ;
algebra_term      = algebra_primary { ( ":" | "," | "<:" | ":>" | "~" ) algebra_primary } ;
algebra_primary   = "_" | identifier [ "(" assignment_list ")" ] | "(" algebra_expression ")" ;
```

The optional `[N]` after the processor name declares a **voice bank** (see
[2.11](#211-events)): the runtime runs `N` independent copies and sums their
outputs. Voice banks and oversampling (`*N`) are mutually exclusive on
the same node. The trailing `[[ ... ]]` block carries the node's voice-mode
annotations (`mode`, `stealing`, `priority`).

A rate change (`*N` or `/N`) requires every stream of the node's processor to be
`float32`: the runtime resamples through `Oversampler<float, ...>`, so a wider
element type would be addressed at the wrong stride. Both directions support a
factor of 2, 4 or 8.

`sampleRate` inside a rate-changed kernel reports the rate that kernel is
actually running at — `* 4` sees four times the graph's rate and `/ 4` a quarter
of it — so a processor written rate-agnostically against it keeps its time
constants in seconds wherever it is deployed, and the rate change stays a
graph-level decision rather than a rewrite.

`/N` runs the node at 1/N of the graph's rate through a real windowed-sinc
decimator — its inputs are band-limited and decimated, the kernel runs at the
lower rate, and its outputs are interpolated back up. It is a CPU optimisation
for work that does not need full bandwidth (an envelope follower, a slow
modulator), and it costs bandwidth: anything above the node's own Nyquist is
filtered away and does not come back.

Because only whole groups of N input samples can be consumed, a block size that
is not a multiple of N leaves a remainder; the runtime carries it across blocks
so any block size works, at the price of N - 1 samples of latency on top of the
resampler's own. Total `/N` latency is `16 * N + (N - 1)` graph-rate samples, all
of it artifact latency that delay compensation removes automatically.

### 3.4 Automatic kernel fusion

A chain of nodes that only feed each other is compiled into a **single kernel**.
`x : Osc : Filter : Gain : y` — or the same wiring written as a `connection`
block — would otherwise be three kernel calls per block passing two `blockSize`
scratch buffers between them; fused, it is one call that keeps both
intermediates in registers. Measured at **2.05x** on a three-stage chain
(`YdspBenchmarkTests`).

Like vectorisation this is an optimisation, not a language feature: nothing in
the patch selects it, and a chain that does not qualify simply keeps its
separate kernels. It runs after subgraph inlining, so a chain assembled out of a
subgraph's nodes fuses exactly like a hand-written one.

A link between two nodes is fused when:

- the producer feeds **nothing else** and the consumer is fed by **nothing
  else**, so the intermediate signal is unobservable — tapping `a.out` to a
  second destination keeps the chain intact and unfused;
- the connection carries no inline delay (`[N]`), which only the runtime's
  delay buffer can provide;
- both nodes are per-sample (`process { }`, not `process block { }`) with
  exactly one input and one output stream;
- neither is a voice bank, oversampled/undersampled, or event-driven.

**The host-visible surface is unchanged.** Parameters and meters keep the names
the patch gave them — `first.gain` stays `first.gain` and `first.level` stays
`first.level`, even though the node they belonged to no longer exists on its own
— a graph parameter aliased onto a fused member's parameter still drives it, and
a member's meter wired to a graph `output value` still reports through it. State,
`'`, `@` and `smooth` slots are private per member, so two fused stages may use
the same names for their state, locals and functions without interfering.

The members themselves are dropped once absorbed: their kernels are never
compiled and do not appear in `getExecutionReport()`. A processor still
instantiated somewhere else in the graph is of course kept.

Fusion is bit-exact: a feed-forward chain computes each stage's sample *n*
before any stage's sample *n + 1*, which is the same order separate kernels
produce, and the intermediate is float32 in a register exactly as it was in the
scratch buffer.

### 3.5 Latency and delay compensation

Once paths can reconverge, misalignment matters: an oversampled branch summed
against a dry one is comb-filtered rather than merely late. The compiler
therefore equalises the branches automatically and reports what is left to the
host via `YdspAudioGraph::getLatencySamples()`.

The rule is a principle, not a list of syntax. Latency is an **artifact** when
the sample count is a leaked consequence of an implementation choice the author
did not make; it is **intentional** when the count *is* the semantics, because
the author typed the number.

| Written | Latency | Why |
|---|---|---|
| `x -> [400] -> y`, `x @ 400` | **intentional** | the author typed 400, and 400 is the meaning |
| `node d = Dist * 4;` | **artifact** (16 samples) | the author typed 4, meaning "run hotter"; the 16 is a property of the resampler's sinc radius and would change if the filter did, with the patch untouched |
| `processor P [[ latency: N ]]` | **artifact** | a leak only the processor's author can know about |

Only artifact latency is compensated, and only at the point where paths
reconverge — never hoisted upstream. A branch with one incoming edge always gets
zero, so a plain chain is untouched.

The decisive case is a **dry/wet delay effect**: the wet path's `@` is
intentional, so both branches have artifact latency 0, nothing is inserted, and
the dry stays dry. A naive "total group delay" model would instead delay the dry
branch — destroying the effect — and tell the host that a 500 ms echo is 500 ms
of plugin latency.

**Separate graph outputs are equalised against each other too.** That is forced,
not chosen: every plugin format reports one scalar (CLAP
`clap_plugin_latency_t::get`, VST3 `getLatencySamples()`, AUv3 `latency`, AAX
`SetSignalLatency(int)`, one LV2 port). A per-output latency vector is
unrepresentable, so a patch whose left is 16 samples later than its right would
be permanently skewed in every host with no diagnostic — and the point where the
two reconverge is often outside the patch (a mono fold, a correlation meter). A
*wanted* skew is written `-> [16] ->`, which compensation never touches.

```ydsp
processor Lookahead [[ latency: 64 ]] {   // declared in this processor's OWN
    input stream in;                      // sample domain
    output stream out;
    // ...
}
```

`[[ latency: N ]]` is in the processor's own sample domain, so an instance
running at `* 4` divides it by 4 — and a factor that does not divide it is a
compile error naming both numbers, because rounding would leave a sub-sample
residual inside the one feature whose job is phase alignment.

**The reported value is a compile-time constant.** A YDSP graph is a fixed DAG:
no parameter can reroute it or change an oversampling factor, so latency cannot
vary while a compiled patch runs and a host never has to be told it changed. A
plugin that offers an oversampling *selector* implements it by recompiling the
patch on the control thread and calling `setLatencySamples()` again — which is
also the only thing the plugin formats support, since changing latency requires a
restart request (`clap_host::request_restart`,
`restartComponent (kLatencyChanged)`) rather than a realtime notification.

Compensation uses the same float32 delay ring as `-> [N] ->`, so a compensated
non-float32 stream is a compile error in this version (only reachable by putting
`[[ latency ]]` on a float64 processor).

## 4. Type system

- Primitive types: `float32` (f32), `float64` (f64), `int32` (i32), `int64`
  (i64), `bool` (stored as i32). The bare keywords `float` and `int` are
  aliases for `float32` and `int32` respectively.
- Array types: `float32[n]`, `float64[n]`, `int32[n]`, `int64[n]` —
  compile-time constant sizes only.
- Stream types: per-sample scalar in sample mode, array of `blockSize`
  elements in block mode. The element type follows the declared keyword
  (`float32` default).
- Endpoint kinds: `input stream` / `output stream` / `input value` /
  `output value`; value endpoints may be any primitive type.
- **Strict typing**: implicit conversions are not allowed. Operands of
  binary/ternary/`select` expressions must have the same type; the only
  implicit adaptation is *contextual literals* — an integer literal may bind
  to `int32`/`int64` or any float width, a float literal to `float32`/
  `float64` (never to int).
- Explicit conversions: `int32(x)`/`int(x)`, `int64(x)`, `float32(x)`/
  `float(x)`, `float64(x)` — narrowing and widening both require them.
- `if`, `&&`, `||`, `!` and `select()` conditions require `bool` (no implicit
  `int -> bool`).
- Result type of `a op b` is the common operand type (`float32 + float64`
  requires a cast); comparisons and `&& || !` produce `bool`.
- Indexing and `for` loop bounds require an `int32` index; `int64` indices
  need an explicit `int32(...)` cast. The delay primitives (`'`, `@`, `mem`)
  require `float32` operands in v1.
- Assignment type-checks exactly (after contextual literal adaptation).

The type system rejects, at compile time: unknown symbols, arity mismatches on
graph composition, wrong endpoint kinds in connections, writes to `input
value` endpoints in sample mode, unbounded loops, non-constant state/buffer
sizes, negative `@` delays, and any use of recursion or dynamic allocation.

## 5. Realtime contract

Compilation and code generation happen on the **control thread**
(`YdspCompiler::compile`, `YdspAudioGraph::prepare`). The audio callback only
calls `YdspAudioGraph::process`, which:

1. never allocates, locks, or throws;
2. walks the compiled nodes in topological order;
3. calls each generated kernel with pre-arranged pointers;
4. reads parameters sampled once per block (host writes land between blocks —
   writes are performed with atomic stores by the host API);
5. for event-driven nodes, splits the block at each event/automation sample
   offset and interleaves kernel sub-blocks with compiled event-handler
   calls (see [2.12](#212-sample-accurate-dispatch-and-automation)).

The generated kernel ABI is fixed for every node:

```c
struct YdspKernelContext {
    void* const* inputs;     // input stream pointers (graph order)
    void* const* outputs;    // output stream pointers (graph order)
    void*        params;     // input value endpoint values (sampled once per block)
    void*        paramOut;   // output value endpoint values
    void*        state;      // scalar segment: every scalar slot
    void*        stateArrays;// array segment: every array (delay lines, rings)
    float        sampleRate; // host sample rate
    int32_t      numSamples; // samples in this block
};
void kernel (YdspKernelContext* ctx);   // the generated function
```

The pointers are `void*` because a stream, parameter or meter may be `float32`,
`float64`, `int32` or `int64`: the element type is declared per endpoint
(`inputTypes`/`outputTypes`/`paramTypes`/`paramOutTypes` on the IR function) and
drives the generated addressing.

Persistent state is one preallocated buffer segmented as
`[scalar segment][array segment]`: every scalar slot (including the ring
write-pointers of the `@` delay primitives) lives in the head, and every array
(delay lines, delay rings) after it. The runtime hands both base pointers
through the ABI (`stateArrays = state + scalar segment size`), so scalar
accesses always use small offsets and array state can grow arbitrarily (e.g.
multi-second delay lines) without displacing them. `YdspAsmJitCodegen::stateSize`
reports the total and `stateScalarSize` the scalar segment; the runtime
allocates and zeroes the whole buffer.

Event handlers use a separate, smaller ABI:

```c
struct YdspEventContext {
    float*  state;       // this voice's scalar segment
    float*  stateArrays; // this voice's array segment
    float*  params;      // the node's param block (shared across voices)
    float   sampleRate;

    float   pitch;       // MIDI note scale
    float   velocity;    // 0..1
    float   pressure;    // 0..1
    float   slide;       // 0..1
    float   bend;        // semitones, signed
    float   value;       // control value, 0..1
    int32_t index;       // control: CC number; programChange: program number
    int32_t flags;       // bit 0 = this noteOn continues a mono legato phrase
};
void eventHandler (YdspEventContext* ctx);   // the generated function
```

Every shape shares this one payload block; the fields a shape does not carry
are zero (see the shape table in [2.11](#211-events), which is the single
source of truth). The IR addresses the payload by byte offset, so a shape may
grow a field without a new opcode. A handler has full read/write access to its
voice's scalar *and* array state, and read access to the node's parameters.

Because loops are statically bounded, recursion is absent, and every buffer is
preallocated, worst-case execution time is **provable at compile time**. The
execution report covers kernel functions; event-handler functions are not
analysed by the report in v1.

## 6. Public API (C++)

```cpp
#include <yup_dsp_jit/yup_dsp_jit.h>

yup::YdspCompiler compiler;
auto result = compiler.compile (sourceString);   // control thread
if (result.wasOk())
{
    auto graph = std::move (result).getValue();  // YdspAudioGraph (move-only)
    graph.prepare (44100.0, 512);                // preallocates everything
    graph.setParameter ("sat.drive", 1.5f);          // host write between blocks
    graph.process (inputs, outputs, 512);        // audio thread, zero-alloc
    auto level = graph.getOutputValue ("sat.level");
}
```

- `YdspCompiler::compile(source, importBasePath = {}, threadPool = nullptr)
  -> ResultValue<YdspAudioGraph>` plus `YdspDiagnostics`
  (line/column/message/severity) on failure. `importBasePath` anchors the
  top-level imports (see [1.3](#13-imports)). When `threadPool` is non-null,
  reading, lexing and parsing the imported files runs in parallel on that
  pool — the merge into the program stays single-threaded, the results are
  identical to the sequential path, and the pool remains caller-owned (the
  compiler never removes jobs it did not add).
- `YdspCompiler::compile(source, options, importBasePath = {}, threadPool =
  nullptr)` selects native-code policy for one compilation. The default
  `YdspOptimizationTier::automatic` uses the host target; use
  `YdspTargetPolicy::baseline` with `baselineTarget` for deterministic
  portability checks. `fastMath` defaults to false, so the compiler does not
  contract `a * b + c`; setting it true permits scalar or packed FMA where the
  selected target supports it. Set `emitOptimizationReport` to inspect the
  selected ISA, lane width, rejected transforms, generated code size and
  compile time with `YdspCompiler::getOptimizationReport()`.
- `YdspAudioGraph::prepare(sampleRate, maxBlockSize, maxEventsPerVoicePerBlock =
  64, maxAutomationPerNodePerBlock = 32)` preallocates state, buffers and
  parameter storage; must be called before `process`. Each distinct event
  offset costs one extra kernel invocation per voice, so the per-voice
  capacity bounds the worst-case dispatch cost of a block.
- `YdspAudioGraph::setLegacyMidiMode(pitchbendRangeSemitones = 2)` /
  `YdspAudioGraph::setMpeZoneLayout(layout)` — choose how incoming MIDI is
  decoded (see [2.11](#211-events)). Legacy (plain MIDI) is the default; both
  are control-thread calls that discard every playing note (voice *state* is
  left alone, so pair them with `reset()` when switching mid-playback).
- `YdspAudioGraph::process(yup::Span<const YdspInputBuffer> inputs,
  yup::Span<YdspOutputBuffer> outputs, int numSamples)` — realtime-safe.
  Stream buffers are typed spans (one per declared stream): the active
  variant alternative carries the element type, so a buffer of the wrong type
  cannot be silently reinterpreted — mismatches are ignored and the call
  reports the problem through the returned `YdspProcessResult` value
  (never asserted, so hosts may probe freely).
- `YdspAudioGraph::process(inputs, outputs, numSamples, const yup::MidiBuffer*,
  const YdspAutomationEvent*, int)` — the same call plus MIDI events and
  sample-accurate parameter automation (see [2.12](#212-sample-accurate-dispatch-and-automation)).
  Passing null/empty MIDI and automation is exactly equivalent to the
  3-argument overload and takes the identical fast path.
- `YdspAudioGraph::process(inputs, outputs, numSamples,
  yup::Span<const yup::MidiBuffer*> eventInputs, automation, numAutomationEvents)` —
  the same call with one `MidiBuffer` per graph event input, in declaration
  order. Each input is decoded and routed independently to whichever nodes
  the patch explicitly wired it to (`connection { }`); a shorter span or a
  null entry means no events on that input. The single-buffer overload above
  is equivalent to feeding the patch's first event input.
- `YdspAudioGraph::getEventInputCount()` / `getEventInputName(index)` — enumerate
  the graph's event inputs (declaration order), so a host can build the
  `eventInputs` span for the overload above.
- `YdspAudioGraph::getParameterSlot("node.param")` — resolves a parameter's integer
  slot once on the control thread for use in `YdspAutomationEvent`;
  returns -1 for unknown names.
- `YdspAudioGraph::getParameterCount()` / `YdspAudioGraph::getParameterInfo(slot)` — build a
  host UI from the patch: one `YdspParameterInfo` per parameter (in slot
  order), carrying the qualified name, the annotation style `[[ name: ... ]]`
  display name, the type, the declared default value and the `[[ min: ... ]]`
  / `[[ max: ... ]]` bounds (falling back to `0`/`1` when unannotated).
- `YdspAudioGraph::getInputStreamCount()` / `getOutputStreamCount()` — stream
  counts (0 for a MIDI-only synth patch).
- `YdspAudioGraph::getDroppedEventCount()` — how many MIDI/automation events were
  dropped because a fixed-capacity per-block queue overflowed (never
  allocated on the audio thread).
- `YdspAudioGraph::getParameter(name)` / `setParameter(name, value)` — host-side,
  atomic; `getOutputValue(name)` for meters.
- `YdspAudioGraph::getExecutionReport()` — the optimiser's worst-case report
  (see below).

## 7. Realtime safety and the execution report

After optimisation the compiler emits, per kernel:

- the number of IR instructions after optimisation;
- every loop's bound provenance (`constant` or `blockSize`-derived);
- the product of loop bounds — the kernel's worst-case iterations. This stays
  the scalar count when a loop is vectorised: it answers "how many iterations
  could this loop run", not "how many instructions were emitted";
- whether any loop was vectorised, and at what lane count (1 when none was, and
  always 1 on a wasm build compiled without `-msimd128` - see
  [2.6](#automatic-vectorisation));
- a boolean "proven realtime-safe" that is `true` only if every loop bound is
  statically known and no realtime-unsafe construct remains.

`YdspAudioGraph::getExecutionReport()` exposes this data so a host can verify,
before starting audio, that the patch fits its CPU budget.

## 8. Known limitations (v1)

- Mono streams only (the ABI already passes `float*` arrays; multichannel is a
  natural extension). `stream[N]` with `N != 1` is a compile error rather than
  a silently-truncated channel count. An event-driven processor with at least
  one audio stream must declare exactly one float32 output stream (the
  runtime's per-voice summing path); a processor with no streams at all
  (MIDI-only) is exempt.
- A subgraph that declares any `input event` can only ever be the main graph,
  never instantiated as a node inside another graph.
- An event endpoint's declared name is a channel identifier, not a shape: it
  need not equal any of the seven shape names, and one channel may carry any
  number of different shapes over its lifetime (see [2.11](#211-events)).
  MIDI clock and transport, SysEx and MIDI 2.0 UMP are not decoded.
- Under MPE, the number of concurrently sounding notes is capped by the
  zone's member-channel count (≤ 15) regardless of `Voice[N]`: a `Voice[16]`
  patch cannot reach 16 notes from an MPE controller.
- Two same-pitch notes cannot coexist on one MIDI channel; the second note-on
  retriggers (releases) the first. They can coexist on separate MPE member
  channels.
- Expression events (`pitchBend`, `pressure`, `slide`) for a note that has no
  allocated voice are discarded and counted in `getDroppedEventCount()`.
- `mode: mono` cannot produce release tails: one voice means one tail.
- Event-driven nodes (voice banks or not) cannot be combined with
  oversampling/undersampling.
- Idle voices are only skipped when the processor opts in with a
  `[[ role: voiceActivity ]]` state; without it every voice of a bank runs
  every block. Skipping applies to *released* voices only — a held voice always
  runs — and is decided per block for the whole kernel, never per sample.
- The realtime-safety report covers kernel functions only; event-handler
  functions are not analysed (their loops are still statically bounded by the
  analyzer).
- Parameter automation is stepped (exact-sample value changes), not
  interpolated; opt in per endpoint with `smooth()` or `[[ smoothing ]]`.
- `[[ smoothing ]]` rewrites the per-sample body only: event handlers and
  `func` bodies that read the parameter directly still see the stepped value.
- Stateful primitives (`'`, `mem`, `@`, `smooth`) are per-sample-only.
- Transcendentals call `libm` (deterministic but not cycle-bounded);
  polynomial approximations are future work.
- The wasm backend (emscripten/browser) emits a self-contained wasm module
  per kernel: it imports the host's shared linear memory as `env.memory`, so
  generated kernels address host buffers directly (no marshalling), and libm
  intrinsics are imported from `env` backed by `Math.*` with C-exact
  semantics for `round`/`copysign`/`fmod`. Kernels are instantiated per JS
  realm (the main thread and the audio-worklet thread each get their own
  copy); `YdspAudioGraph::prewarmKernels()` pre-registers them in the
  calling realm to avoid a one-time cost on the first audio block.
- The recursion operator `~` is parsed and arity-checked but rejected (needs
  cross-node fusion, planned).
- A rate change (`*N` or `/N`) is `float32`-only, on every stream of the node,
  and supports a factor of 2, 4 or 8 only.
- `/N` is lossy by construction: it band-limits to the node's own Nyquist, so it
  suits control-rate work rather than anything that must stay full-bandwidth.
- Every feedback cycle is rejected, including one whose edges carry an inline
  delay: `[N]` does not break a cycle in this version — so a feedback delay
  network has to be written inside a single processor with `@`, not as a graph.
- Latency compensation reuses the `float32` inline-delay ring, so a compensated
  non-`float32` stream is a compile error (only reachable by declaring
  `[[ latency ]]` on a `float64` processor).
- The reported latency is a compile-time constant. A graph is a fixed DAG with no
  conditional routing, so it cannot change while a patch runs; a plugin offering
  an oversampling *selector* recompiles the patch and reports again.
- `_` cannot be combined with another operand inside `,` — `(_ , Gain)` and
  `(Gain , _)` alike: the identity contributes no ports of its own, so the
  result would claim an arity nothing backs. Write the pass-through with an
  explicit `connection` block. `(_ , _)` is fine (it stays an identity).
- A graph input or output identifier used as an algebra leaf carries a port on
  one side only, so nothing can be sequenced *past* a graph output
  (`dry : wet : Gain`) or *into* a graph input (`Gain : dry`); both are compile
  errors.
- A subgraph is inlined, not instantiated at runtime, so a subgraph node cannot
  be a voice bank, cannot be over/undersampled, and cannot declare graph-scope
  MIDI (see [3.3](#33-subgraphs)).
- Float comparisons with NaN operands follow the platform's unordered-compare
  behaviour and may differ slightly between x86-64 and AArch64 for `>=`.
- Cross-node fusion (compiling a whole graph to a single machine function) is
  a future optimiser milestone; v1 calls one generated function per node per
  block, which is a fixed, small overhead.
