# Filters

The filter layer is split into two halves:

- **Primitives** (`base/`) - the coefficient containers and low-level
  processing classes: `FirstOrder`, `Biquad`, `BiquadCascade`, and the shared
  `FilterBase` interface.
- **Parameterized filters** (`filters/`) - ready-to-use classes that track
  human parameters (cutoff, Q, gain, mode) and (re)design their coefficients
  automatically: RBJ, Zoelzer, state-variable, Butterworth, Linkwitz-Riley,
  direct FIR, analog-model, and comb filters.

The general pattern is: call `prepare (sampleRate, maximumBlockSize)` once
during initialization, then either `processSample` or `processBlock` from the
audio thread. Setter methods are `noexcept`, compare against the current value
with `approximatelyEqual`, and only recompute coefficients when a parameter
actually changed.

```{note}
Frequencies are expressed in Hz, Q factors as dimensionless numbers, and gain
in dB. All processing classes are templated on `SampleType` (the audio buffer
type) and `CoeffType` (the internal math type, default `double`), and come
with `Float` / `Double` aliases such as `BiquadFloat`, `RbjFilterDouble`.
```

## The shared interface

`FilterBase<SampleType, CoeffType>` is the pure-virtual base of almost every
filter in the module:

- `prepare (double sampleRate, int maximumBlockSize)` - initialization, not
  realtime-safe.
- `reset()` - zero the internal state.
- `processSample (SampleType) noexcept` / `processBlock (input, output, n)
  noexcept` / `processInPlace (buffer, n) noexcept`.
- `getSupportedModes() const` / `supportsMode (mode) const` - which
  `FilterMode` values a filter implements.
- `getComplexResponse (frequency)`, `getMagnitudeResponse (frequency)`,
  `getPhaseResponse (frequency)` - analysis, in linear magnitude / radians.
- `getPolesZeros (poles, zeros)` - pole/zero decomposition.

The response helpers in `yup_FilterCharacteristics.h` sweep any filter over a
logarithmic frequency range into a `Span<Complex<FloatType>>`:
`calculateFilterMagnitudeResponse` (dB), `calculateFilterPhaseResponse`
(degrees), `calculateFilterGroupDelay` (samples), and
`calculateFilterStepResponse`.

## Primitives

### FirstOrder

`FirstOrder<SampleType, CoeffType>` implements the one-pole / one-zero
difference equation `y[n] = b0·x[n] + b1·x[n−1] − a1·y[n−1]` from a
`FirstOrderCoefficients` (members `a1`, `b0`, `b1`, with `a0 = 1`).

```cpp
FirstOrder<float> filter;
filter.setCoefficients ({ 0.1f, 0.0f, -0.9f });
filter.prepare (48000.0, 512);
float out = filter.processSample (in);
```

### Biquad

`Biquad<SampleType, CoeffType>` is the workhorse second-order IIR, available in
three topologies:

```cpp
enum class Topology { directFormI, directFormII, transposedDirectFormII };
```

`setCoefficients` **auto-normalizes** (`a0 → 1`), and `setTopology` resets the
internal state (a topology change with old state would be unstable).
`BiquadCoefficients` holds `a0, a1, a2, b0, b1, b2` and offers `normalize()`
and `getComplexResponse (frequency, sampleRate)`.

```cpp
Biquad<double> b (Biquad<double>::Topology::transposedDirectFormII);
b.setCoefficients ({ 1.0, 0.0, 0.0, 1.0, -0.9, 0.0 }); // normalized automatically
b.prepare (48000.0, 512);
double out = b.processSample (in);
```

### BiquadCascade

`BiquadCascade<SampleType, CoeffType>` chains N biquads, so the overall
response is the product of the sections. Use it for the multi-section outputs
of the [Butterworth and Linkwitz-Riley designers](filter-design.md).

