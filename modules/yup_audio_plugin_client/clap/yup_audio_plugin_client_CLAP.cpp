/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "../yup_audio_plugin_client.h"

#include "../common/yup_AudioPluginEntryPoint.h"
#include "../common/yup_AudioPluginUtilities.h"

#if ! defined(YUP_AUDIO_PLUGIN_ENABLE_CLAP)
#error "YUP_AUDIO_PLUGIN_ENABLE_CLAP must be defined"
#endif

#include <array>
#include <optional>
#include <string_view>

#include <clap/clap.h>

extern "C" yup::AudioProcessor* YUP_AUDIO_PLUGIN_CREATE_FUNCTION();

namespace yup
{

//==============================================================================

std::optional<MidiMessage> clapEventToMidiMessage (const clap_event_header_t* event)
{
    const auto clampMidiChannel = [] (int channel) noexcept
    {
        return jlimit (1, 16, channel < 0 ? 1 : channel + 1);
    };

    const auto clampMidiValue = [] (double value) noexcept
    {
        if (! (value >= 0.0))
            return 0;

        if (value >= 127.0)
            return 127;

        return static_cast<int> (value);
    };

    const auto clampPitchBendValue = [] (double value) noexcept
    {
        if (! (value >= 0.0))
            return 0;

        if (value >= 16383.0)
            return 16383;

        return static_cast<int> (value);
    };

    switch (event->type)
    {
        case CLAP_EVENT_NOTE_ON:
        {
            const auto* noteEvent = reinterpret_cast<const clap_event_note_t*> (event);
            if (! isPositiveAndBelow (noteEvent->key, 128))
                return std::nullopt;

            return MidiMessage::noteOn (clampMidiChannel (noteEvent->channel),
                                        noteEvent->key,
                                        static_cast<uint8> (clampMidiValue (noteEvent->velocity * 127.0)));
        }

        case CLAP_EVENT_NOTE_OFF:
        {
            const auto* noteEvent = reinterpret_cast<const clap_event_note_t*> (event);
            if (! isPositiveAndBelow (noteEvent->key, 128))
                return std::nullopt;

            return MidiMessage::noteOff (clampMidiChannel (noteEvent->channel),
                                         noteEvent->key,
                                         static_cast<uint8> (clampMidiValue (noteEvent->velocity * 127.0)));
        }

        case CLAP_EVENT_NOTE_CHOKE:
        {
            const auto* noteEvent = reinterpret_cast<const clap_event_note_t*> (event);
            if (! isPositiveAndBelow (noteEvent->key, 128))
                return std::nullopt;

            return MidiMessage::noteOff (clampMidiChannel (noteEvent->channel), noteEvent->key);
        }

        case CLAP_EVENT_MIDI:
        {
            const auto* midiEvent = reinterpret_cast<const clap_event_midi_t*> (event);
            if (midiEvent->data[0] < 0x80)
                return std::nullopt;

            const int messageLength = MidiMessage::getMessageLengthFromFirstByte (midiEvent->data[0]);
            if (messageLength <= 0 || messageLength > 3)
                return std::nullopt;

            for (int byteIndex = 1; byteIndex < messageLength; ++byteIndex)
                if (midiEvent->data[byteIndex] >= 0x80)
                    return std::nullopt;

            return MidiMessage (midiEvent->data, messageLength);
        }

        case CLAP_EVENT_NOTE_EXPRESSION:
        {
            const auto* ev = reinterpret_cast<const clap_event_note_expression_t*> (event);
            const int channel = clampMidiChannel (ev->channel);

            if (ev->expression_id == CLAP_NOTE_EXPRESSION_TUNING)
            {
                const int pitchBendValue = clampPitchBendValue (ev->value * 8192.0 + 8192.0);
                return MidiMessage::pitchWheel (channel, pitchBendValue);
            }

            if (ev->expression_id == CLAP_NOTE_EXPRESSION_PRESSURE)
                return MidiMessage::channelPressureChange (channel, clampMidiValue (ev->value * 127.0));

            if (ev->expression_id == CLAP_NOTE_EXPRESSION_BRIGHTNESS)
                return MidiMessage::controllerEvent (channel, 74, clampMidiValue (ev->value * 127.0));

            break;
        }

        case CLAP_EVENT_MIDI_SYSEX:
        {
            const auto* sysexEvent = reinterpret_cast<const clap_event_midi_sysex_t*> (event);
            if (sysexEvent->buffer == nullptr || sysexEvent->size == 0)
                return std::nullopt;

            return MidiMessage (sysexEvent->buffer, static_cast<int> (sysexEvent->size));
        }

        default:
            break;
    }

    return std::nullopt;
}

//==============================================================================

void clapEventToParameterChange (const clap_event_header_t* event, AudioProcessor& audioProcessor)
{
    if (event->type != CLAP_EVENT_PARAM_VALUE)
        return;

    const clap_event_param_value_t* paramEvent = reinterpret_cast<const clap_event_param_value_t*> (event);

    auto parameterIndex = audioProcessor.getParameterIndexByHostID (paramEvent->param_id);
    auto parameters = audioProcessor.getParameters();
    if (! isPositiveAndBelow (parameterIndex, static_cast<int> (parameters.size())))
        return;

    if (parameters[parameterIndex]->isReadOnly()
        || parameters[parameterIndex]->isPerformingChangeGesture())
    {
        return;
    }

    parameters[parameterIndex]->setValue (static_cast<float> (paramEvent->value));
}

bool addParameterModByCLAPEvent (AudioProcessor& processor,
                                 ParameterChangeBuffer& changes,
                                 const clap_event_param_mod_t* modEvent)
{
    if (modEvent->note_id != -1 || modEvent->key != -1)
        return false; // per-voice modulation needs voice infrastructure

    const auto parameters = processor.getParameters();
    const auto parameterIndex = processor.getParameterIndexByHostID (modEvent->param_id);

    if (! isPositiveAndBelow (parameterIndex, static_cast<int> (parameters.size())))
        return false;

    const auto& param = parameters[parameterIndex];

    if (! param->isModulatable() || param->isReadOnly())
        return false;

    const auto modulatedValue = jlimit (param->getMinimumValue(),
                                        param->getMaximumValue(),
                                        param->getValue() + static_cast<float> (modEvent->amount));

    return changes.addChange (parameterIndex,
                              param->convertToNormalizedValue (modulatedValue),
                              static_cast<int> (modEvent->header.time));
}

bool addParameterChangeByCLAPValue (AudioProcessor& processor,
                                    ParameterChangeBuffer& changes,
                                    uint32 hostParameterID,
                                    float value,
                                    int sampleOffset)
{
    const auto parameters = processor.getParameters();
    const auto parameterIndex = processor.getParameterIndexByHostID (hostParameterID);

    if (! isPositiveAndBelow (parameterIndex, static_cast<int> (parameters.size())))
        return false;

    if (parameters[parameterIndex]->isReadOnly()
        || parameters[parameterIndex]->isPerformingChangeGesture())
    {
        return false;
    }

    return changes.addChange (parameterIndex,
                              parameters[parameterIndex]->convertToNormalizedValue (value),
                              sampleOffset);
}

clap_param_info_flags getCLAPParameterFlags (const AudioParameter& parameter) noexcept
{
    clap_param_info_flags flags = 0;

    if (parameter.isStepped() || parameter.isEnum())
        flags |= CLAP_PARAM_IS_STEPPED;

    if (parameter.isEnum())
        flags |= CLAP_PARAM_IS_ENUM;

    if (parameter.isReadOnly())
        flags |= CLAP_PARAM_IS_READONLY;

    if (parameter.isAutomatable() && ! parameter.isReadOnly())
        flags |= CLAP_PARAM_IS_AUTOMATABLE;

    if (parameter.isModulatable())
        flags |= CLAP_PARAM_IS_MODULATABLE;

    if (parameter.isPerNoteModulatable())
        flags |= CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID;

    return flags;
}

constexpr int clapWrapperStateMagic = 0x504c4359; // "YCLP"
constexpr int clapWrapperStateVersion = 1;

static bool writeAllToCLAPStream (const clap_ostream_t* stream, const void* data, size_t dataSize)
{
    const auto* bytes = static_cast<const char*> (data);
    size_t bytesWritten = 0;

    while (bytesWritten < dataSize)
    {
        const auto remaining = dataSize - bytesWritten;
        const auto written = stream->write (stream, bytes + bytesWritten, static_cast<uint64_t> (remaining));

        if (written <= 0 || static_cast<uint64_t> (written) > remaining)
            return false;

        bytesWritten += static_cast<size_t> (written);
    }

    return true;
}

//==============================================================================

/*
void pluginSyncMainToAudio (AudioProcessor& audioProcessor, const clap_output_events_t* out)
{
    auto sl = CriticalSection::ScopedLockType (plugin->syncParameters);

    for (uint32_t i = 0; i < P_COUNT; i++)
    {
        if (plugin->mainChanged[i])
        {
            plugin->parameters[i] = plugin->mainParameters[i];
            plugin->mainChanged[i] = false;

            clap_event_param_value_t event = {};
            event.header.size = sizeof(event);
            event.header.time = 0;
            event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            event.header.type = CLAP_EVENT_PARAM_VALUE;
            event.header.flags = 0;
            event.param_id = i;
            event.cookie = NULL;
            event.note_id = -1;
            event.port_index = -1;
            event.channel = -1;
            event.key = -1;
            event.value = plugin->parameters[i];
            out->try_push(out, &event.header);
        }
    }
}

bool pluginSyncAudioToMain (AudioProcessor& audioProcessor)
{
    bool anyChanged = false;
    auto sl = CriticalSection::ScopedLockType (plugin->syncParameters);

    for (uint32_t i = 0; i < P_COUNT; i++)
    {
        if (plugin->changed[i])
        {
            plugin->mainParameters[i] = plugin->parameters[i];
            plugin->changed[i] = false;
            anyChanged = true;
        }
    }

    return anyChanged;

    return false;
}
*/

//==============================================================================

#ifdef YupPlugin_CLAP_Features
static const char* pluginFeatures[] = { YupPlugin_CLAP_Features nullptr };
#else
static const char* pluginFeatures[] = {
#if YupPlugin_IsSynth
    CLAP_PLUGIN_FEATURE_INSTRUMENT,
    CLAP_PLUGIN_FEATURE_SYNTHESIZER,
#else
    CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
#endif
#if YupPlugin_IsMono
    CLAP_PLUGIN_FEATURE_MONO,
#else
    CLAP_PLUGIN_FEATURE_STEREO,
#endif
    nullptr
};
#endif

static const clap_plugin_descriptor_t pluginDescriptor = {
    .clap_version = CLAP_VERSION_INIT,
    .id = YupPlugin_Id,
    .name = YupPlugin_Name,
    .vendor = YupPlugin_Vendor,
    .url = YupPlugin_URL,
    .manual_url = YupPlugin_URL,
    .support_url = YupPlugin_URL,
    .version = YupPlugin_Version,
    .description = YupPlugin_Description,
    .features = pluginFeatures,
};

#if YUP_MAC
static const char* const preferredApi = CLAP_WINDOW_API_COCOA;
#elif YUP_WINDOWS
static const char* const preferredApi = CLAP_WINDOW_API_WIN32;
#elif YUP_LINUX
static const char* const preferredApi = CLAP_WINDOW_API_X11;
#endif

//==============================================================================

class AudioPluginProcessorCLAP;

//==============================================================================

class AudioPluginPlayHeadCLAP final : public AudioPlayHead
{
public:
    explicit AudioPluginPlayHeadCLAP (float sampleRate, const clap_process_t* process)
        : process (process)
        , sampleRate (sampleRate)
    {
    }

