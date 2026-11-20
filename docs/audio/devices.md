# Audio and MIDI devices

`yup_audio_devices` connects your code to the sound card and to MIDI ports. An
`AudioDeviceManager` opens an audio device for the current platform and calls
your `AudioIODeviceCallback` on the audio thread for every block of samples;
`MidiInput` and `MidiOutput` give direct access to MIDI hardware and virtual
ports.

**Module:** `yup_audio_devices`

## Opening the default device

Create one `AudioDeviceManager` for the lifetime of the app, usually as a member
of the application or main component, and initialize it on the message thread:

```cpp
yup::AudioDeviceManager deviceManager;

// 0 inputs, 2 outputs, on the system's default device
const auto error = deviceManager.initialiseWithDefaultDevices (0, 2);

if (error.isNotEmpty())
    yup::Logger::writeToLog ("Audio device error: " + error);
```

Both `initialise` and `initialiseWithDefaultDevices` return an error string,
which is empty on success. The channel counts are the maximum number of input
and output channels the app wants.

To restore the user's previous choice, pass the XML saved by `createStateXml()`
to `initialise`:

```cpp
// At startup
std::unique_ptr<yup::XmlElement> savedState = loadSettings(); // may be null

deviceManager.initialise (2,                  // input channels
                          2,                  // output channels
                          savedState.get(),   // previous state, or nullptr
                          true);              // fall back to the default device if it fails

// When the settings change, or at shutdown
if (auto state = deviceManager.createStateXml())
    saveSettings (*state);
```

`createStateXml()` returns null when the user never changed anything from the
defaults, so check before saving. `initialise` also accepts a preferred default
device name and a preferred `AudioDeviceSetup` as its last two arguments.

## Changing the device setup

`AudioDeviceSetup` describes the device, sample rate, buffer size and active
channels. Read the current one, change what you need, and apply it:

```cpp
auto setup = deviceManager.getAudioDeviceSetup();
setup.sampleRate = 48000.0;
setup.bufferSize = 256;

const auto error = deviceManager.setAudioDeviceSetup (setup, true);
```

| Field | Meaning |
| --- | --- |
| `outputDeviceName`, `inputDeviceName` | Device names. An empty name means no device. |
| `sampleRate` | Requested rate, `0` lets the device choose. |
| `bufferSize` | Requested block size in samples, `0` lets the device choose. |
| `inputChannels`, `outputChannels` | `BigInteger` bit masks of the active channels. |
| `useDefaultInputChannels`, `useDefaultOutputChannels` | When true (the default), the channel masks are ignored and the device's default channels are used. |

The second argument of `setAudioDeviceSetup` marks the setup as chosen by the
user, so that it is stored by `createStateXml()`. Check the device's supported
values before requesting them:

```cpp
if (auto* device = deviceManager.getCurrentAudioDevice())
{
    const auto rates = device->getAvailableSampleRates();     // Array<double>
    const auto sizes = device->getAvailableBufferSizes();     // Array<int>
    const auto outputs = device->getOutputChannelNames();     // StringArray
    const auto latency = device->getOutputLatencyInSamples();
}
```

