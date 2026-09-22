# Oscillators

The `yup_dsp` oscillator classes synthesize bandlimited periodic waveforms from
their Fourier coefficients. Stationary additive synthesis excludes harmonics at
or above Nyquist. Wavetable interpolation, parameter modulation and time-domain
synchronization have finite antialiasing accuracy, described below.

The implementation follows Roth, Keller, Castaneda and Studer, *"Alias-Free
Oscillator Synchronization via Additive Synthesis"* (DAFx26, paper 49).

**Headers:** `yup_dsp/oscillators/` - `yup_FourierSeries.h`,
`yup_SyncSpectralResampler.h`, `yup_AdditiveOscillator.h`,
`yup_WavetableOscillator.h`, `yup_SyncOscillator.h`, `yup_WaveformBank.h`,
`yup_ModulatedOscillator.h`.

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

Three details of the published method are worth knowing before using it:

- **Mirrored sync sounds an octave below the leader.** Its period is `2 T_lead`,
  which is exactly how the paper defines it; `SyncOscillator::getOutputFrequency()`
  reports the fundamental that is actually synthesized.
- **The absolute phase follows the paper's pre-rotation.** Relative to the raw
  time-domain constructions above, the output is the same waveform delayed by half
  a period of the output fundamental, i.e. its coefficients carry a factor
  `(-1)^n`. Only the phase is affected, never the magnitude spectrum.
- **Input and output bandwidth are independent.** Even a single sine can produce
  an infinite harmonic tail after a fractional hard-sync reset. The follower's
  finite series limits how accurately it represents the input waveform, not how
  many output harmonics the transform can produce. The facade computes output
  coefficients up to its harmonic budget and current Nyquist limit.
  `getRecommendedOutputHarmonics()` remains available as a brightness heuristic,
  not as a bound on the synchronized spectrum.

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
    osc.setFrequency (leaderHz);              // phase continuous; update refreshes bandwidth
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
- Pitch changes are phase-continuous. Call `update()` after pitch changes to
  refresh table bandwidth and restore output harmonics after lowering the pitch.
- In additive mode, `update()` skips wavetable rendering. After switching to the
  wavetable backend, call `update()` before processing to refresh its stale table.

| operation | cost |
|---|---|
| `SyncSpectralResampler::transform` | `O (N_in * N_out)` SIMD multiply-accumulates, `O (N_in)` transcendental calls. |
| `AdditiveOscillator::processSample` | SIMD harmonic accumulation; two fundamental-phasor trig calls every 64 samples and two rotation trig calls per frequency change. |
| `WavetableOscillator::render` | One inverse FFT of the oversampled table. |
| `WavetableOscillator::processSample` | One Hermite table read (two while crossfading). |

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

The additive backend provides a stationary bandlimited reference. Wavetable
interpolation introduces small images, and moving parameters can create sidebands
even when both endpoint waveforms are bandlimited.

The wavetable backend crossfades from silence on its first render, using the
crossfade length passed to `prepare()` (64 samples by default). When comparing
it with additive synthesis, let this ramp finish and align the playback phases
before measuring. Subsequent renders crossfade from the current table.

## Prepared waveform banks

`WaveformBank<SampleType, CoeffType>` renders any number of Fourier-series frames
at progressively smaller harmonic counts. Preparation allocates tables and FFT
storage and must happen outside the audio callback. A prepared bank is immutable
during playback and can be shared by multiple voices and threads; its owner must
keep it alive until all readers have stopped.

Reads interpolate adjacent frames at a common phase. Bandwidth selection blends
two levels strictly below the requested harmonic bound, avoiding abrupt level
switches. This conservative transition can darken the sound before Nyquist. A
bound of twice `getNumHarmonics()` selects the full spectrum; a zero bound returns
only DC. Tables retain each frame's DC coefficient.

Banks intentionally reuse `WavetableOscillator` rendering and interpolation. Each
frame/level currently retains its FFT scratch storage as well as its table; share
banks across voices to amortize preparation and memory. No bank rebuild is needed
for pitch, morph, FM, PM or phase-distortion changes.