    bool canControlTransport() override
    {
        return false;
    }

    void transportPlay (bool shouldSartPlaying) override
    {
        if (! canControlTransport())
            return;
    }

    void transportRecord (bool shouldStartRecording) override
    {
        if (! canControlTransport())
            return;
    }

    void transportRewind() override
    {
        if (! canControlTransport())
            return;
    }

    std::optional<PositionInfo> getPosition() const override
    {
        if (process == nullptr || process->transport == nullptr)
            return {};

        const auto& transport = *process->transport;
        PositionInfo result;

        if ((transport.flags & CLAP_TRANSPORT_HAS_SECONDS_TIMELINE) != 0)
        {
            const auto timeInSeconds = transport.song_pos_seconds / (double) CLAP_SECTIME_FACTOR;
            result.setTimeInSeconds (timeInSeconds);
            result.setTimeInSamples ((int64) (sampleRate * timeInSeconds));
        }

        if ((transport.flags & CLAP_TRANSPORT_HAS_BEATS_TIMELINE) != 0)
        {
            result.setPpqPosition (transport.song_pos_beats / (double) CLAP_BEATTIME_FACTOR);
            result.setPpqPositionOfLastBarStart (transport.bar_start / (double) CLAP_BEATTIME_FACTOR);
            result.setBarCount (transport.bar_number);
        }

        if ((transport.flags & CLAP_TRANSPORT_HAS_TIME_SIGNATURE) != 0)
            result.setTimeSignature (TimeSignature { transport.tsig_num, transport.tsig_denom });

        if ((transport.flags & CLAP_TRANSPORT_HAS_TEMPO) != 0)
            result.setBpm (transport.tempo);

        result.setIsPlaying ((transport.flags & CLAP_TRANSPORT_IS_PLAYING) != 0);
        result.setIsRecording ((transport.flags & CLAP_TRANSPORT_IS_RECORDING) != 0);
        result.setIsLooping ((transport.flags & CLAP_TRANSPORT_IS_LOOP_ACTIVE) != 0);

        if ((transport.flags & CLAP_TRANSPORT_IS_LOOP_ACTIVE) != 0)
            result.setLoopPoints (LoopPoints {
                transport.loop_start_beats / (double) CLAP_BEATTIME_FACTOR,
                transport.loop_end_beats / (double) CLAP_BEATTIME_FACTOR });

        result.setFrameRate (AudioPlayHead::fpsUnknown);

        return result;
    }

private:
    const clap_process_t* process = nullptr;
    float sampleRate = 44100.0f;
};

//==============================================================================

class AudioPluginEditorCLAP final : public Component
{
public:
    AudioPluginEditorCLAP (AudioPluginProcessorCLAP* wrapper, AudioProcessorEditor* editor)
        : wrapper (wrapper)
        , processorEditor (editor)
    {
        addAndMakeVisible (*processorEditor);
    }

    ~AudioPluginEditorCLAP() override
    {
        if (processorEditor != nullptr)
        {
            setVisible (false);
            removeFromDesktop();

            removeChildComponent (processorEditor.get());
            processorEditor.reset();
        }
    }

    AudioProcessorEditor* getAudioProcessorEditor() { return processorEditor.get(); }

    void contentScaleChanged (float dpiScale) override;

