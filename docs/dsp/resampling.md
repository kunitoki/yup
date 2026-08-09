# Resampling

The resampling stack is built on precomputed windowed-sinc interpolation
tables with per-channel history buffers, so it operates seamlessly across
audio blocks (real-time safe). It covers integer-factor oversampling, async
sample-rate conversion, and the two building blocks: a compile-time circular
buffer and the sinc lookup table.

## Building blocks

### CircularBuffer

`CircularBuffer<SampleType, BufferSize>` is a fixed-size compile-time ring
buffer for O(1) push plus random-access sample history — the per-channel
history primitive used by the resamplers:

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
table.applyKaiserWindow (5.0);                 // optional Kaiser windowing, beta = 5

double v = table (tap, delta);   // fractional-phase access; negative taps mirrored
```

`configure` sets the cutoff to `sampleRate/2` (correct for integer-factor
upsampling); `configureWithCutoff` takes an explicit cutoff in
`(0, sampleRate/2]` (correct for downsampling, where the anti-aliasing cutoff
is the target Nyquist). `applyKaiserWindow` multiplies the stored half-kernel
by the second half of a Kaiser window without touching the center coefficient.

## Oversampler

`Oversampler<SampleType, OversampleFactor, SincRadius, CoeffType>` provides
multi-channel integer-factor oversampling (typically 2×/4×/8×) for
processing chains that need headroom — distortion, nonlinear filters, etc.
Compile-time constraints: `OversampleFactor >= 2`, `SincRadius >= 1`.

```cpp
yup::Oversampler<float, 4, 8> os;             // 4x oversampling, sinc radius 8
os.prepare (44100.0, 2, 512);

// audio thread:
os.upsample (inPtrs, numChannels, numSamples);
os.processOversampledBlock ([] (auto& buffer) { applyDistortion (buffer); });
os.downsample (outPtrs, numChannels, numSamples);
```

- `prepare` builds the interpolation table (Kaiser β = 5), the decimation
  table (cutoff at `0.45 × input Nyquist`, leaving transition bandwidth), and
  allocates the per-channel history and staging buffers. **Not** realtime-safe.
- `upsample` writes `numSamples × OversampleFactor` bandlimited samples per
  channel into an internal buffer; exact phase multiples pass through
  directly, fractional phases use the `2·SincRadius + 1`-tap sinc.
- `processOversampledBlock (callback)` hands the internal oversampled
  `AudioBuffer` to your callback for the nonlinear processing.
- `downsample` applies the anti-aliasing FIR and decimates back; it must be
  called after the oversampled block was processed, with matching channel and
  sample counts.
- `getLatencyInSamples()` returns `2 × SincRadius` (input-rate samples).
- `reset()` clears history without re-preparing.

Convenience aliases: `Oversampler2xFloat`, `Oversampler4xFloat`,
`Oversampler8xFloat` and the `Double` variants (all radius 8).

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
