# Audio utilities

`yup_audio_basics` ships a set of small building blocks that most audio code needs:
decibel conversion, parameter smoothing, a simple ADSR envelope, a reverb, IIR
filters, interpolators for varispeed playback, a CPU load measurer for the audio
callback, and audio workgroups for realtime threads on Apple platforms. They are
lightweight and dependency-free. For richer and more configurable versions of some
of them, see the [DSP](../dsp/index.md) documentation and
[Choosing between yup_audio_basics and yup_dsp](#choosing-between-yup_audio_basics-and-yup_dsp).

**Module:** `yup_audio_basics`

## Decibels

`Decibels` converts between linear gain and decibels. Every function is a static
template that works with `float` and `double`:

```cpp
const float gain = yup::Decibels::decibelsToGain (-6.0f);   // about 0.501
const float db = yup::Decibels::gainToDecibels (0.5f);      // about -6.02
```

Both functions take an optional floor, `minusInfinityDb`, which defaults to -100 dB.
`decibelsToGain()` returns exactly 0 at or below the floor, and `gainToDecibels()`
returns the floor for a gain of 0 or a gain below it, so you never get `-inf` or
`NaN` from silence.

| Function | Does |
| --- | --- |
| `decibelsToGain (db, minusInfinityDb = -100)` | Decibels to linear gain, 0 at or below the floor. |
| `gainToDecibels (gain, minusInfinityDb = -100)` | Linear gain to decibels, clamped to the floor. |
| `gainWithLowerBound (gain, lowerBoundDb)` | Clamps a gain so it is never quieter than `lowerBoundDb` (which must be negative). |
| `toString (db, decimalPlaces = 2, minusInfinityDb = -100, includeSuffix = true, customMinusInfinityString = {})` | Formats a level for display, such as `"+3.00 dB"` or `"-INF dB"`. |

`toString()` builds a `String`, which allocates: use it for labels on the message
thread, not in the audio callback.

## SmoothedValue

Changing a gain, cutoff or pan position abruptly between two blocks produces
clicks and zipper noise. `SmoothedValue<FloatType, SmoothingType>` moves from its
current value to a target over a fixed time, one sample at a time:

| Smoothing type | Ramp | Use for |
| --- | --- | --- |
| `yup::ValueSmoothingTypes::Linear` (the default) | Adds a constant step per sample. | Linear gains, mix amounts, pan positions. |
| `yup::ValueSmoothingTypes::Multiplicative` | Multiplies by a constant factor per sample, so it is linear in decibels or octaves. | Frequencies. The value can never be 0, and the ramp can't cross 0. |

`LinearSmoothedValue<FloatType>` is a shorter name for the linear version. A
linear value starts at 0 and a multiplicative one at 1, unless you pass an initial
value to the constructor.

```cpp
class GainStage
{
public:
    void prepare (double sampleRate)
    {
        gain.reset (sampleRate, 0.05);    // 50 ms ramps
        gain.setCurrentAndTargetValue (targetGain.load());
    }

    void process (yup::AudioBuffer<float>& buffer)
    {
        gain.setTargetValue (targetGain.load());
        gain.applyGain (buffer, buffer.getNumSamples());
    }

    std::atomic<float> targetGain { 1.0f };    // written by the UI

private:
    yup::SmoothedValue<float> gain { 1.0f };
};
```

| Method | Does |
| --- | --- |
| `reset (sampleRate, rampLengthSeconds)`, `reset (numSteps)` | Sets the ramp length and jumps to the target. Call it when preparing. |
| `setTargetValue (value)` | Starts a ramp from the current value to `value`. If `reset()` was never called, the ramp has zero length and the value jumps. |
| `setCurrentAndTargetValue (value)` | Jumps to `value` with no ramp. |
| `getNextValue()` | Advances by one sample and returns the value. |
| `skip (numSamples)` | Advances by several samples at once and returns the value. |
| `isSmoothing()`, `getCurrentValue()`, `getTargetValue()` | Query the state. |
| `applyGain (samples, numSamples)`, `applyGain (out, in, numSamples)`, `applyGain (buffer, numSamples)` | Multiplies samples by the smoothed value, advancing it for each sample. When not smoothing, applies the constant gain instead. |

For anything other than gain, call `getNextValue()` per sample, or `skip()` when a
block doesn't need sample accuracy:

```cpp
for (int i = 0; i < numSamples; ++i)
{
    filter.setCutoff (cutoff.getNextValue());
    samples[i] = filter.processSample (samples[i]);
}
```

A `SmoothedValue` is not thread-safe. Own it on the audio thread and hand new
targets to it through an atomic, as above. Audio processor parameters already
come with smoothing: see [processors](processors.md).

## ADSR

`ADSR` is a linear attack, decay, sustain, release envelope, typically one per
synthesizer voice:

```cpp
yup::ADSR envelope;

void prepare (double sampleRate)
{
    envelope.setSampleRate (sampleRate);
    envelope.setParameters ({ 0.01f, 0.2f, 0.7f, 0.5f });   // attack, decay, sustain, release
}

void startNote() { envelope.noteOn(); }
void stopNote()  { envelope.noteOff(); }

void render (yup::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    renderOscillator (buffer, startSample, numSamples);
    envelope.applyEnvelopeToBuffer (buffer, startSample, numSamples);

    if (! envelope.isActive())
        voiceFinished();
}
```

`ADSR::Parameters` holds `attack`, `decay` and `release` times in seconds and a
linear `sustain` level from 0 to 1. The defaults are 0.1 s, 0.1 s, 1.0 and 0.1 s.

| Method | Does |
| --- | --- |
| `setSampleRate (sampleRate)` | Must be called before `setParameters()`. |
| `setParameters (parameters)`, `getParameters()` | Set or read the envelope shape. |
| `noteOn()`, `noteOff()` | Start the attack, or the release from the current level. A zero release ends the envelope at once. |
| `getNextSample()` | Advances by one sample and returns the envelope level. |
| `applyEnvelopeToBuffer (buffer, startSample, numSamples)` | Multiplies every channel of a region by the envelope. If the envelope is idle, clears the region instead. |
| `isActive()` | True from `noteOn()` until the release has finished. |
| `reset()` | Returns to idle immediately. |

All of these are `noexcept` and allocation-free. Avoid changing the parameters
while a note is playing: if you change them before a release has finished, call
`reset()` before the next `noteOn()`.

For curved segments, loops and arbitrary breakpoints, use the yup_dsp
[envelopes](../dsp/envelopes.md).

## Reverb

`Reverb` is a stereo or mono algorithmic reverb based on the public domain
FreeVerb algorithm. It processes `float` samples in place.

```cpp
yup::Reverb reverb;

void prepare (double sampleRate)
{
    reverb.setSampleRate (sampleRate);    // allocates the delay lines

    yup::Reverb::Parameters parameters;
    parameters.roomSize = 0.8f;
    parameters.wetLevel = 0.25f;
    reverb.setParameters (parameters);
}

void process (yup::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() > 1)
        reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());
    else
        reverb.processMono (buffer.getWritePointer (0), buffer.getNumSamples());
}
```

| Parameter | Range | Default |
| --- | --- | --- |
| `roomSize` | 0 (small) to 1 (big) | 0.5 |
| `damping` | 0 (not damped) to 1 (fully damped) | 0.5 |
| `wetLevel` | 0 to 1 | 0.33 |
| `dryLevel` | 0 to 1 | 0.4 |
| `width` | 0 (narrow) to 1 (wide) | 1.0 |
| `freezeMode` | Above 0.5 holds the current tail indefinitely | 0.0 |

`setParameters()` smooths the gains over about 10 ms, so it can be called between
blocks, but it is not locked: call it from the audio thread, or protect it
yourself. `setSampleRate()` allocates and must be called before processing, not on
the audio thread. `reset()` clears the tail.

To add reverb to an `AudioSource` chain, wrap the source in a `ReverbAudioSource`
(see [audio sources](sources.md)).

## IIR filters

`IIRFilter` is a second-order (biquad) IIR filter for one channel of `float`
samples. Its coefficients come from `IIRCoefficients`, which provides the classic
filter shapes:

| Factory | Parameters |
| --- | --- |
| `makeLowPass`, `makeHighPass`, `makeBandPass`, `makeNotchFilter`, `makeAllPass` | `sampleRate, frequency [, Q]` |
| `makeLowShelf`, `makeHighShelf` | `sampleRate, cutOffFrequency, Q, gainFactor` |
| `makePeakFilter` | `sampleRate, centreFrequency, Q, gainFactor` |

`gainFactor` is a linear gain: convert from decibels with
[`Decibels::decibelsToGain()`](#decibels). Use one filter per channel, since each
one keeps its own state:

```cpp
std::array<yup::IIRFilter, 2> filters;

void prepare (double sampleRate)
{
    const auto coefficients = yup::IIRCoefficients::makeLowPass (sampleRate, 2000.0, 0.707);

    for (auto& filter : filters)
    {
        filter.setCoefficients (coefficients);
        filter.reset();
    }
}

void process (yup::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        filters[(size_t) channel].processSamples (buffer.getWritePointer (channel), buffer.getNumSamples());
}
```

A new filter is inactive and passes audio through until you set coefficients;
`makeInactive()` returns it to that state. `processSingleSampleRaw (sample)`
processes one sample.

`IIRFilter` guards its coefficients with a spin lock, so `setCoefficients()` can be
called from another thread while the audio thread processes.
`SingleThreadedIIRFilter` has the same interface without the lock, for filters
that are only touched by the audio thread.

For more filter types, higher orders, per-sample modulation and `double` precision,
use the yup_dsp [filters](../dsp/filters.md).

## Interpolators

Interpolators resample a stream of `float` samples at an arbitrary speed ratio,
which is what varispeed playback, pitch shifting by resampling and simple sample
rate conversion need. All share the same interface and differ in quality and cost:

| Class | Algorithm | Base latency (samples) |
| --- | --- | --- |
| `ZeroOrderHoldInterpolator` | Repeats the previous sample. | 0 |
| `LinearInterpolator` | Straight line between two samples. | 1 |
| `CatmullRomInterpolator` | Catmull-Rom spline over 4 samples. | 2 |
| `LagrangeInterpolator` | 4-point Lagrange polynomial. | 2 |
| `WindowedSincInterpolator` | Windowed sinc over 200 samples. Highest quality, highest cost. | 100 |

They are also available as `Interpolators::Linear`, `Interpolators::Lagrange` and
so on.

```cpp
yup::LagrangeInterpolator interpolator;   // one per channel

// speedRatio is the number of input samples consumed per output sample:
// 2.0 plays an octave up, 0.5 an octave down.
const int used = interpolator.process (speedRatio, input, output, numOutputSamples);
input += used;
```

| Method | Does |
| --- | --- |
| `process (speedRatio, input, output, numOutputSamples)` | Writes `numOutputSamples` and returns how many input samples were consumed. The input must hold enough samples. |
| `process (speedRatio, input, output, numOutputSamples, numInputSamplesAvailable, wrapAround)` | Reads at most `numInputSamplesAvailable` samples. When they run out, wraps back by `wrapAround` samples (for looping a circular buffer), or uses silence if `wrapAround` is 0. |
| `processAdding (...)` | The same overloads with an extra `gain`, adding the scaled result to the output instead of replacing it. |
| `reset()` | Clears the history. Call it when the input jumps, for example after a seek. |
| `getBaseLatency()` | The latency in input samples. The output latency is `getBaseLatency() / speedRatio`. |

Each interpolator remembers the last input samples, so use one instance per
channel and process consecutive blocks of the same stream with it. They are
`noexcept` and allocation-free.

```{note}
These interpolators don't low-pass filter the input, so when `speedRatio` is above
1 (downsampling) high frequencies can alias. For high-quality sample rate
conversion and oversampling, use the yup_dsp [resampling](../dsp/resampling.md)
classes. The [audio sources](sources.md) page covers `ResamplingAudioSource` for
resampling a whole source.
```

## AudioProcessLoadMeasurer

`AudioProcessLoadMeasurer` measures how much of the available time your audio
callback uses: the time spent rendering a block divided by the duration of that
block. Reset it with the sample rate and block size when preparing, then time each
callback with a `ScopedTimer`:

```cpp
yup::AudioProcessLoadMeasurer loadMeasurer;

void prepare (double sampleRate, int maximumBlockSize)
{
    loadMeasurer.reset (sampleRate, maximumBlockSize);
}

void process (yup::AudioBuffer<float>& buffer)
{
    yup::AudioProcessLoadMeasurer::ScopedTimer timer (loadMeasurer, buffer.getNumSamples());

    render (buffer);
}

// On the message thread, for example in a timer
const double percent = loadMeasurer.getLoadAsPercentage();
```

| Method | Does |
| --- | --- |
| `reset()`, `reset (sampleRate, blockSize)` | Clears the statistics, and sets the timing used to compute the load. |
| `ScopedTimer (measurer)`, `ScopedTimer (measurer, numSamplesInBlock)` | Times its own lifetime. The first form uses the block size given to `reset()`, which must not be 0. |
| `registerBlockRenderTime (milliseconds)`, `registerRenderTime (milliseconds, numSamples)` | Record a time you measured yourself. |
| `getLoadAsProportion()`, `getLoadAsPercentage()` | The smoothed load, from 0 to 1 or 0 to 100. |
| `getXRunCount()` | How many blocks took longer to render than their duration. |

Recording never blocks the audio thread: if it races with a `reset()` on another
thread the measurement is skipped. The getters are safe to call from any thread.

`AudioDeviceManager` already measures its callbacks this way, and reports the
result through `getCpuUsage()` and `getXRunCount()` (see [devices](devices.md)).

## AudioWorkgroup

On macOS 11 and later and iOS 14 and later, Core Audio groups the realtime threads
that work on the same audio deadline into an *audio workgroup*. A helper thread
that renders audio for the device callback, for example a thread rendering part of
a graph in parallel, should join the device's workgroup so the system schedules it
together with the audio thread and gives it enough CPU performance.

`AudioWorkgroup` wraps such a workgroup. You get it from the device:

- `AudioIODevice::getWorkgroup()`
- `AudioDeviceManager::getDeviceAudioWorkgroup()`

and a thread joins it by passing a `WorkgroupToken`. The thread stays in the
workgroup while the token is alive:

```cpp
class RenderThread : public yup::Thread
{
public:
    RenderThread (yup::AudioWorkgroup workgroupToJoin)
        : yup::Thread ("Render")
        , workgroup (std::move (workgroupToJoin))
    {
    }

    void run() override
    {
        yup::WorkgroupToken token;
        workgroup.join (token);    // a no-op where workgroups are unsupported

        while (! threadShouldExit())
            renderNextBlock();
    }    // token leaves the workgroup here, on the same thread

private:
    yup::AudioWorkgroup workgroup;
};
```

Start such a thread as a realtime thread, telling the system how much audio it
processes per period:

```cpp
renderThread.startRealtimeThread (yup::Thread::RealtimeOptions {}
                                      .withApproximateAudioProcessingTime (blockSize, sampleRate));
```

See [realtime threads](../core/multithreading/threads.md#realtime-threads) for the
options.

| Member | Does |
| --- | --- |
| `AudioWorkgroup::join (token)` | Joins the calling thread to the workgroup, storing the membership in `token`. |
| `AudioWorkgroup::getMaxParallelThreadCount()` | How many threads can usefully join, or 0 when unsupported. |
| `AudioWorkgroup::operator bool` | True if this refers to a real workgroup. |
| `WorkgroupToken::operator bool` | True while the thread is a member. |
| `WorkgroupToken::reset()` | Leaves the workgroup early. |

`AudioWorkgroup` is cheap to copy. `WorkgroupToken` is move-only, and must be
destroyed or reset on the thread that joined.

On other platforms, and on older Apple systems, workgroups are always empty and
joining does nothing, so the same code runs everywhere. The
[audio graph](audio-graph.md) can use a workgroup for its worker threads: pass one
to `AudioGraphProcessor::setAudioWorkgroup()` during setup.

## Choosing between yup_audio_basics and yup_dsp

| Need | `yup_audio_basics` | `yup_dsp` |
| --- | --- | --- |
| Decibel conversion | `Decibels`, with a silence floor and string formatting | `dbToGain` / `gainToDb` in [math](../dsp/math.md), plain formulas |
| Parameter smoothing | `SmoothedValue` | No equivalent |
| Envelopes | `ADSR`: four linear segments | `CurveEnvelope` and breakpoint curves in [envelopes](../dsp/envelopes.md) |
| Filters | `IIRFilter`: one biquad, float, one channel | Many topologies, orders and designs in [filters](../dsp/filters.md) |
| Resampling | Interpolators: cheap, no anti-aliasing | Band-limited resamplers and oversamplers in [resampling](../dsp/resampling.md) |

Pick the `yup_audio_basics` classes for simple, cheap processing with few
dependencies, such as a gain stage, a basic synthesizer voice or a preview player.
Move to `yup_dsp` when you need sound quality or control beyond that.

## Related pages

- [Audio buffers](buffers.md) - the `AudioBuffer` these utilities process.
- [Audio devices](devices.md) - CPU usage reporting and device workgroups.
- [Audio sources](sources.md) - `ReverbAudioSource` and `ResamplingAudioSource`.
- [Audio processors](processors.md) - parameters and their smoothing.
- [Synthesizers](synthesizers.md) - voices that use envelopes.
- [Audio graph](audio-graph.md) - worker threads and workgroups.
- [DSP](../dsp/index.md) - the richer processing library.
- [Threads](../core/multithreading/threads.md#realtime-threads) - realtime threads.