    void resized() override;

private:
    ScopedYupInitialiser_Windowing scopeInitialiser;
    AudioPluginProcessorCLAP* wrapper = nullptr;
    std::unique_ptr<AudioProcessorEditor> processorEditor;
};

//==============================================================================

class AudioPluginProcessorCLAP final
    : private AudioParameter::Listener
    , private AudioProcessor::Listener
{
public:
    AudioPluginProcessorCLAP (const clap_host_t* host);
    ~AudioPluginProcessorCLAP();

    bool initialise();
    void destroy();

    bool activate (float sampleRate, int samplesPerBlock);
    void deactivate();

    bool startProcessing();
    void stopProcessing();

    void reset();

    void registerTimer (uint32_t periodMs, clap_id* timerId);
    void unregisterTimer (clap_id timerId);

    const void* getExtension (std::string_view id);
    const clap_plugin_t* getPlugin() const;
    AudioProcessor* getProcessor() const noexcept;

    void editorResized();
    ScopedValueSetter<bool> scopedHostEditorResizing();

private:
    void addParameterListeners();
    void removeParameterListeners();
    bool isValidProcessorParameterIndex (int indexInContainer) const;

    void parameterValueChanged (const AudioParameter::Ptr& parameter, int indexInContainer) override;
    void parameterGestureBegin (const AudioParameter::Ptr& parameter, int indexInContainer) override;
    void parameterGestureEnd (const AudioParameter::Ptr& parameter, int indexInContainer) override;

    void audioProcessorChanged (AudioProcessorBase* processor, const AudioProcessor::ChangeDetails& details) override;

    void enqueueParameterEvent (uint16 eventType, clap_id parameterId, double value = 0.0) noexcept;
    void drainParameterEvents (const clap_output_events_t* out) noexcept;
    void requestParameterFlush() const noexcept;
    void requestMainThreadCallback() const noexcept;
    void handleMainThreadNotifications() noexcept;
    void handleAudioThreadNotifications() noexcept;

    ScopedYupInitialiser_GUI scopeInitialiser;

    std::unique_ptr<AudioProcessor> audioProcessor;
    std::unique_ptr<AudioPluginEditorCLAP> audioPluginEditor;

    const clap_host_t* host = nullptr;

    clap_plugin_t plugin;

    clap_plugin_note_ports_t extensionNotePorts;
    clap_plugin_audio_ports_t extensionAudioPorts;
    clap_plugin_params_t extensionParams;
    clap_plugin_state_t extensionState;
    clap_plugin_tail_t extensionTail;
    clap_plugin_latency_t extensionLatency;
    clap_plugin_timer_support_t extensionTimerSupport;
    clap_plugin_gui_t extensionGUI;
    clap_plugin_render_t extensionRender;
    clap_plugin_voice_info_t extensionVoiceInfo;

    const clap_host_params_t* hostParams = nullptr;
    const clap_host_state_t* hostState = nullptr;
    const clap_host_tail_t* hostTail = nullptr;
    const clap_host_latency_t* hostLatency = nullptr;
    const clap_host_timer_support_t* hostTimerSupport = nullptr;
    const clap_host_gui_t* hostGUI = nullptr;

    clap_id guiTimerId;
    bool hostTriggeredResizing = false;

    MidiBuffer midiEvents;
    ParameterChangeBuffer paramChangeBuffer;
    ParameterChangeBuffer hostParameterChangeBuffer;
    std::vector<AudioParameter::Ptr> listenedParameters;
    std::vector<float*> outputChannelsFloat;
    std::vector<double*> outputChannelsDouble;

    std::vector<AudioBusBufferView<const float>> inputBusViewsFloat;
    std::vector<AudioBusBufferView<float>> outputBusViewsFloat;
    std::vector<AudioBusBufferView<const double>> inputBusViewsDouble;
    std::vector<AudioBusBufferView<double>> outputBusViewsDouble;

    bool isBypassed = false;
    std::atomic<bool> isActive { false };

    //==============================================================================
    /** Returns true when the CLAP audio bus role is Main. */
    bool isCLAPAudioBusMain (uint32_t clapAudioBusIndex, bool isInput) const noexcept
    {
        return audioProcessor->getBusLayout().getAudioBusRole (static_cast<int> (clapAudioBusIndex), isInput) == AudioBus::Role::Main;
    }

    std::atomic<bool> isInsideProcessBlock { false };
    std::atomic<bool> callLatencyChangeOnNextActivate { false };
    std::atomic<bool> tailChangedPending { false };
    std::atomic<bool> stateDirtyPending { false };
    std::atomic<uint32> parameterRescanFlagsPending { 0 };

    struct QueuedParameterEvent
    {
        uint16 eventType = 0;
        clap_id parameterId = CLAP_INVALID_ID;
        double value = 0.0;
    };

    static constexpr int parameterEventQueueSize = 4096;
    AbstractFifo parameterEventFifo { parameterEventQueueSize };
    std::array<QueuedParameterEvent, parameterEventQueueSize> parameterEvents {};

    static std::atomic_int instancesCount;
};

//==============================================================================

std::atomic_int AudioPluginProcessorCLAP::instancesCount = 0;

//==============================================================================

AudioPluginProcessorCLAP* getWrapper (const clap_plugin_t* plugin)
{
    return reinterpret_cast<AudioPluginProcessorCLAP*> (plugin->plugin_data);
}

//==============================================================================

AudioPluginProcessorCLAP::AudioPluginProcessorCLAP (const clap_host_t* host)
    : host (host)
{
    jassert (host != nullptr);

    plugin.desc = &pluginDescriptor;
    plugin.plugin_data = this;

    plugin.init = [] (const clap_plugin* plugin) -> bool
    {
        return getWrapper (plugin)->initialise();
    };

    plugin.destroy = [] (const clap_plugin* plugin)
    {
        getWrapper (plugin)->destroy();
    };

    plugin.activate = [] (const clap_plugin* plugin, double sampleRate, uint32_t minimumFramesCount, uint32_t maximumFramesCount) -> bool
    {
        return getWrapper (plugin)->activate (static_cast<float> (sampleRate), static_cast<int> (maximumFramesCount));
    };

    plugin.deactivate = [] (const clap_plugin* plugin)
    {
        getWrapper (plugin)->deactivate();
    };

    plugin.start_processing = [] (const clap_plugin* plugin) -> bool
    {
        return getWrapper (plugin)->startProcessing();
    };

    plugin.stop_processing = [] (const clap_plugin* plugin)
    {
        getWrapper (plugin)->stopProcessing();
    };

    plugin.reset = [] (const clap_plugin* plugin)
    {
        getWrapper (plugin)->reset();
    };

    plugin.process = [] (const clap_plugin* plugin, const clap_process_t* process) -> clap_process_status
    {
        auto wrapper = getWrapper (plugin);

        auto& audioProcessor = *wrapper->audioProcessor;
        auto& midiBuffer = wrapper->midiEvents;

        wrapper->handleAudioThreadNotifications();
        wrapper->drainParameterEvents (process->out_events);

        auto lock = CriticalSection::ScopedTryLockType (audioProcessor.getProcessLock());
        if (! lock.isLocked() || audioProcessor.isSuspended())
            return CLAP_PROCESS_CONTINUE;

        jassert (process->audio_outputs_count == audioProcessor.getNumAudioOutputs());
        jassert (process->audio_inputs_count == audioProcessor.getNumAudioInputs());

        // Process incoming parameter and MIDI events (CLAP guarantees time-sorted order)
        midiBuffer.clear();
        wrapper->paramChangeBuffer.clear();
        wrapper->hostParameterChangeBuffer.clear();

        bool bypassed = wrapper->isBypassed;
        const auto bypassParameterID = getBypassHostParameterID (audioProcessor);

        const uint32_t inputEventCount = process->in_events->size (process->in_events);
        for (uint32_t eventIndex = 0; eventIndex < inputEventCount; ++eventIndex)
        {
            const clap_event_header_t* event = process->in_events->get (process->in_events, eventIndex);

            if (event->space_id != CLAP_CORE_EVENT_SPACE_ID)
                continue;

            if (event->type == CLAP_EVENT_PARAM_VALUE)
            {
                const auto* paramEvent = reinterpret_cast<const clap_event_param_value_t*> (event);

                if (paramEvent->param_id == bypassParameterID)
                {
                    bypassed = paramEvent->value >= 0.5;
                    wrapper->isBypassed = bypassed;
                    continue;
                }

                addParameterChangeByCLAPValue (audioProcessor,
                                               wrapper->paramChangeBuffer,
                                               paramEvent->param_id,
                                               static_cast<float> (paramEvent->value),
                                               static_cast<int> (event->time));

                addParameterChangeByCLAPValue (audioProcessor,
                                               wrapper->hostParameterChangeBuffer,
                                               paramEvent->param_id,
                                               static_cast<float> (paramEvent->value),
                                               static_cast<int> (event->time));
            }
            else if (event->type == CLAP_EVENT_PARAM_MOD)
            {
                const auto* modEvent = reinterpret_cast<const clap_event_param_mod_t*> (event);
                addParameterModByCLAPEvent (audioProcessor, wrapper->paramChangeBuffer, modEvent);
            }
            else if (auto convertedEvent = clapEventToMidiMessage (event))
            {
                midiBuffer.addEvent (*convertedEvent, static_cast<int> (event->time));
            }
        }

        // CLAP events arrive sorted - no sort needed; apply final values for backward compat
        applyParameterChangesToProcessor (audioProcessor, wrapper->hostParameterChangeBuffer);

        AudioPluginPlayHeadCLAP playHead (audioProcessor.getSampleRate(), process);
        auto* const playHeadPtr = process->transport != nullptr ? &playHead : nullptr;

        const bool useDoublePrecision = audioProcessor.supportsDoublePrecisionProcessing()
                                     && process->audio_outputs_count > 0
                                     && process->audio_outputs[0].data64 != nullptr;

        if (useDoublePrecision)
        {
            // Copy main input audio into matching main output buffers for effects.
            // Auxiliary (sidechain) inputs are NOT copied to outputs.
            for (uint32_t busIdx = 0; busIdx < std::min (process->audio_inputs_count, process->audio_outputs_count); ++busIdx)
            {
                if (! wrapper->isCLAPAudioBusMain (busIdx, true))
                    continue;

                const auto& inBus = process->audio_inputs[busIdx];
                const auto& outBus = process->audio_outputs[busIdx];
                const uint32_t chCount = std::min (inBus.channel_count, outBus.channel_count);

                for (uint32_t ch = 0; ch < chCount; ++ch)
                {
                    const auto* in = inBus.data64[ch];
                    auto* out = outBus.data64[ch];
                    if (in != out)
                        std::memcpy (out, in, process->frames_count * sizeof (double));
                }
            }

            wrapper->outputChannelsDouble.clear();
            for (uint32_t busIdx = 0; busIdx < process->audio_outputs_count; ++busIdx)
                for (uint32_t ch = 0; ch < process->audio_outputs[busIdx].channel_count; ++ch)
                    wrapper->outputChannelsDouble.push_back (process->audio_outputs[busIdx].data64[ch]);

            AudioBuffer<double> audioBuffer (wrapper->outputChannelsDouble.data(),
                                             static_cast<int> (wrapper->outputChannelsDouble.size()),
                                             0,
                                             static_cast<int> (process->frames_count));

            // Build per-bus input views (all input buses)
            wrapper->inputBusViewsDouble.clear();
            for (uint32_t busIdx = 0; busIdx < process->audio_inputs_count; ++busIdx)
            {
                const auto& inBus = process->audio_inputs[busIdx];
                const bool isSilent = inBus.constant_mask != 0; // CLAP silence flag
                wrapper->inputBusViewsDouble.emplace_back (
                    isSilent ? nullptr : reinterpret_cast<const double* const*> (inBus.data64),
                    static_cast<int> (inBus.channel_count),
                    audioProcessor.getBusLayout().getAudioBusRole (static_cast<int> (busIdx), true));
            }

            // Build per-bus output views (all output buses)
            wrapper->outputBusViewsDouble.clear();
            for (uint32_t busIdx = 0; busIdx < process->audio_outputs_count; ++busIdx)
            {
                const auto& outBus = process->audio_outputs[busIdx];
                wrapper->outputBusViewsDouble.emplace_back (
                    reinterpret_cast<double* const*> (outBus.data64),
                    static_cast<int> (outBus.channel_count),
                    audioProcessor.getBusLayout().getAudioBusRole (static_cast<int> (busIdx), false));
            }

            AudioProcessContext<double> context {
                audioBuffer,
                midiBuffer,
                wrapper->paramChangeBuffer,
                playHeadPtr,
                { wrapper->inputBusViewsDouble.data(), wrapper->inputBusViewsDouble.size() },
                { wrapper->outputBusViewsDouble.data(), wrapper->outputBusViewsDouble.size() }
            };

            wrapper->isInsideProcessBlock.store (true);
            processAudioBlock (audioProcessor, context, bypassed);
            wrapper->isInsideProcessBlock.store (false);
        }
        else
        {
            // Copy main input audio into matching main output buffers for effects.
            // Auxiliary (sidechain) inputs are NOT copied to outputs.
            for (uint32_t busIdx = 0; busIdx < std::min (process->audio_inputs_count, process->audio_outputs_count); ++busIdx)
            {
                if (! wrapper->isCLAPAudioBusMain (busIdx, true))
                    continue;

                const auto& inBus = process->audio_inputs[busIdx];
                const auto& outBus = process->audio_outputs[busIdx];
                const uint32_t chCount = std::min (inBus.channel_count, outBus.channel_count);

                for (uint32_t ch = 0; ch < chCount; ++ch)
                {
                    const auto* in = inBus.data32[ch];
                    auto* out = outBus.data32[ch];
                    if (in != out)
                        std::memcpy (out, in, process->frames_count * sizeof (float));
                }
            }

            wrapper->outputChannelsFloat.clear();
            for (uint32_t busIdx = 0; busIdx < process->audio_outputs_count; ++busIdx)
                for (uint32_t ch = 0; ch < process->audio_outputs[busIdx].channel_count; ++ch)
                    wrapper->outputChannelsFloat.push_back (process->audio_outputs[busIdx].data32[ch]);

            AudioSampleBuffer audioBuffer (wrapper->outputChannelsFloat.data(),
                                           static_cast<int> (wrapper->outputChannelsFloat.size()),
                                           0,
                                           static_cast<int> (process->frames_count));

            // Build per-bus input views (all input buses)
            wrapper->inputBusViewsFloat.clear();
            for (uint32_t busIdx = 0; busIdx < process->audio_inputs_count; ++busIdx)
            {
                const auto& inBus = process->audio_inputs[busIdx];
                const bool isSilent = inBus.constant_mask != 0; // CLAP silence flag
                wrapper->inputBusViewsFloat.emplace_back (
                    isSilent ? nullptr : reinterpret_cast<const float* const*> (inBus.data32),
                    static_cast<int> (inBus.channel_count),
                    audioProcessor.getBusLayout().getAudioBusRole (static_cast<int> (busIdx), true));
            }

            // Build per-bus output views (all output buses)
            wrapper->outputBusViewsFloat.clear();
            for (uint32_t busIdx = 0; busIdx < process->audio_outputs_count; ++busIdx)
            {
                const auto& outBus = process->audio_outputs[busIdx];
                wrapper->outputBusViewsFloat.emplace_back (
                    reinterpret_cast<float* const*> (outBus.data32),
                    static_cast<int> (outBus.channel_count),
                    audioProcessor.getBusLayout().getAudioBusRole (static_cast<int> (busIdx), false));
            }

            AudioProcessContext<float> context {
                audioBuffer,
                midiBuffer,
                wrapper->paramChangeBuffer,
                playHeadPtr,
                { wrapper->inputBusViewsFloat.data(), wrapper->inputBusViewsFloat.size() },
                { wrapper->outputBusViewsFloat.data(), wrapper->outputBusViewsFloat.size() }
            };

            wrapper->isInsideProcessBlock.store (true);
            processAudioBlock (audioProcessor, context, bypassed);
            wrapper->isInsideProcessBlock.store (false);
        }

        wrapper->handleAudioThreadNotifications();
        wrapper->drainParameterEvents (process->out_events);

        // Send output events back to host
        if (process->out_events != nullptr)
        {
            for (const MidiMessageMetadata metadata : midiBuffer)
            {
                const auto& message = metadata.getMessage();

                if (message.isNoteOff())
                {
                    clap_event_note_t ev = {};
                    ev.header.size = sizeof (ev);
                    ev.header.time = static_cast<uint32_t> (metadata.samplePosition);
                    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                    ev.header.type = CLAP_EVENT_NOTE_END;
                    ev.header.flags = 0;
                    ev.note_id = -1;
                    ev.key = message.getNoteNumber();
                    ev.channel = message.getChannel() - 1;
                    ev.port_index = 0;
                    process->out_events->try_push (process->out_events, &ev.header);
                }
                else if (message.getRawDataSize() > 0 && message.getRawDataSize() <= 3)
                {
                    clap_event_midi_t ev = {};
                    ev.header.size = sizeof (ev);
                    ev.header.time = static_cast<uint32_t> (metadata.samplePosition);
                    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                    ev.header.type = CLAP_EVENT_MIDI;
                    ev.header.flags = 0;
                    ev.port_index = 0;
                    std::memcpy (ev.data, message.getRawData(), static_cast<size_t> (message.getRawDataSize()));
                    process->out_events->try_push (process->out_events, &ev.header);
                }
            }
        }

        return CLAP_PROCESS_CONTINUE;
    };

    plugin.get_extension = [] (const clap_plugin* plugin, const char* id) -> const void*
    {
        return getWrapper (plugin)->getExtension (id);
    };

    plugin.on_main_thread = [] (const clap_plugin* plugin)
    {
        getWrapper (plugin)->handleMainThreadNotifications();
    };
}

//==============================================================================

AudioPluginProcessorCLAP::~AudioPluginProcessorCLAP()
{
    endActiveParameterGestures (audioProcessor.get());
}

//==============================================================================

bool AudioPluginProcessorCLAP::initialise()
{
    jassert (audioProcessor == nullptr);

    audioProcessor.reset (::YUP_AUDIO_PLUGIN_CREATE_FUNCTION());
    if (audioProcessor == nullptr)
        return false;

    // ==== Setup extensions: parameters
    extensionParams.count = [] (const clap_plugin_t* plugin) -> uint32_t
    {
        return static_cast<uint32_t> (getWrapper (plugin)->audioProcessor->getParameters().size() + 1);
    };

    extensionParams.get_info = [] (const clap_plugin_t* plugin, uint32_t index, clap_param_info_t* information) -> bool
    {
        std::memset (information, 0, sizeof (clap_param_info_t));

        auto wrapper = getWrapper (plugin);
        auto parameters = wrapper->audioProcessor->getParameters();

        if (index > static_cast<uint32_t> (parameters.size()))
            return false;

        if (index == static_cast<uint32_t> (parameters.size()))
        {
            information->id = getBypassHostParameterID (*wrapper->audioProcessor);
            information->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_BYPASS;
            information->min_value = 0.0;
            information->max_value = 1.0;
            information->default_value = 0.0;
            std::snprintf (information->name, sizeof (information->name), "%s", "Bypass");
            return true;
        }

        auto& parameter = parameters[index];

        information->id = parameter->getHostParameterID();
        information->cookie = parameter.get();
        information->flags = getCLAPParameterFlags (*parameter);
        information->min_value = parameter->getMinimumValue();
        information->max_value = parameter->getMaximumValue();
        information->default_value = parameter->getDefaultValue();
        parameter->getName().copyToUTF8 (information->name, CLAP_NAME_SIZE);
        parameter->getModulePath().copyToUTF8 (information->module, CLAP_PATH_SIZE);

        return true;
    };

    extensionParams.get_value = [] (const clap_plugin_t* plugin, clap_id parameterId, double* value) -> bool
    {
        auto wrapper = getWrapper (plugin);
        auto parameters = wrapper->audioProcessor->getParameters();

        const auto parameterIndex = wrapper->audioProcessor->getParameterIndexByHostID (parameterId);
        if (! isPositiveAndBelow (parameterIndex, static_cast<int> (parameters.size())))
        {
            if (parameterId == getBypassHostParameterID (*wrapper->audioProcessor))
            {
                *value = wrapper->isBypassed ? 1.0 : 0.0;
                return true;
            }

            return false;
        }

        *value = parameters[parameterIndex]->getValue();

        return true;
    };

    extensionParams.value_to_text = [] (const clap_plugin_t* plugin, clap_id parameterId, double value, char* display, uint32_t size) -> bool
    {
        auto wrapper = getWrapper (plugin);
        auto parameters = wrapper->audioProcessor->getParameters();

        const auto parameterIndex = wrapper->audioProcessor->getParameterIndexByHostID (parameterId);
        if (! isPositiveAndBelow (parameterIndex, static_cast<int> (parameters.size())))
        {
            if (parameterId == getBypassHostParameterID (*wrapper->audioProcessor))
            {
                const auto text = value >= 0.5 ? String ("On") : String ("Off");
                text.copyToUTF8 (display, size);
                return true;
            }

            return false;
        }

        const auto text = parameters[parameterIndex]->convertToString (static_cast<float> (value));
        text.copyToUTF8 (display, size);

        return true;
    };

    extensionParams.text_to_value = [] (const clap_plugin_t* plugin, clap_id parameterId, const char* display, double* value) -> bool
    {
        auto wrapper = getWrapper (plugin);
        auto parameters = wrapper->audioProcessor->getParameters();

        const auto parameterIndex = wrapper->audioProcessor->getParameterIndexByHostID (parameterId);
        if (! isPositiveAndBelow (parameterIndex, static_cast<int> (parameters.size())))
        {
            if (parameterId == getBypassHostParameterID (*wrapper->audioProcessor))
            {
                const String text (display);
                *value = (text == "On" || text == "1") ? 1.0 : 0.0;
                return true;
            }

            return false;
        }

        *value = static_cast<double> (parameters[parameterIndex]->convertFromString (display));

        return true;
    };

    extensionParams.flush = [] (const clap_plugin_t* plugin, const clap_input_events_t* in, const clap_output_events_t* out)
    {
        auto wrapper = getWrapper (plugin);

        wrapper->drainParameterEvents (out);

        if (in == nullptr)
            return;

        const uint32_t count = in->size (in);
        const auto bypassParameterID = getBypassHostParameterID (*wrapper->audioProcessor);

        for (uint32_t i = 0; i < count; ++i)
        {
            const clap_event_header_t* event = in->get (in, i);

            if (event->space_id != CLAP_CORE_EVENT_SPACE_ID)
                continue;

            if (event->type == CLAP_EVENT_PARAM_VALUE)
            {
                const auto* paramEvent = reinterpret_cast<const clap_event_param_value_t*> (event);

                if (paramEvent->param_id == bypassParameterID)
                {
                    wrapper->isBypassed = paramEvent->value >= 0.5;
                    continue;
                }

                clapEventToParameterChange (event, *wrapper->audioProcessor);
            }
            else if (event->type == CLAP_EVENT_PARAM_MOD)
            {
                // Modulation is transient audio-block data; there is no processor context during flush().
            }
        }
    };

    // ==== Setup extensions: note ports
    extensionNotePorts.count = [] (const clap_plugin_t* plugin, bool isInput) -> uint32_t
    {
        auto wrapper = getWrapper (plugin);
        const auto& busses = isInput
                               ? wrapper->audioProcessor->getBusLayout().getInputBuses()
                               : wrapper->audioProcessor->getBusLayout().getOutputBuses();

        uint32_t count = 0;
        for (const auto& bus : busses)
            if (bus.getType() == AudioBus::Type::Midi)
                ++count;

        // Fallback: synths with no declared MIDI input bus always get one
        if (isInput && count == 0 && YupPlugin_IsSynth)
            return 1;

        return count;
    };

    extensionNotePorts.get = [] (const clap_plugin_t* plugin, uint32_t index, bool isInput, clap_note_port_info_t* info) -> bool
    {
        auto wrapper = getWrapper (plugin);
        const auto& busses = isInput
                               ? wrapper->audioProcessor->getBusLayout().getInputBuses()
                               : wrapper->audioProcessor->getBusLayout().getOutputBuses();

        uint32_t midiIndex = 0;
        for (const auto& bus : busses)
        {
            if (bus.getType() != AudioBus::Type::Midi)
                continue;

            if (midiIndex == index)
            {
                info->id = index;
                info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
                info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
                bus.getName().copyToUTF8 (info->name, sizeof (info->name));
                return true;
            }

            ++midiIndex;
        }

        // Fallback port for synths without declared MIDI buses
        if (isInput && index == 0 && midiIndex == 0 && YupPlugin_IsSynth)
        {
            info->id = 0;
            info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
            info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
            std::snprintf (info->name, sizeof (info->name), "%s", "Midi In");
            return true;
        }

        return false;
    };

    // ==== Setup extensions: audio ports
    extensionAudioPorts.count = [] (const clap_plugin_t* plugin, bool isInput) -> uint32_t
    {
        auto wrapper = getWrapper (plugin);
        auto* audioProcessor = wrapper->audioProcessor.get();

        Span<const AudioBus> busses = isInput
                                        ? audioProcessor->getBusLayout().getInputBuses()
                                        : audioProcessor->getBusLayout().getOutputBuses();

        uint32_t count = 0;
        for (const auto& bus : busses)
            if (bus.getType() == AudioBus::Type::Audio)
                ++count;

        return count;
    };

    extensionAudioPorts.get = [] (const clap_plugin_t* plugin, uint32_t index, bool isInput, clap_audio_port_info_t* info) -> bool
    {
        auto wrapper = getWrapper (plugin);
        auto* audioProcessor = wrapper->audioProcessor.get();

        Span<const AudioBus> busses = isInput
                                        ? audioProcessor->getBusLayout().getInputBuses()
                                        : audioProcessor->getBusLayout().getOutputBuses();

        const AudioBus* audioBus = nullptr;
        uint32_t audioBusIndex = 0;

        for (const auto& bus : busses)
        {
            if (bus.getType() != AudioBus::Type::Audio)
                continue;

            if (audioBusIndex == index)
            {
                audioBus = &bus;
                break;
            }

            ++audioBusIndex;
        }

        if (audioBus == nullptr)
            return false;

        info->id = index;
        info->channel_count = audioBus->getNumChannels();

        uint32_t flags = audioBus->getRole() == AudioBus::Role::Main
                           ? CLAP_AUDIO_PORT_IS_MAIN
                           : 0;
        if (audioProcessor->supportsDoublePrecisionProcessing())
            flags |= CLAP_AUDIO_PORT_SUPPORTS_64BITS | CLAP_AUDIO_PORT_PREFERS_64BITS | CLAP_AUDIO_PORT_REQUIRES_COMMON_SAMPLE_SIZE;
        info->flags = flags;

        info->port_type = audioBus->isStereo() ? CLAP_PORT_STEREO : CLAP_PORT_MONO;

        // For output ports, advertise in-place processing when a corresponding input bus exists
        if (! isInput)
        {
            uint32_t inputAudioCount = 0;
            for (const auto& bus : audioProcessor->getBusLayout().getInputBuses())
                if (bus.getType() == AudioBus::Type::Audio)
                    ++inputAudioCount;
            info->in_place_pair = (inputAudioCount > index) ? static_cast<clap_id> (index) : CLAP_INVALID_ID;
        }
        else
        {
            info->in_place_pair = CLAP_INVALID_ID;
        }

        audioBus->getName().copyToUTF8 (info->name, sizeof (info->name));

        return true;
    };

    // ==== Setup extensions: state
    extensionState.save = [] (const clap_plugin_t* plugin, const clap_ostream_t* stream) -> bool
    {
        auto wrapper = getWrapper (plugin);
        MemoryBlock data;

        wrapper->audioProcessor->suspendProcessing (true);
        const bool saved = wrapper->audioProcessor->saveStateIntoMemory (data).wasOk();
        wrapper->audioProcessor->suspendProcessing (false);

        const auto wrapperState = writeWrapperBypassState (clapWrapperStateMagic,
                                                           clapWrapperStateVersion,
                                                           wrapper->isBypassed,
                                                           data,
                                                           saved);

        return writeAllToCLAPStream (stream, wrapperState.getData(), wrapperState.getSize());
    };

    extensionState.load = [] (const clap_plugin_t* plugin, const clap_istream_t* stream) -> bool
    {
        auto wrapper = getWrapper (plugin);
        MemoryBlock data;

        char buf[4096];
        for (;;)
        {
            const int64_t n = stream->read (stream, buf, sizeof (buf));
            if (n < 0)
                return false;

            if (n == 0)
                break;

            data.append (buf, static_cast<size_t> (n));
        }

        if (data.isEmpty())
            return false;

        const auto wrapperState = readWrapperBypassState (data, clapWrapperStateMagic, clapWrapperStateVersion);
        if (wrapperState.hasWrapperState)
        {
            wrapper->isBypassed = wrapperState.isBypassed;

            if (! wrapperState.hasProcessorState)
                return true;
        }

        const auto& processorState = wrapperState.hasWrapperState ? wrapperState.processorState : data;

        wrapper->audioProcessor->suspendProcessing (true);
        const bool ok = wrapper->audioProcessor->loadStateFromMemory (processorState).wasOk();
        wrapper->audioProcessor->suspendProcessing (false);

        return ok;
    };

    // ==== Setup extensions: tail
    extensionTail.get = [] (const clap_plugin_t* plugin) -> uint32_t
    {
        auto wrapper = getWrapper (plugin);
        return static_cast<uint32_t> (wrapper->audioProcessor->getTailSamples());
    };

    // ==== Setup extensions: latency
    extensionLatency.get = [] (const clap_plugin_t* plugin) -> uint32_t
    {
        auto wrapper = getWrapper (plugin);
        return static_cast<uint32_t> (wrapper->audioProcessor->getLatencySamples());
    };

    // ==== Setup extensions: timer support
    extensionTimerSupport.on_timer = [] (const clap_plugin_t* plugin, clap_id timerId)
    {
#if YUP_LINUX
        if (auto wrapper = getWrapper (plugin); wrapper->guiTimerId == timerId)
            MessageManager::getInstance()->runDispatchLoopUntil (10);
#endif
    };

    // ==== Setup extensions: render
    extensionRender.has_hard_realtime_requirement = [] (const clap_plugin_t*) -> bool
    {
        return false;
    };

    extensionRender.set = [] (const clap_plugin_t* plugin, clap_plugin_render_mode mode) -> bool
    {
        getWrapper (plugin)->audioProcessor->setOfflineProcessing (mode == CLAP_RENDER_OFFLINE);
        return true;
    };

    // ==== Setup extensions: voice info
    extensionVoiceInfo.get = [] (const clap_plugin_t* plugin, clap_voice_info_t* info) -> bool
    {
        const int voices = getWrapper (plugin)->audioProcessor->getNumVoices();
        if (voices <= 0)
            return false;

        info->voice_count = static_cast<uint32_t> (voices);
        info->voice_capacity = static_cast<uint32_t> (voices);
        info->flags = CLAP_VOICE_INFO_SUPPORTS_OVERLAPPING_NOTES;
        return true;
    };

    // ==== Setup extensions: gui
    extensionGUI.is_api_supported = [] (const clap_plugin_t* plugin, const char* api, bool isFloating) -> bool
    {
        auto wrapper = getWrapper (plugin);
        if (wrapper->audioProcessor == nullptr || ! wrapper->audioProcessor->hasEditor())
            return false;

        return std::string_view (api) == preferredApi && ! isFloating;
    };

    extensionGUI.get_preferred_api = [] (const clap_plugin_t* plugin, const char** api, bool* isFloating) -> bool
    {
        *api = preferredApi;
        *isFloating = false;
        return true;
    };

    extensionGUI.create = [] (const clap_plugin_t* plugin, const char* api, bool isFloating) -> bool
    {
        if (api == nullptr || std::string_view (api) != preferredApi || isFloating)
            return false;

        auto wrapper = getWrapper (plugin);

        auto processorEditor = wrapper->audioProcessor->createEditor();
        if (processorEditor == nullptr)
            return false;

        wrapper->audioPluginEditor = std::make_unique<AudioPluginEditorCLAP> (wrapper, processorEditor);

        if (isFloating)
        {
            auto audioProcessorEditor = wrapper->audioPluginEditor->getAudioProcessorEditor();
            if (audioProcessorEditor == nullptr)
                return false;

            ComponentNative::Flags flags = ComponentNative::defaultFlags;

            if (audioProcessorEditor->shouldRenderContinuous())
                flags.set (ComponentNative::renderContinuous);

            auto options = ComponentNative::Options()
                               .withFlags (flags)
                               .withResizableWindow (audioProcessorEditor->isResizable());

            wrapper->audioPluginEditor->addToDesktop (options);
            wrapper->audioPluginEditor->setVisible (true);

            audioProcessorEditor->attachedToNative();
        }

        return true;
    };

    extensionGUI.destroy = [] (const clap_plugin_t* plugin)
    {
        auto wrapper = getWrapper (plugin);
        endActiveParameterGestures (wrapper->audioProcessor.get());
        wrapper->audioPluginEditor.reset();
    };

    extensionGUI.set_scale = [] (const clap_plugin_t* plugin, double scale) -> bool
    {
        auto wrapper = getWrapper (plugin);
        if (wrapper->audioPluginEditor == nullptr)
            return false;

        wrapper->audioPluginEditor->contentScaleChanged (static_cast<float> (scale));
        return true;
    };

    extensionGUI.get_size = [] (const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) -> bool
    {
        auto wrapper = getWrapper (plugin);
        if (wrapper->audioPluginEditor == nullptr)
            return false;

        auto audioProcessorEditor = wrapper->audioPluginEditor->getAudioProcessorEditor();

        if (audioProcessorEditor->isResizable() && audioProcessorEditor->getWidth() != 0)
        {
            *width = static_cast<uint32_t> (audioProcessorEditor->getWidth());
            *height = static_cast<uint32_t> (audioProcessorEditor->getHeight());
        }
        else
        {
            *width = static_cast<uint32_t> (audioProcessorEditor->getPreferredSize().getWidth());
            *height = static_cast<uint32_t> (audioProcessorEditor->getPreferredSize().getHeight());
        }

        return true;
    };

    extensionGUI.can_resize = [] (const clap_plugin_t* plugin) -> bool
    {
        auto wrapper = getWrapper (plugin);
        if (wrapper->audioPluginEditor == nullptr)
            return false;

        return wrapper->audioPluginEditor->getAudioProcessorEditor()->isResizable();
    };

    extensionGUI.get_resize_hints = [] (const clap_plugin_t* plugin, clap_gui_resize_hints_t* hints) -> bool
    {
        auto wrapper = getWrapper (plugin);
        if (wrapper->audioPluginEditor == nullptr)
            return false;

        auto audioProcessorEditor = wrapper->audioPluginEditor->getAudioProcessorEditor();

        hints->can_resize_horizontally = audioProcessorEditor->isResizable();
        hints->can_resize_vertically = audioProcessorEditor->isResizable();
        hints->preserve_aspect_ratio = audioProcessorEditor->shouldPreserveAspectRatio();
        hints->aspect_ratio_width = audioProcessorEditor->getPreferredSize().getWidth();
        hints->aspect_ratio_height = audioProcessorEditor->getPreferredSize().getHeight();

        return true;
    };

    extensionGUI.adjust_size = [] (const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) -> bool
    {
        auto wrapper = getWrapper (plugin);
        if (wrapper->audioPluginEditor == nullptr)
            return false;

        auto audioProcessorEditor = wrapper->audioPluginEditor->getAudioProcessorEditor();

        const auto preferredSize = audioProcessorEditor->getPreferredSize();

        if (! audioProcessorEditor->isResizable())
        {
            *width = static_cast<uint32_t> (preferredSize.getWidth());
            *height = static_cast<uint32_t> (preferredSize.getHeight());
        }
        else if (audioProcessorEditor->shouldPreserveAspectRatio())
        {
            if (preferredSize.getWidth() > preferredSize.getHeight())
                *height = static_cast<uint32_t> (*width * (preferredSize.getWidth() / static_cast<float> (preferredSize.getHeight())));
            else
                *width = static_cast<uint32_t> (*height * (preferredSize.getHeight() / static_cast<float> (preferredSize.getWidth())));
        }

        return true;
    };

    extensionGUI.set_size = [] (const clap_plugin_t* plugin, uint32_t width, uint32_t height) -> bool
    {
        auto wrapper = getWrapper (plugin);
        if (wrapper->audioPluginEditor == nullptr)
            return false;

        auto audioProcessorEditor = wrapper->audioPluginEditor->getAudioProcessorEditor();

        if (! audioProcessorEditor->isResizable())
        {
            const auto preferredSize = audioProcessorEditor->getPreferredSize();

            width = static_cast<uint32_t> (preferredSize.getWidth());
            height = static_cast<uint32_t> (preferredSize.getHeight());
        }

        const auto scoped = wrapper->scopedHostEditorResizing();

        wrapper->audioPluginEditor->setSize ({ static_cast<float> (width), static_cast<float> (height) });

        return true;
    };

    extensionGUI.set_parent = [] (const clap_plugin_t* plugin, const clap_window_t* window) -> bool
    {
        jassert (std::string_view (window->api) == preferredApi);

        auto wrapper = getWrapper (plugin);
        if (wrapper->audioPluginEditor == nullptr)
            return false;

        auto audioProcessorEditor = wrapper->audioPluginEditor->getAudioProcessorEditor();
        if (audioProcessorEditor == nullptr)
            return false;

        ComponentNative::Flags flags = ComponentNative::defaultFlags & ~ComponentNative::decoratedWindow;

        if (audioProcessorEditor->shouldRenderContinuous())
            flags.set (ComponentNative::renderContinuous);

        auto options = ComponentNative::Options()
                           .withFlags (flags)
                           .withResizableWindow (audioProcessorEditor->isResizable());

        wrapper->audioPluginEditor->addToDesktop (
            options,
#if YUP_MAC
            window->cocoa);
#elif YUP_WINDOWS
            window->win32);
#elif YUP_LINUX
            reinterpret_cast<void*> (window->x11));
#else
            nullptr);
