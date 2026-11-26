# Plugin hosting

`yup_audio_plugin_host` loads third-party VST3, CLAP, LV2 and Audio Unit plugins
into your application. You register the formats you want, scan for plugins
(optionally caching the results in a database and opening suspicious plugins in
a separate process), then load a plugin as an `AudioPluginInstance`, which is a
regular `AudioProcessor` you can play directly or drop into an
[audio graph](audio-graph.md).

**Module:** `yup_audio_plugin_host`

## Formats and platforms

Each format is compiled only when you enable it. The flag also makes the build
fetch and link the SDK it needs:

| Format | Class | Enable flag | SDK | Platforms |
|---|---|---|---|---|
| VST3 | `VST3Format` | `YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3` | vst3sdk | macOS, Windows, Linux |
| CLAP | `CLAPFormat` | `YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP` | clap | macOS, Windows, Linux |
| LV2 | `LV2Format` | `YUP_AUDIO_PLUGIN_HOST_ENABLE_LV2` | lv2, lilv | macOS, Windows, Linux |
| Audio Unit | `AUFormat` | `YUP_AUDIO_PLUGIN_HOST_ENABLE_AU` | System frameworks | macOS only |

`AUFormat` hosts both AUv2 components and AUv3 app extensions
(`AudioPluginDescription::isV3` tells them apart). Pass the flags as
`DEFINITIONS` of your target:

```cmake
yup_standalone_app (
    TARGET_NAME my_host
    DEFINITIONS
        YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3=1
        YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP=1
        YUP_AUDIO_PLUGIN_HOST_ENABLE_AU=1
    MODULES
        yup::yup_audio_processors
        yup::yup_audio_plugin_host)
```

Guard the format classes in code with the same flags, and the Audio Unit format
with `YUP_MAC` too. Set `YUP_ENABLE_PLUGIN_HOST_VST3_LOGGING`,
`YUP_ENABLE_PLUGIN_HOST_CLAP_LOGGING`, `YUP_ENABLE_PLUGIN_HOST_LV2_LOGGING` or
`YUP_ENABLE_PLUGIN_HOST_AU_LOGGING` to `1` to log scanning and loading for a
format.

Sandboxing (out-of-process scanning and `RemoteAudioPluginInstance`) launches
helper processes and is only available on desktop platforms (`YUP_DESKTOP`).

Plugin editors need the `yup_gui` module. A target that does not link it still
scans, loads and processes plugins, and every instance reports no editor. See
[Headless audio](headless.md).

## Registering formats

`AudioPluginScanner` owns the format backends. Adding a format of a type that is
already registered replaces it:

```cpp
yup::AudioPluginScanner scanner;

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3
scanner.addFormat (std::make_unique<yup::VST3Format>());
#endif

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP
scanner.addFormat (std::make_unique<yup::CLAPFormat>());
#endif

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_AU && YUP_MAC
scanner.addFormat (std::make_unique<yup::AUFormat>());
#endif
```

`getNumFormats()`, `getFormat (index)` and `getFormatForType (type)` give the
registered backends back. Each `AudioPluginFormat` reports its type
(`AudioPluginFormatType::vst3`, `clap`, `lv2`, `audioUnit`), a name, its file
extensions and its default search paths:

| Format | Extension | Default search paths |
|---|---|---|
| VST3 | `.vst3` | macOS: `/Library/Audio/Plug-Ins/VST3`, `~/Library/Audio/Plug-Ins/VST3`. Windows: `%CommonProgramFiles%\VST3`, `%APPDATA%\VST3`. Linux: `/usr/lib/vst3`, `/usr/local/lib/vst3`, `~/.vst3` |
| CLAP | `.clap` | The same locations, with `CLAP` (`clap` and `~/.clap` on Linux) |
| LV2 | `.lv2` | macOS: `/Library/Audio/Plug-Ins/LV2`, `~/Library/Audio/Plug-Ins/LV2`. Linux: `/usr/lib/lv2`, `/usr/local/lib/lv2`, `~/.lv2`. Windows: `LV2` in the common and user application data folders |
| Audio Unit | none | None: plugins come from the system registry |

## Scanning plugins

