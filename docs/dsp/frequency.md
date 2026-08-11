# Frequency domain

The frequency-domain layer of `yup_dsp` covers FFT processing, the realtime
spectrum-analyzer sample collection model, and the low-level Ooura FFT
implementation the module can fall back on.

## FFTProcessor

`FFTProcessor` is a multi-backend, float-only FFT engine with a unified
interface. The best available backend is selected **at compile time**, in this
priority order:

1. **PFFFT** (`YUP_FFT_USING_PFFFT`)
2. **Apple vDSP** (`YUP_FFT_USING_VDSP`, via the `Accelerate` framework)
3. **Intel IPP** (`YUP_FFT_USING_IPP`)
4. **FFTW3** (`YUP_FFT_USING_FFTW3`)
5. **Ooura FFT** (`YUP_FFT_USING_OOURA`) — always-available fallback

The engine is non-copyable and move-only; `getBackendName()` reports which
backend is active (`"PFFFT"`, `"Apple vDSP"`, `"Intel IPP"`, `"FFTW3"`,
`"Ooura FFT"`, or `"Unknown"`).

### Supported sizes and layout

FFT sizes are powers of two in `[64, 65536]`. Buffers are **interleaved
complex pairs** — `[re0, im0, re1, im1, ...]` — so an N-point complex spectrum
occupies `2 * N` floats. The engine handles backend-specific packed layouts
(e.g. PFFFT's `[DC, Nyquist, re1, im1, ...]`, Ooura's real-DFT packing)
internally, presenting the same interleaved format to the caller for every
backend.

### Scaling

`FFTScaling` selects how transforms are normalized:

| Mode | Behavior |
| --- | --- |
| `none` | no scaling applied |
| `unitary` | forward scaled by `1/√N`, inverse unscaled |
| `asymmetric` | inverse scaled by `1/N`, forward unscaled |

```cpp
FFTProcessor fft (512);
std::vector<float> realInput (512), complexOutput (1024);

fft.performRealFFTForward (realInput.data(), complexOutput.data());  // R → C, 512 reals → 1024 floats
fft.performRealFFTInverse (complexOutput.data(), realInput.data());  // C → R

fft.performComplexFFTForward (complexInput, complexOutput);          // C → C
fft.performComplexFFTInverse (complexInput, complexOutput);

fft.setScaling (FFTProcessor::FFTScaling::unitary);
fft.setSize (1024);   // re-initialize for a new power-of-two size
```

`setSize()` (re)creates the engine, so call it during initialization, not from
the audio thread. FFTW3 plans are also created during `setSize` (with
`FFTW_ESTIMATE`) rather than per transform.

## SpectrumAnalyzerState

`SpectrumAnalyzerState` is the realtime-safe sample collector for spectrum
analyzers. It follows the classic split between data collection and rendering:

- **Audio thread** pushes samples with `pushSample` / `pushSamples` — lock-free
  writes through an `AbstractFifo`, no allocation.
- **UI thread** polls `isFFTDataReady()`, then pulls overlapping FFT frames with
  `getFFTData()`.

Defaults: FFT size 2048, overlap factor `0.75` (hop size 512 at 2048), FIFO
capacity `4 × fftSize`.

```cpp
SpectrumAnalyzerState analyzer (2048);

// audio thread:
analyzer.pushSamples (buffer, numSamples);

// UI thread:
if (analyzer.isFFTDataReady())
    if (analyzer.getFFTData (fftInput.data()))
        fft.performRealFFTForward (fftInput.data(), spectrum.data());
```

Key methods:

- `pushSample (float)` / `pushSamples (const float*, int)` — audio-thread
  writes; raise the ready flag once at least `fftSize` samples are buffered.
- `isFFTDataReady() const` — `true` when a full frame is available.
- `getFFTData (float* destBuffer)` — copies the oldest `fftSize` samples into
  `destBuffer` and advances the read position by only `hopSize`, so consecutive
  frames overlap. Returns `false` when no data is ready.
- `setOverlapFactor (float)` — overlap in `[0, 1)`, clamped to `[0.0, 0.95]`;
  recomputes the hop size (`min 1`).
- `setFftSize (int)` — changes size and reinitializes the FIFO (clears buffered
  data).
- `reset()` — clears the FIFO and the ready flag.

## OouraFFT8g

`yup_OouraFFT8g.h` exposes Takuya Ooura's classic **FFT8g** suite: single-
dimension, power-of-two, split-radix, decimation-in-frequency, in-place,
table-based transforms (public-domain ISC license, © 1996–2001 Ooura). These
are the primitives used by the Ooura backend of `FFTProcessor`, and are also
available directly:

- `cdft (n, isgn, a, ip, w)` — complex DFT; `n = 2 × (#complex points)`;
  `isgn = 1` forward, `-1` inverse; in-place.
- `rdft (n, isgn, a, ip, w)` — real DFT; packed output
  `[DC, Nyquist, Re1, Im1, Re2, Im2, ...]`; in-place.
- `ddct` / `ddst` — discrete cosine / sine transforms.
- `dfct` / `dfst` — cosine / sine transforms of a real DFT, needing an extra
  scratch buffer `t`.

The work areas follow Ooura's original contract: `ip[0]` must be `0` on first
use (initialization flag), `ip` needs `2 + sqrt(n/2)` ints, and `w` needs
`n/2` floats:

```cpp
std::vector<float> a (1024);               // 512 complex points
std::vector<int>   ip (2 + int (std::sqrt (512)));
std::vector<float> w (512);
ip[0] = 0;                                 // first call only
yup::cdft (1024, 1, a.data(), ip.data(), w.data());  // forward complex FFT, in-place
```

## Related

- [Windowing](math.md) — pair `FFTProcessor` with `WindowFunctions` for
  windowed spectral analysis.
- [Onset detection](onsets.md) — `Spectrogram`, `FilterBank`, and the spectral
  flux ODFs build on the same frequency-domain machinery.
