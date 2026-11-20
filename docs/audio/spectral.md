# Spectral processing

Some effects are easier to write on frequency bins than on samples: brickwall filters,
spectral gates, freezes, denoisers and cross-synthesis all edit magnitudes and phases. The
spectral framework lets you write such an effect as a `SpectralProcessor` that only ever sees
FFT bins, and run it as a regular `AudioProcessor` through `SpectralBridge`, which does the
short-time Fourier transform (STFT), windowing and overlap-add for you.

**Module:** `yup_audio_processors`

## How it fits together

| Class | Role |
| --- | --- |
| `SpectralProcessor` | Your effect: receives one STFT frame of bins per call |
| `SpectralSpec` | The configuration passed to `SpectralProcessor::prepareToPlay` |
| `SpectralProcessContext` | The bins and parameter changes of one frame |
| `SpectralBridge` | An `AudioProcessor` that converts audio to frames and back |

The bridge collects incoming samples, and every hop it windows the last `fftSize` samples,
runs a real forward FFT on each channel, calls your `processBlock`, runs the inverse FFT and
overlap-adds the result into the output. Because a frame needs `fftSize` samples, the bridge
reports a latency of exactly `fftSize` samples to the host. A `SpectralProcessor` that leaves
the bins untouched reproduces the input, delayed by that latency.

`SpectralProcessor` and `AudioProcessor` share the `DomainProcessor` and
`AudioProcessorBase` bases, so a spectral processor has the parameters, presets and state
interface described in
[Audio processors](processors.md).

## The spec and the context

`prepareToPlay` receives a `SpectralSpec`:

| Field | Meaning |
| --- | --- |
| `sampleRate` | Sample rate in Hz (a `float`) |
| `maxBlockSize` | Maximum audio block size of the bridge |
| `numChannels` | Number of channels the bridge transforms |
| `fftSize` | FFT size in samples, so each channel has `fftSize / 2 + 1` bins |

`processBlock` receives a `SpectralProcessContext<float>` with two fields:

- `bins` - an `AudioSpectralBuffer` (`SpectralBuffer<float>`) holding one channel per audio
  channel. Each bin is a pair of real and imaginary values, from DC (bin 0) to Nyquist
  (bin `fftSize / 2`). Bin `k` is centered on `k * sampleRate / fftSize` Hz. Modify it in place.
- `params` - a `ParameterChangeBuffer` with the parameters that changed since the previous
  frame. Indices refer to the spectral processor's own parameters, each changed parameter
  appears once with its latest value, and every sample offset is 0.

Access bins with `getBinRef (channel, bin)`, which offers `real()`, `imag()`, `set()`,
`magnitude()` and `phase()`, or with `getWritePointer (channel)` for raw interleaved
`[real, imag]` data and vector operations. See [Audio buffers](buffers.md) for the full
`SpectralBuffer` API.

Most processors simply read `getValue()` at the start of each frame, as below; host
automation reaches the parameters because the bridge registers them as its own. Use `params`
to react only when something changed, for example to recompute a gain curve.

## Writing a spectral processor

Derive from `SpectralProcessor`, then implement `prepareToPlay`, `releaseResources`,
`processBlock` and the preset methods. This brickwall low-pass zeroes every bin above a cutoff:

```cpp
class BrickwallFilter : public yup::SpectralProcessor
{
public:
    BrickwallFilter()
        : yup::SpectralProcessor ("Brickwall",
                                  yup::AudioBusLayout ({ yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                                       { yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        addParameter (cutoff = yup::AudioParameterBuilder()
                                   .withID ("cutoff")
                                   .withName ("Cutoff")
                                   .withHostID (1)
                                   .withRange (20.0f, 20000.0f)
                                   .withDefault (2000.0f)
                                   .withUnit (yup::AudioParameter::ParameterUnit::Hertz)
                                   .build());
    }

    void prepareToPlay (const yup::SpectralSpec& spec) override
    {
        binWidth = spec.sampleRate / static_cast<float> (spec.fftSize);
    }

    void releaseResources() override {}

    void processBlock (yup::SpectralProcessContext<float>& context) override
    {
        auto& bins = context.bins;
        const auto firstCut = yup::jlimit (0, bins.getNumBins(), static_cast<int> (cutoff->getValue() / binWidth) + 1);

        for (int channel = 0; channel < bins.getNumChannels(); ++channel)
            for (int bin = firstCut; bin < bins.getNumBins(); ++bin)
                bins.getBinRef (channel, bin).set (0.0f, 0.0f);
    }

    int getCurrentPreset() const noexcept override { return 0; }
    void setCurrentPreset (int) noexcept override {}
    int getNumPresets() const override { return 0; }
    yup::String getPresetName (int) const override { return {}; }
    void setPresetName (int, yup::StringRef) override {}

private:
    yup::AudioParameter::Ptr cutoff;
    float binWidth = 1.0f;
};
```