`refreshFrames(frames)` replaces the coefficients of an already prepared bank
without allocating, the counterpart of `WavetableOscillator`'s `setSeries`/`render`
split. Table sizes and the per-level harmonic limits chosen by `prepare()` are
kept, so each level stays correctly bandlimited; frames with fewer harmonics are
zero-extended. It returns `false` and changes nothing when the frame count differs
from `prepare()` or a frame exceeds `getNumHarmonics()`. It is still one inverse FFT
per frame and level, so it belongs off the audio thread, and it mutates the bank in
place - refresh a second bank and hand it to `ModulatedOscillator::setBank()` or
`PrismOscillator::setBank()` (both allocation-free pointer swaps) rather than
rewriting tables that voices are reading.

## Oversampled audio-rate modulation

`ModulatedOscillator<SampleType, OversampleFactor = 4, CoeffType = double>`
reads a shared bank and provides signed linear FM, exponential pitch modulation,
PM, frame morphing, breakpoint phase distortion and fractional hard sync.

```cpp
std::array<yup::FourierSeries<double>, 2> frames {
    yup::FourierSeries<double>::create (yup::Waveform::sawtooth, 128),
    yup::FourierSeries<double>::create (yup::Waveform::square, 128)
};
yup::WaveformBank<float> bank;
bank.prepare ({ frames.data(), frames.size() });

using Voice = yup::ModulatedOscillator<float>;
Voice voice;
voice.prepare (48000.0, 512, bank);
Voice::Parameters controls;
controls.frequency = 220.0;
controls.syncFrequency = 110.0;
controls.morph = 0.3;
controls.phaseDistortion = 0.4;
// voice.processBlock (output, numSamples, controls);
```

For modulation, `processModulatedBlock(output, numSamples, callback)` invokes the
callback once per **internal-rate** sample. It returns a `Parameters` value.
Maintain modulator phase in caller-owned state across calls: callback indices
restart at zero in each block. Generate coupled modulators at this rate or
interpolate external controls to it. Callbacks must not allocate, block or throw.

| Parameter | Meaning |
|---|---|
| `frequency` | Signed carrier frequency in Hz. |
| `linearFM` | Signed Hz added to the carrier; crossing zero reverses phase. |
| `exponentialFM` | Octave offset, clamped to +/-16; scales carrier plus linear FM. |
| `phaseModulation` | Read-phase offset in periods; leaves accumulated phase unchanged. |
| `morph` | Normalized bank position, clamped to [0, 1]. |
| `phaseDistortion` | Input phase that maps to half a waveform cycle; 0.5 is identity, clamped to [0.01, 0.99]. |
| `syncFrequency` | Nonnegative leader frequency; zero disables hard sync. |
| `bandwidthFrequency` | Optional Hz floor compared against the post-FM frequency when picking a bandwidth level; raise it when a modulator will push the carrier above its current pitch. |

Carrier and leader increments are limited to half an internal sample-rate cycle.
Leader wraps reset the follower at the fractional event time, preserving its
post-reset phase remainder. Two-sample polynomial BLEP/BLAMP residuals correct
value and slope discontinuities at resets and phase-map corners. The bandwidth
estimate includes carrier speed, PM differences and the maximum phase-map slope.
It is a conservative table selection heuristic, not a bound on modulation sidebands.

The entire synthesis/modulation path is generated at the elevated rate, low-pass
filtered, then decimated through a `HalfbandOversampler`. `prepare()` accepts an
optional `HalfbandOversamplerDesign` (default: 100 dB linear-phase FIR, flat to
0.45 of the output rate); the latency follows that design, so read
`getLatencyInSamples()` after `prepare()` and report it to the owning audio
processor. `reset()` clears phase,
residuals and filter history. Processing is allocation-free within the block size
passed to `prepare()`. Invalid block sizes or null output return false without
advancing state. All parameter values must be finite.

These are **antialiased**, not unconditionally alias-free, modulation algorithms.
Finite correction kernels do not correct all higher derivatives of an arbitrary
waveform. Parameters are treated as constant within each internal sample interval;
abrupt control changes are not automatically smoothed. Higher oversampling and a
stricter decimator design improve different error sources at increased CPU cost. Extreme
modulation needs explicit bandwidth/depth constraints. Do not upsample an already
aliased base-rate oscillator and expect its aliases to disappear.

`HalfbandOversampler::beginGeneration()` (and its `SincOversampler` twin) exposes
the same allocation-free generation path for other sources. Fill its high-rate buffer directly and call `downsample()`;
`getGenerationLatencyInSamples()` reports decimation-only latency, while the
existing `getLatencyInSamples()` still describes the complete up/down path.

## Prism spectral shaping

