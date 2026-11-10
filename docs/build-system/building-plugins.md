# Building Plugins with YUP

This guide explains how to create audio plugins using the YUP framework. YUP supports multiple plugin formats including CLAP, VST3, and more.

## Basic Plugin Structure

A basic YUP plugin consists of the following components:

1. A processor class that handles audio processing
2. An editor class for the plugin's user interface
3. Plugin format-specific wrapper code

Here's a minimal example of a plugin:

```cpp
#include <yup_audio_plugin_client/yup_audio_plugin_client.h>

class MyPluginProcessor : public yup::AudioProcessor
{
public:
    MyPluginProcessor()
    {
        // Add parameters
        addParameter (gainParameter = yup::AudioParameterBuilder{}
            .withID ("gain")
            .withName ("Gain")
            .withRange (0.0f, 1.0f)
            .withDefault (0.5f)
            .build());
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        // Initialize processing resources

        gain = yup::AudioParameterHandle (*gainParameter, sampleRate);
    }

    void releaseResources() override
    {
        // Clean up resources
    }

    void processBlock (yup::AudioBuffer<float>& audioBuffer, yup::MidiBuffer& midiMessages) override
    {
        if (gain.updateNextAudioBlock())
        {
            float currentGain = gain.getCurrentValue();
            float destinationGain = gain.skip (audioBuffer.getNumSamples());

            audioBuffer.applyGainRamp (0, audioBuffer.getNumSamples(), currentGain, destinationGain);
        }
        else
        {
            audioBuffer.applyGain (gain.getCurrentValue());
        }
    }

    bool hasEditor() const override { return true; }
    yup::AudioProcessorEditor* createEditor() override;

private:
    yup::AudioParameter::Ptr gainParameter;
    yup::AudioParameterHandle gain;
};

class MyPluginEditor : public yup::AudioProcessorEditor
{
public:
    MyPluginEditor (MyPluginProcessor& p)
        : AudioProcessorEditor (&p)
        , processor (p)
    {
        setSize (400, 300);
    }

    void paint (yup::Graphics& g) override
    {
        g.fillAll (findColour (yup::ResizableWindow::backgroundColourId));
    }

private:
    MyPluginProcessor& processor;
};

yup::AudioProcessorEditor* MyPluginProcessor::createEditor() override
{
    return new MyPluginEditor (*this);
}

// Plugin entry point
extern "C" yup::AudioProcessor* createPluginProcessor()
{
    return new MyPluginProcessor();
}
```

### Resizable editors

When `isResizable()` returns true, the CLAP, VST3 and AU wrappers place a
`yup::ResizableCorner` on top of the editor, in its bottom-right corner, so users can resize
it even in hosts without resizable plugin windows. The corner belongs to the wrapper, not to the
editor, so it is drawn outside any effect the editor applies to its own content. Override
`shouldShowResizableCorner()` to return false to hide it. Every size requested
by the host or the corner goes through `getConstrainedSize()`, which applies
`shouldPreserveAspectRatio()` and `getPreferredAspectRatioSize()`. An editor
calling `setSize()` on itself resizes the host window as well.

Wrappers open a new editor at `getInitialSize()`, which returns `getPreferredSize()` by
default. To let each plugin instance reopen at the size the user left it, remember the size
in the processor state (for example from the editor's `resized()`) and return it from
`getInitialSize()`. Keep `getPreferredSize()` as the design size the editor is laid out for.

Editor sizes are always in YUP points. On Windows and Linux, where VST3 and CLAP
hosts size plugin views in physical pixels, the wrappers convert them with the
editor window's display scale (or the host's suggested scale before the window
exists), so editors keep their layout on high density displays.

### Host integration

Processors can describe themselves to hosts through optional overrides and metadata:

- **Bypass**: build a parameter with `AudioParameterBuilder::withBypass()`. Wrappers expose it as the
  host bypass control (VST3 `kIsBypass`, CLAP `CLAP_PARAM_IS_BYPASS`, AU bypass property, AAX master
  bypass) instead of adding their own, and call `processBlockBypassed()` while it is on.
  `getBypassParameter()` returns it.
- **MIDI CC mapping**: override `getParameterIndexForMidiController (channel, controller)` to map
  controllers to parameter indices (return -1 for none).