The scanner offers every file whose extension matches a format to that format.
The in-process scan only reads static metadata (VST3 `moduleinfo.json` or factory
info, CLAP descriptors, the AU registry, LV2 manifests) and never runs plugin
code, so channel and MIDI port counts may be estimates.

```cpp
const auto result = scanner.scanDefaults();

for (const auto& description : result.discovered)
    YUP_DBG (description.name << " by " << description.vendor);

for (const auto& path : result.failedPaths)
    YUP_DBG ("Failed: " << path);
```

`scan (searchPath)` scans a `FileSearchPath` of your own, and `scanDefaults()`
scans the default paths of every format. Registry-based formats (Audio Units) are
always scanned, whatever the path. The `ScanResult` holds:

| Field | Content |
|---|---|
| `discovered` | The `AudioPluginDescription` of every plugin found. |
| `failedPaths` | Files that could not be scanned. |
| `blacklistedPaths` | Files skipped because the database blacklisted them. |

Scanning can take a while. `scanAsync()` and `scanDefaultsAsync()` run it on a
background thread and call you back from that thread, so post the result to the
message thread before touching the UI. `cancelPendingScans()` stops queued and
running scans.

```cpp
// alive is a std::shared_ptr<std::atomic<bool>> set to false in the destructor
scanner.scanDefaultsAsync ([this, alive = alive] (yup::AudioPluginScanner::ScanResult result)
{
    yup::MessageManager::callAsync ([this, alive, result = std::move (result)]
    {
        if (alive->load())
            showPlugins (result.discovered);
    });
});
```

```{warning}
Do not add or replace formats while a scan is running: the scanner may use them
from several threads at once.
```

## The plugin database

Give the scanner an `AudioPluginDatabase` to cache the results in SQLite. Later
scans only open new or changed files, return the cached descriptions and failures
for the rest, and skip blacklisted files:

```cpp
auto database = std::make_shared<yup::AudioPluginDatabase>();

if (database->open (appDataDir.getChildFile ("plugins.db")).wasOk())
    scanner.setDatabase (database);

auto result = scanner.scanDefaults(); // discovered, failedPaths, blacklistedPaths
```

If the process dies while scanning a file, that file is blacklisted the next
time the database is opened. It is scanned again once it changes on disk or after
`clearBlacklist()`. Call `setScanVersion()` with a higher number to rescan
everything else, for example after updating a format backend. Registry formats
are never cached.

Every method of the database is thread-safe. Besides the scanner, you can use it
directly:

| Method | Description |
|---|---|
| `open (file)`, `openInMemory()`, `close()`, `isOpen()` | Open or close the database. |
| `getPlugins()` | All cached descriptions, for a plugin list that is ready before any scan. |
| `getPlugins (file, formatType)` | The descriptions cached for one file. |
| `getFailures()` | Every failed file with its `reason` (`error`, `crash`, `timeout`), `message`, failure `count`, `lastFailureTime` and whether it is `blacklisted`. |
| `getFileState (file, formatType)` | `needsScan`, `upToDate`, `failed` or `blacklisted`. |
| `invalidate (file)` | Forces a file to be scanned again. |
| `clearBlacklist (file)`, `clearBlacklist()` | Gives blacklisted files another chance. |
| `pruneMissingFiles()` | Removes entries for files that no longer exist and returns how many. |

## Out-of-process scanning

To get exact channel and MIDI port counts, plugins must be opened, which can
crash or hang the host. `setOutOfProcessScanning()` opens them in worker
processes instead, usually the host application relaunched with a special
command line. A plugin that crashes or exceeds the timeout only kills its worker:
the file is reported in `failedPaths` and blacklisted in the database.

```cpp
// At startup, before creating any window
void initialise (const yup::String& commandLine) override
{
    auto worker = std::make_unique<yup::AudioPluginScannerWorker>();
    worker->addFormat (std::make_unique<yup::VST3Format>());
    worker->addFormat (std::make_unique<yup::CLAPFormat>());

    if (worker->initialiseFromCommandLine (commandLine))
    {
        scannerWorker = std::move (worker); // this process is now a scanner worker
        return;
    }

    // ... normal startup
}

// Where the scanner is set up
yup::AudioPluginScanner::OutOfProcessOptions options;
options.executable = yup::File::getSpecialLocation (yup::File::currentExecutableFile);
options.timeoutMs = 60000; // per file
scanner.setOutOfProcessScanning (options);
```

