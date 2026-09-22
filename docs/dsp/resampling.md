# Resampling

The resampling stack is built on precomputed windowed-sinc interpolation
tables with per-channel history buffers, so it operates seamlessly across
audio blocks (real-time safe). It covers integer-factor oversampling (a
halfband cascade for power-of-two factors and a single-stage sinc for any
factor), async sample-rate conversion, and the two building blocks: a
compile-time circular buffer and the sinc lookup table.

## Building blocks

### CircularBuffer

`CircularBuffer<SampleType, BufferSize>` is a fixed-size compile-time ring
buffer for O(1) push plus random-access sample history — the per-channel
history primitive used by `Resampler`:

```cpp
yup::CircularBuffer<float, 512> history;
history.push (sample);
float oldest = history[0];          // logical index: 0 = oldest, BufferSize-1 = newest
history.clear();
```

`BufferSize` is enforced `> 0` at compile time; the constructor can prefill
every entry with a value (`explicit CircularBuffer (SampleType initValue)`).

### SincTable

`SincTable<CoeffType, OversampleFactor, SincRadius>` precomputes the positive
half of a symmetric windowed-sinc kernel; entry `(tap, delta)` holds the sinc
at `t = tap + delta / OversampleFactor`. It stores `(SincRadius + 1) ×
OversampleFactor` entries.

```cpp
yup::SincTable<double, 256, 8> table;
table.configureWithCutoff (20000.0, 44100.0);  // explicit cutoff (downsampling)
table.configure (44100.0);                     // or cutoff = sampleRate/2 (upsampling)
table.applyKaiserWindow (9.0);                 // optional Kaiser windowing, beta = 9

double v = table (tap, delta);   // fractional-phase access; negative taps mirrored
```

`configure` sets the cutoff to `sampleRate/2` (correct for integer-factor
upsampling); `configureWithCutoff` takes an explicit cutoff in
`(0, sampleRate/2]` (correct for downsampling, where the anti-aliasing cutoff
is the target Nyquist). `applyKaiserWindow` multiplies the stored half-kernel
by the second half of a Kaiser window spanning exactly the kernel radius
(`2 · SincRadius · OversampleFactor + 1` samples) without touching the center
coefficient; the entries beyond the radius (tap `SincRadius` with a nonzero
fractional phase) are zeroed, so the kernel decays smoothly to zero at its edge.

## SincOversampler

`SincOversampler<SampleType, OversampleFactor, SincRadius, CoeffType>` provides
multi-channel integer-factor oversampling (typically 2×/4×/8×) for
processing chains that need headroom — distortion, nonlinear filters, etc.
Compile-time constraints: `OversampleFactor >= 2`, `SincRadius >= 1`.

```cpp
yup::SincOversampler<float, 4, 16> os;        // 4x oversampling, sinc radius 16
os.prepare (44100.0, 2, 512);

// audio thread:
os.upsample (inPtrs, numChannels, numSamples);
os.processOversampledBlock ([] (auto& buffer) { applyDistortion (buffer); });
os.downsample (outPtrs, numChannels, numSamples);
```

- `prepare` designs the interpolation kernel (cutoff at the input Nyquist) and
  the decimation kernel (cutoff at `0.45 × input sample rate`, leaving
  transition bandwidth), both Kaiser-windowed with β = 9 (~90 dB stopband when
  the radius allows), normalized to unity DC gain per phase and stored with the
  gain baked in, and allocates the per-channel staging buffers. The kernels are
  designed in `CoeffType` (default `double`) and accumulate in `CoeffType`
  regardless of `SampleType`. **Not** realtime-safe.
- `upsample` writes `numSamples × OversampleFactor` bandlimited samples per
  channel into an internal buffer; exact phase multiples pass through
  directly, fractional phases use the `2·SincRadius + 1`-tap sinc. Each
  channel keeps the previous `2·SincRadius` input samples contiguously in
  front of its staging buffer, so every output sample is one contiguous
  `dotProduct` (SIMD for `float`/`float` and `double`/`double`) and any block
  size up to `maxBlockSize` produces identical results.
- `processOversampledBlock (callback)` hands the internal oversampled
  `AudioBuffer` to your callback for the nonlinear processing.
- `beginGeneration (numChannels, numSamples)` starts a block generated directly
  at the oversampled rate (an oscillator, for example) without any input
  interpolation; fill `getOversampledChannelData()` and call `downsample`.
  Returns `false` for nonpositive sizes or sizes beyond the prepared capacity.
- `downsample` applies the anti-aliasing FIR (`2·SincRadius·OversampleFactor + 1`
  taps, `2·SincRadius·OversampleFactor` samples of contiguous history) and
  decimates back; it must be called after the oversampled block was processed,
  with matching channel and sample counts.
- `getLatencyInSamples()` returns `2 × SincRadius` (input-rate samples);
  `getGenerationLatencyInSamples()` returns `SincRadius`, the latency of
  generation followed by `downsample`.
- `reset()` clears history without re-preparing.

This single-stage design remains for arbitrary integer factors and fixed,
compile-time kernel sizes. For power-of-two factors prefer
`HalfbandOversampler` below, which is what the `Oversampler2xFloat` …
`Oversampler32xDouble` aliases refer to.

