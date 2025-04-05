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

#if ! defined(YUP_AUDIO_PLUGIN_ENABLE_CLAP)
#error "YUP_AUDIO_PLUGIN_ENABLE_CLAP must be defined"
#endif

#include <string_view>
#include <optional>

#include <clap/clap.h>

extern "C" yup::AudioProcessor* createPluginProcessor();

namespace yup
{

//==============================================================================

std::optional<MidiMessage> clapEventToMidiNoteMessage (const clap_event_header_t* event)
{
    switch (event->type)
    {
        case CLAP_EVENT_NOTE_ON:
        {
            const clap_event_note_t* noteEvent = reinterpret_cast<const clap_event_note_t*> (event);
            const int channel = noteEvent->channel < 0 ? 1 : noteEvent->channel + 1;

            return MidiMessage::noteOn (channel, noteEvent->key, static_cast<uint8> (noteEvent->velocity * 127.0f));
        }

        case CLAP_EVENT_NOTE_OFF:
        {
            const clap_event_note_t* noteEvent = reinterpret_cast<const clap_event_note_t*> (event);
            const int channel = noteEvent->channel < 0 ? 1 : noteEvent->channel + 1;

            return MidiMessage::noteOff (channel, noteEvent->key, static_cast<float> (noteEvent->velocity));
        }

        case CLAP_EVENT_NOTE_CHOKE:
        {
            const clap_event_note_t* noteEvent = reinterpret_cast<const clap_event_note_t*> (event);
            const int channel = noteEvent->channel < 0 ? 1 : noteEvent->channel + 1;

            return MidiMessage::noteOff (channel, noteEvent->key);
        }

        case CLAP_EVENT_NOTE_END:
        case CLAP_EVENT_NOTE_EXPRESSION:
        case CLAP_EVENT_PARAM_VALUE:
        case CLAP_EVENT_PARAM_MOD:
        case CLAP_EVENT_MIDI:
        case CLAP_EVENT_MIDI_SYSEX:
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

    auto parameters = audioProcessor.getParameters();

    auto parameterIndex = static_cast<int> (paramEvent->param_id);
    if (! isPositiveAndBelow (parameterIndex, static_cast<int> (parameters.size())))
        return;

    parameters[parameterIndex]->setValue (static_cast<float> (paramEvent->value));
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

#if JUCE_MAC
static const char* const preferredApi = CLAP_WINDOW_API_COCOA;
#elif JUCE_WINDOWS
static const char* const preferredApi = CLAP_WINDOW_API_WIN32;
#elif JUCE_LINUX
static const char* const preferredApi = CLAP_WINDOW_API_X11;
#endif

//==============================================================================

class AudioPluginProcessorCLAP;

//==============================================================================

class AudioPluginPlayHeadCLAP final : public AudioPlayHead
{
public:
    explicit AudioPluginPlayHeadCLAP (float sampleRate, const clap_process_t* process)
        : process (*process)
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

    Optional<PositionInfo> getPosition() const override
    {
        if (process.transport == nullptr)
            return {};

        PositionInfo result;

        result.setTimeInSeconds (process.transport->song_pos_seconds / (double) CLAP_SECTIME_FACTOR);
        result.setTimeInSamples ((int64) (sampleRate * (process.transport->song_pos_seconds / (double) CLAP_SECTIME_FACTOR)));
        result.setTimeSignature (TimeSignature { process.transport->tsig_num, process.transport->tsig_denom });
        result.setBpm (process.transport->tempo);
        result.setBarCount (process.transport->bar_number);
        result.setPpqPositionOfLastBarStart (process.transport->bar_start / (double) CLAP_BEATTIME_FACTOR);
        result.setIsPlaying (process.transport->flags & CLAP_TRANSPORT_IS_PLAYING);
        result.setIsRecording (process.transport->flags & CLAP_TRANSPORT_IS_RECORDING);
        result.setIsLooping (process.transport->flags & CLAP_TRANSPORT_IS_LOOP_ACTIVE);
        result.setLoopPoints (LoopPoints {
            process.transport->loop_start_beats / (double) CLAP_BEATTIME_FACTOR,
            process.transport->loop_end_beats / (double) CLAP_BEATTIME_FACTOR });
        result.setFrameRate (AudioPlayHead::fpsUnknown);

        return result;
    }

private:
    clap_process_t process;
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

    AudioProcessorEditor* getAudioProcessorEditor() { return processorEditor.get(); }

    void resized() override;

private:
    AudioPluginProcessorCLAP* wrapper = nullptr;
    std::unique_ptr<AudioProcessorEditor> processorEditor;
};

//==============================================================================

class AudioPluginProcessorCLAP final
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

    void editorResized();

private:
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

    const clap_host_params_t* hostParams = nullptr;
    const clap_host_state_t* hostState = nullptr;
    const clap_host_tail_t* hostTail = nullptr;
    const clap_host_latency_t* hostLatency = nullptr;
    const clap_host_timer_support_t* hostTimerSupport = nullptr;
    const clap_host_gui_t* hostGUI = nullptr;

    clap_id guiTimerId;

    MidiBuffer midiEvents;

    static std::atomic_int instancesCount;
};

//==============================================================================

std::atomic_int AudioPluginProcessorCLAP::instancesCount = 0;

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
        JUCE_DBG ("clap_plugin_t::init");

        return getWrapper (plugin)->initialise();
    };

