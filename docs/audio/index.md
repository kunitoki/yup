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

## Related

- [Building audio plugins](../build-system/building-plugins.md)
