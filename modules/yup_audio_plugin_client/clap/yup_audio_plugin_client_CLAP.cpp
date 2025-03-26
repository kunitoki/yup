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

    auto parameterIndex = static_cast<int> (paramEvent->param_id);
    if (! yup::isPositiveAndBelow (parameterIndex, audioProcessor.getNumParameters()))
        return;

    auto parameterValue = static_cast<float> (paramEvent->value);
    audioProcessor.getParameter (parameterIndex).setValue (parameterValue);
}

//==============================================================================

/*
void pluginSyncMainToAudio (AudioProcessor& audioProcessor, const clap_output_events_t* out)
{
    auto sl = yup::CriticalSection::ScopedLockType (plugin->syncParameters);

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
    auto sl = yup::CriticalSection::ScopedLockType (plugin->syncParameters);

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

class AudioPluginWrapperCLAP
{
public:
    AudioPluginWrapperCLAP (const clap_host_t* host);
    ~AudioPluginWrapperCLAP();

    bool initialise();
    void destroy();

    bool activate (float sampleRate, int samplesPerBlock);
    void deactivate();

    bool startProcessing();
    void stopProcessing();

    void reset();

    const void* getExtension (std::string_view id);
    const clap_plugin_t* getPlugin() const;

private:
    std::unique_ptr<AudioProcessor> audioProcessor;
    std::unique_ptr<AudioProcessorEditor> audioProcessorEditor;

    const clap_host_t* host = nullptr;
    clap_plugin_t plugin;

    clap_plugin_params_t extensionParams;
    clap_plugin_note_ports_t extensionNotePorts;
    clap_plugin_audio_ports_t extensionAudioPorts;
    clap_plugin_state_t extensionState;

    clap_plugin_timer_support_t extensionTimerSupport;
    clap_plugin_gui_t extensionGUI;

    const clap_host_timer_support_t* hostTimerSupport = nullptr;
    clap_id timerID;

    yup::MidiBuffer midiEvents;

    static std::atomic_int instancesCount;
};

//==============================================================================

std::atomic_int AudioPluginWrapperCLAP::instancesCount = 0;

AudioPluginWrapperCLAP* getWrapper (const clap_plugin_t* plugin)
{
    return reinterpret_cast<yup::AudioPluginWrapperCLAP*> (plugin->plugin_data);
}

//==============================================================================

AudioPluginWrapperCLAP::AudioPluginWrapperCLAP (const clap_host_t* host)
    : host (host)
{
    jassert (host != nullptr);

    plugin.desc = &pluginDescriptor;
    plugin.plugin_data = this;

    plugin.init = [] (const clap_plugin* plugin) -> bool
    {
        DBG ("clap_plugin_t::init");

        return yup::getWrapper (plugin)->initialise();
    };

    plugin.destroy = [] (const clap_plugin* plugin)
    {
        DBG ("clap_plugin_t::destroy");

        yup::getWrapper (plugin)->destroy();
    };

    plugin.activate = [] (const clap_plugin* plugin, double sampleRate, uint32_t minimumFramesCount, uint32_t maximumFramesCount) -> bool
    {
        DBG ("clap_plugin_t::activate " << sampleRate << "hz (" << (int) minimumFramesCount << ".." << (int) maximumFramesCount << ")");

        return yup::getWrapper (plugin)->activate (static_cast<float> (sampleRate), static_cast<int> (maximumFramesCount));
    };

    plugin.deactivate = [] (const clap_plugin* plugin)
    {
        DBG ("clap_plugin_t::deactivate");

        yup::getWrapper (plugin)->deactivate();
    };

    plugin.start_processing = [] (const clap_plugin* plugin) -> bool
    {
        DBG ("clap_plugin_t::start_processing");

        return yup::getWrapper (plugin)->startProcessing();
    };

    plugin.stop_processing = [] (const clap_plugin* plugin)
    {
        DBG ("clap_plugin_t::stop_processing");

        yup::getWrapper (plugin)->stopProcessing();
    };

    plugin.reset = [] (const clap_plugin* plugin)
    {
        DBG ("clap_plugin_t::reset");

        yup::getWrapper (plugin)->reset();
    };

    plugin.process = [] (const clap_plugin* plugin, const clap_process_t* process) -> clap_process_status
    {
        auto wrapper = yup::getWrapper (plugin);

        auto& audioProcessor = *wrapper->audioProcessor;
        auto& midiBuffer = wrapper->midiEvents;

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

        // Prepare audio buffers
        float* buffers[2] = {
            process->audio_outputs[0].data32[0],
            process->audio_outputs[0].data32[1]
        };

        AudioSampleBuffer audioBuffer (&buffers[0], 2, 0, static_cast<int> (process->frames_count));

        // Process block
        audioProcessor.processBlock (audioBuffer, midiBuffer);

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
        DBG ("clap_plugin_t::get_extension " << id);

        return yup::getWrapper (plugin)->getExtension (id);
    };

    plugin.on_main_thread = [] (const clap_plugin* plugin)
    {
        DBG ("clap_plugin_t::on_main_thread");
    };
}

//==============================================================================

AudioPluginWrapperCLAP::~AudioPluginWrapperCLAP()
{
}

//==============================================================================

bool AudioPluginWrapperCLAP::initialise()
{
    jassert (audioProcessor == nullptr);

    audioProcessor.reset (createPluginProcessor());
    if (audioProcessor == nullptr)
        return false;

    // ==== Setup extensions: parameters
    extensionParams.count = [] (const clap_plugin_t* plugin) -> uint32_t
    {
        return static_cast<uint32_t> (getWrapper (plugin)->audioProcessor->getNumParameters());
    };

    extensionParams.get_info = [] (const clap_plugin_t* plugin, uint32_t index, clap_param_info_t* information) -> bool
    {
        std::memset (information, 0, sizeof (clap_param_info_t));

        auto wrapper = getWrapper (plugin);

        if (yup::isPositiveAndBelow (index, wrapper->audioProcessor->getNumParameters()))
        {
            const auto& parameter = wrapper->audioProcessor->getParameter (index);

            information->id = index;
            information->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID;
            information->min_value = parameter.getMinimumValue();
            information->max_value = parameter.getMaximumValue();
            information->default_value = parameter.getDefaultValue();
            std::strncpy (information->name, parameter.getName().toRawUTF8(), CLAP_NAME_SIZE);

            return true;
        }
        else
        {
            return false;
        }
    };

    extensionParams.get_value = [] (const clap_plugin_t* plugin, clap_id id, double* value) -> bool
    {
        auto wrapper = getWrapper (plugin);

        const int index = static_cast<int> (id);
        if (! yup::isPositiveAndBelow (index, wrapper->audioProcessor->getNumParameters()))
            return false;

        *value = wrapper->audioProcessor->getParameter (index).getValue();
        return true;
    };

    extensionParams.value_to_text = [] (const clap_plugin_t* plugin, clap_id id, double value, char* display, uint32_t size) -> bool
    {
        auto wrapper = getWrapper (plugin);

        const int index = static_cast<int> (id);
        if (! yup::isPositiveAndBelow (index, wrapper->audioProcessor->getNumParameters()))
            return false;

        std::snprintf (display, size, "%f", value);

        return true;
    };

    extensionParams.text_to_value = [] (const clap_plugin_t* plugin, clap_id param_id, const char* display, double* value) -> bool
    {
        return false;
    };

    extensionParams.flush = [] (const clap_plugin_t* plugin, const clap_input_events_t* in, const clap_output_events_t* out)
    {
        auto wrapper = getWrapper (plugin);

        /*
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
        return isInput ? 0 : 1;
    };

    extensionAudioPorts.get = [] (const clap_plugin_t* plugin, uint32_t index, bool isInput, clap_audio_port_info_t* info) -> bool
    {
        if (isInput || index)
            return false;

        info->id = 0;
        info->channel_count = 2;
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        info->port_type = CLAP_PORT_STEREO;
        info->in_place_pair = CLAP_INVALID_ID;

        std::snprintf (info->name, sizeof (info->name), "%s", "Audio Output");

        return true;
    };

    // ==== Setup extensions: state
    extensionState.save = [] (const clap_plugin_t* plugin, const clap_ostream_t* stream) -> bool
    {
        auto wrapper = getWrapper (plugin);

        yup::Array<float> params;
        params.resize (wrapper->audioProcessor->getNumParameters());

        for (int i = 0; i < wrapper->audioProcessor->getNumParameters(); ++i)
            params.set (i, wrapper->audioProcessor->getParameter (i).getValue());

        const auto totalSize = sizeof (float) * params.size();
        return stream->write (stream, params.data(), totalSize) == totalSize;
    };

    extensionState.load = [] (const clap_plugin_t* plugin, const clap_istream_t* stream) -> bool
    {
        auto wrapper = getWrapper (plugin);

        yup::Array<float> params;
        params.resize (wrapper->audioProcessor->getNumParameters());

        const auto totalSize = sizeof (float) * params.size();
        bool success = stream->read (stream, params.data(), totalSize) == totalSize;

        for (int i = 0; i < wrapper->audioProcessor->getNumParameters(); ++i)
            wrapper->audioProcessor->getParameter (i).setValue (params.getReference (i));

        return success;
    };

    // ==== Setup extensions: timer support
    extensionTimerSupport.on_timer = [] (const clap_plugin_t* plugin, clap_id timerID)
    {
#if JUCE_LINUX
        yup::MessageManager::getInstance()->runDispatchLoopUntil (10);
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
        DBG ("clap_plugin_gui_t::create");

        if (std::string_view (api) != preferredApi || isFloating)
            return false;

        auto wrapper = getWrapper (plugin);

        wrapper->audioProcessorEditor.reset (wrapper->audioProcessor->createEditor());
        if (wrapper->audioProcessorEditor == nullptr)
            return false;

        return true;
    };

    extensionGUI.destroy = [] (const clap_plugin_t* plugin)
    {
        DBG ("clap_plugin_gui_t::destroy");

        auto wrapper = getWrapper (plugin);
        wrapper->audioProcessorEditor.reset();
    };

    extensionGUI.set_scale = [] (const clap_plugin_t* plugin, double scale) -> bool
    {
        DBG ("clap_plugin_gui_t::set_scale " << scale);

        return false;
    };

    extensionGUI.get_size = [] (const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) -> bool
    {
        DBG ("clap_plugin_gui_t::get_size");

        auto wrapper = getWrapper (plugin);
        if (wrapper->audioProcessorEditor == nullptr)
            return false;

        if (wrapper->audioProcessorEditor->isResizable() && wrapper->audioProcessorEditor->getWidth() != 0)
        {
            *width = static_cast<uint32_t> (wrapper->audioProcessorEditor->getWidth());
            *height = static_cast<uint32_t> (wrapper->audioProcessorEditor->getHeight());
        }
        else
        {
            *width = static_cast<uint32_t> (wrapper->audioProcessorEditor->getPreferredSize().getWidth());
            *height = static_cast<uint32_t> (wrapper->audioProcessorEditor->getPreferredSize().getHeight());
        }

        return true;
    };

    extensionGUI.can_resize = [] (const clap_plugin_t* plugin) -> bool
    {
        DBG ("clap_plugin_gui_t::can_resize");

        auto wrapper = getWrapper (plugin);
        if (wrapper->audioProcessorEditor == nullptr)
            return false;

        return wrapper->audioProcessorEditor->isResizable();
    };

    extensionGUI.get_resize_hints = [] (const clap_plugin_t* plugin, clap_gui_resize_hints_t* hints) -> bool
    {
        DBG ("clap_plugin_gui_t::get_resize_hints");

        auto wrapper = getWrapper (plugin);
        if (wrapper->audioProcessorEditor == nullptr)
            return false;

        hints->can_resize_horizontally = wrapper->audioProcessorEditor->isResizable();
        hints->can_resize_vertically = wrapper->audioProcessorEditor->isResizable();
        hints->preserve_aspect_ratio = wrapper->audioProcessorEditor->shouldPreserveAspectRatio();
        hints->aspect_ratio_width = wrapper->audioProcessorEditor->getPreferredSize().getWidth();
        hints->aspect_ratio_height = wrapper->audioProcessorEditor->getPreferredSize().getHeight();

        return true;
    };

    extensionGUI.adjust_size = [] (const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) -> bool
    {
        DBG ("clap_plugin_gui_t::adjust_size " << (int32_t) *width << "," << (int32_t) *height);

        auto wrapper = getWrapper (plugin);
        if (wrapper->audioProcessorEditor == nullptr)
            return false;

        const auto preferredSize = wrapper->audioProcessorEditor->getPreferredSize();

        if (! wrapper->audioProcessorEditor->isResizable())
        {
            *width = static_cast<uint32_t> (preferredSize.getWidth());
            *height = static_cast<uint32_t> (preferredSize.getHeight());
        }
        else if (wrapper->audioProcessorEditor->shouldPreserveAspectRatio())
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
        DBG ("clap_plugin_gui_t::set_size " << (int32_t) width << "," << (int32_t) height);

        auto wrapper = getWrapper (plugin);
        if (wrapper->audioProcessorEditor == nullptr)
            return false;

        if (! wrapper->audioProcessorEditor->isResizable())
        {
            const auto preferredSize = wrapper->audioProcessorEditor->getPreferredSize();

            width = static_cast<uint32_t> (preferredSize.getWidth());
            height = static_cast<uint32_t> (preferredSize.getHeight());
        }

        wrapper->audioProcessorEditor->setSize ({ static_cast<float> (width), static_cast<float> (height) });

        return true;
    };

    extensionGUI.set_parent = [] (const clap_plugin_t* plugin, const clap_window_t* window) -> bool
    {
        DBG ("clap_plugin_gui_t::set_parent");

        jassert (std::string_view (window->api) == preferredApi);

        auto wrapper = getWrapper (plugin);
        if (wrapper->audioProcessorEditor == nullptr)
            return false;

        yup::ComponentNative::Flags flags = yup::ComponentNative::defaultFlags
                                          & ~yup::ComponentNative::decoratedWindow;

        if (wrapper->audioProcessorEditor->shouldRenderContinuous())
            flags.set (yup::ComponentNative::renderContinuous);

        yup::ComponentNative::Options options;
        options.flags = flags;
        wrapper->audioProcessorEditor->addToDesktop (options, window->cocoa);
        wrapper->audioProcessorEditor->attachedToNative();

        return true;
    };

    extensionGUI.set_transient = [] (const clap_plugin_t* plugin, const clap_window_t* window) -> bool
    {
        DBG ("clap_plugin_gui_t::set_transient");

        return false;
    };

    extensionGUI.suggest_title = [] (const clap_plugin_t* plugin, const char* title)
    {
        DBG ("clap_plugin_gui_t::suggest_title " << title);
    };

    extensionGUI.show = [] (const clap_plugin_t* plugin) -> bool
    {
        DBG ("clap_plugin_gui_t::show");

        auto wrapper = getWrapper (plugin);
        if (wrapper->audioProcessorEditor == nullptr)
            return false;

        wrapper->audioProcessorEditor->setVisible (true);
        return true;
    };

    extensionGUI.hide = [] (const clap_plugin_t* plugin) -> bool
    {
        DBG ("clap_plugin_gui_t::hide");

        auto wrapper = getWrapper (plugin);
        if (wrapper->audioProcessorEditor == nullptr)
            return false;

        wrapper->audioProcessorEditor->setVisible (false);
        return true;
    };

    return true;
}

//==============================================================================

void AudioPluginWrapperCLAP::destroy()
{
    plugin.plugin_data = nullptr;

    delete this;
}

//==============================================================================

bool AudioPluginWrapperCLAP::activate (float sampleRate, int samplesPerBlock)
{
    if (instancesCount.fetch_add (1) == 0)
    {
        hostTimerSupport = reinterpret_cast<const clap_host_timer_support_t*> (
            host->get_extension (host, CLAP_EXT_TIMER_SUPPORT));

        if (hostTimerSupport && hostTimerSupport->register_timer)
            hostTimerSupport->register_timer (host, 16, &timerID);
    }

    audioProcessor->prepareToPlay (sampleRate, samplesPerBlock);
    return true;
}

//==============================================================================

void AudioPluginWrapperCLAP::deactivate()
{
    audioProcessor->releaseResources();

    if (instancesCount.fetch_sub (1) == 1)
    {
        if (hostTimerSupport && hostTimerSupport->register_timer)
        {
            hostTimerSupport->unregister_timer (host, timerID);
            hostTimerSupport = nullptr;
        }
    }
}

//==============================================================================

bool AudioPluginWrapperCLAP::startProcessing()
{
    return true;
}

//==============================================================================

void AudioPluginWrapperCLAP::stopProcessing()
{
}

//==============================================================================

void AudioPluginWrapperCLAP::reset()
{
    audioProcessor->flush();
}

//==============================================================================

const void* AudioPluginWrapperCLAP::getExtension (std::string_view id)
{
    if (id == CLAP_EXT_NOTE_PORTS)
        return std::addressof (extensionNotePorts);
    if (id == CLAP_EXT_AUDIO_PORTS)
        return std::addressof (extensionAudioPorts);
    if (id == CLAP_EXT_PARAMS)
        return std::addressof (extensionParams);
    if (id == CLAP_EXT_STATE)
        return std::addressof (extensionState);
    if (id == CLAP_EXT_TIMER_SUPPORT)
        return std::addressof (extensionTimerSupport);
    if (id == CLAP_EXT_GUI)
        return std::addressof (extensionGUI);

    return nullptr;
}

//==============================================================================

const clap_plugin_t* AudioPluginWrapperCLAP::getPlugin() const
{
    return std::addressof (plugin);
}

} // namespace yup

//==============================================================================

static const clap_plugin_factory_t pluginFactory = {
    .get_plugin_count = [] (const clap_plugin_factory* factory) -> uint32_t
{
    DBG ("clap_plugin_factory_t::get_plugin_count");

    return 1;
},

    .get_plugin_descriptor = [] (const clap_plugin_factory* factory, uint32_t index) -> const clap_plugin_descriptor_t*
{
    DBG ("clap_plugin_factory_t::get_plugin_descriptor " << (int32_t) index);

    return index == 0 ? &yup::pluginDescriptor : nullptr;
},

    .create_plugin = [] (const clap_plugin_factory* factory, const clap_host_t* host, const char* pluginID) -> const clap_plugin_t*
{
    DBG ("clap_plugin_factory_t::create_plugin " << pluginID);

    if (! clap_version_is_compatible (host->clap_version) || std::string_view (pluginID) != yup::pluginDescriptor.id)
        return nullptr;

    auto wrapper = new yup::AudioPluginWrapperCLAP (host);
    return wrapper->getPlugin();
},
};

//==============================================================================

extern "C" const CLAP_EXPORT clap_plugin_entry_t clap_entry = {
    .clap_version = CLAP_VERSION_INIT,

    .init = [] (const char* path) -> bool
{
    DBG ("clap_plugin_entry_t::init " << path);

    yup::initialiseJuce_GUI();
    yup::initialiseYup_Windowing();

    return true;
},

    .deinit = []
{
    DBG ("clap_plugin_entry_t::deinit");

    yup::shutdownYup_Windowing();
    yup::shutdownJuce_GUI();
},

    .get_factory = [] (const char* factoryID) -> const void*
{
    DBG ("clap_plugin_entry_t::get_factory " << factoryID);

    if (std::string_view (factoryID) == CLAP_PLUGIN_FACTORY_ID)
        return std::addressof (pluginFactory);

    return nullptr;
},
};