    plugin.destroy = [] (const clap_plugin* plugin)
    {
        JUCE_DBG ("clap_plugin_t::destroy");

        getWrapper (plugin)->destroy();
    };

    plugin.activate = [] (const clap_plugin* plugin, double sampleRate, uint32_t minimumFramesCount, uint32_t maximumFramesCount) -> bool
    {
        JUCE_DBG ("clap_plugin_t::activate " << sampleRate << "hz (" << (int) minimumFramesCount << ".." << (int) maximumFramesCount << ")");

        return getWrapper (plugin)->activate (static_cast<float> (sampleRate), static_cast<int> (maximumFramesCount));
    };

    plugin.deactivate = [] (const clap_plugin* plugin)
    {
        JUCE_DBG ("clap_plugin_t::deactivate");

        getWrapper (plugin)->deactivate();
    };

    plugin.start_processing = [] (const clap_plugin* plugin) -> bool
    {
        JUCE_DBG ("clap_plugin_t::start_processing");

        return getWrapper (plugin)->startProcessing();
    };

    plugin.stop_processing = [] (const clap_plugin* plugin)
    {
        JUCE_DBG ("clap_plugin_t::stop_processing");

        getWrapper (plugin)->stopProcessing();
    };

    plugin.reset = [] (const clap_plugin* plugin)
    {
        JUCE_DBG ("clap_plugin_t::reset");

        getWrapper (plugin)->reset();
    };

