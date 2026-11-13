# Expressions

The `yup_expression` module contains a small, statically typed expression language. Programs are
compiled once into a compact instruction list and evaluated many times without
allocating - one row at a time or in batches of rows. It is designed for
data-driven code where values are computed from runtime data: computed layouts,
user formulas, and [data-driven drawing](../graphics/expressions.md).

Include `<yup_expression/yup_expression.h>`. It depends only on `yup_core`, and
`yup_graphics` depends on it.

The pieces are:

| Class | Role |
|---|---|
| `Expression` | Immutable syntax tree of a program, with parser and builders. |
| `ExpressionContext` | Types, inputs, arrays, constants, sub-expressions and functions a program may use. |
| `CompiledExpression` | Type-checked, immutable program. Cheap to copy and independent from its context. |
| `ExpressionState` | Per-thread evaluation state holding input values and outputs. |

## Quick start

```cpp
ExpressionContext context;
auto width = context.addInput<float> ("width");
auto values = context.addArray<float> ("values");
auto index = context.addInput<int> ("i");

auto program = CompiledExpression::compile (R"(
    let w = width / len (values);
    x = i * w;
    height = values[i] > 0.5 ? values[i] * 100 : 0;
)", context);

if (program.failed())
    return; // program.getErrorMessage() is like "2:13: Unknown symbol 'widht'"

auto height = program.getReference().getOutput<float> ("height").getValue();

ExpressionState state (program.getReference());
state.set (width, 400.0f);
state.bindArray (values, data.data(), (int) data.size());

for (int row = 0; row < (int) data.size(); ++row)
{
    state.set (index, row);
    state.evaluate();
    use (state.get (height));
}
```

## Compile once, evaluate many times

Compiling parses, resolves symbols and overloads, folds constants and allocates
registers. Do it once, keep the `CompiledExpression`, and evaluate it as often
as needed through an `ExpressionState`:

- `ExpressionState::evaluate (row)` evaluates one row.
- `ExpressionState::evaluateBlock (startRow, numRows)` evaluates up to
  `getBlockSize()` rows at once; the outputs of row `startRow + i` are in lane `i`.
  `getLanes (output)` returns them all as a `Span`.
- `ExpressionState::evaluateRows (numRows)` evaluates all the rows, writing the
  outputs to columns bound with `bindOutputColumn`.

Evaluation is `noexcept`, never fails and does not allocate unless the program
uses strings (`CompiledExpression::isAllocationFree()`).

Values that only depend on inputs set with `set` (not on bound columns or the
row index) are computed once and reused until one of those inputs is set again,
so `let scaleX = width / len (values);` costs nothing per row. Arrays are read
again on every evaluation, so their elements can be changed in place, and
functions added as not pure always run once per row.

Setting an input to the value it already has does nothing, and a computed value
that comes out the same as before doesn't trigger the values that depend on it,
so unchanged inputs don't re-run their dependent functions. After an evaluation,
`hasChanged (output)` (or `hasChanged()` for any output) tells whether an output
may differ from the previous evaluation. Use it to skip work like relayouts.
Outputs computed per row always report a change:

```cpp
state.set (width, newWidth);
state.evaluate();

if (state.hasChanged (layout))
    applyLayout (state.get (layout));
```

A `CompiledExpression`
is immutable and can be shared between threads; use one `ExpressionState` per
thread.

When the same texts are compiled again and again (for example expressions read
from a document), `ExpressionCache` compiles each distinct text once and returns
the shared program afterwards. Errors are cached too. It compiles against its own
copy of the context, is thread safe, and `removeUnused()` drops the programs
nobody references any more:

```cpp
ExpressionCache cache (context);
auto program = cache.compile ("width * 0.5"); // compiled
auto again = cache.compile ("width * 0.5");   // the same program, not compiled again
```

`Expression::evaluate()` is a convenience that compiles and evaluates in one
call - avoid it in hot paths.

## Batch evaluation

Per-row data can be read straight from memory, so tables are evaluated in
blocks where every instruction processes many rows:

```cpp
struct Sample { float time; float value; };
std::vector<Sample> samples = ...;
std::vector<float> heights (samples.size());

state.bindColumn (value, &samples[0].value, sizeof (Sample)); // array of structures
state.bindRowIndex (index);                                    // int input = row index
state.bindOutputColumn (height, heights.data());
state.evaluateRows ((int) samples.size());
```

Inputs not bound to a column keep the value given with `set` for all rows.

## Language

```
let w = width / 4;            // immutable local
area = w * height;            // output named 'area'
visible = area > 10 && w > 1; // another output
area * 2                      // trailing expression: output named 'result'
```

- **Types**: `bool`, `int` (32 bit), `float`, `double`, `string`, plus custom types
  registered in the context.
- **Literals**: `42`, `0xFF`, `1.5`, `1e-3`, `2.5f`, `true`, `false`, `"text"`.
  An unsuffixed `1.5` adopts the precision of the value it is combined with, so
  `w * 0.5` stays `float` when `w` is a `float`.
- **Units**: numbers accept `px`, `s` and `rad` (unchanged), `deg` (converted to
  radians), `ms` (converted to seconds) and `%` (divided by 100), so `90deg`,
  `250ms` and `50%` are `pi / 2`, `0.25` and `0.5`. A `%` directly followed by
  an operand (`10%3`, `10 % (x)`) is the modulo operator instead.
- **Color literals**: `#rgb`, `#rgba`, `#rrggbb` and `#rrggbbaa` call the
  `color` constructor with the ARGB value, so they need a `color` type in the
  context (registered by `yup_graphics`).
