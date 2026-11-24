# Audio processors

An `AudioProcessor` is a self-contained audio unit with buses, parameters, state and an
optional editor. The same class runs as a CLAP, VST3, AU, AUv3, LV2 or AAX plugin, or as a
standalone application, and can also be driven directly by your own code or by an
[audio graph](audio-graph.md). This page is the reference for writing one.

**Module:** `yup_audio_processors`

## The processor lifecycle

A processor moves through a fixed sequence. Wrappers and hosts drive it; your class only
overrides the virtual methods.

| Step | Method | Thread | What to do |
| --- | --- | --- | --- |
| Construct | constructor | message | Declare the bus layout, add every parameter |
| Configure | `isBusLayoutSupported`, `setBusLayout` | message | Accept or reject channel layouts while released |
| Prepare | `prepareToPlay (const yup::AudioSpec&)` | message | Allocate buffers, create parameter handles, reset DSP |
| Process | `processBlock (yup::AudioProcessContext<float>&)` | audio | Render one block in place |
| Reset | `flush()` | any, often audio | Clear tails and delay lines without allocating |
| Release | `releaseResources()` | message | Free what `prepareToPlay` allocated |
| Destroy | destructor | message | Editors are already gone, parameter writers have stopped |

`yup::AudioSpec` carries the playback configuration: `sampleRate` (a `float`),
`maxBlockSize` and `numChannels`. Blocks passed to `processBlock` never exceed
`maxBlockSize` but may be shorter, so always read the block length from the buffer.
`getSampleRate()` and `getSamplesPerBlock()` return the values of the last prepare.