    plugin.process = [] (const clap_plugin* plugin, const clap_process_t* process) -> clap_process_status
    {
        auto wrapper = getWrapper (plugin);

        auto& audioProcessor = *wrapper->audioProcessor;
        auto& midiBuffer = wrapper->midiEvents;

        auto lock = CriticalSection::ScopedTryLockType (audioProcessor.getProcessLock());
        if (! lock.isLocked() || audioProcessor.isSuspended())
            return CLAP_PROCESS_CONTINUE;

        jassert (process->audio_outputs_count == audioProcessor.getNumAudioOutputs());
        jassert (process->audio_inputs_count == audioProcessor.getNumAudioInputs());

        // PluginSyncMainToAudio(plugin, process->out_events);

        // Prepare midi events
        midiBuffer.clear();

        const uint32_t inputEventCount = process->in_events->size (process->in_events);
        for (uint32_t eventIndex = 0; eventIndex < inputEventCount; ++eventIndex)
        {
            const clap_event_header_t* event = process->in_events->get (process->in_events, eventIndex);

            if (event->space_id != CLAP_CORE_EVENT_SPACE_ID)
                continue;

            if (auto convertedEvent = clapEventToMidiNoteMessage (event))
                midiBuffer.addEvent (*convertedEvent, static_cast<int> (event->time));
            else
                clapEventToParameterChange (event, audioProcessor);
        }

        // Prepare audio buffers, play head and process block
        float* buffers[2] = {
            process->audio_outputs[0].data32[0],
            process->audio_outputs[0].data32[1]
        };

        AudioSampleBuffer audioBuffer (&buffers[0], 2, 0, static_cast<int> (process->frames_count));

        AudioPluginPlayHeadCLAP playHead (audioProcessor.getSampleRate(), process);
        audioProcessor.setPlayHead (&playHead);

        audioProcessor.processBlock (audioBuffer, midiBuffer);

        audioProcessor.setPlayHead (nullptr);

        // Send back note end to host
        for (const MidiMessageMetadata metadata : midiBuffer)
        {
            if (const auto& message = metadata.getMessage(); message.isNoteOff())
            {
                clap_event_note_t event = {};
                event.header.size = sizeof (event);
                event.header.time = 0;
                event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                event.header.type = CLAP_EVENT_NOTE_END;
                event.header.flags = 0;
                event.note_id = -1;
                event.key = message.getNoteNumber();
                event.channel = message.getChannel() - 1;
                event.port_index = 0;

                process->out_events->try_push (process->out_events, &event.header);
            }
        }

        return CLAP_PROCESS_CONTINUE;
    };

    plugin.get_extension = [] (const clap_plugin* plugin, const char* id) -> const void*
    {
        JUCE_DBG ("clap_plugin_t::get_extension " << id);

        return getWrapper (plugin)->getExtension (id);
    };

    plugin.on_main_thread = [] (const clap_plugin* plugin)
    {
        JUCE_DBG ("clap_plugin_t::on_main_thread");
    };
}

//==============================================================================

AudioPluginProcessorCLAP::~AudioPluginProcessorCLAP()
{
}

//==============================================================================