#endif

        wrapper->audioPluginEditor->setVisible (true);

        audioProcessorEditor->attachedToNative();

        return true;
    };

    extensionGUI.set_transient = [] (const clap_plugin_t* plugin, const clap_window_t* window) -> bool
    {
        return false;
    };

    extensionGUI.suggest_title = [] (const clap_plugin_t* plugin, const char* title) {};

    extensionGUI.show = [] (const clap_plugin_t* plugin) -> bool
    {
        auto wrapper = getWrapper (plugin);
        if (wrapper->audioPluginEditor == nullptr)
            return false;

        wrapper->audioPluginEditor->setVisible (true);
        return true;
    };

    extensionGUI.hide = [] (const clap_plugin_t* plugin) -> bool
    {
        auto wrapper = getWrapper (plugin);
        if (wrapper->audioPluginEditor == nullptr)
            return false;

        wrapper->audioPluginEditor->setVisible (false);
        return true;
    };

    // ==== Setup extensions: host
    hostParams = reinterpret_cast<const clap_host_params_t*> (host->get_extension (host, CLAP_EXT_PARAMS));
    hostState = reinterpret_cast<const clap_host_state_t*> (host->get_extension (host, CLAP_EXT_STATE));
    hostTail = reinterpret_cast<const clap_host_tail_t*> (host->get_extension (host, CLAP_EXT_TAIL));
    hostLatency = reinterpret_cast<const clap_host_latency_t*> (host->get_extension (host, CLAP_EXT_LATENCY));
    hostTimerSupport = reinterpret_cast<const clap_host_timer_support_t*> (host->get_extension (host, CLAP_EXT_TIMER_SUPPORT));
    hostGUI = reinterpret_cast<const clap_host_gui_t*> (host->get_extension (host, CLAP_EXT_GUI));

    audioProcessor->addListener (this);
    addParameterListeners();