Use `setCurrentAudioDeviceType` to switch backends (for example from "Windows
Audio" to "ASIO"), `closeAudioDevice()` to close the device and
`restartLastAudioDevice()` to reopen it.

## Writing an audio callback

Derive from `AudioIODeviceCallback` and register it with `addAudioCallback`:

```cpp
class SineOutput : public yup::AudioIODeviceCallback
{
public:
    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        // Allocate and prepare everything here, not in the callback
        sampleRate = device->getCurrentSampleRate();
        phase = 0.0;
    }

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext& context) override
    {
        const auto increment = yup::MathConstants<double>::twoPi * 440.0 / sampleRate;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto sample = static_cast<float> (0.2 * std::sin (phase));
            phase += increment;

            for (int ch = 0; ch < numOutputChannels; ++ch)
                if (outputChannelData[ch] != nullptr)
                    outputChannelData[ch][i] = sample;
        }
    }

    void audioDeviceStopped() override
    {
        // Release what audioDeviceAboutToStart allocated
    }

    void audioDeviceError (const yup::String& errorMessage) override
    {
        // May be called from any thread
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
};

SineOutput sine;
deviceManager.addAudioCallback (&sine);
// ...
deviceManager.removeAudioCallback (&sine);
```

The rules of the callback:

- `audioDeviceIOCallbackWithContext` runs on the audio thread. It must fill
  every output channel: the buffers start with undefined contents.
- `numSamples` usually equals the device buffer size, but it can change from
  one call to the next (Android splits blocks on some devices), so cope with
  reasonable variations.
- `context.hostTimeNs` points to the host time of the block in nanoseconds, or
  is null when the backend doesn't provide it.
- `addAudioCallback` calls `audioDeviceAboutToStart` if a device is already
  running, and `removeAudioCallback` calls `audioDeviceStopped`. Both are also
  called whenever the device restarts, for example after a sample rate change.
  The `AudioIODevice` pointer stays valid until `audioDeviceStopped`.
- With several callbacks registered, each receives the same input and their
  outputs are summed.
- `audioDeviceError` is optional, and not every backend calls it.

Follow the realtime rules of [the audio thread](../core/multithreading/index.md#the-audio-thread):
no allocation, no blocking locks, no file or message-thread calls. To run an
`AudioSource` or an `AudioProcessor` instead of a raw callback, see
[Audio sources](sources.md) and [Audio processors](processors.md).

## The callback lock

`getAudioCallbackLock()` is held by the device manager while it calls the
registered callbacks. Take it briefly on the message thread to swap state that
the callback reads:

```cpp
auto newVoice = std::make_unique<Voice>(); // allocate outside the lock

{
    const yup::AudioLockType::ScopedLockType lock (deviceManager.getAudioCallbackLock());
    std::swap (currentVoice, newVoice);
}
// newVoice (the old one) is destroyed here, outside the lock
```

`AudioLockType` is a `CriticalSection` on native platforms and a
`RecursiveSpinLock` on WebAssembly, where the audio thread must not block (see
[Web audio](web.md)). Keep the locked region short and allocation-free.
`getMidiCallbackLock()` is the equivalent lock for the MIDI input callbacks.

## Load, xruns and levels

| Call | Returns |
| --- | --- |
| `getCpuUsage()` | Proportion of the available time spent in the callbacks, from 0 to 1. Not measured on Emscripten. |
| `getXRunCount()` | Number of under- and overruns since the device started, or -1 when the device can't report them. |
| `getInputLevelGetter()`, `getOutputLevelGetter()` | A `LevelMeter::Ptr`; `getCurrentLevel()` returns a decaying peak level. |

Level metering only runs while something holds the `LevelMeter::Ptr`, so keep
it as a member and poll it from a `Timer`:

```cpp
yup::AudioDeviceManager::LevelMeter::Ptr outputLevel = deviceManager.getOutputLevelGetter();

void timerCallback() override
{
    meterValue = outputLevel->getCurrentLevel();
    repaint();
}
```

`playTestSound()` plays a short beep through the current device, useful in a
settings panel.

## Change notifications

`AudioDeviceManager` is a `ChangeBroadcaster`. It sends a change message when
the device starts or stops, when the setup or device type changes, when the
device list changes and when a MIDI input is enabled or disabled. Listeners
are called on the message thread, which makes this the right place to update
a settings UI or save the state:

```cpp
class App : public yup::ChangeListener
{
    void changeListenerCallback (yup::ChangeBroadcaster*) override
    {
        if (auto state = deviceManager.createStateXml())
            saveSettings (*state);
    }
};

deviceManager.addChangeListener (&app);
```

## Enumerating devices

Each backend is an `AudioIODeviceType`. `getAvailableDeviceTypes()` returns all
the types compiled into the app, already scanned:

```cpp
for (auto* type : deviceManager.getAvailableDeviceTypes())
{
    yup::Logger::writeToLog (type->getTypeName());

    for (const auto& name : type->getDeviceNames (false))   // outputs
        yup::Logger::writeToLog ("  out: " + name);

    if (type->hasSeparateInputsAndOutputs())
        for (const auto& name : type->getDeviceNames (true)) // inputs
            yup::Logger::writeToLog ("  in: " + name);
}
```

To be told when devices are plugged in or removed, add an
`AudioIODeviceType::Listener` (its `audioDeviceListChanged()` is called) to a
type, or simply listen to the device manager's change messages. A type created
by hand must call `scanForDevices()` before `getDeviceNames()`.

## Backends per platform

The device manager creates every backend available on the platform. The first
type that finds a device becomes the current one.

| Platform | Type names | Config flag (default) |
| --- | --- | --- |
| Windows | "Windows Audio", "Windows Audio (Exclusive Mode)", "Windows Audio (Low Latency Mode)" (WASAPI; low latency mode needs Windows 10) | `YUP_WASAPI` (1) |
| Windows | "DirectSound" | `YUP_DIRECTSOUND` (1) |
| Windows | "ASIO" (needs the Steinberg ASIO SDK) | `YUP_ASIO` (0) |
| macOS | "CoreAudio" | always on |
| iOS | "iOS Audio" | always on |
| Linux, BSD | "ALSA" (needs the ALSA development package) | `YUP_ALSA` (1) |
| Linux, macOS, Windows, BSD | "JACK" | `YUP_JACK` (0) |
| Linux (Bela boards) | "Bela" | `YUP_BELA` (0) |
| Android | "Android Oboe" | `YUP_USE_ANDROID_OBOE` (1) |
| Android | "Android OpenSL" | `YUP_USE_ANDROID_OPENSLES` (1 only when Oboe is off) |
| Android | "Android Audio" (Java fallback, when neither Oboe nor OpenSL is enabled) | - |
| Emscripten | "AudioWorklet" | always on, see [Web audio](web.md) |

Other options of the module:

| Flag | Default | Effect |
| --- | --- | --- |
| `YUP_USE_ANDROID_OBOE_STABILIZED_CALLBACK` | 0 | Uses Oboe's stabilized callback, which keeps the CPU busy to avoid frequency scaling. |
| `YUP_DISABLE_AUDIO_MIXING_WITH_OTHER_APPS` | 0 | On iOS, stops the app's audio from mixing with other apps. |
| `YUP_USE_WINRT_MIDI` | 0 | Uses Windows Runtime MIDI on Windows 10 1809 and later, which adds Bluetooth LE MIDI devices. |
| `YUP_ENABLE_CORE_AUDIO_LOGGING` | 0 | Logs CoreAudio activity on macOS. |

Set the flags as compile definitions of your target. You can also add a custom
backend with `addAudioDeviceType (std::make_unique<MyDeviceType>())`, or
override `createAudioDeviceTypes` in a subclass to control the list.

## Microphone permission

On Android, opening a device with input channels fails unless the app was
granted the record permission. Request it first with
[`RuntimePermissions`](../core/system-and-app.md#runtimepermissions), then
initialize the device with inputs:

```cpp
yup::RuntimePermissions::request (yup::RuntimePermissions::recordAudio, [this] (bool granted)
{
    deviceManager.initialiseWithDefaultDevices (granted ? 2 : 0, 2);
});
```

`RuntimePermissions` only asks at runtime on Android; on every other platform
it calls back immediately with `true`, so the same code is portable. On iOS
and macOS the system asks the user the first time the input is opened,
and Apple requires an `NSMicrophoneUsageDescription` entry in the app's
`Info.plist` (see the `CUSTOM_PLIST` option in the
[CMake API](../build-system/cmake-api.md)). In the browser, see
[Web audio](web.md).

## MIDI inputs

`MidiInput::getAvailableDevices()` lists the ports as `MidiDeviceInfo` values
(`name`, `identifier`, `protocol`, `supportsMidi2`). Open one by identifier,
with a `MidiInputCallback`, and start it:

```cpp
class MidiLogger : public yup::MidiInputCallback
{
public:
    void handleIncomingMidiMessage (yup::MidiInput* source, const yup::MidiMessage& message) override
    {
        // Called on a high-priority MIDI thread, not the message thread
    }
};

MidiLogger logger;
std::unique_ptr<yup::MidiInput> input;

for (const auto& info : yup::MidiInput::getAvailableDevices())
    yup::Logger::writeToLog (info.name + " (" + info.identifier + ")");

input = yup::MidiInput::openDevice (yup::MidiInput::getDefaultDevice().identifier, &logger);

if (input != nullptr)
    input->start();
```

`openDevice` returns null on failure. Messages are delivered until `stop()` is
called or the object is destroyed. Their timestamp is in seconds, on the
`Time::getMillisecondCounter()` clock. Long SysEx messages can also be received
in pieces through `handlePartialSysexMessage`.

The overload `openDevice (identifier, protocol, receiver)` delivers Universal
MIDI Packets to a `ump::Receiver` instead, see [MIDI 2.0 and UMP](ump.md).

On Linux, BSD, Apple platforms and Emscripten, `MidiInput::createNewDevice (name, callback)`
creates a virtual port that other apps can connect to (on iOS this needs the
background audio capability; the browser can't create virtual ports, so it
returns null there).

## MIDI outputs

```cpp
auto output = yup::MidiOutput::openDevice (yup::MidiOutput::getDefaultDevice().identifier);

if (output != nullptr)
{
    output->sendMessageNow (yup::MidiMessage::noteOn (1, 60, 0.8f));
    output->sendMessageNow (yup::MidiMessage::noteOff (1, 60));
}
```

| Call | Use |
| --- | --- |
| `sendMessageNow (message)` | Sends immediately. Also accepts a `ump::View` or `ump::Packets`. |
| `sendBlockOfMessagesNow (buffer)` | Sends every event of a `MidiBuffer` immediately. |
| `sendBlockOfMessages (buffer, millisecondCounterToStartAt, samplesPerSecondForBuffer)` | Schedules a `MidiBuffer` at a future time on `Time::getMillisecondCounter()`, converting sample positions to milliseconds. Needs `startBackgroundThread()` first. |
| `clearAllPendingMessages()` | Drops scheduled messages. |

`MidiOutput::createNewDevice (name)` creates a virtual output on the same
platforms as virtual inputs.

## MIDI through the device manager

Instead of opening inputs yourself, let the device manager own them. Its state
XML then remembers which inputs the user enabled:

```cpp
if (auto defaultInput = yup::MidiInput::getDefaultDevice(); defaultInput != yup::MidiDeviceInfo())
    deviceManager.setMidiInputDeviceEnabled (defaultInput.identifier, true);

// An empty identifier receives messages from every enabled input
deviceManager.addMidiInputDeviceCallback ({}, &midiCollector);

// At shutdown
deviceManager.removeMidiInputDeviceCallback ({}, &midiCollector);
```

`setDefaultMidiOutputDevice (identifier)` opens an output that is shared
through `getDefaultMidiOutput()` and also saved in the state.

## Feeding MIDI to the audio thread

MIDI arrives on its own thread, while the audio callback wants a `MidiBuffer`
with sample positions. `MidiMessageCollector` bridges the two: it is a
`MidiInputCallback`, queues the incoming messages, and hands them out block by
block:

```cpp
class Synth : public yup::AudioIODeviceCallback
{
public:
    yup::MidiMessageCollector midiCollector;

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        midiCollector.reset (device->getCurrentSampleRate());
        midiBuffer.ensureSize (4096);
    }

    void audioDeviceIOCallbackWithContext (const float* const*, int,
                                           float* const* outputChannelData, int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext&) override
    {
        midiBuffer.clear();
        midiCollector.removeNextBlockOfMessages (midiBuffer, numSamples);

        // Render audio, consuming the events in midiBuffer
    }

    void audioDeviceStopped() override {}

private:
    yup::MidiBuffer midiBuffer;
};
```

- Call `reset (sampleRate)` before the first block; the collector asserts
  otherwise.
- `removeNextBlockOfMessages` adds to the buffer without clearing it.
- To inject messages from your own code, call `addMessageToQueue` with a
  timestamp in seconds, such as `yup::Time::getMillisecondCounterHiRes() * 0.001`.
- The collector is also a `MidiKeyboardState::Listener`, so
  `keyboardState.addListener (&midiCollector)` routes an on-screen keyboard
  into the same stream (see [Audio GUI](audio-gui.md)).
- `ensureStorageAllocated (bytes)` preallocates the queue to avoid allocations.

`UMPPacketCollector` does the same for Universal MIDI Packets, filling a
`UMPPacketBuffer`; see [MIDI 2.0 and UMP](ump.md).

## MIDI device disconnection

`MidiInput` and `MidiOutput` report when the device behind them goes away, for
example when a USB controller is unplugged. Add a `ump::DisconnectionListener`
with `addDisconnectionListener`; its `disconnected()` is called once, on the
message thread, and `isDisconnected()` returns true from then on:

```cpp
class InputWatcher : public yup::ump::DisconnectionListener
{
public:
    void disconnected() override
    {
        // Mark the input as gone; reopen it later. Don't destroy it here.
        needsReopen = true;
    }

    bool needsReopen = false;
};

InputWatcher watcher;
input->addDisconnectionListener (watcher);
```

A disconnected device never delivers or sends again, so open a new one when
the device returns. Don't destroy the device from inside `disconnected()`.

Disconnections are reported by CoreMIDI (macOS, iOS), Windows, ALSA (Linux) and
Web MIDI. On Windows they arrive with the system's device-change notification,
about half a second after the device goes away. Android doesn't report them
yet, so listeners are never called there.

`AudioDeviceManager` uses this for its enabled MIDI inputs: an input whose
device disappears is closed, and it is opened again when the device comes back.

## Watching the MIDI device list

`MidiDeviceListConnection::make` registers a function that is called on the
message thread whenever MIDI devices are added or removed. The callback stays
registered for as long as the returned object lives:

```cpp
yup::MidiDeviceListConnection midiDevicesConnection = yup::MidiDeviceListConnection::make ([this]
{
    refreshMidiDeviceMenu();
});
```

Call `reset()` on it, or destroy it, to stop listening.

## System volume

`SystemAudioVolume` reads and sets the volume of the system's default output:

```cpp
const auto gain = yup::SystemAudioVolume::getGain();   // 0 to 1
yup::SystemAudioVolume::setGain (0.5f);                // returns false on failure
yup::SystemAudioVolume::setMuted (! yup::SystemAudioVolume::isMuted());
```

It is implemented on macOS and Windows only. Elsewhere it asserts in debug
builds and returns `0` or `false`.

## Related pages

- [Audio sources](sources.md) - play `AudioSource` chains on a device with `AudioSourcePlayer`
- [Audio files](audio-files.md) - read and write audio files
- [Web audio](web.md) - the AudioWorklet device, Web MIDI and browser rules
- [MIDI](midi.md) - `MidiMessage`, `MidiBuffer` and keyboard state
- [MIDI 2.0 and UMP](ump.md) - Universal MIDI Packets and conversions
- [Audio processors](processors.md) - processing audio with `AudioProcessor`
- [The audio thread](../core/multithreading/index.md#the-audio-thread) - realtime rules for audio callbacks
- [Audio](index.md) - overview of the audio modules
