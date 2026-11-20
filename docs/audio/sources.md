# Audio sources

An `AudioSource` is a pull-based producer of audio: whoever plays it asks for
the next block of samples, and the source fills it. Sources can wrap each other
to build simple playback chains - a file reader inside a resampler inside a
mixer - and an `AudioSourcePlayer` plays the outermost one on an audio device.
They are the lightweight alternative to [audio processors](processors.md) when
all you need is playback.

**Modules:** `yup_audio_basics` (the sources), `yup_audio_devices` (`AudioSourcePlayer`, `AudioTransportSource`)

## The AudioSource contract

Every source implements three methods:

| Method | Thread | Purpose |
| --- | --- | --- |
| `prepareToPlay (int samplesPerBlockExpected, double sampleRate)` | Message or device thread, before playback | Allocate buffers and reset state for the given rate and block size. |
| `getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)` | Audio thread | Fill `bufferToFill.numSamples` samples, starting at `bufferToFill.startSample`, in every channel of `bufferToFill.buffer`. |
| `releaseResources()` | Message or device thread, after playback | Free what `prepareToPlay` allocated. |

`prepareToPlay` and `releaseResources` can be called several times, for
example every time the device restarts. `getNextAudioBlock` must follow the
rules of [the audio thread](../core/multithreading/index.md#the-audio-thread):
no allocation, no blocking, no file access.

`AudioSourceChannelInfo` describes the region to fill. Only touch that region
of the buffer, and call `clearActiveBufferRegion()` when the source has
nothing to play:

```cpp
class NoiseSource : public yup::AudioSource
{
public:
    void prepareToPlay (int, double) override {}
    void releaseResources() override {}

    void getNextAudioBlock (const yup::AudioSourceChannelInfo& info) override
    {
        if (muted)
        {
            info.clearActiveBufferRegion();
            return;
        }

        for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch)
        {
            auto* samples = info.buffer->getWritePointer (ch, info.startSample);

            for (int i = 0; i < info.numSamples; ++i)
                samples[i] = random.nextFloat() * 0.2f - 0.1f;
        }
    }

    std::atomic<bool> muted { false };

private:
    yup::Random random;
};
```

The buffer can already contain data, for example the device input when played
through `AudioSourcePlayer`, so a source can also process what it receives.

### Positionable sources

A `PositionableAudioSource` has a length and a read position, measured in
samples. File readers, memory buffers and the transport are positionable:

| Method | Purpose |
| --- | --- |
| `setNextReadPosition (int64)` | Seeks. May be called from another thread while playing. |
| `getNextReadPosition()` | The position of the next block. |
| `getTotalLength()` | Length in samples. |
| `isLooping()`, `setLooping (bool)` | Loop the source when it reaches the end (if supported). |

## Playing a source on a device

`AudioSourcePlayer` is an `AudioIODeviceCallback` that pulls from a source.
Register it with the [device manager](devices.md) and give it a source:

```cpp
yup::AudioDeviceManager deviceManager;
yup::AudioSourcePlayer sourcePlayer;
yup::ToneGeneratorAudioSource tone;

deviceManager.initialiseWithDefaultDevices (0, 2);
deviceManager.addAudioCallback (&sourcePlayer);
sourcePlayer.setSource (&tone);

// Shutdown: detach in reverse order
sourcePlayer.setSource (nullptr);
deviceManager.removeAudioCallback (&sourcePlayer);
```

- `setSource` does not take ownership. It prepares the new source with the
  current device settings, swaps it in under a lock and releases the old one,
  so it can be called while audio is running.
- `setGain (float)` scales the output.
- When the device starts or stops, the player calls `prepareToPlay` and
  `releaseResources` on its source.
- Device input channels are copied into the buffer before
  `getNextAudioBlock` is called.

Don't destroy the player, or the current source, while the device is using
them.

## AudioTransportSource

`AudioTransportSource` adds transport controls to a positionable source: start,
stop, seek, gain, optional read-ahead on a background thread and optional
sample rate conversion.

```cpp
transportSource.setSource (readerSource.get(),   // PositionableAudioSource, not owned
                           32768,                // samples to read ahead, 0 for none
                           &readAheadThread,     // TimeSliceThread used for read-ahead
                           reader->sampleRate,   // the source's rate, 0 to skip conversion
                           2);                   // maximum channels

transportSource.setPosition (10.0);  // seconds
transportSource.start();
```

| Call | Notes |
| --- | --- |
| `setSource (...)` | Stops playback and rewinds. Pass `nullptr` to detach the current source. |
| `start()`, `stop()` | Both send a change message. |
| `isPlaying()`, `hasStreamFinished()` | Playback state. |
| `setPosition (seconds)`, `getCurrentPosition()`, `getLengthInSeconds()` | Position in seconds. |
| `setGain (float)`, `getGain()` | Gain changes are ramped to avoid clicks. |

- **Read-ahead.** With a non-zero read-ahead size and a running
  `TimeSliceThread`, the source is read on that thread into a buffer, so the
  audio thread never waits for the disk. Use it for files; skip it for sources
  already in memory. The thread must outlive the transport's source.
- **Sample rate conversion.** When the source's rate differs from the device
  rate, pass it as `sourceSampleRateToCorrectFor` and the transport resamples
  to the device rate.
- **Stopping.** `stop()` fades out the last block and waits (up to about a
  second) for the audio thread to acknowledge it. Call it from the message
  thread, never from the audio thread.
- **End of stream.** When a non-looping source reaches its end, playback
  stops and a change message is sent from the audio thread. It is a
  `ChangeBroadcaster`: listen to it to update a play button, or poll
  `isPlaying()` from a `Timer` (preferred on the web, see [Web audio](web.md)).

## Playing an audio file

This combines the pieces: an [audio file](audio-files.md) reader wrapped in an
`AudioFormatReaderSource`, a transport with read-ahead and resampling, and a
source player on the device:

```cpp
class FilePlayer : public yup::ChangeListener
{
public:
    FilePlayer()
    {
        formatManager.registerDefaultFormats();
        readAheadThread.startThread();

        deviceManager.initialiseWithDefaultDevices (0, 2);
        deviceManager.addAudioCallback (&sourcePlayer);
        sourcePlayer.setSource (&transportSource);
        transportSource.addChangeListener (this);
    }

    ~FilePlayer() override
    {
        transportSource.removeChangeListener (this);
        transportSource.stop();
        transportSource.setSource (nullptr);
        sourcePlayer.setSource (nullptr);
        deviceManager.removeAudioCallback (&sourcePlayer);
        readAheadThread.stopThread (1000);
    }

    bool load (const yup::File& file)
    {
        auto reader = formatManager.createReaderFor (file);
        if (reader == nullptr)
            return false;

        const auto fileSampleRate = reader->sampleRate;
        const auto numChannels = reader->numChannels;
        auto newSource = std::make_unique<yup::AudioFormatReaderSource> (std::move (reader));

        // Swap the transport to the new source before destroying the old one
        transportSource.setSource (newSource.get(), 32768, &readAheadThread, fileSampleRate, numChannels);
        readerSource = std::move (newSource);
        return true;
    }

    void play() { transportSource.start(); }
    void stop() { transportSource.stop(); }

private:
    void changeListenerCallback (yup::ChangeBroadcaster*) override
    {
        // Message thread: update the UI with transportSource.isPlaying()
    }

    yup::AudioFormatManager formatManager;
    yup::TimeSliceThread readAheadThread { "Audio file read-ahead" };
    yup::AudioDeviceManager deviceManager;
    yup::AudioSourcePlayer sourcePlayer;
    yup::AudioTransportSource transportSource;
    std::unique_ptr<yup::AudioFormatReaderSource> readerSource;
};
```

Call `readerSource->setLooping (true)` to loop the file. For short sounds,
read the whole file into an `AudioBuffer` and play it with a
`MemoryAudioSource` instead, without read-ahead.

## Choosing a source

| Class | Positionable | Use it to |
| --- | --- | --- |
| `AudioFormatReaderSource` | yes | Play an `AudioFormatReader` (an audio file). Lives in `yup_audio_formats`. |
| `MemoryAudioSource` | yes | Play an `AudioBuffer` already in memory, optionally looping. |
| `BufferingAudioSource` | yes | Read another positionable source ahead on a background thread. |
| `AudioTransportSource` | yes | Start, stop, seek and resample a positionable source. |
| `ToneGeneratorAudioSource` | no | Generate a sine wave for tests. |
| `MixerAudioSource` | no | Sum several sources. |
| `ResamplingAudioSource` | no | Change the playback speed and pitch, or convert sample rates. |
| `ChannelRemappingAudioSource` | no | Route and reorder channels. |
| `IIRFilterAudioSource` | no | Apply a simple IIR filter. |
| `ReverbAudioSource` | no | Apply the basic `Reverb`. |

Wrapping sources take a pointer to their input and a flag that says whether
they delete it. Pass `true` to hand over ownership, `false` when you keep the
input alive yourself.

### MemoryAudioSource

```cpp
yup::MemoryAudioSource memorySource (sampleBuffer, // AudioBuffer<float>&
                                     false,        // copyMemory: false references the buffer
                                     true);        // loop
```

With `copyMemory` false the buffer is referenced, so it must outlive the
source and must not be resized while it plays.

### ToneGeneratorAudioSource

Writes the same sine to every channel. `setFrequency (double)` defaults to
1000 Hz and `setAmplitude (float)` to 0.5.

### MixerAudioSource

```cpp
yup::MixerAudioSource mixer;
mixer.addInputSource (&drums, false);
mixer.addInputSource (new yup::ToneGeneratorAudioSource(), true); // owned by the mixer

sourcePlayer.setSource (&mixer);
```

Inputs added while the mixer is playing are prepared automatically.
`removeInputSource` releases the input (and deletes it if owned) and
`removeAllInputs()` clears the list.

### ResamplingAudioSource

```cpp
yup::ResamplingAudioSource resampler (&input, false, 2);
resampler.setResamplingRatio (44100.0 / 48000.0); // source rate / output rate
```

The ratio is the number of input samples consumed per output sample: `1.0`
passes the input through, higher values speed it up, lower values slow it
down. It can be changed while playing. The interpolation is simple and cheap;
for higher quality conversion or time stretching see
[Resampling](../dsp/resampling.md).

### BufferingAudioSource

```cpp
yup::BufferingAudioSource buffered (readerSource,  // PositionableAudioSource*
                                    readAheadThread,
                                    false,         // deleteSourceWhenDeleted
                                    65536,         // samples to buffer
                                    2);            // channels
```

The `TimeSliceThread` must be running and must outlive the source. When
rendering offline, call `waitForNextAudioBlockReady (info, timeoutMs)` before
`getNextAudioBlock` so the read-ahead has caught up. `AudioTransportSource`
creates one of these for you when you pass a read-ahead size.

### ChannelRemappingAudioSource

```cpp
yup::ChannelRemappingAudioSource remapper (&stereoSource, false);
remapper.setNumberOfChannelsToProduce (2);
remapper.setInputChannelMapping (0, 1); // the source reads device channel 1 as its channel 0
remapper.setOutputChannelMapping (0, 1); // source channel 0 goes to output channel 1
remapper.setOutputChannelMapping (1, 0); // source channel 1 goes to output channel 0
```

Until you add mappings it produces silence. `createXml()` and
`restoreFromXml (xml)` save and restore the mapping.

### IIRFilterAudioSource and ReverbAudioSource

```cpp
yup::IIRFilterAudioSource filter (&input, false);
filter.setCoefficients (yup::IIRCoefficients::makeLowPass (sampleRate, 2000.0));

yup::ReverbAudioSource reverb (&filter, false);
auto parameters = reverb.getParameters();
parameters.roomSize = 0.8f;
reverb.setParameters (parameters);
reverb.setBypassed (false);
```

`filter.makeInactive()` turns the filter off. Both are convenience wrappers
around the basic processors; for a full set of filters and effects see the
[DSP](../dsp/index.md) and [Filters](../dsp/filters.md) pages.

## Threading summary

- Build chains, call `setSource`, `addInputSource` and the transport controls
  on the message thread.
- `getNextAudioBlock` runs on the audio thread. A custom source shares state
  with the UI through atomics or a short `AudioLockType` lock.
- Changing a wrapped source's input while playing is not safe unless the
  class says so: detach from the player first (`setSource (nullptr)`), or use
  the players and mixers, which lock for you.
- Never destroy a source that is still attached to a player, mixer or
  transport.

## Related pages

- [Audio devices](devices.md) - open a device and register callbacks
- [Audio files](audio-files.md) - readers, writers and supported formats
- [Web audio](web.md) - playback rules in the browser
- [Audio buffers](buffers.md) - `AudioBuffer` and channel layouts
- [Audio processors](processors.md) - the full processing model for effects and instruments
- [Resampling](../dsp/resampling.md) - high quality sample rate conversion and time stretching
- [Threads](../core/multithreading/threads.md) - `TimeSliceThread` for read-ahead
- [Audio](index.md) - overview of the audio modules
