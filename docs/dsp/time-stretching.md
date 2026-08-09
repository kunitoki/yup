# Time-stretching & pitch-shifting

`TimeStretchProcessor` is the block-based, backend-agnostic facade for
time-stretching (tempo change) and pitch-shifting. It delegates to one of two
engines behind a common `Engine` interface:

- **Time-domain backend** (always available) - a built-in WSOLA-style
  synchronous overlap-add engine that also pitch-shifts through resampling.
- **Bungee backend** (only when `YUP_ENABLE_BUNGEE`) - wraps the external
  [Bungee](https://github.com/spulliam/bungee) library, grain-based and pull
  driven.

`Backend::automatic` (the default) picks Bungee when compiled in, otherwise
the time-domain backend. `getAvailableBackends()` and `isBackendAvailable()`
report what the current build supports, and `setBackend()` can switch at
runtime (reinitializing the engine).

## Basic usage

```cpp
using namespace yup;

TimeStretchProcessor processor;
TimeStretchProcessor::ProcessSpec spec;
spec.inputSampleRate  = 48000.0;
spec.outputSampleRate = 48000.0;   // 0 = same as input
spec.maximumBlockSize = 512;
spec.numChannels      = 2;

processor.prepare (spec);                 // NOT realtime-safe; call at init
processor.setTimeRatio (1.5);             // 1.5x slower (output/input length)
processor.setPitchRatio (1.0);            // no pitch shift

const int outputFrames = processor.getExpectedOutputFrameCount (512);
auto result = processor.process (inputPointers, 512, outputPointers, outputFrames);
```

- `timeRatio` — output length / input length (`1.0` = none, `> 1.0` slows
  down, `< 1.0` speeds up). Non-positive values are clamped to `1.0`.
- `pitchRatio` — frequency multiplier (`1.0` = none, `> 1.0` shifts up).
- `process (…)` takes an explicit output frame count and returns the number of
  frames actually written via `ResultValue<int>` (may differ by ±1 depending
  on the backend's rounding). `processUsingTimeRatio (…)` derives the output
  count from the stored `timeRatio` instead.
- `AudioBuffer<float>` overloads wrap the raw-pointer versions.

## Input models

The processor accepts input in two ways:

1. **Direct pointers** - `process (inputChannels, inputFrameCount, …)` copies
   the provided block. Input may not exceed `maximumBlockSize`.
2. **InputProvider** - a callback-based pull model used by the granular
   backends:

```cpp
processor.setInputProvider (
    [](int64 beginFrame, int numFrames, float* const* destChannels,
       int channelStride, int& muteHead, int& muteTail)
    {
        // fill destChannels[ch] with numFrames starting at beginFrame;
        // may set muteHead/muteTail to zero-fill block edges
    });
```

When a provider is set, the `inputChannels`/`inputFrameCount` arguments of
`process` are ignored. `getMaxInputFrameCount()` reports the largest request
the provider may receive.

## Controls and introspection

- `setInputPosition (int64)` — seek to an input frame position.
- `setParameters ({ timeRatio, pitchRatio })` / `setTimeRatio` /
  `setPitchRatio` — realtime-safe updates.
- `getExpectedOutputFrameCount (inputFrames)` — `round (inputFrames ×
  timeRatio)`.
- `getLatencyInFrames ()` — backend-reported latency in input frames.
- `getBackend ()` / `getBackendName ()` (`"Time Domain"`, `"Bungee"`,
  `"None"` when unprepared).
- `reset ()` — clears state and latency (not guaranteed realtime-safe).

```{note}
`prepare`, `reset`, and `setBackend` are not realtime-safe; call them during
initialization or while the audio thread is paused. `process`, the parameter
setters, `setInputPosition`, and `setInputProvider` form the realtime path.
```

## The backends

### Time-domain backend

`TimeDomainTimeStretchBackend` is the built-in engine (backend name
`"Time Domain"`). It splits the input into analysis sequences
(`defaultSequenceLengthMs = 82` ms), cross-fades `12` ms overlaps, and finds
the best overlap position within a `14` ms seek window using a hierarchical
`quickScanOffsets` table (coarse-to-fine cross-correlation summed over all
channels). Pitch-shifting (`pitchRatio ≠ 1`) is implemented by
time-stretching first and then resampling with a `ResamplerFloat` — this is
why pitch shifts change duration unless the time ratio compensates.

Tempo (`1 / timeRatio`) is clamped to `[1, 4]`, so the effective stretch
ratio is at least `0.25` (4× speed-up). Unity tempo takes a direct-copy fast
path. Latency is reported as the input/output FIFO backlog difference
(`max (0, input − output)`).

### Bungee backend

`BungeeTimeStretchBackend` (backend name `"Bungee"`, compiled only under
`YUP_ENABLE_BUNGEE`) drives `Bungee::Stretcher<Bungee::Basic>` with a
grain-based pull loop: it requests grains with `specifyGrain`, fetches input
from the **InputProvider** (which is *required* — without one, `process`
returns 0 frames), and analyses/synthesises grains until the requested output
is produced. It sends `speed = 1/timeRatio` and `pitch = pitchRatio` with
`resampleMode_autoOut`, and prerolls on seek. Latency is derived from the
input/output grain positions.

## Related

- [Resampling](resampling.md) - the `Resampler` used by the time-domain
  backend for pitch shifting.
- [Onset detection](onsets.md) - offline analysis that also consumes full
  buffers of audio.