#if YUP_LINUX
    if (instancesCount.fetch_add (1) == 0)
        registerTimer (16, &guiTimerId);
#endif

    return true;
}

//==============================================================================

void AudioPluginProcessorCLAP::destroy()
{
    removeParameterListeners();

    if (audioProcessor != nullptr)
        audioProcessor->removeListener (this);

#if YUP_LINUX
    if (instancesCount.fetch_sub (1) == 1)
        unregisterTimer (guiTimerId);
#endif

    plugin.plugin_data = nullptr;
    delete this;
}

//==============================================================================

bool AudioPluginProcessorCLAP::activate (float sampleRate, int samplesPerBlock)
{
    audioProcessor->setPlaybackConfiguration (sampleRate, samplesPerBlock);

    if (callLatencyChangeOnNextActivate.exchange (false)
        && hostLatency != nullptr
        && hostLatency->changed != nullptr)
    {
        hostLatency->changed (host);
    }

    midiEvents.ensureSize (4096);
    paramChangeBuffer.reserve (getDefaultParameterChangeCapacity (*audioProcessor));
    hostParameterChangeBuffer.reserve (getDefaultParameterChangeCapacity (*audioProcessor));

    const int totalOutputChannels = getTotalAudioOutputChannels (*audioProcessor);
    outputChannelsFloat.reserve (static_cast<size_t> (totalOutputChannels));
    outputChannelsDouble.reserve (static_cast<size_t> (totalOutputChannels));

    // Pre-allocate per-bus view storage
    {
        const auto numAudioInputs = static_cast<size_t> (audioProcessor->getNumAudioInputs());
        const auto numAudioOutputs = static_cast<size_t> (audioProcessor->getNumAudioOutputs());

        inputBusViewsFloat.reserve (numAudioInputs);
        outputBusViewsFloat.reserve (numAudioOutputs);
        inputBusViewsDouble.reserve (numAudioInputs);
        outputBusViewsDouble.reserve (numAudioOutputs);
    }

    isActive.store (true);

    return true;
}

