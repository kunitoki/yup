# Headless audio

The audio engine of YUP does not need a window, a GPU or the `yup_gui` module. A
console application can open audio devices, run processors and an
[audio graph](audio-graph.md), load and save state and host third-party plugins.
This page lists what to link and what to expect at runtime.

## Modules

None of these modules depends on `yup_gui` or `yup_graphics`:

| Module | What it gives a headless engine |
|---|---|
| `yup_audio_basics` | Buffers, MIDI, sources and synthesizers |
| `yup_audio_devices` | Audio and MIDI devices |
| `yup_audio_formats` | Reading and writing audio files |
| `yup_audio_processors` | `AudioProcessor`, parameters and state |
| `yup_audio_graph` | Routing processors in a graph |
| `yup_audio_plugin_host` | Hosting VST3, CLAP, LV2 and Audio Unit plugins |
| `yup_dsp`, `yup_dsp_jit` | DSP building blocks and YDSP |

`yup_audio_gui` and `yup_audio_plugin_client` always need `yup_gui`.

## A console target

Pass `TARGET_CONSOLE ON` to `yup_standalone_app`: the target is built without the
windowing backend and, on macOS, without an application bundle. The
[console example](https://github.com/kunitoki/yup/tree/main/examples/console) is
the smallest such target.

```cmake
yup_standalone_app (
    TARGET_NAME my_engine
    TARGET_CONSOLE ON
    DEFINITIONS
        YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP=1
        YUP_AUDIO_PLUGIN_HOST_ENABLE_LV2=1
    MODULES
        yup::yup_audio_devices
        yup::yup_audio_formats
        yup::yup_audio_processors
        yup::yup_audio_graph
        yup::yup_audio_plugin_host)
```

Leave `yup::yup_gui` out of `MODULES`. `yup_audio_processors` and
`yup_audio_plugin_host` only use it as an
[optional dependency](../modules.md#required-vs-optional-dependencies): their
editor code is compiled when the module is linked and skipped otherwise.

## Choosing plugin formats

Every hosted format is off until you enable it, and its SDK is only fetched when
you do. Enable what the target platform offers and nothing else:

| Flag | Platforms |
|---|---|
| `YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3` | macOS, Windows, Linux |
| `YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP` | macOS, Windows, Linux |
| `YUP_AUDIO_PLUGIN_HOST_ENABLE_LV2` | macOS, Windows, Linux |
| `YUP_AUDIO_PLUGIN_HOST_ENABLE_AU` | macOS |

On an embedded ARM64 Linux device, for example, a host enabling only CLAP and
LV2 fetches and compiles neither the VST3 SDK nor anything related to Audio
Units. See [Plugin hosting](plugin-hosting.md#formats-and-platforms).

Plugins you build yourself pick their formats in the same way, with the
`PLUGIN_CREATE_CLAP`, `PLUGIN_CREATE_VST3`, `PLUGIN_CREATE_LV2` and related
options of `yup_audio_plugin` (see
[Building plugins](../build-system/building-plugins.md)). Those targets always
link `yup_gui`.

## Runtime

Processors, parameters, state and the audio graph need no initialization: they
never touch the message thread, timers, fonts or graphics. You can construct an
`AudioProcessor`, prepare it and call `processBlock()` from a plain `main()`, for
example to render a file offline.

```cpp
int main()
{
    MyProcessor processor;
    processor.prepareToPlay (yup::AudioSpec (48000.0f, 512));

    yup::AudioBuffer<float> buffer (2, 512);
    yup::MidiBuffer midi;
    yup::ParameterChangeBuffer parameterChanges;

    for (int block = 0; block < numBlocks; ++block)
    {
        readInput (buffer);

        yup::AudioProcessContext<float> context { buffer, midi, parameterChanges };
        processor.processBlock (context);

        writeOutput (buffer);
    }

    processor.releaseResources();
    return 0;
}
```

A message loop is needed as soon as something has to call you back on the
message thread:

| Feature | Needs a message loop |
|---|---|
| `AudioProcessor`, parameters, state, `AudioGraphProcessor` | No |
| Audio files | No |
| `AudioDeviceManager` | Yes, for device change notifications |
| Hosted plugins | Yes: plugins are loaded on the message thread and serviced from its timers |
| Out-of-process scanning and `RemoteAudioPluginInstance` | Yes |

Create the message manager with `ScopedYupInitialiser_GUI`, which despite its
name lives in `yup_events` and opens no window, then run the loop:

```cpp
int main()
{
    const yup::ScopedYupInitialiser_GUI initialiser;

    Engine engine; // devices, graph, hosted plugins

    yup::MessageManager::getInstance()->runDispatchLoop(); // until stopDispatchLoop()
    return 0;
}
```

## Without `yup_gui`

- `AudioProcessorEditor`, `AudioParameterAttachment` and the `yup_audio_gui`
  components are not available.
- Hosted plugin instances return `false` from `hasEditor()` and `nullptr` from
  `createEditor()`.
- `RemoteAudioPluginInstance` has no `showEditor()`, `hideEditor()`,
  `isEditorShowing()` and `onEditorClosed`, and an `AudioPluginBridgeServer`
  built without `yup_gui` reports no editor to its host.
- CLAP plugins are not offered the `gui` host extension.

Processing, parameters, state, presets, latency and tail work the same in both
builds.

## Linux packages

`yup_audio_devices` needs the ALSA development package at configure time
(`libasound2-dev` on Debian and Ubuntu). X11 is only linked by `yup_gui`, so a
headless build does not need it or a running display server.

## Related pages

- [Audio processors](processors.md) - the processor model and its realtime rules.
- [Audio graph](audio-graph.md) - routing processors without a UI.
- [Plugin hosting](plugin-hosting.md) - scanning and loading plugins.
- [Audio and MIDI devices](devices.md) - opening devices from a console application.
- [Standalone applications](../build-system/building-standalone.md) - the CMake target options.