Besides `processBlock`, every processor must implement `hasEditor()`, `createEditor()`
and the preset methods described in [State and presets](#state-and-presets).

## Realtime rules

`processBlock` runs on the [audio thread](../core/multithreading/index.md#the-audio-thread)
with a hard deadline. Inside it:

- Never allocate, lock a mutex, wait, log, or touch files and the message thread.
- Reserve everything in `prepareToPlay`; `ParameterChangeBuffer` and the MIDI buffers
  are already sized by the wrapper.
- Read parameters through `getValue()` or an `AudioParameterHandle`, both lock-free.
- Share results with the editor through atomics, then let the editor poll them.
- Plugin wrappers and `AudioGraphProcessor` call `processBlock` with denormals disabled. Use
  `yup::ScopedNoDenormals` around DSP that runs anywhere else, such as a thread of your own.

Wrappers try-lock the processor's process lock before each block. When the lock is busy or
processing is suspended (see [Suspending processing](#suspending-processing)) the block
is skipped or silenced, so `processBlock` never races with a reconfiguration.

## Buses and layouts

The bus layout is passed to the constructor and lists the input and output buses. Each
bus is audio or MIDI, has a channel count (or an explicit `AudioChannelSet`) and a role.

```cpp
MyCompressor()
    : yup::AudioProcessor ("Compressor",
                           yup::AudioBusLayout (
                               { yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Input, 2),
                                 yup::AudioBus ("sidechain", yup::AudioBus::Audio, yup::AudioBus::Input, 2,
                                                yup::AudioBus::Role::Auxiliary) },
                               { yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
{
}
```

| Role | Meaning |
| --- | --- |
| `Role::Main` | The main input or output, copied into `context.audio` |
| `Role::Auxiliary` | Sidechains and extra outputs, read through the bus views |

A MIDI bus makes `acceptsMidi()` (input) or `producesMidi()` (output) return true. Override
them if MIDI support depends on something else.

By default the processor accepts only the layout it was built with. To offer other channel
counts, override `isBusLayoutSupported()`. Candidate layouts always have the same buses;
only channel counts or channel sets differ. `setBusLayout()` applies a supported layout while
the processor is released and notifies the host.

```cpp
bool isBusLayoutSupported (const yup::AudioBusLayout& layout) const override
{
    const auto channels = layout.getAudioBusNumChannels (0, false);
    return (channels == 1 || channels == 2) && layout.getAudioBusNumChannels (0, true) == channels;
}
```

## The process context

`processBlock` receives everything for one block in an `AudioProcessContext`:

| Field | Content |
| --- | --- |
| `audio` | Flat buffer of all output channels, processed in place |
| `midi` | Incoming MIDI; what is left in it becomes the MIDI output |
| `params` | Sample-accurate parameter changes for this block, sorted by sample offset |
| `playHead` | Transport and tempo information, or `nullptr` when there is none |
| `inputs` / `outputs` | Per-bus views of the host buffers |
| `noteExpressions` | Per-note expressions, or `nullptr` unless `supportsNoteExpression()` is true |

Before calling `processBlock`, wrappers copy the main input into `context.audio`, so an
effect reads and writes the same buffer. Auxiliary inputs are not copied: read them through
the views, which return null pointers for silent or inactive buses.

```cpp
if (const float* key = context.getAuxiliaryInput (0).getReadPointer (0))
    detector.process (key, context.audio.getNumSamples());
```

When the processor produces MIDI, clear `context.midi` before adding events if the input
should not be echoed.

## Parameters

Parameters are created with `AudioParameterBuilder` and added in the constructor with
`addParameter()`. The order of addition is the parameter index.

```cpp
addParameter (cutoff = yup::AudioParameterBuilder()
                           .withID ("cutoff")
                           .withName ("Cutoff")
                           .withHostID (2)
                           .withRange (yup::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.25f))
                           .withDefault (1000.0f)
                           .withUnit (yup::AudioParameter::ParameterUnit::Hertz)
                           .withSmoothing (20.0f)
                           .withModulePath ("Filter")
                           .build());
```

| Builder method | Effect |
| --- | --- |
| `withID`, `withName` | String ID used in state, display name (both required) |
| `withHostID` | Stable numeric ID exposed to hosts, defaults to the index |
| `withRange`, `withDefault` | Real value range (with optional step and skew) and default |
| `withValueToString`, `withStringToValue` | Text shown and parsed by hosts |
| `withUnit`, `withCategory` | Unit for host display; input, output or meter category |
| `withStepped`, `withEnum` | Discrete values, enum choices |
| `withSmoothing` | Smoothing time used by `AudioParameterHandle` |
| `withAutomatable`, `withReadOnly` | Host automation and editing (mutually exclusive) |
| `withModulatable`, `withPerNoteModulatable` | CLAP modulation support |
| `withModulePath` | Grouping such as `"Filter/Envelope"` (VST3 units, CLAP modules, AU clumps) |
| `withBypass` | Makes it the host bypass parameter (a 0-1 toggle) |

Meter categories make a parameter read-only and non-automatable. Without string converters
the value is shown with two decimals and parsed as a number.

`getValue()` and `setValue()` work on real values and are lock-free; `setValue()` snaps to
the range and ignores non-finite values. The `Normalized` variants map to and from 0-1, and
`toString()` and `convertFromString()` use the string converters.

Editors change parameters inside a gesture so hosts record automation correctly: call
`beginChangeGesture()` on mouse down, `setValueNotifyingHost()` while dragging and
`endChangeGesture()` on mouse up. `setValueNotifyingHost()` calls the
`AudioParameter::Listener`s, which is how wrappers learn about the change, so call it from the
message thread, not from `processBlock`. Gestures nest and must be balanced. Host automation
for a parameter under an active gesture is ignored.

```{warning}
Host IDs and string IDs are the identity of a parameter in saved sessions and automation.
Never reuse or renumber them in a later release. Give every parameter an explicit
`withHostID()` (at most `AudioParameter::maximumHostParameterID`) so that inserting new
parameters does not shift the IDs of the old ones.
```

A parameter object can be added to more than one processor; each owner gets notified with
its own index. Add parameters only during construction, before any thread writes values.

## Sample-accurate automation

`AudioParameterHandle` turns a parameter into a smoothed per-sample value. Create it in
`prepareToPlay`. Before each block, wrappers store the last automated value in every
parameter, so the simple path calls `updateNextAudioBlock()` once per block and ramps toward
the new value, as in the [complete example](#a-complete-gain-processor).

For changes at their exact sample offset, use `prepareBlock()` with the block's `params` and
the parameter index, then call `advanceToSample()` while walking the block:

```cpp
cutoffHandle.prepareBlock (context.params, cutoff->getIndexInContainer());

for (int i = 0; i < numSamples; ++i)
{
    if (cutoffHandle.advanceToSample (i))
        updateCoefficients();

    const auto value = cutoffHandle.getNextValue();
    // ...
}
```

`context.params` holds `ParameterChange` entries (`parameterIndex`, `normalizedValue`,
`sampleOffset`). CLAP modulation appears only there, as the modulated value, and is never
stored in the parameter itself.

## Note expressions

Return true from `supportsNoteExpression()` to receive per-note volume, pan, tuning,
vibrato, expression, brightness and pressure in `context.noteExpressions`. Each
`NoteExpression` has a sample offset, note ID, channel (1-16), key, type and value. Volume is
a linear gain (1 is unity), pan is 0-1 (0.5 is center), tuning is in semitones, the rest are 0-1.

Otherwise `NoteExpressionMidiConverter` turns them into MIDI, channel-wide or as MPE when
`supportsMpe()` returns true, keeping only tuning (pitch bend), pressure and brightness (CC74).
See [MPE](mpe.md).

## State and presets

Hosts save and restore state through `saveStateIntoMemory()` and `loadStateFromMemory()`.
The easiest path is a `DataTree`: return true from `supportsDataTreeState()` and implement
`saveStateIntoDataTree()` and `loadStateFromDataTree()`; the base class stores the tree as
XML. Override the memory methods instead for an opaque binary format. All four return
`yup::Result`.

```cpp
bool supportsDataTreeState() const noexcept override { return true; }

yup::Result saveStateIntoDataTree (yup::DataTree& state) override
{
    state = yup::DataTree ("GainState");
    auto transaction = state.beginTransaction();
    transaction.setProperty ("gain", gainParameter->getValue());
    return yup::Result::ok();
}

yup::Result loadStateFromDataTree (const yup::DataTree& state) override
{
    if (state.getType() != yup::Identifier ("GainState"))
        return yup::Result::fail ("Unknown state");

    gainParameter->setValue (static_cast<float> (state.getProperty ("gain", gainParameter->getDefaultValue())));
    updateHostDisplay (ChangeDetails().withParameterValuesChanged (true));
    return yup::Result::ok();
}
```

State calls come from the host, usually on the message thread, and can overlap processing.
Parameters are atomic; protect other state with atomics or a suspension.

Presets are exposed through `getNumPresets()`, `getCurrentPreset()`, `setCurrentPreset()`,
`getPresetName()` and `setPresetName()`. A processor without presets returns 0 from
`getNumPresets()`.

## Latency, tail and change notifications

Report latency with `setLatencySamples()`, which notifies the host when the value changes.
Override `getTailSamples()` when the output rings on after the input stops (reverbs, delays).

`updateHostDisplay()` tells hosts that something changed, using `ChangeDetails` flags:
`withLatencyChanged`, `withTailChanged`, `withParameterValuesChanged`,
`withParameterInfoChanged`, `withNonParameterStateChanged`, `withProgramChanged` and
`withBusLayoutChanged`. Call it, and `setLatencySamples()`, from the message thread.
`AudioProcessorBase::Listener` receives the same notifications in your own code.

`getHostInfo()` returns the track name and color when the host shares them
(`hostInfoChanged()` is called on the message thread when they change), and
`isOfflineProcessing()` is true while the host renders offline.

## Suspending processing

To reconfigure a running processor, for example after a quality change that alters latency,
suspend it. `ScopedProcessSuspension` suspends on construction and resumes on destruction;
wrappers output silence meanwhile. Take the process lock as well so a block that already
started finishes first:

```cpp
void setQuality (int newQuality)
{
    const yup::AudioProcessorBase::ScopedProcessSuspension suspension (*this);
    const yup::ScopedLock lock (getProcessLock());

    engine.setOversampling (newQuality);
    setLatencySamples (engine.getLatency());
}
```

## Play head and transport

`context.playHead` reports the host transport. It is `nullptr` when the host provides none,
for example in the standalone application, and every field is optional:

```cpp
const auto position = context.playHead != nullptr ? context.playHead->getPosition() : std::nullopt;
const auto bpm = position ? position->getBpm().value_or (120.0) : 120.0;
const auto playing = position && position->getIsPlaying();
```

`PositionInfo` also offers `getPpqPosition()`, `getTimeInSamples()`, `getTimeSignature()`,
`getLoopPoints()`, `getIsLooping()` and `getIsRecording()`.

## Double precision and bypass

Override `processBlock (yup::AudioProcessContext<double>&)` and return true from
`supportsDoublePrecisionProcessing()` to process 64-bit buffers when the host provides them.

While the host bypasses the plugin, wrappers call `processBlockBypassed()` instead of
`processBlock`. The default does nothing, so the main input copied into the buffer passes
through. Override it to keep latency compensation aligned or let tails ring out. Build a
parameter `withBypass (true)` to own the bypass control, returned by `getBypassParameter()`;
otherwise wrappers add one.

## Editors

`createEditor()` returns a new `AudioProcessorEditor`, a `yup::Component` owned by the
wrapper and destroyed before the processor, so it can safely hold a reference to it.

| Method | Purpose |
| --- | --- |
| `getPreferredSize()` | Design size the editor is laid out for (required) |
| `isResizable()` | Whether hosts and users can resize it (required) |
| `getInitialSize()` | Size when the editor opens, defaults to the preferred size |
| `shouldPreserveAspectRatio()` | Constrain resizing to `getPreferredAspectRatioSize()` |
| `shouldShowResizableCorner()` | Show the wrapper's resize corner, defaults to `isResizable()` |
| `shouldRenderContinuous()` | Repaint every frame instead of on demand |
| `getParameterAt()` | Parameter under the mouse, for host lookups |

`getConstrainedSize()` applies these rules to a requested size. To reopen at the size the
user left, store the size in the processor state and return it from `getInitialSize()`.
`getHostContext()` returns host services when available, for example
`showParameterContextMenu()` to show the host menu for a parameter.

## Reactive parameter updates

An editor that refreshes its controls by reading every parameter each frame does
O(all parameters) work even when nothing moved. `yup_audio_processors` offers a
reactive path instead: parameters report their own changes, and the editor
refreshes just the controls bound to them.

The pieces are:

- **`ParameterChangeQueue`** - a lock-free set of changed parameter indices,
  owned by every `AudioProcessorBase`. Each parameter added with `addParameter()`
  marks itself from `setValue()` whenever its value actually changes, from any
  thread, so host automation, UI edits and preset loads are all reported, whether
  or not audio is running. Shared parameters mark each owning processor's queue
  using that processor's index. Marking uses one lock-free atomic OR per owner
  and coalesces: a parameter changed many times between two refreshes is reported
  once. Draining scans one word per 32 parameters, then visits the changed indices.
- **`AudioProcessorEditor::startParameterListening()`** - starts a message-thread
  timer that drains the queue and notifies the attachments bound to the changed
  parameters in every listening editor. Editors share one message-thread
  dispatcher, so draining from one does not consume another editor's notifications.
  Pass a rate of zero to drive `dispatchPendingParameterChanges()` yourself,
  for example from a test.
- **`AudioParameterAttachment`** - binds one control's parameter to a callback
  (typically the control's existing refresh-from-parameter). The editor fires the
  callback, on the message thread, when that parameter changes. This is the
  parameter-to-control direction; the reverse stays with gestures. Callbacks may
  remove or rebind attachments, including themselves.

Create, rebind and destroy attachments, and start, stop and dispatch editor
listening, on the message thread. The processor must outlive its editors, and
the editor must outlive its attachments. Parameter registration and processor
destruction must not overlap parameter writes, including writes through other
owners of shared parameters. Register parameters before starting those writers.
Queue growth preserves pending changes; explicit `resize()` clears them.

The queue has a single consumer. While editors are listening, leave draining to
their shared dispatcher rather than calling `readChanges()` directly.

The processor needs no code at all. In the editor:

```cpp
MyEditor::MyEditor (MyProcessor& p) : processor (p)
{
    startParameterListening (processor); // 60 Hz by default

    gainAttachment = std::make_unique<yup::AudioParameterAttachment> (
        *this, processor.getParameters()[gainIndex],
        [this] { gainKnob.syncFromParameter(); });
}
```

The attachment calls the callback once on creation unless its last argument is false.

## Host and wrapper detection

| Call | Returns |
| --- | --- |
| `AudioProcessor::getWrapperType()` | The plugin format (`AudioPluginWrapperType::clap`, `vst3`, `au`, `auv3`, `aax`, `lv2`, `standalone`, `unknown`), already valid in constructors |
| `AudioProcessor::getHostType()` | A `PluginHostType` with checks such as `isAbletonLive()`, `isCubaseOrNuendo()`, `isProTools()`, `isReaper()`, `isValidator()`; unknown in the standalone application |
| `StandalonePluginHost::getInstance()` | The standalone application host, or `nullptr` inside a plugin host |

Use the host type only to work around host quirks. `StandalonePluginHost` offers
`showAudioSettings()` and `setToolbarVisible()` so an editor can replace the standalone
toolbar; call them on the message thread.

## DomainProcessor

`AudioProcessor` derives from `DomainProcessor<AudioProcessContext, AudioSpec>`, a template
that provides the prepare, process, bypass, flush and precision interface for any processing
domain. The [spectral framework](spectral.md) uses the same base with spectral types. You
derive from `AudioProcessor` or `SpectralProcessor`, not from `DomainProcessor` directly.

## A complete gain processor

```cpp
class GainProcessor : public yup::AudioProcessor
{
public:
    GainProcessor()
        : yup::AudioProcessor ("Gain",
                               yup::AudioBusLayout ({ yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                                    { yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        addParameter (gainParameter = yup::AudioParameterBuilder()
                                          .withID ("gain")
                                          .withName ("Gain")
                                          .withHostID (1)
                                          .withRange (-60.0f, 12.0f)
                                          .withDefault (0.0f)
                                          .withUnit (yup::AudioParameter::ParameterUnit::Decibels)
                                          .withValueToString ([] (float db) { return yup::Decibels::toString (db, 1); })
                                          .withSmoothing (20.0f)
                                          .build());
    }

    void prepareToPlay (const yup::AudioSpec& spec) override
    {
        gain = yup::AudioParameterHandle (*gainParameter, spec.sampleRate);
    }

    void releaseResources() override {}

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        auto& audio = context.audio;
        const auto numSamples = audio.getNumSamples();

        if (! gain.updateNextAudioBlock())
        {
            audio.applyGain (yup::Decibels::decibelsToGain (gain.getCurrentValue()));
            return;
        }

        const auto startGain = yup::Decibels::decibelsToGain (gain.getCurrentValue());
        const auto endGain = yup::Decibels::decibelsToGain (gain.skip (numSamples));
        audio.applyGainRamp (0, numSamples, startGain, endGain);
    }

    bool supportsDataTreeState() const noexcept override { return true; }
    yup::Result saveStateIntoDataTree (yup::DataTree& state) override;   // as shown above
    yup::Result loadStateFromDataTree (const yup::DataTree& state) override;

    int getCurrentPreset() const noexcept override { return 0; }
    void setCurrentPreset (int) noexcept override {}
    int getNumPresets() const override { return 0; }
    yup::String getPresetName (int) const override { return {}; }
    void setPresetName (int, yup::StringRef) override {}

    bool hasEditor() const override { return false; }
    yup::AudioProcessorEditor* createEditor() override { return nullptr; }

private:
    yup::AudioParameter::Ptr gainParameter;
    yup::AudioParameterHandle gain;
};
```

To ship it as a plugin, add the entry point described in [Writing plugins](plugin-client.md).

## Related pages

- [Writing plugins](plugin-client.md) - turning a processor into CLAP, VST3, AU, LV2, AAX and standalone
- [Spectral processing](spectral.md) - processors that work on FFT bins
- [Audio graph](audio-graph.md) - connecting processors together
- [Plugin hosting](plugin-hosting.md) - loading third-party plugins
- [Audio buffers](buffers.md) - the `AudioBuffer` used in `context.audio`
- [MIDI](midi.md) - the `MidiBuffer` used in `context.midi`
- [Building plugins](../build-system/building-plugins.md) - CMake setup and packaging