`PrismSpectrum<CoeffType = double>` shapes one `FourierSeries` into another. It is a
pure spectral transform with no DSP state and no transform behind it, so it composes
in front of whichever synthesis backend you were already going to use.

Applied to harmonic `h`, writing `u = log2 (h)` and `c` for the harmonic's complex
coefficient (`cosine + i sine`), the pipeline runs in this order:

| Stage | Operation | Control |
|---|---|---|
| Ridge | `gain = 0.15 + 0.85 * ridge^2`, `ridge = 0.5 + 0.5 * cos (2 pi (u / ridgeSpacing - color))` | `ridgeSpacing` in octaves, clamped to [0.25, 8]; `color` slides the comb and is periodic. |
| Tilt | `gain *= 2^(-tilt * u)` | `tilt` in gain octaves per harmonic octave, clamped to +/-4; 0 is flat. The ridge is periodic in `u`, so it cannot express an overall slope. |
| Odd/even | odd and even harmonics scaled against each other | `oddEven` clamped to [0, 1]; 0 keeps only odd, 1 only even, 0.5 is neutral. No transcendentals at all. |
| Formant | `gain *= 2^(formant * exp (-((u - formantPosition) / formantWidth)^2))` | `formant` is a signed depth in gain octaves, clamped to +/-4, 0 is bypass; `formantPosition` is the center in harmonic octaves, clamped to [0, 12]. One movable resonance against the ridge's periodic comb. |
| Squash | `\|c\|` becomes `\|c\|^squash`, phase kept | `squash` clamped to [0.1, 4]; 1 is bypass. |
| Squeeze | `c` multiplied by `1 - d * cis (-2 pi h w)` | `squeeze` is a pulse width in periods, clamped to [0, 0.5]; the depth `d` opens from 0 to 1 over the first `squeezeFadeWidth`, so 0 is a true bypass. |
| Dispersion + scatter | `c` multiplied by `cis ((dispersion - 0.5) * u^2 + scatter * angle[h])` | `dispersion` clamped to [0, 1], 0.5 is flat; `scatter` clamped to [0, 1], 0 is bypass. Both are rotations, so their angles add and one `sincos` serves both - scatter is free on top of dispersion. `angle[h]` is fixed per harmonic, so a shape stays a timbre instead of re-scattering every block. |
| Normalize | whole series scaled so `sum \|c\|` matches the source's | - |

Every stage is a per-harmonic scale or rotation, so **no stage can produce a
frequency the source did not already contain**. That is the whole antialiasing
argument: shaping cannot alias, and whatever plays the result still bandlimits per
pitch. It is also the rule any further stage must obey: `c[h]` may be multiplied by
anything, but a harmonic's frequency is pinned at `h` times the fundamental and cannot
be moved - stretched or inharmonic partials are not expressible in a periodic series at
all, and need a different oscillator rather than another stage here. Squeeze deserves the emphasis because it looks like it should alias -
pulse-width modulation normally does - but subtracting a phase-shifted copy of a
bandlimited signal is still bandlimited, so this is exact PWM. At `w = 0.5` the factor
is zero for even `h` and two for odd, the square-from-sawtooth identity.

The faded depth is not cosmetic. The raw factor tends to `i * h * 2 pi w` as the width
falls, and renormalizing that leaves a differentiator rather than the source spectrum,
so the stage has no bypass width - a knob stepping off zero would jump. Fading `d` in
over `squeezeFadeWidth` is what makes zero continuous with its neighbourhood, and it
costs one multiply. The factor stays a per-harmonic complex scale either way, so the
antialiasing argument is untouched.

Normalization preserves `sum |c|` rather than the waveform's peak, and the two are not
the same: `sum |c|` bounds a peak from well above (about three times over for a
sawtooth), and how close the waveform comes to that bound depends on how aligned its
harmonic phases are. `dispersion` and `scatter` control exactly that, so if constant
output level matters, measure the shaped waveform's peak and rescale - once per patch
alongside the shaping, not per voice.

Normalization is what makes the controls level-safe. The ridge gain, the companding
and the pulse-width factor (whose magnitude reaches 2) all change level, and
`sum |c|` is an upper bound on the waveform's peak, so preserving it guarantees the
output can never be louder than the source's worst case. The sum runs over harmonics
only; DC is not carried across and the destination's DC is always zero.