bool AudioPluginProcessorCLAP::initialise()
{
    jassert (audioProcessor == nullptr);

    audioProcessor.reset (::createPluginProcessor());
    if (audioProcessor == nullptr)
        return false;

    // ==== Setup extensions: parameters
    extensionParams.count = [] (const clap_plugin_t* plugin) -> uint32_t
    {
        return static_cast<uint32_t> (getWrapper (plugin)->audioProcessor->getParameters().size());
    };

    extensionParams.get_info = [] (const clap_plugin_t* plugin, uint32_t index, clap_param_info_t* information) -> bool
    {
        std::memset (information, 0, sizeof (clap_param_info_t));

        auto wrapper = getWrapper (plugin);
        auto parameters = wrapper->audioProcessor->getParameters();

        if (index >= static_cast<uint32_t> (parameters.size()))
            return false;

        auto& parameter = parameters[index];

        information->id = index;
        information->cookie = parameter.get();
        information->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID;
        information->min_value = parameter->getMinimumValue();
        information->max_value = parameter->getMaximumValue();
        information->default_value = parameter->getDefaultValue();
        parameter->getName().copyToUTF8 (information->name, CLAP_NAME_SIZE);

        return true;
    };

    extensionParams.get_value = [] (const clap_plugin_t* plugin, clap_id parameterId, double* value) -> bool
    {
        auto wrapper = getWrapper (plugin);
        auto parameters = wrapper->audioProcessor->getParameters();

        if (parameterId >= static_cast<uint32_t> (parameters.size()))
            return false;

        *value = parameters[parameterId]->getValue();

        return true;
    };

    extensionParams.value_to_text = [] (const clap_plugin_t* plugin, clap_id parameterId, double value, char* display, uint32_t size) -> bool
    {
        auto wrapper = getWrapper (plugin);
        auto parameters = wrapper->audioProcessor->getParameters();

        if (parameterId >= static_cast<uint32_t> (parameters.size()))
            return false;

        const auto text = parameters[parameterId]->convertToString (static_cast<float> (value));
        text.copyToUTF8 (display, size);

        return true;
    };

    extensionParams.text_to_value = [] (const clap_plugin_t* plugin, clap_id parameterId, const char* display, double* value) -> bool
    {
        auto wrapper = getWrapper (plugin);
        auto parameters = wrapper->audioProcessor->getParameters();

        if (parameterId >= static_cast<uint32_t> (parameters.size()))
            return false;

        *value = static_cast<double> (parameters[parameterId]->convertFromString (display));

        return true;
    };

    extensionParams.flush = [] (const clap_plugin_t* plugin, const clap_input_events_t* in, const clap_output_events_t* out)
    {
        /* // TODO
        auto wrapper = getWrapper (plugin);

        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;
        const uint32_t eventCount = in->size(in);

        // For parameters that have been modified by the main thread, send CLAP_EVENT_PARAM_VALUE events to the host.
        PluginSyncMainToAudio(plugin, out);

        // Process events sent to our plugin from the host.
        for (uint32_t eventIndex = 0; eventIndex < eventCount; eventIndex++)
        {
            PluginProcessEvent(plugin, in->get(in, eventIndex));
        }
        */
    };

    // ==== Setup extensions: note ports
    extensionNotePorts.count = [] (const clap_plugin_t* plugin, bool isInput) -> uint32_t
    {
        // TODO - this depends on the YupPlugin_IsSynth, but we might want to probe for midi input buses
        return isInput ? 1 : 0;
    };

    extensionNotePorts.get = [] (const clap_plugin_t* plugin, uint32_t index, bool isInput, clap_note_port_info_t* info) -> bool
    {
        if (! isInput || index)
            return false;

        info->id = 0;
        info->supported_dialects = CLAP_NOTE_DIALECT_CLAP; // TODO Also support the MIDI dialect.
        info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;

        std::snprintf (info->name, sizeof (info->name), "%s", "Note Port");

        return true;
    };

    // ==== Setup extensions: audio ports
    extensionAudioPorts.count = [] (const clap_plugin_t* plugin, bool isInput) -> uint32_t
    {
        auto wrapper = getWrapper (plugin);
        auto* audioProcessor = wrapper->audioProcessor.get();

        return static_cast<uint32_t> (isInput
                                          ? audioProcessor->getBusLayout().getInputBuses().size()
                                          : audioProcessor->getBusLayout().getOutputBuses().size());
    };

    extensionAudioPorts.get = [] (const clap_plugin_t* plugin, uint32_t index, bool isInput, clap_audio_port_info_t* info) -> bool
    {
        auto wrapper = getWrapper (plugin);
        auto* audioProcessor = wrapper->audioProcessor.get();

        Span<const AudioBus> busses = isInput
                                        ? audioProcessor->getBusLayout().getInputBuses()
                                        : audioProcessor->getBusLayout().getOutputBuses();

        if (index >= static_cast<uint32_t> (busses.size()))
            return false;

        const AudioBus& bus = busses[index];

        info->id = index;
        info->channel_count = bus.getNumChannels();
        info->flags = (index == 0) ? CLAP_AUDIO_PORT_IS_MAIN : 0;
        info->port_type = bus.isStereo() ? CLAP_PORT_STEREO : CLAP_PORT_MONO;
        info->in_place_pair = CLAP_INVALID_ID;
        bus.getName().copyToUTF8 (info->name, sizeof (info->name));

        return true;
    };

    // ==== Setup extensions: state
    extensionState.save = [] (const clap_plugin_t* plugin, const clap_ostream_t* stream) -> bool
    {
        auto wrapper = getWrapper (plugin);

        MemoryBlock data;

        // TODO - should we suspend ?

        if (auto result = wrapper->audioProcessor->saveStateIntoMemory (data); result.failed())
            return false;

        // TODO - should we resume ?

        return stream->write (stream, data.getData(), data.getSize()) == data.getSize();
    };

    extensionState.load = [] (const clap_plugin_t* plugin, const clap_istream_t* stream) -> bool
    {
        auto wrapper = getWrapper (plugin);
        auto parameters = wrapper->audioProcessor->getParameters();

        MemoryBlock data;
        if (auto result = stream->read (stream, data.getData(), data.getSize()); result <= 0)
            return false;

        // TODO - should we suspend ?

        auto result = wrapper->audioProcessor->loadStateFromMemory (data);

        // TODO - should we resume ?

        return result.wasOk();
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
#if JUCE_LINUX
        if (auto wrapper = getWrapper (plugin); wrapper->guiTimerId == timerId)
            MessageManager::getInstance()->runDispatchLoopUntil (10);
#endif
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
        JUCE_DBG ("clap_plugin_gui_t::create");

        if (api == nullptr || std::string_view (api) != preferredApi || isFloating)
            return false;

        auto wrapper = getWrapper (plugin);

        auto processorEditor = wrapper->audioProcessor->createEditor();
        if (processorEditor == nullptr)
            return false;

        wrapper->audioPluginEditor = std::make_unique<AudioPluginEditorCLAP> (wrapper, processorEditor);

        return true;
    };

    extensionGUI.destroy = [] (const clap_plugin_t* plugin)
    {
        JUCE_DBG ("clap_plugin_gui_t::destroy");

        auto wrapper = getWrapper (plugin);
        wrapper->audioPluginEditor.reset();
    };

    extensionGUI.set_scale = [] (const clap_plugin_t* plugin, double scale) -> bool
    {
        JUCE_DBG ("clap_plugin_gui_t::set_scale " << scale);

        return false;
    };

    extensionGUI.get_size = [] (const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) -> bool
    {
        JUCE_DBG ("clap_plugin_gui_t::get_size");

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
        JUCE_DBG ("clap_plugin_gui_t::can_resize");

        auto wrapper = getWrapper (plugin);
        if (wrapper->audioPluginEditor == nullptr)
            return false;

        return wrapper->audioPluginEditor->getAudioProcessorEditor()->isResizable();
    };

    extensionGUI.get_resize_hints = [] (const clap_plugin_t* plugin, clap_gui_resize_hints_t* hints) -> bool
    {
        JUCE_DBG ("clap_plugin_gui_t::get_resize_hints");

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
        JUCE_DBG ("clap_plugin_gui_t::adjust_size " << (int32_t) *width << "," << (int32_t) *height);

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
        JUCE_DBG ("clap_plugin_gui_t::set_size " << (int32_t) width << "," << (int32_t) height);

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

        wrapper->audioPluginEditor->setSize ({ static_cast<float> (width), static_cast<float> (height) });

        return true;
    };

    extensionGUI.set_parent = [] (const clap_plugin_t* plugin, const clap_window_t* window) -> bool
    {
        JUCE_DBG ("clap_plugin_gui_t::set_parent");

        jassert (std::string_view (window->api) == preferredApi);

        auto wrapper = getWrapper (plugin);
        if (wrapper->audioPluginEditor == nullptr)
            return false;

        auto audioProcessorEditor = wrapper->audioPluginEditor->getAudioProcessorEditor();

        ComponentNative::Flags flags =
            ComponentNative::defaultFlags & ~ComponentNative::decoratedWindow;

        if (audioProcessorEditor->shouldRenderContinuous())
            flags.set (ComponentNative::renderContinuous);

        ComponentNative::Options options;
        options.flags = flags;
        wrapper->audioPluginEditor->addToDesktop (options, window->cocoa);

        audioProcessorEditor->attachedToNative();

        return true;
    };

    extensionGUI.set_transient = [] (const clap_plugin_t* plugin, const clap_window_t* window) -> bool
    {
        JUCE_DBG ("clap_plugin_gui_t::set_transient");

        return false;
    };

    extensionGUI.suggest_title = [] (const clap_plugin_t* plugin, const char* title)
    {
        JUCE_DBG ("clap_plugin_gui_t::suggest_title " << title);
    };

    extensionGUI.show = [] (const clap_plugin_t* plugin) -> bool
    {
        JUCE_DBG ("clap_plugin_gui_t::show");

        auto wrapper = getWrapper (plugin);
        if (wrapper->audioPluginEditor == nullptr)
            return false;

        wrapper->audioPluginEditor->setVisible (true);
        return true;
    };

    extensionGUI.hide = [] (const clap_plugin_t* plugin) -> bool
    {
        JUCE_DBG ("clap_plugin_gui_t::hide");

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

    return true;
}

//==============================================================================

void AudioPluginProcessorCLAP::destroy()
{
    plugin.plugin_data = nullptr;
    delete this;
}

//==============================================================================

bool AudioPluginProcessorCLAP::activate (float sampleRate, int samplesPerBlock)
{
#if JUCE_LINUX
    if (instancesCount.fetch_add (1) == 0)
        registerTimer (16, &guiTimerId);
#endif

    audioProcessor->setPlaybackConfiguration (sampleRate, samplesPerBlock);
    return true;
}

//==============================================================================

void AudioPluginProcessorCLAP::deactivate()
{
    audioProcessor->releaseResources();

#if JUCE_LINUX
    if (instancesCount.fetch_sub (1) == 1)
        unregisterTimer (guiTimerId);
#endif
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

    return nullptr;
}

//==============================================================================

const clap_plugin_t* AudioPluginProcessorCLAP::getPlugin() const
{
    return std::addressof (plugin);
}

//==============================================================================

void AudioPluginProcessorCLAP::editorResized()
{
    if (audioPluginEditor == nullptr)
        return;

    if (hostGUI != nullptr && hostGUI->request_resize != nullptr)
        hostGUI->request_resize (host, audioPluginEditor->getWidth(), audioPluginEditor->getHeight());
}

//==============================================================================

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
        JUCE_DBG ("clap_plugin_factory_t::get_plugin_count");

        return 1;
    };

    factory.get_plugin_descriptor = [] (const clap_plugin_factory* factory, uint32_t index) -> const clap_plugin_descriptor_t*
    {
        JUCE_DBG ("clap_plugin_factory_t::get_plugin_descriptor " << (int32_t) index);

        return index == 0 ? &yup::pluginDescriptor : nullptr;
    };

    factory.create_plugin = [] (const clap_plugin_factory* factory, const clap_host_t* host, const char* pluginId) -> const clap_plugin_t*
    {
        JUCE_DBG ("clap_plugin_factory_t::create_plugin " << pluginId);

        if (! clap_version_is_compatible (host->clap_version) || std::string_view (pluginId) != yup::pluginDescriptor.id)
            return nullptr;

        auto wrapper = new yup::AudioPluginProcessorCLAP (host);
        return wrapper->getPlugin();
    };

    return factory;
}();

//==============================================================================

extern "C" const CLAP_EXPORT clap_plugin_entry_t clap_entry = []
{
    clap_plugin_entry_t plugin;

    plugin.clap_version = CLAP_VERSION_INIT;

    plugin.init = [] (const char* path) -> bool
    {
        JUCE_DBG ("clap_plugin_entry_t::init " << path);

        yup::initialiseJuce_GUI();
        yup::initialiseYup_Windowing();

        return true;
    };

    plugin.deinit = []
    {
        JUCE_DBG ("clap_plugin_entry_t::deinit");

        yup::shutdownYup_Windowing();
        yup::shutdownJuce_GUI();
    };

    plugin.get_factory = [] (const char* factoryId) -> const void*
    {
        JUCE_DBG ("clap_plugin_entry_t::get_factory " << factoryId);

        if (std::string_view (factoryId) == CLAP_PLUGIN_FACTORY_ID)
            return std::addressof (plugin_factory);

        return nullptr;
    };

    return plugin;
}();