The worker needs a running message loop, so keep it alive and open no window;
it quits by itself when the scanner disconnects. `OutOfProcessOptions` also has a
`commandLineUniqueID` (default `"yupPluginScanner"`) that must match the one
passed to `initialiseFromCommandLine()`. `setHostContext()` sets the context the
worker opens plugins with. Pass `std::nullopt` to `setOutOfProcessScanning()` to
return to in-process scanning.

Files are scanned in-process with the metadata-only scan when the worker
executable cannot be launched. Registry-based formats (AUv2) are always scanned
in-process.

## Plugin descriptions

`AudioPluginDescription` identifies a plugin. Keep it to load the plugin later or
store it in a session:

| Field | Description |
|---|---|
| `formatType` | The `AudioPluginFormatType` that loads it. |
| `name`, `vendor`, `version`, `category` | Display information. |
| `identifier` | Unique ID within the format: a base64 FUID for VST3, the descriptor ID for CLAP, `"type/subt/mfgr"` for Audio Units. |
| `fileOrBundlePath` | The file the plugin lives in. |
| `isV3` | True for AUv3 app extensions. |
| `isInstrument`, `isEffect` | Plugin kind. |
| `numInputChannels`, `numOutputChannels` | Audio channel counts (estimates after an in-process scan). |
| `numMidiInputPorts`, `numMidiOutputPorts` | MIDI port counts (estimates after an in-process scan). |

Descriptions compare with `==` and `!=`.

## Loading a plugin

Ask the format of the description to load it. `loadPlugin()` returns an instance
that is created and activated but not prepared yet:

```cpp
yup::AudioPluginHostContext context;
context.sampleRate = 48000.0f;
context.maxBlockSize = 256;

auto* format = scanner.getFormatForType (description.formatType);
if (format == nullptr)
    return;

auto result = format->loadPlugin (description, context);
if (result.failed())
{
    YUP_DBG (result.getErrorMessage());
    return;
}

std::unique_ptr<yup::AudioPluginInstance> instance = std::move (result).getValue();
```

`AudioPluginHostContext` tells the plugin about the host. One context can be
shared by every instance:

| Field | Default | Description |
|---|---|---|
| `sampleRate` | `44100` | Sample rate the plugin will be prepared at. |
| `maxBlockSize` | `512` | Largest block passed to `processBlock()`. |
| `preferDoublePrecision` | `false` | Use double precision where the plugin supports it. |
| `isNonRealtime` | `false` | Prepare for offline rendering. |
| `hostName`, `hostVendor`, `hostVersion` | `"YUP Audio Plugin Host"`, `"YUP"`, the YUP version | Identity reported to the plugin. |

Fill the rate and block size from the current audio device when you have one.
Transport information does not go through the context: plugins read it from the
play head of each process call.

## Processing

An `AudioPluginInstance` is an `AudioProcessor`, so it is prepared, processed and
released like any other (see [Processors](processors.md)):

```cpp
instance->prepareToPlay (yup::AudioSpec (48000.0f, 256));

// Audio thread
yup::AudioProcessContext<float> processContext { buffer, midiBuffer, parameterChanges };
processContext.playHead = &playHead;
instance->processBlock (processContext);

// When done
instance->releaseResources();
```

The instance adds a few host controls:

- `setBypassed (true)` bypasses the plugin. When the plugin has its own bypass
  parameter, the instance drives it, so the plugin keeps handling its latency and
  tails; editing that parameter from the plugin editor or automation updates
  `isBypassed()` too. Otherwise inputs are copied to the matching outputs and the
  other outputs are cleared.
- `setNonRealtime (true)` switches the plugin to offline rendering.
- Plugins that return `true` from `supportsDoublePrecisionProcessing()` can
  process `AudioProcessContext<double>` after
  `setProcessingPrecision (yup::AudioProcessor::ProcessingPrecision::doublePrecision)`.
- `setHostInfo()` forwards the track name and color to the plugin. Call it on the
  message thread.

`getDescription()` returns the description the instance was loaded from, and
`getLatencySamples()` the latency the plugin reports.

### Tail and draining

