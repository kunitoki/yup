# Oscillators

The `yup_dsp` oscillator classes synthesize bandlimited periodic waveforms from
their Fourier coefficients, and synchronize one oscillator to another without
aliasing: the output spectrum is built from scratch for every period ratio, so no
harmonic ever folds back below Nyquist.

The implementation follows Roth, Keller, Castaneda and Studer, *"Alias-Free
Oscillator Synchronization via Additive Synthesis"* (DAFx26, paper 49).

**Headers:** `yup_dsp/oscillators/` - `yup_FourierSeries.h`,
`yup_SyncSpectralResampler.h`, `yup_AdditiveOscillator.h`,
`yup_WavetableOscillator.h`, `yup_SyncOscillator.h`.

## The idea

A free-running *follower* oscillator is described by its Fourier series
`{a0, a_k, b_k}`, with `t` in units of the follower period `T_follow`. A *leader*
runs at a period ratio `P = T_lead / T_follow`. When the follower is restarted,
mirrored or windowed on every leader period, the result is periodic again, and its
Fourier coefficients turn out to be a **linear transform** of the follower's: the
sinc and versinc kernels of the paper resample the follower's spectrum onto the
leader's harmonic grid.

Synthesizing that transformed series with only the harmonics below Nyquist is
alias-free *by construction* - nothing has to be filtered out afterwards.

| mode | construction | pre-rotation | output fundamental |
|---|---|---|---|
| `hard` | `s (t) = r (t + P / 2)` on `[-P/2, P/2)`, period `P` | `-P/2` | `f_lead` |
| `mirrored` | `s (t) = r (P - abs (t))` on `[-P, P)`, period `2 P` | `-P` | `f_lead / 2` |
| `pulsar` | one follower period in a window of width 1, period `P` | `-1/2` | `f_lead` |

Two details of the published method are worth knowing before using it:

- **Mirrored sync sounds an octave below the leader.** Its period is `2 T_lead`,
  which is exactly how the paper defines it; `SyncOscillator::getOutputFrequency()`
  reports the fundamental that is actually synthesized.
- **The absolute phase follows the paper's pre-rotation.** Relative to the raw
  time-domain constructions above, the output is the same waveform delayed by half
  a period of the output fundamental, i.e. its coefficients carry a factor
  `(-1)^n`. Only the phase is affected, never the magnitude spectrum.
- **Bandlimiting the follower caps the output.** The transform cannot create
  harmonics the follower does not have, so a follower with `N` harmonics produces
  an output whose top harmonic sits at `N * P` times the leader frequency.
  `getRecommendedOutputHarmonics()` reports the harmonic count needed to keep all
  of them; the facade clamps it to the configured maximum.

## The classes

| class | role |
|---|---|
| `FourierSeries<CoeffType>` | Coefficient container: `dc`, `std::vector` cosine and sine coefficients, waveform presets (`sine`, `cosine`, `sawtooth`, `square`, `triangle`, `pulse`), a direct DFT (`setFromCycle`) and the pre-rotation (`timeShift`). |
| `SyncSpectralResampler<CoeffType>` | The synchronization transform itself: follower coefficients plus a period ratio and a `SyncMode` in, synchronized coefficients out. |
| `AdditiveOscillator<SampleType, CoeffType>` | Exact additive synthesis of a series at a fundamental, Nyquist limited, evaluated a SIMD width of harmonics at a time. |
| `WavetableOscillator<SampleType, CoeffType>` | Renders a series into a single-cycle table with an inverse FFT and plays it back with 4-point Hermite interpolation, crossfading between renders. |
| `SyncOscillator<SampleType, CoeffType>` | The synth-ready facade: a follower series, a mode, a ratio and a pitch in, audio out, with both synthesis backends selectable at runtime. |

Both oscillators are usable on their own as plain bandlimited oscillators; the
resampler is usable on its own for analysis or visualization, and the series can be
inspected through `SyncOscillator::getSyncedSeries()`.

## Real-time contract

```cpp
yup::SyncOscillator<double> osc;

void prepare (double sampleRate, int maxHarmonics)
{
    osc.prepare (sampleRate, maxHarmonics);   // allocates, renders the first table
    osc.setWaveform (yup::Waveform::sawtooth);
    osc.setSyncMode (yup::SyncMode::hard);
}

void setPitch (double leaderHz)
{
    osc.setFrequency (leaderHz);              // phase continuous, no update needed
}

void setSync (double ratio, yup::SyncMode mode)
{
    osc.setFollowerRatio (ratio);             // marks the oscillator dirty
    osc.setSyncMode (mode);
}

void processBlock (double* output, int numSamples)
{
    osc.update();                             // resample + render, once per block
    osc.processBlock (output, numSamples);
}
```

- `prepare()` is the only method that allocates. It must run outside the audio
  callback.
- Parameter setters only store values and mark the oscillator dirty; nothing heavy
  happens inside `update()`, `processSample()` or `processBlock()` beyond the work
  described below. `update()` is a no-op when nothing is dirty, so it is safe to
  call unconditionally once per block.
- `update()` is allocation-free, so it may be called from the audio callback.
- Pitch changes are phase-continuous and need no `update()`; synchronization
  parameter changes need exactly one.

| operation | cost |
|---|---|
| `SyncSpectralResampler::transform` | `O (N_in * N_out)` multiply-accumulates, `O (N_in)` transcendental calls. About 10 us for 128 harmonics, 100 us for 512. |
| `AdditiveOscillator::processSample` | One multiply accumulate per active harmonic, plus 2 transcendental calls. |
| `WavetableOscillator::render` | One inverse FFT of the oversampled table, 10 to 50 us for the default table. |
| `WavetableOscillator::processSample` | Two Hermite table reads (two while crossfading). |

## Choosing a synthesis backend

- **Wavetable** (the default) renders the synchronized series into a single-cycle
  table and reads it back with interpolation. The per-sample cost is independent of
  the harmonic count, at the price of interpolation images (the table is
  oversampled 8x, which pushes them below roughly -70 dB) and of a table that is
  only refreshed on `update()`. Pitch changes between updates can push the top
  rendered harmonic slightly past Nyquist, which is why a synth should call
  `update()` once per block.
- **Additive** synthesizes the harmonics directly, so it is exact and reacts to
  every change immediately, but its per-sample cost grows with the harmonic count.

Both honor the Nyquist limit, so both are alias free.

The wavetable backend crossfades from silence on its first render, using the
crossfade length passed to `prepare()` (64 samples by default). When comparing
it with additive synthesis, let this ramp finish and align the playback phases
before measuring. Subsequent renders crossfade from the current table.

## Related areas

- [Frequency domain](frequency.md) - `FFTProcessor`, which the wavetable renderer
  uses for its inverse transform.
- [Math, windowing & noise](math.md) - `DspMath`, which provides the harmonic
  phasor table used by the transform and the pre-rotation.
- [Resampling](resampling.md) - `Oversampler` and `Resampler` for sample-rate
  conversion, as opposed to the spectral resampling done here.

## Reference

Roth, J., Keller, D., Castaneda, L., Studer, C. *Alias-Free Oscillator
Synchronization via Additive Synthesis*. DAFx26, paper 49. Reference Python
implementation: `github.com/IIP-Group/hasy-python`.