`processBlock` runs on the [audio thread](../core/multithreading/index.md#the-audio-thread),
zero or more times per audio block (once per hop), so the realtime rules of
[Audio processors](processors.md#realtime-rules) apply: no allocation, no locks. Keep
per-frame state, such as previous magnitudes for a freeze, in buffers sized in
`prepareToPlay` from `numChannels` and `fftSize`. `flush()` is forwarded from the bridge and
should clear that state.

## Running it with SpectralBridge

`SpectralBridge` is an `AudioProcessor`. Give it a bus layout and a spectral processor, which
it shares ownership of:

```cpp
auto bridge = std::make_unique<yup::SpectralBridge> ("Brickwall",
    yup::AudioBusLayout ({ yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                         { yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }));

bridge->setSpectralProcessor (std::make_shared<BrickwallFilter>());
bridge->setFFTSize (2048);
bridge->setOverlapFactor (4);
bridge->prepareToPlay (yup::AudioSpec (48000.0f, 512, 2));
```

The bridge's own bus layout decides how many channels are transformed (the larger of its
input and output channel counts). The spectral processor's bus layout is descriptive only.

| Setting | Default | Effect |
| --- | --- | --- |
| `setFFTSize` | 1024 | Power of two. Larger sizes give finer frequency resolution, more latency and more smearing in time |
| `setOverlapFactor` | 4 | Frames per FFT size; the hop is `fftSize / overlapFactor`. Higher values are smoother and cost more CPU |
| `setWindowType` | `WindowType::hann` | Analysis and synthesis window, see [Math, windowing and noise](../dsp/math.md) |
| `setWindowParameter` | 8.0 | Shape parameter of Kaiser (beta), Gaussian (sigma), Tukey (alpha) and Rakshit-Ullah windows |

Call the setters from the message thread. On a prepared bridge they suspend processing,
rebuild the buffers, prepare the spectral processor again and, for the FFT size, update the
reported latency. The settings persist when the host releases and prepares the bridge again.
The overlap-add is normalized for the chosen window and overlap, so an
untouched spectrum passes the input through.

## Shipping a spectral plugin

To turn the bridge into a plugin, derive from it and return your subclass from the plugin
entry point. The bridge saves and restores the values of the spectral processor's parameters,
so automatable settings round-trip with the host session. It has no editor, so override
`hasEditor` and `createEditor` when you need one:

```cpp
class BrickwallPlugin : public yup::SpectralBridge
{
public:
    BrickwallPlugin()
        : yup::SpectralBridge ("Brickwall",
                               yup::AudioBusLayout ({ yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                                    { yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        setFFTSize (2048);
        setSpectralProcessor (std::make_shared<BrickwallFilter>());
    }
};
```

Override `saveStateIntoDataTree` and `loadStateFromDataTree` to store more than parameter
values, see [Audio processors](processors.md#state-and-presets), and
[Writing plugins](plugin-client.md) for the entry point. The bridge processes single precision
only.

## Related pages

- [Audio processors](processors.md) - lifecycle, parameters and state shared by spectral processors
- [Writing plugins](plugin-client.md) - shipping the bridge as a plugin
- [Audio buffers](buffers.md) - `AudioSpectralBuffer` and bin access
- [Frequency domain](../dsp/frequency.md) - `FFTProcessor`, spectrum layouts and analysis
- [Math, windowing and noise](../dsp/math.md) - window types for the STFT
