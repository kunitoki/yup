# Math, windowing & noise

The foundation layer of `yup_dsp`: mathematical conversion and analysis
helpers, a full window-function toolkit, and two sample-rate-independent noise
generators. Everything here is `noexcept`, stateless (or trivially stateful),
and safe to call from the audio thread.

## DSP math utilities

`yup_DspMath.h` provides free functions used throughout the module. All are
templated on the floating-point type and most are `constexpr`.

### Units and conversions

| Function | Converts |
| --- | --- |
| `frequencyToAngular (frequency, sampleRate)` | Hz → radians per sample (`twoPi * f / fs`) |
| `angularToFrequency (omega, sampleRate)` | radians per sample → Hz |
| `qToBandwidth (q)` | filter Q → bandwidth in octaves |
| `bandwidthToQ (bandwidth)` | bandwidth in octaves → Q |
| `dbToGain (decibels)` | dB → linear gain (`10^(dB/20)`) |
| `gainToDb (gain)` | linear gain → dB (`20 * log10(gain)`) |

### Fast trig and dot product

```cpp
// Fast, cheap sine/cosine — accurate for small angles
auto s = yup::fastSin (0.01f);
auto c = yup::fastCos (0.01f);

// SIMD-accelerated dot product for float; generic fallback for other types
auto energy = yup::dotProduct (coeffs, samples, length);
```

`dotProduct` has an explicit SIMD specialization for `float` (implemented via
the `yup_simd` module) and a generic scalar fallback for other coefficient and
sample types. The result accumulates in the coefficient type.

### Bilinear transform

`bilinearTransform` maps an analog (s-plane) biquad to the digital (z-plane)
domain in place, with frequency pre-warping. All six coefficients are modified;
after the call `a0` is normalized to `1.0`:

```cpp
double a0 = 1.0, a1 = 1.41421356, a2 = 1.0; // analog prototype (2nd-order)
double b0 = 1.0, b1 = 0.0, b2 = 0.0;
yup::bilinearTransform (a0, a1, a2, b0, b1, b2, 1000.0, 48000.0);
// a0..a2, b0..b2 are now z-plane coefficients ready for a Biquad
```

### Pole / zero extraction

Three overloads extract the poles and zeros of a transfer function into
`ComplexVector<FloatType>` containers, handling real vs. complex-conjugate
roots and degenerate (first-order / constant) cases:

```cpp
yup::ComplexVector<double> poles, zeros;
yup::extractPolesZerosFromSecondOrderBiquad (b0, b1, b2, a0, a1, a2, poles, zeros);
```

Available for first-order sections, second-order biquads, and fourth-order
sections (factored into two quadratics).

### Linearizing nonlinear step filters

For nonlinear or stateful filters expressed as a step function, the module can
measure an *effective* linear frequency response:

```cpp
auto step = [](float input, std::array<float, 1> state)
{
    return yup::LinearStepResult<1, float> { state[0] + input, { state[0] } };
};

auto response = yup::getLinearizedComplexResponse<1> (step, 1000.0f, 48000.0);
```

`getLinearizedComplexResponse` probes the step function with zero/unit inputs
and basis states, builds the state-space matrices, and solves the complex
linear system `(zI − A) x = B` via Gaussian elimination with partial pivoting —
fully stack-allocated and `noexcept`, so it is realtime-friendly.

## Window functions

`WindowFunctions<FloatType>` is a static-only class implementing all standard
audio windows plus a novel adjustable one. `FloatType` defaults to `double`;
the aliases `WindowFunctionsFloat` and `WindowFunctionsDouble` cover the common
cases.

### Available windows

`WindowType` enumerates 16 windows:

`rectangular`, `hann`, `hamming`, `blackman`, `blackmanHarris`, `kaiser`,
`gaussian`, `tukey`, `bartlett`, `welch`, `flattop`, `cosine`, `lanczos`,
`nuttall`, `blackmanNuttall`, and `rakshitUllah`.

Several windows take a shape parameter — Kaiser `beta`, Gaussian `sigma`,
Tukey `alpha`, and Rakshit-Ullah `r` — passed as the trailing `parameter`
argument (default `8` in the dispatch APIs, but each named method has its own
sensible default such as `sigma = 0.4` or `alpha = 0.5`).

### Single-sample evaluation

```cpp
auto v = yup::WindowFunctions<float>::getValue (yup::WindowType::hann, 64, 128);

// Continuous-phase Tukey — handy for per-sample use in oscillators/modulators,
// where no fixed buffer length exists (phi is normalized phase in [0, 1])
auto amp = yup::WindowFunctions<double>::tukeyFromPhase (0.25, 0.5);
```

### Generating and applying windows

```cpp
std::vector<float> window (512);
yup::WindowFunctions<float>::generate (yup::WindowType::kaiser, window, 8.0f); // Span or (ptr, len)

// Apply a window to a signal, in place or out of place
yup::WindowFunctions<float>::apply (yup::WindowType::blackman, signal.begin(), signal.end());
yup::WindowFunctions<float>::apply (yup::WindowType::hann, signal.data(), windowed.data(), windowed.size());
```

The `kaiser` window uses a private constexpr order-0 modified Bessel function
(25-term series with early termination). `rakshitUllah` is the adjustable
window from *FIR Filter Design Using An Adjustable Novel Window and Its
Applications* (Rakshit & Ullah, IJET 2015); its `r` parameter controls
side-lobe roll-off (paper-suggested values: `0.0005`, `1.18`, `1.618`, `30`,
`75`).

## Noise generators

Both generators are lightweight, `noexcept`, and realtime-safe: they keep a
single PRNG (and a few filter states) as member data and never allocate.

### WhiteNoise

Uniformly distributed white noise in `[-1, 1]`, backed by `yup::Random`:

```cpp
yup::WhiteNoise noise (42);   // optional seed for reproducibility
float sample = noise.getNextSample();  // or noise()
noise.setSeed (1234);         // re-seed at any time
```

### PinkNoise

Pink noise (approximately −3 dB/octave) using Paul Kellett's refined 7-filter
method — six one-pole filters plus a white path, scaled by `0.11`:

```cpp
yup::PinkNoise noise (42);
float sample = noise.getNextSample();  // or noise()
```

The default constructor seeds from the system clock; the `int64` seed
constructors produce reproducible sequences.

## Related

- [Windowing in the frequency domain](frequency.md) — `FFTProcessor` and
  `SpectrumAnalyzerState` consume these windows for spectral analysis.
- [Filter design](filter-design.md) — the designers use `DspMath` conversions
  and `bilinearTransform` internally.