- **Bus layouts**: override `isBusLayoutSupported()` to accept layouts other than the current one,
  `setBusLayout()` applies a supported layout and notifies listeners. VST3 and AU wrappers offer the
  accepted main bus channel counts to hosts and switch to the one the host picks.
- **Host details**: `getHostInfo()` returns the track name and color when the host provides them
  (VST3 and CLAP, and the track name from AUv3 `contextName`; AU v2, AAX and LV2 have no such API),
  `hostInfoChanged()` is called when they change. `AudioProcessor::getHostType()` identifies the host
  (`PluginHostType`) for host-specific workarounds.
- **Note expression**: return true from `supportsNoteExpression()` to receive per-note expressions in
  `AudioProcessContext::noteExpressions` (from CLAP and VST3 hosts, and from AUv3 hosts sending MIDI 2.0
  per-note pitch bend and controllers, which is requested automatically). Otherwise they are converted to
  MIDI, either channel-wide or as MPE when `supportsMpe()` returns true; only tuning (pitch bend),
  pressure and brightness (CC74) survive the conversion.
- **Speaker layouts**: give buses an explicit `AudioChannelSet` (`AudioBus (name, direction, channelSet)`),
  otherwise the canonical layout for the channel count is used. VST3 speaker arrangements, AU channel
  layouts and AAX stem formats are translated to and from it.
- **Parameter categories**: `AudioParameterBuilder::withCategory()` marks input and output gains and
  meters. Meter categories make the parameter read-only; AAX shows them as host meters (gain
  reduction included), VST3 and CLAP as read-only parameters.
- **Presets**: processor presets are the VST3 program list (with a program change parameter), the AU and
  AUv3 factory presets. AUv3 hosts can also save user presets.
- **Editor and host**: override `AudioProcessorEditor::getParameterAt()` to report the parameter under
  the mouse (VST3 `IParameterFinder`). `getHostContext()` returns the wrapper services when the host
  offers them, for example `showParameterContextMenu()` to show the host menu for a parameter on right
  click (VST3 `IComponentHandler3`, CLAP `context-menu`).

### Format notes

- **CLAP**: supports MIDI 2.0 note ports, SysEx output, `state-context`, `remote-controls` (pages of 8
  automatable parameters) and the host `context-menu`.
- **LV2**: plugins with an editor ship an LV2 UI (Cocoa, Windows or X11) using `instance-access`, with host
  resize and idle support. Plugins require `bufsz:boundedBlockLength` and `bufsz:maxBlockLength`, so no
  allocation happens in `run()`. Parameter units, integer, toggle and enumeration ranges (with scale
  points) and non-automatable parameters are written to the TTL.
- **AU**: MIDI output reaches hosts through the MIDI output callback, and parameter values convert to and
  from strings.

## Building with CMake

Create a `CMakeLists.txt` file for your plugin:

```cmake
cmake_minimum_required (VERSION 3.31)

set (target_name my_plugin)
set (target_version "0.0.1")
project (${target_name} VERSION ${target_version})

include (FetchContent)

FetchContent_Declare(
  yup
  GIT_REPOSITORY https://github.com/kunitoki/yup.git
  GIT_TAG        main)

set (YUP_BUILD_EXAMPLES OFF)
set (YUP_BUILD_TESTS OFF)
FetchContent_MakeAvailable(yup)

# Create the plugin target
yup_audio_plugin (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "MyPlugin"
    TARGET_APP_ID "com.mycompany.${target_name}"
    TARGET_APP_NAMESPACE "com.mycompany"
    TARGET_CXX_STANDARD 20
    PLUGIN_ID "com.mycompany.MyPlugin"
    PLUGIN_NAME "MyPlugin"
    PLUGIN_VENDOR "com.mycompany"
    PLUGIN_EMAIL "support@mycompany.com"
    PLUGIN_VERSION "${target_version}"
    PLUGIN_DESCRIPTION "The best audio plugin ever."
    PLUGIN_URL "https://www.mycompany.com"
    PLUGIN_IS_SYNTH OFF
    PLUGIN_IS_MONO OFF
    PLUGIN_CREATE_CLAP ON
    PLUGIN_CREATE_VST3 ON
    PLUGIN_CREATE_AU ON
    PLUGIN_CREATE_STANDALONE ON
    MODULES
        yup::yup_gui
        yup::yup_audio_processors)

# Add source files
file (GLOB sources "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name}_shared PRIVATE ${sources})
```