```cpp
BiquadCascade<double> cascade (4);   // 4 biquads = up to 8th order
cascade.setSectionCoefficients (0, FilterDesigner<double>::designRbjPeak (1000.0, 1.4, 6.0, 48000.0));
cascade.setNumSections (2);          // grows/shrinks, preserving existing sections
```

`setNumSections` preserves already-configured sections, and `setSectionCoefficients`
is bounds-checked (no-op when out of range).

## Parameterized filters

### FirstOrderFilter

`FirstOrderFilter` wraps `FirstOrder` with parameter tracking for the modes
`lowpass | highpass | lowshelf | highshelf | allpass`:

```cpp
FirstOrderFilter<float> lp;
lp.setParameters (yup::FilterMode::lowpass, 500.0f, 0.0, 48000.0);
float out = lp.processSample (in);
```

### RBJ biquads — RbjFilter

`RbjFilter` implements Robert Bristow-Johnson's *Audio EQ Cookbook* biquads
(analog prototype + bilinear transform). It supports the full EQ palette:
peaking, low/high shelf, lowpass, highpass, bandpass, notch, and allpass.

```cpp
RbjFilter<float> eq (yup::FilterMode::peak);
eq.setParameters (yup::FilterMode::peak, 1000.f, 1.0f, 6.0, 48000.0);
eq.setQ (1.4);          // parameter-only setters: setFrequency, setGain, setMode
```

### Zoelzer biquads — ZoelzerFilter

`ZoelzerFilter` is API-identical to `RbjFilter` but designs coefficients with
Udo Zoelzer's approach (`K = tan(ω/2)`), from *Digital Audio Signal
Processing*. One difference: Zoelzer offers both band-pass gain conventions
explicitly via `FilterMode::bandpassCsg` (constant skirt gain) and
`FilterMode::bandpassCpg` (constant peak gain).

### StateVariableFilter

`StateVariableFilter` is a Chamberlin-style state-variable filter producing
lowpass, highpass, bandpass and bandstop **simultaneously**. It is stable over
the full frequency range and clamps Q to `0.707..20`.

```cpp
StateVariableFilter<float> svf;
svf.setParameters (yup::FilterMode::lowpass, 800.f, 0.9f, 48000.0);

auto outs = svf.processAllOutputs (inSample);       // outs.lowpass / .highpass / .bandpass / .bandstop
svf.processMultipleOutputs (in, lp, hp, bp, bs, n); // any output buffer may be nullptr
```

`processSample` returns the output selected by the current mode (default
lowpass). Coefficients are `k = 1/Q`, `g = tan(ω/2)`, normalized as
`g / (1 + g·(k + g))`.

### ButterworthFilter

`ButterworthFilter` is a mathematically correct Butterworth design (analog
prototype + bilinear transform with prewarping) over a `BiquadCascade`.
Orders run from `2` to `maxOrder = 16` (snapped to even); modes are lowpass,
highpass, bandpass, bandstop, and allpass.

```cpp
ButterworthFilter<float> bw (yup::FilterMode::lowpass, 4, 1000.f);
bw.setParameters (yup::FilterMode::lowpass, 4, 1000.f, 0.0f, 48000.0);
bw.processBlock (in, out, numSamples);
```

### LinkwitzRileyFilter

`LinkwitzRileyFilter<SampleType, CoeffType, Order>` is the "Butterworth
squared" crossover: it cascades two Butterworth filters of order `Order/2`
(total order `Order`), giving the classic −6 dB crossover with complementary,
phase-aligned low-pass and high-pass outputs. `Order` is a template parameter
(even, `≥ 2`); use the aliases `LinkwitzRiley2Filter`, `LinkwitzRiley4Filter`
(2 stages), `LinkwitzRiley8Filter`.

```cpp
LinkwitzRiley4Filter<float> xover;
xover.setParameters (2500.f, 48000.0);
xover.processBuffer (inL, inR, outLowL, outLowR, outHighL, outHighR, n);
```