## HalfbandOversampler

`HalfbandOversampler<SampleType, OversampleFactor, CoeffType>` oversamples by
a power of two through a cascade of 2× halfband stages. Every other tap of a
halfband filter is zero, so a stage costs about a quarter of its nominal
length, and only the stage next to the base rate has to be steep: each further
stage only rejects what would fold into the passband and shrinks to a handful
of taps. The result is a deeper stopband and a steeper edge than
`SincOversampler` for less work, and the advantage grows with the factor.

```cpp
yup::HalfbandOversampler<float, 4> os;       // 4x, linear-phase FIR, 100 dB, flat to 0.45 fs
os.prepare (44100.0, 2, 512);

yup::HalfbandOversamplerDesign design;       // shared by every instantiation
design.filterType = yup::HalfbandFilterType::polyphaseIIR;
design.stopbandAttenuationDb = 120.0;
design.passbandEdge = 0.40;                  // fraction of the input rate
yup::HalfbandOversampler<float, 8> lowLatency;
lowLatency.prepare (44100.0, 2, 512, design);

// audio thread, identical to SincOversampler:
os.upsample (inPtrs, numChannels, numSamples);
os.processOversampledBlock ([] (auto& buffer) { applyDistortion (buffer); });
os.downsample (outPtrs, numChannels, numSamples);
```

- `HalfbandOversamplerDesign` (aliased as `Design` inside the class) selects
  the filter family and the targets every stage must meet:
  `stopbandAttenuationDb` (default 100) and `passbandEdge` as a fraction of the
  input rate (default 0.45, i.e. 19.8 kHz at 44.1 kHz). Content between
  `passbandEdge` and `1 - passbandEdge` of the input Nyquist folds back into
  the top of the band, as with every halfband design.
- `HalfbandFilterType::linearPhaseFIR` designs Kaiser-windowed halfbands and
  verifies each stage's stopband numerically at `prepare()` time, lengthening
  the filter until the target is met. Phase is exactly linear and both
  latencies are whole input samples: a small delay at the top rate rounds the
  cascade's fractional delay up. With the defaults, 4× costs about 94 MACs per
  input sample to decimate (188 for the round trip) with a 72-sample round-trip
  latency; 32× costs about 330 / 660 MACs. `SincOversampler` at radius 16
  needs 128 / 256 and 1024 / 2048 for 90 dB and a passband to 0.36 fs.
- `HalfbandFilterType::polyphaseIIR` designs elliptic halfbands realised as
  two allpass branches (Valenzuela & Constantinides). A 100 dB first stage is
  order 17, eight multiplies per sample, and the whole 32× cascade decimates in
  about 76 multiplies per input sample. Latency is a few samples but the phase
  is nonlinear near the passband edge; `getLatencyInSamples()` reports the
  low-frequency group delay rounded to the nearest sample.
- `prepare` is **not** realtime-safe. `sampleRate` is accepted for symmetry
  with `SincOversampler`; the design itself is rate independent.
- `upsample`, `beginGeneration`, `processOversampledBlock`,
  `getOversampledChannelData`, `downsample`, `reset`, `getLatencyInSamples`
  and `getGenerationLatencyInSamples` behave exactly as on `SincOversampler`, so
  the two classes are drop-in replacements for each other.
- `getDesign()` returns the applied design; `getStageFilterOrder (stage)`
  returns the FIR length or the elliptic order of a stage (stage 0 runs next to
  the input rate) for diagnostics.

Convenience aliases: `Oversampler2xFloat`, `Oversampler4xFloat`,
`Oversampler8xFloat`, `Oversampler16xFloat`, `Oversampler32xFloat` and the
`Double` variants are `HalfbandOversampler` instantiations with the default
design.

## Resampler

`Resampler<SampleType, SincRadius, Resolution, CoeffType>` is an async
resampler for **arbitrary (including non-integer)** sample-rate conversion
using a polyphase windowed-sinc filter with high-resolution phase lookup.
Phase state persists across blocks, so streams stay gapless.

```cpp
yup::Resampler<float, 8> r;                  // radius 8, default 256 phases
r.prepare (44100.0, 48000.0, 2, 512);
int produced = r.resample (inPtrs, outPtrs, numChannels, numSamples);
```

- `prepare` builds a sinc table with cutoff `min (source, target) / 2`
  (Kaiser β = 5) and computes the ratio `target / source`.
- `resample` converts `numSamples` per channel and returns the number of
  output samples written per channel. Output buffers must hold at least
  `ceil (numSamples × target / source) + 1`. When downsampling, the gain is
  auto-scaled by the ratio; exact phase multiples pass through directly.
- `getLatencyInSamples()` returns `SincRadius` (input-rate samples).
- `reset()` resets the phase accumulator and clears history — use it after a
  transport discontinuity.

Aliases: `ResamplerFloat = Resampler<float, 8>`,
`ResamplerDouble = Resampler<double, 8>`.

## Related

- [Convolution & delay](convolution-and-delay.md) - the convolver and FAD line
  are sibling realtime processors.
- [Time-stretching](time-stretching.md) - the time-domain stretch backend uses
  a resampler for pitch shifting.
- [Dynamics](dynamics.md) - the canonical oversampling use case (distortion
  with headroom).
