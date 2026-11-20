# Web audio

YUP apps built with Emscripten play audio through a Web Audio
`AudioWorklet` and talk to MIDI devices through the Web MIDI API. The same
`AudioDeviceManager`, callbacks and MIDI classes work in the browser, but the
browser adds rules of its own: audio starts only after a user gesture, the
block size is fixed, permissions are asynchronous, and the audio thread must
never block. This page covers what changes compared to
[native devices](devices.md).

**Module:** `yup_audio_devices`

## Build requirements

The audio worklet runs on its own thread, so audio needs a threaded build:

- `YUP_ENABLE_EMSCRIPTEN_THREADS` must be ON (the default). A target that
  uses `yup_audio_devices` without threads is refused at configure time.
- Threaded builds link with `-pthread`, `-sWASM_WORKERS=1`,
  `-sAUDIO_WORKLET=1` and `-sSHARED_MEMORY=1`. The pthread pool size comes
  from the `PTHREAD_POOL_SIZE` argument of `yup_standalone_app` (8 by
  default).
- Shared memory needs a cross-origin isolated page. The build copies
  `mini-coi.js` next to the generated page, and the default shell loads it to
  provide the isolation headers through a service worker. A custom shell must
  load it too, or the server must send the `Cross-Origin-Opener-Policy` and
  `Cross-Origin-Embedder-Policy` headers itself.

See the [CMake API](../build-system/cmake-api.md) for the options and
[Building standalone applications](../build-system/building-standalone.md) for
the build itself.

## The AudioWorklet device

On Emscripten the device manager has a single device type, "AudioWorklet",
with a single device, "Audio Worklet". Open it like any other device:

```cpp
deviceManager.initialiseWithDefaultDevices (0, 2);
deviceManager.addAudioCallback (&player);
```

| Property | Value |
| --- | --- |
| Sample rate | The browser's `AudioContext` rate (usually 44100 or 48000 Hz), the only one available |
| Buffer size | Always 128 samples |
| Output channels | Up to the destination's `maxChannelCount`, at most 32 |
| Input channels | 2, from the microphone |
| Latency | Reported as 0 |
| CPU usage | Not measured, `getCpuUsage()` returns 0 |
| Xruns | Estimated from the callback timing |

Requesting any other sample rate or buffer size in an `AudioDeviceSetup`
fails, so leave them at 0 (device default), or read them from
`getAvailableSampleRates()` and `getAvailableBufferSizes()`. Code that
assumes a particular rate must read it in `audioDeviceAboutToStart`.

## Web Audio activation

Browser autoplay policy keeps the audio context suspended until the user
interacts with the page. The device resumes it on the first click on the
`canvas` element:

- Before that click, the device is open but silent. `audioDeviceAboutToStart`
  of your callbacks is called on the click, not when you call
  `addAudioCallback`, so don't rely on it having been called at startup.
- After the first activation, callbacks added later start immediately.

The default Emscripten shell shows a "Click the canvas to enable audio" hint
over the canvas after startup, and its top-right `audio` and `midi` indicators
show the browser's permission and runtime state. This activation is provided
by the shell and the device backend, so apps don't need their own
"enable audio" button.

```{note}
The shell reads `Module.yupAudioContext` (the current `AudioContext`, set
while the device is open), `Module.yupMidiAccess` and
`Module.yupMidiRequestPending` to drive its indicators. A custom shell given
with `CUSTOM_SHELL` can use the same properties.
```

## Microphone input

Opening the device with input channels asks the browser for microphone access
and feeds the microphone to those inputs, with echo cancellation, noise
suppression and automatic gain turned off:

```cpp
deviceManager.initialiseWithDefaultDevices (2, 2); // prompts for the microphone
```

- Access needs a secure context (HTTPS or `localhost`).
- Until the user allows it, or if they deny it, the inputs are silent; the
  device still opens and runs.
- A mono microphone feeds both channels of a stereo input.

There is no separate permission call: `RuntimePermissions` reports every
permission as granted on the web, and the browser's own prompt is the
permission.

## Audio thread rules on the web

The audio callback runs on the browser's audio worklet, which cannot block.
Whenever the audio callback has to wait - a `CriticalSection` or `std::mutex`
held by another thread, or a `malloc` / `free` racing one on another thread -
the page aborts with
`Assertion failed: emscripten_is_main_browser_thread()`. It only happens under
contention, so it shows up as an occasional crash, not a reproducible one.

In the audio callback:

- Share state with the UI through atomics, or guard it with `AudioLockType`,
  which is a `RecursiveSpinLock` on WebAssembly - for example the device
  manager's `getAudioCallbackLock()`:

  ```cpp
  {
      const yup::AudioLockType::ScopedLockType lock (deviceManager.getAudioCallbackLock());
      synth = std::move (newSynth);
  }
  ```

- Size every buffer in `audioDeviceAboutToStart`, never in the callback.
- Don't post messages (`triggerAsyncUpdate`, `callAsync`, change broadcasts),
  as queuing them locks and allocates. Set a flag or an atomic and poll it
  from a `Timer` on the UI side.

These are the general [audio thread](../core/multithreading/index.md#the-audio-thread)
rules, but on the web breaking them crashes the page instead of causing a
glitch. Some library classes are affected:

| Class | Caveat in the browser |
| --- | --- |
| `AudioTransportSource` | Sends a change message from the audio thread when a stream ends. Don't add `ChangeListener`s to it; poll `isPlaying()` or `hasStreamFinished()` from a `Timer`. |
| `AudioFormatWriter::ThreadedWriter` | Its FIFO uses a `CriticalSection`; avoid writing to it from the audio callback. |
| `UMPPacketCollector` | Uses a `CriticalSection`; prefer `MidiMessageCollector`, which uses `AudioLockType`. |
| `AudioSourcePlayer`, `MixerAudioSource`, `MidiMessageCollector` | Use `AudioLockType`, safe to use. |

Keep the locked regions on the UI side short: the audio thread spins while
the UI holds the lock, and a block that is not ready in time is a dropout.

## Web MIDI

On Emscripten builds `MidiInput` and `MidiOutput` are backed by the browser's
[Web MIDI API](https://www.w3.org/TR/webmidi/). A few behaviors differ from
desktop platforms:

- **Permission is asynchronous.** `requestMIDIAccess()` is a browser prompt,
  so call `MidiInput::getAvailableDevices()` (or
  `MidiOutput::getAvailableDevices()`) once early in startup to trigger it.
  Until the user grants access, device lists are empty and `openDevice()`
  returns null. Once granted, the lists populate and `MidiDeviceListConnection`
  callbacks fire, so build your device menus from there:

  ```cpp
  midiDevicesConnection = yup::MidiDeviceListConnection::make ([this]
  {
      refreshMidiDeviceMenu(); // the lists are now populated
  });

  yup::MidiInput::getAvailableDevices(); // triggers the permission prompt
  ```

- **SysEx is enabled.** The backend requests `{ sysex: true }`, so incoming
  and outgoing system exclusive messages are not masked by the browser (the
  prompt is the slightly stronger "MIDI + SysEx" permission).
- **Devices are MIDI 1.0.** Web MIDI exposes MIDI 1.0 byte streams only.
  Ports are reported with `ump::PacketProtocol::MIDI_1_0`. Opening an input
  with the `MIDI_2_0` protocol and a `ump::Receiver` converts the stream to UMP
  on the way in, and `ump::View` / `ump::Packets` sent to an output are
  converted to MIDI 1.0 bytes on the way out. Opening an output with any
  protocol other than `MIDI_1_0` returns null.
- **No virtual ports.** `createNewDevice()` is unsupported (the Web MIDI API
  cannot create virtual ports) and returns null.
- **Defaults.** The default input and output are the first available ones.
- **Threading.** Incoming messages arrive on the browser main thread.
  Sending (`sendMessageNow`) is thread-safe: calls from other threads are
  proxied to the main thread synchronously, so don't send from the audio
  callback.
- **Disconnection.** Unplugged devices are reported to
  `ump::DisconnectionListener`s, as on desktop (see
  [Audio devices](devices.md)).
- **Environment.** Web MIDI requires a secure context (HTTPS or `localhost`)
  and a browser that supports the API. On WASM targets other than Emscripten,
  MIDI is not implemented and all queries return empty results.

## Related pages

- [Audio devices](devices.md) - the device manager, callbacks and MIDI devices on every platform
- [Audio sources](sources.md) - playback chains and `AudioTransportSource`
- [The audio thread](../core/multithreading/index.md#the-audio-thread) - realtime rules for audio code
- [The message thread](../core/multithreading/message-thread.md) - timers for polling audio state
- [WebGPU](../graphics/platforms/webgpu.md) - graphics in the browser
- [Writing plugins](plugin-client.md) - running a plugin as a standalone web app
- [Audio](index.md) - overview of the audio modules