//==============================================================================

void AudioPluginProcessorCLAP::deactivate()
{
    isActive.store (false);
    audioProcessor->releaseResources();
}

//==============================================================================

bool AudioPluginProcessorCLAP::startProcessing()
{
    audioProcessor->suspendProcessing (false);
    return true;
}

//==============================================================================

void AudioPluginProcessorCLAP::stopProcessing()
{
    audioProcessor->suspendProcessing (true);
}

//==============================================================================

void AudioPluginProcessorCLAP::reset()
{
    audioProcessor->flush(); // TODO - should we just call releaseResources()?
}

//==============================================================================

void AudioPluginProcessorCLAP::registerTimer (uint32_t periodMs, clap_id* timerId)
{
    if (hostTimerSupport != nullptr && hostTimerSupport->register_timer)
        hostTimerSupport->register_timer (host, periodMs, timerId);
}

void AudioPluginProcessorCLAP::unregisterTimer (clap_id timerId)
{
    if (hostTimerSupport != nullptr && hostTimerSupport->register_timer)
        hostTimerSupport->unregister_timer (host, timerId);
}

//==============================================================================

const void* AudioPluginProcessorCLAP::getExtension (std::string_view id)
{
    if (id == CLAP_EXT_NOTE_PORTS)
        return std::addressof (extensionNotePorts);
    if (id == CLAP_EXT_AUDIO_PORTS)
        return std::addressof (extensionAudioPorts);
    if (id == CLAP_EXT_PARAMS)
        return std::addressof (extensionParams);
    if (id == CLAP_EXT_STATE)
        return std::addressof (extensionState);
    if (id == CLAP_EXT_TAIL)
        return std::addressof (extensionTail);
    if (id == CLAP_EXT_LATENCY)
        return std::addressof (extensionLatency);
    if (id == CLAP_EXT_TIMER_SUPPORT)
        return std::addressof (extensionTimerSupport);
    if (id == CLAP_EXT_GUI)
        return std::addressof (extensionGUI);
    if (id == CLAP_EXT_RENDER)
        return std::addressof (extensionRender);
    if (id == CLAP_EXT_VOICE_INFO)
        return audioProcessor->getNumVoices() > 0 ? std::addressof (extensionVoiceInfo) : nullptr;

    return nullptr;
}

