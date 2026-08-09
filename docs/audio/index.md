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

## Related

- [Building audio plugins](../build-system/building-plugins.md)