`getTailSamples()` returns the tail the plugin reports, with the meaning it has for
any processor (see
[Latency, tail and change notifications](processors.md#latency-tail-and-change-notifications)):

| Format | Tail |
|---|---|
| VST3 | `IAudioProcessor::getTailSamples()`; `kInfiniteTail` becomes `infiniteTailSamples` |
| CLAP | The `tail` extension; 0 without it and while the plugin is not activated |
| Audio Unit | `kAudioUnitProperty_TailTime`, converted to samples |
| LV2 | Always 0: the format has no tail |

A `RemoteAudioPluginInstance` returns the value of the plugin in the server process.

Add an `AudioProcessorBase::Listener` to the instance to follow changes. VST3, CLAP and
Audio Unit plugins report `latencyChanged`; CLAP and Audio Unit plugins also report
`tailChanged`. Both getters ask the plugin, so read them on the message thread, at load
time and in the listener, and keep the values for the audio thread.

To let a plugin ring out before you remove it, keep calling `processBlock()` with silent
input for `getLatencySamples() + getTailSamples()` samples. A plugin reporting
`infiniteTailSamples` never falls silent: fade its output out instead.

## Parameters

`getParameters()` exposes the plugin parameters as `AudioParameter` objects.
`getValue()` returns the real (plain) value and `getNormalizedValue()` the value in
`[0, 1]`. For VST3 and CLAP plugins, edits made in the plugin editor update these
objects, so a host UI can show them.

For automation, pass the changes of each block in the `ParameterChangeBuffer` of
the process context. Indexes refer to `getParameters()`, values are normalized and
offsets are in samples:

```cpp
// Message thread, when preparing
parameterChanges.reserve (static_cast<int> (instance->getParameters().size()) * 4);

// Audio thread, every block
parameterChanges.clear();
parameterChanges.addChange (cutoffIndex, 0.25f, 0);
parameterChanges.addChange (cutoffIndex, 0.75f, 128);
parameterChanges.sort();

yup::AudioProcessContext<float> processContext { buffer, midiBuffer, parameterChanges };
instance->processBlock (processContext);
```

VST3 and CLAP instances deliver these changes to the plugin at their sample
offsets.

## State and presets

`saveStateIntoMemory()` and `loadStateFromMemory()` save and restore the plugin
state as an opaque `MemoryBlock`, to store with your session or a graph. Plugins
that expose factory presets list them through `getNumPresets()`,
`getPresetName (index)` and `getCurrentPreset()`, and `setCurrentPreset (index)`
selects one.

## Editors

When `hasEditor()` is `true`, `createEditor()` returns a new
`AudioProcessorEditor` that you own. Put it in a window sized to its preferred
size, and call `attachedToNative()` once it is in a native window:

```cpp
auto editor = std::unique_ptr<yup::AudioProcessorEditor> (instance->createEditor());
if (editor == nullptr)
    return;

const auto size = editor->getPreferredSize();

window = std::make_unique<PluginWindow> (std::move (editor)); // a DocumentWindow showing the editor
window->centreWithSize (size);
window->setVisible (true);
```

Inside the window, after `addAndMakeVisible (*editor)`, call
`editor->attachedToNative()`. Make the window resizable only when
`isResizable()` is `true`, and keep `getPreferredAspectRatioSize()` when
`shouldPreserveAspectRatio()` is `true`. Destroy the editor before the instance.

## Running plugins out of process

`RemoteAudioPluginInstance` runs a plugin in its own `AudioPluginBridgeServer`
process, so a crash or a hang can't take the host down. It behaves like any other
`AudioPluginInstance`. Audio, MIDI and parameter changes go through shared memory
with a synchronous hand-off, so no latency is added, only the cost of switching
between processes. As with scanning, the server is usually the host application
relaunched with a special command line:

```cpp
// At startup, before creating any window
auto server = std::make_unique<yup::AudioPluginBridgeServer>();
server->addFormat (std::make_unique<yup::VST3Format>());
server->addFormat (std::make_unique<yup::CLAPFormat>());

if (server->initialiseFromCommandLine (commandLine))
{
    bridgeServer = std::move (server); // this process now hosts a single plugin
    return;
}

// Loading a plugin sandboxed
yup::RemoteAudioPluginInstance::Options options;
options.executable = yup::File::getSpecialLocation (yup::File::currentExecutableFile);

auto result = yup::RemoteAudioPluginInstance::load (description, hostContext, options);
if (result.failed())
    return; // result.getErrorMessage()

auto instance = std::move (result.getReference());
instance->onCrashed = [&] { /* tell the user, offer instance->reload() */ };
```

`Options::executable` is the binary inside the application bundle on macOS.
`commandLineUniqueID` (default `"yupPluginBridge"`) must match the server, and
`timeoutMs` (default 20000) limits loading, state and other control requests, and
offline processing.

When the server crashes or misses a block deadline, the instance outputs silence,
`hasCrashed()` returns `true` and `onCrashed` is called on the message thread.
`reload()`, called on the message thread, starts a new server, restores the last
saved or loaded state and the current parameter values, and prepares the plugin
again. While crashed, `saveStateIntoMemory()` returns the last known state.

Current limitations:

- Only single precision processing is supported.
- Parameter metadata changes made after loading are not reflected.
- MIDI controller mappings are not exposed.
- The plugin editor opens in a separate window owned by the server: call
  `showEditor()` / `hideEditor()` instead of embedding `createEditor()`, which only
  returns a placeholder. Pass a component of the host window to `showEditor()` to
  make the editor behave as its child: on macOS and Windows it stays above the host
  while the host or the editor is active, on X11 it is transient for the host
  window (Wayland has no equivalent). `isEditorShowing()` and `onEditorClosed`
  track the window when the user closes it, and `onEditorClosed` is also called
  when the editor fails to open.

`AudioPluginDatabase::setRunSandboxed()` stores a per-plugin preference (pass
`std::nullopt` to clear it), and `setDefaultRunSandboxed()` sets the default for
plugins without one (initially `false`). The preference survives rescans. Read
the result with `shouldRunSandboxed()` to choose between the format's
`loadPlugin()` and `RemoteAudioPluginInstance::load()`:

```cpp
yup::ResultValue<std::unique_ptr<yup::AudioPluginInstance>> loadPlugin (const yup::AudioPluginDescription& description)
{
    if (database->shouldRunSandboxed (description))
    {
        auto result = yup::RemoteAudioPluginInstance::load (description, hostContext, bridgeOptions);
        if (result.failed())
            return yup::makeResultValueFail (result.getErrorMessage());

        std::unique_ptr<yup::AudioPluginInstance> instance = std::move (result.getReference());
        return yup::makeResultValueOk (std::move (instance));
    }

    if (auto* format = scanner.getFormatForType (description.formatType))
        return format->loadPlugin (description, hostContext);

    return yup::makeResultValueFail ("No format for " + description.name);
}
```

## Threading rules

| Call | Thread |
|---|---|
| `scan()`, `scanDefaults()` | Block the calling thread until done: prefer the async versions from the UI |
| `scanAsync()` and `scanDefaultsAsync()` callbacks | A scanner thread: post to the message thread |
| `AudioPluginDatabase` methods | Any thread |
| `RemoteAudioPluginInstance::reload()`, `setHostInfo()` | Message thread |
| `createEditor()`, `showEditor()`, `hideEditor()` | Message thread |
| `processBlock()` | Audio thread |
| `onCrashed`, `onEditorClosed` | Called on the message thread |

The audio graph example also loads plugins and saves or restores their state on
the message thread, and prepares the graph holding them from the device start
callback.

Plugins run their own code when they are loaded, so a crashing or hanging plugin
takes the host with it unless it runs out of process. See
[the audio thread](../core/multithreading/index.md#the-audio-thread) for the
general rules.

## Related pages

- [Audio graph](audio-graph.md) - routing hosted plugins together with your own processors.
- [Headless audio](headless.md) - hosting plugins without `yup_gui`.
- [Processors](processors.md) - the `AudioProcessor` interface every instance implements.
- [Writing plugins](plugin-client.md) - building your own plugins.
- [Audio devices](devices.md) - playing an instance on an audio device.
- [Audio GUI](audio-gui.md) - device settings, meters and the graph editor for a host UI.
- [Building plugins](../build-system/building-plugins.md) - CMake setup for plugin targets.