//==============================================================================

const clap_plugin_t* AudioPluginProcessorCLAP::getPlugin() const
{
    return std::addressof (plugin);
}

//==============================================================================

AudioProcessor* AudioPluginProcessorCLAP::getProcessor() const noexcept
{
    return audioProcessor.get();
}

//==============================================================================

void AudioPluginProcessorCLAP::addParameterListeners()
{
    removeParameterListeners();

    if (audioProcessor == nullptr)
        return;

    for (const auto& parameter : audioProcessor->getParameters())
    {
        parameter->addListener (this);
        listenedParameters.push_back (parameter);
    }
}

void AudioPluginProcessorCLAP::removeParameterListeners()
{
    for (auto& parameter : listenedParameters)
        parameter->removeListener (this);

    listenedParameters.clear();
}

bool AudioPluginProcessorCLAP::isValidProcessorParameterIndex (int indexInContainer) const
{
    return audioProcessor != nullptr
        && isPositiveAndBelow (indexInContainer, static_cast<int> (audioProcessor->getParameters().size()));
}

void AudioPluginProcessorCLAP::parameterValueChanged (const AudioParameter::Ptr& parameter, int indexInContainer)
{
    if (! isValidProcessorParameterIndex (indexInContainer))
        return;

    enqueueParameterEvent (CLAP_EVENT_PARAM_VALUE,
                           parameter->getHostParameterID(),
                           static_cast<double> (parameter->getValue()));
    requestParameterFlush();
}