- **Operators** (lowest to highest precedence): `?:`, `||`, `&&`, `== !=`,
  `< <= > >=`, `+ -`, `* / %`, unary `- + !`, `^` (power, so `-2 ^ 2 == -4`),
  then calls, members `a.b`, indexing `a[i]` and swizzles.
- **Conversions**: `bool -> int -> float -> double` widen implicitly. Use
  `int (x)`, `float (x)`, `double (x)`, `bool (x)` and `string (x)` to convert
  explicitly. Numbers never convert to `bool` implicitly: write `x != 0`.
- **Arrays**: `values[i]` clamps the index to the array bounds (an empty array
  yields a default value), `len (values)` returns the size.
- **Safety**: integer division or modulo by zero yields 0 and integer overflow
  wraps. `?:`, `&&` and `||` evaluate both sides, so functions should be pure.
- **Built-in functions**: `sin cos tan asin acos atan atan2 sqrt pow exp log log2
  log10 abs floor ceil round trunc sign fmod min max clamp lerp mix map smoothstep
  hypot deg rad len`, and the constants `pi`, `tau`, `e`.
- **Formatting**: `format ("%.1f dB", gain)` formats up to three numbers with a
  printf subset (flags `-+ #0`, width, precision and `d i e E f F g G`); `%%`
  prints a percent sign.

### Expressions embedded in text

`Expression::parseExpression` parses a single expression starting at an offset
inside a larger document, and moves the offset past it (and past any trailing
whitespace or comments), so a host format can read expressions inline:

```cpp
int offset = text.indexOf ("{") + 1;
auto expression = Expression::parseExpression (text, offset); // stops at '}'
```

On error the offset points at the failing character, and the message reports
the line and column within the whole text.

## The context

```cpp
ExpressionContext context;

auto gain = context.addInput<float> ("gain");
context.addConstant ("limit", 0.8f);
context.addExpression ("boosted", "gain * 2");           // inlined where used
context.addFunction ("square", [] (float x) { return x * x; });
context.addFunction ("noise", [&random] { return random.nextFloat(); }, false); // not pure
context.declareOutput<float> ("level");                  // converts 'level' to float
```

Names may contain dots (`row.value`) to group inputs. Context names shadow the
built-ins.

When the available symbols are only known while compiling (for example names
bound by a document), set a resolver. It is asked for each unknown name, longest
dotted prefix first, and returns the type of a new input or `nullptr`:

```cpp
context.setSymbolResolver ([&] (const String& name) -> const ExpressionType*
{
    return model.hasProperty (name) ? context.getType<float>() : nullptr;
});

auto program = CompiledExpression::compile ("knob.value * 2", context);
auto value = program.getReference().getInput<float> ("knob.value"); // set it on the state
```

The returned type must be a built-in or registered in the context. Resolved
inputs belong to the program only, after the context's inputs.

## Custom types

Any copyable C++ type can be registered and used in programs:

```cpp
struct Vec { float x, y; };

context.addType<Vec> ("vec")
    .constructor ([] (float x, float y) { return Vec { x, y }; })
    .member ("x", [] (const Vec& v) { return v.x; })
    .member ("y", [] (const Vec& v) { return v.y; })
    .member ("length", [] (const Vec& v) { return std::hypot (v.x, v.y); })
    .method ("scaled", [] (const Vec& v, float f) { return Vec { v.x * f, v.y * f }; })
    .operation (ExpressionOperator::add, [] (const Vec& a, const Vec& b) { return Vec { a.x + b.x, a.y + b.y }; })
    .components ({ "x", "y" });                              // swizzles: v.yx is vec (v.y, v.x)
```

Members can be read as `v.length` or `v.length()`. Use `equality()` to add `==`
and `!=` for types with `operator==`. A constructor named after the type is
called as `vec (3, 4)`; qualified constructors like
`constructor ("vec.unit", ...)` are called as `vec.unit (...)`.

```
let v = vec (3, 4);
v.length + v.scaled (2).x + v.yx.x
```

Enumerations are registered with their values, written qualified (`cap.round`)
or bare (`round`) when the name isn't used by another symbol, by the symbol
resolver or by another enumeration. A
bare value shared by several enumerations is resolved to the type of the declared
output it is assigned to:

```cpp
enum class Cap { butt, round, square };

context.addEnum<Cap> ("cap", { { "butt", Cap::butt }, { "round", Cap::round }, { "square", Cap::square } });
context.declareOutput<Cap> ("strokeCap");   // strokeCap = hovered ? round : cap.butt;
```

Register all the types before the functions that use them. `yup_graphics`
registers `point`, `size`, `rect`, `line`, `color`, `transform`, `path` and `font` with
`addGraphicsExpressionTypes` - see [Graphics expressions](../graphics/expressions.md).

## Performance

Besides folding constants, sharing repeated sub-expressions and removing unused
code, the compiler fuses common pairs of instructions: `a * b + c` (and
`c - a * b`, `a * b - c`) on `float` and `double` becomes a single instruction,
as does a ternary whose condition is a numeric comparison used only there
(`x > 0 ? a : b`). Products and comparisons that are used more than once, or
assigned to an output, are kept separate.

A benchmark comparing the compiled expressions with `JavascriptEngine` and with
native C++ is part of the core tests, disabled by default:

```bash
GTEST_ALSO_RUN_DISABLED_TESTS=1 just test Release "*ExpressionBenchmark*" yup_expression
```