```cpp
constexpr int numHarmonics = 128;

yup::PrismSpectrum<double> spectrum;
spectrum.prepare (numHarmonics);

const auto source = yup::FourierSeries<double>::create (yup::Waveform::sawtooth, numHarmonics);
yup::FourierSeries<double> shaped (numHarmonics);

yup::PrismSpectrum<double>::Shape shape;
shape.ridgeSpacing = 1.5;
shape.dispersion = 0.65;
spectrum.process (source, shaped, shape, 0.3 /* color */);

// Either backend will do; this one trades an inverse FFT per update for cheap samples.
yup::WavetableOscillator<float> oscillator;
oscillator.prepare (48000.0, numHarmonics);
oscillator.setSeries (shaped);
oscillator.setFrequency (220.0);
oscillator.render();   // crossfades into the new table
```

### Modulating the shape

`prepare (maxHarmonics)` precomputes `log2 (h)` and `log2 (h)^2`, which deletes a
`log2` per harmonic outright. What remains is a handful of transcendentals per
harmonic and **no transform**, which is cheap enough to re-run once per audio block -
so the shape controls are modulatable, not just settable.

Two rules make that affordable:

- **Shape once, not once per voice.** The shape belongs to a patch, not to a note. One
  `process()` per block feeding every voice costs the same at one voice as at sixteen;
  calling it inside each voice is what makes it expensive.
- **Let the backend absorb the update rate.** `WavetableOscillator::render()`
  crossfades into the new table, so a per-block update sounds continuous, and nothing
  is rendered at all while the controls are still. Note the cost is one inverse FFT per
  *sounding table* per block, so it scales linearly with unison width as well as with
  polyphony: a few percent of a core at one table per voice, several times that at five.
  Where that matters, refresh a single-frame `WaveformBank` once per patch instead - it
  is a fixed cost in voices, which is exactly what `refreshFrames()` is for. `AdditiveOscillator` removes the transform entirely and takes new
  coefficients with zero latency, at the price of a multiply-accumulate per harmonic
  per sample.

Pre-rendering a sweep of `color` positions into a `WaveformBank` is the other obvious
move, and it is a trap unless `color` is the only control you modulate: it buys
audio-rate `color` but multiplies the cost of every *other* shape control by the number
of frames times the number of bandwidth levels, which is precisely the work you were
trying to avoid.

## Verification and performance

The tests include scalar spectral references, exact Nyquist boundaries, phasor
reseeding, endpoint-transform linearity, safe bandwidth transitions, through-zero
frequency, PM, fractional resets, and block-partition independence. A continuous
analytic sync/phase-distortion waveform rendered at 64x provides an independent
reference for the time-domain path, with a 32x/64x convergence check and a comparison
against naive base-rate sampling. This is a regression target for the tested
settings, not a quality guarantee for every modulation depth or waveform.

The fused resampler computes both weighted sums in one SIMD pass and masks
resonances before division. Additive synthesis advances a double-precision
fundamental phasor by recurrence and reseeds it every 64 samples. These changes
remove work, but speedups must be measured on each target architecture. Benchmark
128/512 harmonics, float/double coefficients, both backends, and 1/16/64 voices,
including worst-case parameter updates. No timing claims are implied by the API.


## Related areas

- [Frequency domain](frequency.md) - `FFTProcessor`, which the wavetable renderer
  uses for its inverse transform.
- [Math, windowing & noise](math.md) - `DspMath`, which provides the harmonic
  phasor table used by the transform and the pre-rotation.
- [Resampling](resampling.md) - `SincOversampler`, `HalfbandOversampler` and
  `Resampler` for sample-rate
  conversion, as opposed to the spectral resampling done here.

## Reference

Roth, J., Keller, D., Castaneda, L., Studer, C. *Alias-Free Oscillator
Synchronization via Additive Synthesis*. DAFx26, paper 49. Reference Python
implementation: `github.com/IIP-Group/hasy-python`.

Additional algorithm references:

- [A General Antialiasing Method for Sine Hard Sync](https://dafx.de/paper-archive/2022/papers/DAFx20in22_paper_3.pdf)
  discusses why correcting only value and slope jumps remains approximate for sine sync.
- [Vector Phaseshaping Synthesis](https://www.dafx.de/paper-archive/2011/Papers/55_e.pdf)
  describes breakpoint phase maps and modulation.
- [Practical Linear and Exponential Frequency Modulation for Digital Music Synthesis](https://www.dafx.de/paper-archive/2020/proceedings/papers/DAFx2020_paper_61.pdf)
  discusses FM semantics, sideband bandwidth and oversampling.
