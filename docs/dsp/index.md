# DSP

The `yup_dsp` module provides the real-time audio processing building blocks of
the framework: mathematical utilities, windowing, noise, FFTs and spectral
analysis, filter design, filter implementations, crossovers, dynamics
processing, metering, convolution, delay lines, resampling, and
time-stretching / pitch-shifting.

**Modules covered:** `yup_dsp`.

```{note}
`yup_dsp` depends on `yup_core`, `yup_audio_basics` (for `AudioBuffer`,
`ProcessSpec`, and friends) and `yup_simd`. On Apple platforms it can use the
`Accelerate` framework (vDSP) for FFTs, and it optionally links
`pffft_library` and `bungee_library` when those third-party libraries are
available in the build.
```

## In this area

- [Math, windowing & noise](math.md) - `DspMath` conversion and analysis
  helpers, the `WindowFunctions` toolkit (17 window types), and the
  `WhiteNoise` / `PinkNoise` generators.
- [Frequency domain](frequency.md) - `FFTProcessor` (with FFTW3 / vDSP / PFFFT /
  Ooura backends), `SpectrumAnalyzerState`, and the low-level `OouraFFT8g`.
- [Filter design](filter-design.md) - `FilterDesigner` and
  `AnalogFilterDesigner` for Butterworth, Chebyshev and Bessel filters, plus
  the analog prototype helpers (`AnalogPoles`, `AnalogSaturator`,
  `AnalogFilterCoefficients`, `StateVariableCoefficients`).
- [Filters](filters.md) - the processing primitives (`FirstOrder`, `Biquad`,
  cascades, coefficient structs) and the ready-to-use filter classes:
  first-order, RBJ biquad, Zoelzer, state-variable, Butterworth,
  Linkwitz-Riley crossovers, direct FIR, analog-mapped filters, and comb
  filters.
- [Dynamics & metering](dynamics.md) - `HardClipper`, `SoftClipper`,
  `BlunterClipper`, the `AaIirAntialiaser` oversampling helper,
  `LevelProcessor`, the K-weighted `LoudnessFilter`, and the
  `KMeterState` metering model.
- [Onset detection](onsets.md) - `FilterBank`, `Spectrogram`, the
  spectral flux ODFs (`SuperFluxODF`, `ComplexFluxODF`), `OnsetPeakPicker`,
  and the end-to-end `OnsetDetector`.
- [Convolution & delay](convolution-and-delay.md) - the `PartitionedConvolver`
  and the `FractionallyAddressedDelay` interpolation delay line.
- [Resampling](resampling.md) - `Oversampler`, `Resampler`, `SincTable`, and
  the `CircularBuffer` helper.
- [Time-stretching & pitch-shifting](time-stretching.md) - the
  `TimeStretchProcessor` with its time-domain and Bungee backends.
- [YDSP language reference](yup-dsp-language.md) - the `yup_dsp_jit` module's
  JIT-compiled DSP language: syntax, intrinsics, the graph algebra, the
  realtime contract, and the public C++ API.
- [YDSP bundle format](ydsp-bundle-format.md) - the RIFF `YDSP` container that
  carries a patch's editable source closure, metadata, and diagnostics.

## Key building blocks

The module is organized around a few core ideas:

- **Coefficient containers are separate from processing.** Filter design
  produces coefficient structs (`FirstOrderCoefficients`,
  `BiquadCoefficients`, `StateVariableCoefficients`, …); the processing
  classes (`FirstOrder`, `Biquad`, and the higher-level filter wrappers)
  consume them. You can design coefficients on any thread and apply them to a
  real-time-safe processor.
- **Realtime-safe by convention.** The per-sample processing methods are
  `noexcept`, allocation-free, and designed for the audio thread.
  Configuration methods such as `prepare()` are explicitly *not* realtime-safe
  and must be called during initialization.
- **Backend pluggability.** `FFTProcessor` and `TimeStretchProcessor` select
  among several backends at runtime (or compile time) so the same public API
  works across platforms and optional dependencies.
- **Templates over `float` / `double`.** Most processing classes are
  templated on the sample type, with `float` and `double` instantiations
  (`WindowFunctionsFloat`, `WindowFunctionsDouble`, `Biquad<float>`, …).

## Realtime rules of thumb

```{admonition} Audio thread
:class: warning
Only the per-sample processing entry points are safe to call from the audio
thread. Call `prepare()`, `reset()` (where documented as non-realtime),
`setSampleRate()`, and coefficient-design functions outside of the audio
callback, then pass values in via atomic or parameter-change mechanisms.
```

## Related areas

- [Audio basics](../audio/index.md) - `AudioBuffer`, `ProcessSpec`, and the
  audio process load measurer that pair with the DSP processors.
- [Audio processors](../audio/index.md) - the `AudioProcessor` model that
  hosts these DSP blocks inside plugins and the audio graph.
- [Core](../core/index.md) - the math, containers, and `Random` used under the
  hood.

```{toctree}
:hidden:
:maxdepth: 2

math
frequency
filter-design
filters
dynamics
onsets
convolution-and-delay
resampling
time-stretching
yup-dsp-language
ydsp-bundle-format
```
