# Dynamics & metering

The dynamics and metering layer covers three concerns: nonlinear waveshapers
(clipping/saturation), an IIR-based antialiasing technique for static
nonlinearities, and level / loudness measurement for meters and analyzers.

## Clippers

Three clippers are provided. The hard and soft ones are stateless;
`HardClipper` is built on the IIR antialiasing machinery (see below).

### BlunterClipper

`BlunterClipper` is a quadratic bounded soft clipper (from Lorenzo Fiestas,
*Quantifying Clipping Softness*) with constant second-derivative magnitude
`|B″| = 2` across the whole knee — the maximum possible softness. The canonical
curve is `B(x) = x·(2 − |x|)` inside `|x| ≤ 1`, `sign(x)` outside.

```cpp
yup::BlunterClipper<float> clip (1.5f, 0.5f); // input gain 1.5, output gain 0.5
clip.processBlock (in, out, numSamples);
```

```{note}
`B′(0) = 2`, so signals below the clip point are amplified 2×. Use
`outputGain = 0.5` for unity gain at the origin. Gain setters are **not
synchronized** — update them between processing passes, not concurrently with
`processSample`.
```

### SoftClipper

`SoftClipper` is a stateless hyperbolic soft clipper: below the clip threshold
it passes through unchanged, above it it bends asymptotically toward
`maxAmplitude`:

```cpp
yup::SoftClipper<float> sc (1.0f, 0.85f);  // amount 0..1: 0 = softest, 1 = hardest
sc.processBlock (in, out, numSamples);
```

`setMaxAmplitude` and `setAmount` (clamped to `[0, 1]`) recompute the internal
threshold `clipThreshold = maxAmplitude · amount`; outputs are
denormal-guarded.

### HardClipper

`HardClipper` is the antialiased version of `f(x) = clamp(x, −1, 1)` — a type
alias of `AaIirAntialiaser` with the `HardClipperTraits` policy, which
declares the clip breakpoints at `±1` so the integrator splits its intervals
exactly at the discontinuities of the derivative:

```cpp
yup::HardClipper<double> clipper;   // default Chebyshev-II order-10 AA filter
clipper.prepare (fs, blockSize);
clipper.processInPlace (buffer, numSamples); // 1-sample latency
```

## IIR antialiasing for static nonlinearities

`AaIirAntialiaser<SampleType, NonlinearTraits, CoeffType>` implements
*Arbitrary-Order IIR Antiderivative Antialiasing* (La Pastina, D'Angelo &
Gabrielli, DAFx 2021). Instead of oversampling or FIR anti-aliasing, it
discretizes the nonlinear function `f(x)` through a fictitious continuous-time
domain using an IIR anti-aliasing filter, which the paper shows reduces
aliasing more than oversampling at comparable cost. It has an **inherent
1-sample latency**.

The nonlinearity is supplied as a *traits* policy with a static `f(x)` and an
optional `fillBreakpoints` (used for piecewise functions like hard clipping).
`TanhClipperTraits` provides a smooth `std::tanh` policy with no breakpoints.

Two factory presets configure the anti-aliasing filter:

- `makeButterworthOrder2 (cutoffNormalized = 0.45)` - 2nd-order Butterworth
  (AA-IIR-1).
- `makeChebyshevTypeIIOrder10 (stopbandNormalized = 0.61,
  stopbandAttenuationdB = 60)` - 10th-order Chebyshev Type II (AA-IIR-2), the
  default.

```cpp
yup::AaIirAntialiaser<float, yup::HardClipperTraits> aa;
aa.configure (yup::AaIirAntialiaser<float, yup::HardClipperTraits>::makeChebyshevTypeIIOrder10());
aa.prepare (fs, blockSize);
aa.processInPlace (buffer, numSamples);
```

A custom filter shape is possible through the nested `PoleConfig` structure
(complex and real pole/residue lists plus a `constantTerm` for biproper
filters). `prepare` precomputes the per-pole exponentials, so the per-sample
path is pure arithmetic and realtime-safe.

## Level measurement

### LevelProcessor

`LevelProcessor` measures RMS (moving window), peak, and peak-with-fall
levels — the primitive building block used by `KMeterState`:

```cpp
yup::LevelProcessor lp;
lp.setSampleRate (48000.0);
float peak = 0.f, rms = 0.f;
lp.processPeak (buf, n, peak);
lp.processRMS (buf, n, rms);
lp.processPeakWithFall (peak, n / 48000.0, peak);
```

- `setIntegrationTime (seconds)` - RMS window (default `0.6` s).
- `setFallTime (seconds)` - peak fall, `26 dB` per fall time (the K-Meter
  spec), default `3.0` s.
- `static calculateBallistics (current, target, timeConstant, timeDelta)` -
  first-order exponential smoothing, reusable anywhere.

The processing methods are realtime-safe; the setters reallocate the RMS
window and must be called off the audio thread.

### LoudnessFilter

`LoudnessFilter` applies ITU-R BS.1770-4 **K-weighting**: two cascaded biquads
— a high-shelf pre-filter (`+4 dB` above `1681 Hz`) and a second-order
Butterworth highpass at `38 Hz` (Q `0.5`). The coefficients are the exact
BS.1770-4 constants (not rounded) and are valid for any sample rate.

```cpp
yup::LoudnessFilter kf;
kf.prepare (48000.0, 512);
kf.processBlock (buf, n);  // in-place K-weighting

// LUFS conversion: -0.691 + 10 * log10 (meanSquare)
```

### KMeterState

`KMeterState` is the full realtime-safe K-Meter implementation (Bob Katz,
*Mastering Audio*, ch. 13): RMS-flat, ITU BS.1770-4 LUFS, and EBU R128
loudness measurement with the classic K-system meter scales. It follows the
same audio-thread / UI-thread split as `SpectrumAnalyzerState`:

- **Audio thread:** `pushSamples` / `pushMonoSamples` into a lock-free
  `AbstractFifo`, then `processPendingAudio()` (from a processing callback or
  timer) computes the levels and publishes them to atomics.
- **UI thread:** the `get*` readouts are wait-free atomic reads.

```cpp
yup::KMeterState meter (48000.0, 2);
meter.setMeteringStandard (yup::KMeterState::MeteringStandard::ituBS1770_4);
meter.setScale (yup::KMeterState::Scale::k14);

// audio thread:
meter.pushSamples (channels, 2, numSamples);
// processing callback:
meter.processPendingAudio();
// UI thread:
float peakDb = meter.getPeakLevel();          // calibrated, max across channels
float lufs   = meter.getIntegratedLoudness();
bool  clip   = meter.isClipping();
```

Configuration:

- `MeteringStandard` - `rmsFlat`, `ituBS1770_4`, `ebuR128` (K-weighting is
  applied for the latter two; switching resets the loudness filters).
- `Scale` - `k20`, `k14`, `k12` (offsets `+20/+14/+12 dB`, ranges
  `−70/−64/−62 dB` to `+20/+26/+28 dB`).
- Timers: `setIntegrationTime` (default `0.6` s), `setPeakFallTime`
  (default `3.0` s), `setAverageFallTime` (default `0.6` s), `setPeakHoldTime`
  (default `10.0` s with auto-release; `−1.0` = infinite hold).
- `setOverThreshold` (linear, default `0.999` = `−0.001 dBFS`) and
  `OverCounterMode` (`contiguous` | `total`) drive the over/clipping counter.

Readouts: `getPeakLevel`, `getAverageLevel`, `getPeakHoldLevel` (per channel
or `−1` for max across channels, dB-calibrated), `getOverCount`,
`isClipping`, `getIntegratedLoudness` (program loudness), `getShortTermLoudness`
(last 3 s), `getMomentaryLoudness` (last 400 ms), and `getLoudnessRange`
(10th–95th percentile, LU).

```{note}
Peak and over/clipping are always measured on the **original** samples (never
K-weighted), and peak is never allowed to fall below RMS. In RMS-flat mode a
`+3.0103 dB` peak-to-average correction is applied. The FIFO holds 2 seconds
of audio and processes in 512-sample chunks.
```

## Related

- [Nonlinear analog filters](filters.md) - the analog-model filters use
  saturation and resonance clipping internally.
- [Onset detection](onsets.md) - level and spectral measurement feeds the
  detectors.