The API is stereo-only and standalone (it does not derive from
`FilterBase`). `getMagnitudeResponseLowBand` / `getMagnitudeResponseHighBand`
return the warped magnitude-squared response of each band.

### DirectFIR

`DirectFIR` is a direct-form FIR (direct convolution) optimized for
realtime use — SIMD through `FloatVectorOperations`, a circular
double-buffered history with write-duplication for zero-copy windowing, and
zero algorithmic delay. It is best below roughly 512 taps; use the
`PartitionedConvolver` for longer impulse responses.

```cpp
DirectFIR<float> fir;
std::vector<float> taps;
FilterDesigner<float>::designFIRLowpass (taps, 64, 1000.f, 44100.f);
fir.setCoefficients (taps);   // during init — allocates
fir.prepare (44100.f, 512);
fir.processBlock (in, out, n);
```

```{warning}
`setCoefficients` may allocate and is **not** realtime-safe. Also note that
`processBlock` *accumulates* into the output buffer — clear it first if you
need a pure write.
```

### Analog-model filters

The analog filters in `yup_AnalogFilters.h` are topology-preserving
nonlinear models, designed by `AnalogFilterDesigner` and processed with
per-sample saturation and resonance clipping. They share a common shape:

```cpp
AnalogMoogLadderFilter<float> moog (yup::AnalogMoogLadderMode::lowpass24);
moog.setParameters (yup::AnalogMoogLadderMode::lowpass24, 1000.f, 0.5f, 0.2f, 48000.0);
moog.setSignalRange (2.0f);   // normalize to ±2 input range
moog.processBlock (in, out, n);
```

| Class | Character | Modes |
| --- | --- | --- |
| `AnalogTwoPoleFilter` | trapezoidal-integrator SVF, 2 poles | `lowpass`, `highpass`, `bandpassCsg`, `bandpassCpg`, `bandstop`, `peak` |
| `AnalogVowelFilter` | 3 cascaded formant peaks, vowel position `0..1` | `peak` |
| `AnalogKorg35Filter` | Korg 35-inspired 3-pole | `lowpass`, `bandpassCsg`, `highpass` |
| `AnalogMoogLadderFilter` | 4-pole ladder, 10 output modes | via `AnalogMoogLadderMode` |
| `AnalogRolandDiodeFilter` | diode-ladder 4-pole lowpass | `lowpass` |

All take normalized resonance / saturation in `0..1` and expose
`setSignalRange` / `getSignalRange` to normalize the input/output amplitude
(default range `1.0`). Their complex frequency responses are obtained either
in closed form (two-pole) or by linearizing the nonlinear step function
(`getLinearizedComplexResponse`) for the ladder/Korg/diode models.

### CombFilter

`CombFilter` is a fractional-delay feedback comb: a power-of-two circular
delay line (default `16384` samples) with cubic Hermite interpolation for
fractional reads, `tanh`-clipped feedback resonance, and optional `fastAtan`
output saturation. Delay changes ramp at ±4 samples/sample for click-free
pitch sweeps.

```cpp
CombFilter<float> comb;
comb.setParametersFromNote (69.0f, 0.7f, 0.2f, 48000.0);  // A4 = 440 Hz, equal temperament
comb.prepare (48000.0, 512);
comb.processBlock (in, out, n);
```

`setParameters (frequencyHz, feedback, saturation, sampleRate)` sanitizes its
inputs (feedback/saturation clamped to `0..1`, frequency clamped to
`[fs/(N−1), 0.45·fs]`); `setParametersFromNote` accepts a MIDI note number.
`getDelayInSamples()` reports the current target delay. The comb supports only
`FilterMode::peak`.

## Related

- [Filter design](filter-design.md) - the designers that produce the
  coefficients these filters consume.
- [Math](math.md) - `bilinearTransform`, pole/zero extraction, and response
  helpers used across the filter layer.
- [Convolution & delay](convolution-and-delay.md) - `DirectFIR`'s sibling for
  long impulse responses, plus a different fractional-delay line.