The sources added to `${target_name}_shared` are compiled once and shared by every plugin format,
so keep them `PRIVATE`. For the same reason the format macros (`YUP_AUDIO_PLUGIN_ENABLE_CLAP` and
similar) only reach the format wrappers. To check at runtime which format loaded the plugin, use
`AudioProcessor::getWrapperType()`, which is already set when the processor constructor runs:

```cpp
if (yup::AudioProcessor::getWrapperType() == yup::AudioPluginWrapperType::standalone)
    enableStandaloneOnlyFeatures();
```

## Building for Different Platforms

### macOS
```bash
cmake -G "Xcode" . -B build -DYUP_ENABLE_TESTS=OFF -DYUP_ENABLE_EXAMPLES=OFF
cmake --build build --config Release
```

### Windows
```bash
cmake -G "Visual Studio 17 2022" -A x64 . -B build -DYUP_ENABLE_TESTS=OFF -DYUP_ENABLE_EXAMPLES=OFF
cmake --build build --config Release
```

### Linux
```bash
cmake -G "Ninja" . -B build -DYUP_ENABLE_TESTS=OFF -DYUP_ENABLE_EXAMPLES=OFF
cmake --build build --config Release
```

## Standalone Application

`PLUGIN_CREATE_STANDALONE ON` builds the plugin as an application. The target is
`<TARGET_NAME>_standalone_plugin`, while the application itself is named after
`PLUGIN_NAME` (for example `Retrace.app`, shown as Retrace in the macOS menu bar).
The window shows a slim toolbar above the editor:

- **Settings** opens the audio and MIDI device settings inside the window. Close
  it with the Close button, Escape or a click outside the panel.
- **Keyboard** (only for synths and plugins that accept MIDI) shows an on-screen
  keyboard that plays the plugin.

Enabled MIDI inputs play the plugin, and MIDI the plugin produces goes to the
selected MIDI output (desktop only). The device setup and the plugin state are
saved when the settings close and when the application quits, then restored on
the next launch, together with the window position and size (ignored when the
window would no longer be on any screen). On desktop they live in
`<user application data>/<PLUGIN_VENDOR>/<PLUGIN_NAME>/standalone.xml`.

When the editor's `shouldPreserveAspectRatio()` returns true, the window keeps
its proportions while resized. Fullscreen, maximized and full browser windows
are letterboxed: the toolbar, editor and keyboard keep those proportions,
centered, instead of being stretched.

An editor that prefers its own settings button can hide the toolbar. Only the
standalone application has a host, so this code does nothing when the plugin is
loaded as CLAP, VST3 or AU:

```cpp
MyEditor::MyEditor()
{
    if (auto* host = yup::StandalonePluginHost::getInstance())
    {
        host->setToolbarVisible (false);

        settingsButton.onClick = []
        {
            if (auto* host = yup::StandalonePluginHost::getInstance())
                host->showAudioSettings();
        };
        addAndMakeVisible (settingsButton);
    }
}
```

### Standalone in the browser

With Emscripten only the standalone format is built, so the same
`yup_audio_plugin` call works unchanged. For the example plugin:

```bash
just emscripten Debug example_plugin_standalone_plugin
just emscripten_serve
```

Then open the generated `.html` page under `build/emscripten/examples/plugin/Debug/`.

- Click the canvas once to start audio, as browsers require.
- Settings opens inside the canvas. Web MIDI devices can be enabled there
  (Chrome and Edge support Web MIDI).
- Plugins with audio inputs ask for microphone access. The browser only allows
  it on `https` or `localhost`. If permission is denied the input stays silent.
- The device setup and plugin state are kept in the browser `localStorage`
  under `yup.standalone.<PLUGIN_ID>`, saved whenever the page is hidden.
- MIDI output is not sent from the browser standalone.
- `CUSTOM_SHELL`, `PRELOAD_FILES`, `INITIAL_MEMORY`, `PTHREAD_POOL_SIZE` and
  `STACK_SIZE` given to `yup_audio_plugin` are passed to the standalone target.

## Retrace presets

