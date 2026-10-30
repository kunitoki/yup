# Audio

The audio-first stack: device I/O, MIDI, file formats, DSP, an audio graph,
processors, and CLAP / VST3 plugin hosting and client wrappers - all in one
framework.

**Modules covered:** `yup_audio_basics`, `yup_audio_devices`, `yup_audio_formats`,
`yup_audio_graph`, `yup_audio_processors`, `yup_dsp`, `yup_audio_plugin_client`,
`yup_audio_plugin_host`.

```{warning}
**Work in progress.** This area is still being written. Concept guides for the
audio graph, processors, and the plugin model are still to come. The DSP
building blocks now have their own dedicated
[DSP](../dsp/index.md) area.
```

## Topics

- **Audio basics** - buffers, sample formats, and the audio process load
  measurer (`yup_audio_basics`).
- **Devices & MIDI** - audio device management and MIDI I/O, including UMP
  (`yup_audio_devices`).
- **Formats** - reading and writing audio files (`yup_audio_formats`).
- **DSP** - filters, filter designers, crossovers, FFTs, and spectral
  analysis, documented in the dedicated [DSP](../dsp/index.md) area
  (`yup_dsp`).
- **Audio graph** - node-based audio processing and editing (`yup_audio_graph`).
- **Processors** - the `AudioProcessor` model (`yup_audio_processors`).
- **Plugins** - hosting third-party plugins (`yup_audio_plugin_host`) and
  wrapping your own as CLAP / VST3 (`yup_audio_plugin_client`).

## Microtuning

`TuningMap` (`yup_audio_basics`) maps MIDI notes to frequencies under any
tuning, ready to feed a synthesizer voice with `noteToPitch (note)`. It starts
as 12 tone equal temperament with A4 = 440 Hz, and reads the common tuning file
formats:

- **Scala** `.scl` scales with `loadScale()`, and `.kbm` key maps, which lay the
  scale out on the keys and set the reference pitch, with `loadKeyMap()`.
- **AnaMark** `.tun` / `.msf` tunings with `loadTuning()`. All format versions
  are read, including the version 2 `[Functional Tuning]` formulas and
  `[Mapping]`. A file holding several scales gives the one assigned to the MIDI
  channel passed in.

An AnaMark tuning sets every note's frequency and replaces the scale and key
map until the next `loadScale()` or `loadKeyMap()`. Each loader has a
`FromString` variant for tunings kept in memory, e.g. inside a preset. A load
that fails returns a failed `Result` and leaves the current tuning in place.

```cpp
yup::TuningMap tuning;

if (auto result = tuning.loadTuning (file); result.failed())
    showError (result.getErrorMessage());

const auto frequency = tuning.noteToPitch (60); // negative if the key is unmapped
```

Loading allocates, so load on the message thread and hand the audio thread a
copy of the frequencies rather than calling the loaders while it plays.

## MIDI device disconnection

`MidiInput` and `MidiOutput` report when the device behind them goes away, for
example when a USB controller is unplugged. Add a `ump::DisconnectionListener`
with `addDisconnectionListener`; its `disconnected()` is called once, on the
message thread, and `isDisconnected()` returns true from then on. A disconnected
device never delivers or sends again, so open a new one when the device returns.
Don't destroy the device from inside `disconnected()`.

Disconnections are reported by CoreMIDI (macOS, iOS), Windows, ALSA (Linux) and
Web MIDI. On Windows they arrive with the system's device-change notification,
about half a second after the device goes away. Android doesn't report them yet,
so listeners are never called there.

`AudioDeviceManager` uses this for its enabled MIDI inputs: an input whose device
disappears is closed, and it is opened again when the device comes back.

## Web MIDI (WASM)

On Emscripten builds, MIDI I/O is backed by the browser's
[Web MIDI API](https://www.w3.org/TR/webmidi/) (`yup_Midi_wasm.cpp`). A few
behaviors differ from desktop platforms:

- **Permission is asynchronous.** `requestMIDIAccess()` is a browser prompt, so
  call `MidiInput::getAvailableDevices()` (or `MidiOutput::getAvailableDevices()`)
  once early in startup to trigger it. Until the user grants access, device
  lists are empty and `openDevice()` returns an empty object; once granted, the
  lists populate and `MidiDeviceListConnection` listeners fire.
- **SysEx is enabled.** The backend requests `{ sysex: true }`, so incoming and
  outgoing system exclusive messages are not masked by the browser (the prompt
  is the slightly stronger "MIDI + SysEx" permission).
- **Devices are MIDI 1.0.** Web MIDI exposes MIDI 1.0 byte streams only. Ports
  are reported with `PacketProtocol::MIDI_1_0`; opening an input with a
  `MIDI_2_0` protocol and an `ump::Receiver` converts the stream to UMP on the
  way in, and `ump::View` / `ump::Packets` are converted to MIDI 1.0 bytes on
  the way out. `createNewDevice()` is unsupported (the Web MIDI API cannot
  create virtual ports) and returns an empty object.
- **Threading.** Incoming messages arrive on the browser main thread. Sending
  (`sendMessageNow`) is thread-safe: calls from other threads are proxied to the
  main thread synchronously.
- **Environment.** Web MIDI requires a secure context (HTTPS or `localhost`)
  and a browser that supports the API; on other WASM targets MIDI remains
  unimplemented and all queries return empty results.

## Web Audio activation (Emscripten)

Browser autoplay policy initially suspends the Web Audio context. Applications
using the default Emscripten standalone shell show a hint over the canvas after
startup; click the canvas once to resume the shared audio worklet. The shell's
top-right `audio` and `midi` indicators show the current browser permission and
runtime state. This activation is provided by the shell and audio-device
backend, so individual examples do not need their own browser-specific button.

## Microphone input (Emscripten)

Opening the audio worklet device with input channels asks the browser for
microphone access and feeds the microphone to those inputs, with echo
cancellation, noise suppression and automatic gain turned off. Access needs a
secure context (HTTPS or `localhost`). Until the user allows it, or if they
deny it, the inputs are silent. A mono microphone feeds both channels of a
stereo input.

## Audio thread rules on the web

The audio callback runs on the browser's audio worklet, which cannot block.
Whenever the audio callback has to wait - a `CriticalSection` or `std::mutex`
held by another thread, or a `malloc` / `free` racing one on another thread -
the page aborts with
`Assertion failed: emscripten_is_main_browser_thread()`. It only happens under
contention, so it shows up as an occasional crash. In the audio callback:

- Share state with the UI through atomics, or guard it with `AudioLockType`
  (a spin lock on the web) - for example the device manager's
  `getAudioCallbackLock()`.
- Size every buffer in `audioDeviceAboutToStart`, never in the callback.
- Don't post messages (`triggerAsyncUpdate`, `callAsync`, change broadcasts),
  as queuing them locks and allocates. Set a flag and poll it from the UI.

## Related

- [Building audio plugins](../build-system/building-plugins.md)