void AudioPluginProcessorCLAP::parameterGestureBegin (const AudioParameter::Ptr& parameter, int indexInContainer)
{
    if (! isValidProcessorParameterIndex (indexInContainer))
        return;

    enqueueParameterEvent (CLAP_EVENT_PARAM_GESTURE_BEGIN, parameter->getHostParameterID());
    requestParameterFlush();
}

void AudioPluginProcessorCLAP::parameterGestureEnd (const AudioParameter::Ptr& parameter, int indexInContainer)
{
    if (! isValidProcessorParameterIndex (indexInContainer))
        return;

    enqueueParameterEvent (CLAP_EVENT_PARAM_GESTURE_END, parameter->getHostParameterID());
    requestParameterFlush();
}

void AudioPluginProcessorCLAP::audioProcessorChanged (AudioProcessorBase* processor, const AudioProcessor::ChangeDetails& details)
{
    ignoreUnused (processor);

    if (details.latencyChanged)
    {
        callLatencyChangeOnNextActivate.store (true);

        if (isActive.load() && host != nullptr && host->request_restart != nullptr)
            host->request_restart (host);
    }

    if (details.tailChanged)
    {
        tailChangedPending.store (true);

        if (host != nullptr && host->request_process != nullptr)
            host->request_process (host);
    }

    uint32 parameterRescanFlags = 0;

    if (details.parameterValuesChanged)
        parameterRescanFlags |= CLAP_PARAM_RESCAN_VALUES;

    if (details.parameterInfoChanged)
        parameterRescanFlags |= CLAP_PARAM_RESCAN_VALUES
                              | CLAP_PARAM_RESCAN_TEXT
                              | CLAP_PARAM_RESCAN_INFO;

    if (parameterRescanFlags != 0)
    {
        parameterRescanFlagsPending.fetch_or (parameterRescanFlags);
        requestMainThreadCallback();
    }

    if (details.nonParameterStateChanged)
    {
        stateDirtyPending.store (true);
        requestMainThreadCallback();
    }
}

void AudioPluginProcessorCLAP::enqueueParameterEvent (uint16 eventType, clap_id parameterId, double value) noexcept
{
    int start1, size1, start2, size2;
    parameterEventFifo.prepareToWrite (1, start1, size1, start2, size2);

    ignoreUnused (start2, size2);

    if (size1 <= 0)
    {
        jassertfalse; // Increase parameterEventQueueSize if hosts drop CLAP parameter feedback.
        parameterEventFifo.finishedWrite (0);
        return;
    }

    parameterEvents[static_cast<size_t> (start1)] = { eventType, parameterId, value };
    parameterEventFifo.finishedWrite (1);
}

void AudioPluginProcessorCLAP::drainParameterEvents (const clap_output_events_t* out) noexcept
{
    if (out == nullptr)
        return;

    for (;;)
    {
        int start1, size1, start2, size2;
        parameterEventFifo.prepareToRead (1, start1, size1, start2, size2);

        ignoreUnused (start2, size2);

        if (size1 <= 0)
        {
            parameterEventFifo.finishedRead (0);
            return;
        }

        const auto event = parameterEvents[static_cast<size_t> (start1)];
        parameterEventFifo.finishedRead (1);

        if (event.eventType == CLAP_EVENT_PARAM_VALUE)
        {
            clap_event_param_value_t paramEvent {};
            paramEvent.header.size = sizeof (paramEvent);
            paramEvent.header.time = 0;
            paramEvent.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            paramEvent.header.type = CLAP_EVENT_PARAM_VALUE;
            paramEvent.header.flags = 0;
            paramEvent.param_id = event.parameterId;
            paramEvent.cookie = nullptr;
            paramEvent.note_id = -1;
            paramEvent.port_index = -1;
            paramEvent.channel = -1;
            paramEvent.key = -1;
            paramEvent.value = event.value;
            out->try_push (out, &paramEvent.header);
        }
        else if (event.eventType == CLAP_EVENT_PARAM_GESTURE_BEGIN
                 || event.eventType == CLAP_EVENT_PARAM_GESTURE_END)
        {
            clap_event_param_gesture_t gestureEvent {};
            gestureEvent.header.size = sizeof (gestureEvent);
            gestureEvent.header.time = 0;
            gestureEvent.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            gestureEvent.header.type = event.eventType;
            gestureEvent.header.flags = 0;
            gestureEvent.param_id = event.parameterId;
            out->try_push (out, &gestureEvent.header);
        }
    }
}

void AudioPluginProcessorCLAP::requestParameterFlush() const noexcept
{
    if (! isInsideProcessBlock.load()
        && hostParams != nullptr
        && hostParams->request_flush != nullptr)
    {
        hostParams->request_flush (host);
    }
}

void AudioPluginProcessorCLAP::requestMainThreadCallback() const noexcept
{
    if (host != nullptr && host->request_callback != nullptr)
        host->request_callback (host);
}

void AudioPluginProcessorCLAP::handleMainThreadNotifications() noexcept
{
    const auto parameterRescanFlags = parameterRescanFlagsPending.exchange (0);
    if (parameterRescanFlags != 0
        && hostParams != nullptr
        && hostParams->rescan != nullptr)
    {
        hostParams->rescan (host, parameterRescanFlags);
    }

    if (stateDirtyPending.exchange (false)
        && hostState != nullptr
        && hostState->mark_dirty != nullptr)
    {
        hostState->mark_dirty (host);
    }
}

void AudioPluginProcessorCLAP::handleAudioThreadNotifications() noexcept
{
    if (tailChangedPending.exchange (false)
        && hostTail != nullptr
        && hostTail->changed != nullptr)
    {
        hostTail->changed (host);
    }
}

//==============================================================================

void AudioPluginProcessorCLAP::editorResized()
{
    if (audioPluginEditor == nullptr || hostTriggeredResizing)
        return;

    if (hostGUI != nullptr && hostGUI->request_resize != nullptr)
        hostGUI->request_resize (host, audioPluginEditor->getWidth(), audioPluginEditor->getHeight());
}

ScopedValueSetter<bool> AudioPluginProcessorCLAP::scopedHostEditorResizing()
{
    return { hostTriggeredResizing, true };
}

//==============================================================================

void AudioPluginEditorCLAP::contentScaleChanged (float dpiScale)
{
    if (processorEditor == nullptr)
        return;

    processorEditor->contentScaleChanged (dpiScale);
}

void AudioPluginEditorCLAP::resized()
{
    if (processorEditor == nullptr)
        return;

    processorEditor->setBounds (getLocalBounds());

    wrapper->editorResized();
}

} // namespace yup

//==============================================================================

static const clap_plugin_factory_t plugin_factory = []
{
    clap_plugin_factory_t factory;

    factory.get_plugin_count = [] (const clap_plugin_factory* factory) -> uint32_t
    {
        return 1;
    };

    factory.get_plugin_descriptor = [] (const clap_plugin_factory* factory, uint32_t index) -> const clap_plugin_descriptor_t*
    {
        return index == 0 ? &yup::pluginDescriptor : nullptr;
    };

    factory.create_plugin = [] (const clap_plugin_factory* factory, const clap_host_t* host, const char* pluginId) -> const clap_plugin_t*
    {
        if (! clap_version_is_compatible (host->clap_version) || std::string_view (pluginId) != yup::pluginDescriptor.id)
            return nullptr;

        auto wrapper = new yup::AudioPluginProcessorCLAP (host);
        return wrapper->getPlugin();
    };

    return factory;
}();

//==============================================================================

static bool clapInit (const char*) noexcept
{
    return true;
}

static void clapDeinit() noexcept
{
}

static const void* clapGetFactory (const char* factoryId) noexcept
{
    if (std::string_view (factoryId) == CLAP_PLUGIN_FACTORY_ID)
        return std::addressof (plugin_factory);

    return nullptr;
}

extern "C" YUP_PLUGIN_ENTRY_POINT const clap_plugin_entry_t clap_entry = {
    CLAP_VERSION_INIT,
    clapInit,
    clapDeinit,
    clapGetFactory
};