Retrace's factory presets are the `.retrace` files under
`examples/plugin/data/presets`, one folder per category. At configure time CMake
packs them into a `Factory.rebank` zip archive and embeds it in the plugin, so
editing, adding or removing a file there is picked up by the next build. Only
the Init patch is defined in code.

A `.rebank` file is a plain zip archive of `.retrace` presets, in any folder
layout, renamed. Put one in the user preset folder or in a folder added to the
browser and its presets are listed in place, read-only, under the bank's name.
To make one: `cd MyBank && zip -r ../MyBank.rebank .`

`examples/plugin/tools` holds the Python scripts that generated the factory bank
through the MCP server below (`python3 generate_presets.py [names...]`).

## Controlling Retrace from an AI assistant (MCP)

The example plugin, Retrace, embeds an [MCP](../ai/mcp.md) server, so assistants
such as Claude or Copilot can design sounds, play the synth and listen to it.

1. Open the settings menu in the header and enable **MCP Server**. The setting is
   saved and off by default. It does nothing in the browser build.
2. Pick **Copy http://127.0.0.1:7380/mcp** from the same menu. Every open
   instance takes the next free port from 7380.
3. Add the server to the MCP client configuration:

```json
{
  "mcpServers": {
    "retrace": { "type": "http", "url": "http://127.0.0.1:7380/mcp" }
  }
}
```

The server only listens on `127.0.0.1` and rejects requests from web pages.

| Group | Tools |
|-------|-------|
| Synth | `get_synth_info` |
| Parameters | `list_parameters`, `get_parameters`, `set_parameters` (plain or normalized values) |
| Patch | `get_patch`, `set_patch` (merge or replace), `describe_patch_format`, `randomize_patch` |
| Presets | `list_presets`, `load_preset` (also a `.retrace` file or a preset inside a `.rebank` by path), `set_preset_info`, `save_preset` |
| Playing | `note_on`, `note_off`, `all_notes_off`, `send_midi`, `play_notes` |
| Audio | `render_audio` (offline, with parameter automation), `start_capture`, `stop_capture` |
| Editor | `screenshot` |

A patch is the whole sound: every parameter plus partials, curves, modulation
routes, the effect chain, tuning and preset info. `describe_patch_format`
explains it, so an assistant can write patches from scratch. Audio comes back
as a WAV file, inline audio content or both, with an analysis (levels, crest
factor, spectral centroid, octave bands, tail). The patch, parameters, format
and preset list are also readable as `retrace://` resources.

## Plugin Installation

### macOS
- VST3: `~/Library/Audio/Plug-Ins/VST3/`
- AU: `~/Library/Audio/Plug-Ins/Components/`
- AUv3: `~/Library/Audio/Plug-Ins/AppExtensions/`
- CLAP: `~/Library/Audio/Plug-Ins/CLAP/`
- AAX: `~/Application Support/Avid/Audio/Plug-Ins/`

### Windows
- VST3: `C:\Program Files\Common Files\VST3\`
- CLAP: `C:\Program Files\Common Files\CLAP\`

### Linux
- VST3: `~/.vst3/`
- CLAP: `~/.clap/`

## Best Practices

1. **Parameter Management**
   - Use `addParameter()` in the processor constructor
   - Handle parameter changes in `processBlock()`

2. **Editor Design**
   - Keep the editor lightweight
   - Handle resizing appropriately

3. **Resource Management**
   - Initialize resources in `prepareToPlay()`
   - Clean up in `releaseResources()`
   - Use RAII for resource management

4. **Performance**
   - Minimize allocations in the audio thread
   - Use SIMD operations when possible
   - Profile your plugin for CPU usage

5. **Testing**
   - Test your plugin in different DAWs
   - Verify parameter automation
   - Check for memory leaks
   - Test different sample rates and buffer sizes

## Common Issues and Solutions

1. **Plugin Not Showing Up**
   - Verify installation path
   - Check plugin format compatibility
   - Ensure proper permissions

2. **Audio Issues**
   - Verify buffer sizes
   - Check for denormal numbers
   - Monitor CPU usage

3. **UI Problems**
   - Handle window resizing
   - Manage component lifetimes
   - Test different DPI settings

## Additional Resources

- [CMake API Reference](CMake%20API.md) - Full reference of all CMake options and functions
- [CLAP Documentation](https://cleveraudio.org/)
- [VST3 Documentation](https://developer.steinberg.help/hub/display/VST)
