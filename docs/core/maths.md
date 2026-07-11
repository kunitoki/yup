# Maths

`yup_core` bundles numeric utilities used across graphics, audio, and general
code: ranges, parameter mapping, random numbers, arbitrary-precision integers,
and an expression evaluator.

## Range

`Range<T>` is a `[start, end)` interval with a rich, `constexpr`-friendly API.

```cpp
Range<int> r (10, 20);

int  start  = r.getStart();      // 10
int  end    = r.getEnd();        // 20
int  len    = r.getLength();     // 10
bool has    = r.contains (15);
int  clip   = r.clipValue (25);  // 20

auto shifted = r.movedToStartAt (0);          // [0, 10)
auto merged  = r.getUnionWith (Range<int> (18, 30));
```

Handy constructors: `Range::between (a, b)` (orders the endpoints),
`Range::withStartAndLength (start, len)`, and `Range::emptyRange (pos)`.

## NormalisableRange

`NormalisableRange<T>` maps a real-world value range to and from a normalized
`0..1` range, with optional skew and interval snapping. It is the backbone of
audio parameter mapping (faders, frequency controls).

```cpp
NormalisableRange<float> freq (20.0f, 20000.0f, /* interval */ 0.0f, /* skew */ 0.3f);

float norm  = freq.convertTo0to1 (1000.0f);   // 0..1 position of 1 kHz
float value = freq.convertFrom0to1 (0.5f);     // value at the halfway point
float snap  = freq.snapToLegalValue (1234.5f);
```

A skew factor below 1 gives more resolution at the low end (logarithmic-feeling
controls); a symmetric skew centers resolution around a midpoint.

## Random

`Random` is a fast pseudo-random generator. Seed it explicitly for
reproducibility, or use the shared system instance.

```cpp
Random rng (Time::currentTimeMillis());

int    i = rng.nextInt (100);           // 0..99
int    j = rng.nextInt (Range<int> (10, 20));
int64  k = rng.nextInt64();
float  f = rng.nextFloat();             // 0..1
double d = rng.nextDouble();            // 0..1
bool   b = rng.nextBool();
```

## BigInteger

`BigInteger` is an arbitrary-precision integer with bit-manipulation helpers -
used by cryptography and anywhere fixed-width integers overflow.

```cpp
BigInteger n;
n.setBit (128);                 // n = 2^128
n *= 3;
String hex = n.toString (16);   // base-16 string
```

## Expression

`Expression` parses and evaluates arithmetic expressions with named symbols -
useful for user-entered formulas and computed layouts.

```cpp
Expression e ("2 + 3 * x");
Expression::Scope scope;         // supply symbol values via a subclass
double result = e.evaluate (scope);
```

## Statistics & functions

- **`StatisticsAccumulator<T>`** - running mean, variance, min, max, and standard
  deviation over a stream of samples.
- **`MathsFunctions`** - free functions and constants: `jmin`, `jmax`, `jlimit`,
  `jmap`, `roundToInt`, `MathConstants<T>::pi`, degree/radian conversion, and
  more.

```cpp
StatisticsAccumulator<double> stats;
for (double v : samples)
    stats.addValue (v);

double mean = stats.getAverage();
double sd   = stats.getStandardDeviation();

float y = jmap (x, 0.0f, 1.0f, minY, maxY);   // linear remap
float c = jlimit (0.0f, 1.0f, value);          // clamp
```

```{note}
Use the constants and helpers in `MathsFunctions` (e.g. `MathConstants<float>::pi`)
rather than C macros like `M_PI`.
```

## See also

- [Audio: DSP](../audio/index.md) - `NormalisableRange` maps audio parameters.
- [SIMD](index.md) - `yup_simd` for vectorized numeric work.
