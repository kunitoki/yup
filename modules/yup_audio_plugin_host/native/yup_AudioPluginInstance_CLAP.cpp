/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP

namespace yup
{

namespace
{

//==============================================================================
struct CLAPModule
{
    CLAPModuleHandle handle = nullptr;
    const clap_plugin_entry_t* entry = nullptr;

    static std::unique_ptr<CLAPModule> load (const File& file)
    {
        auto m = std::make_unique<CLAPModule>();
        m->handle = clapLoadModule (file.getFullPathName().toRawUTF8());

        if (m->handle == nullptr)
            return nullptr;

        m->entry = reinterpret_cast<const clap_plugin_entry_t*> (
            clapGetAddress (m->handle, "clap_entry"));

        if (m->entry == nullptr || m->entry->init == nullptr)
        {
            clapUnloadModule (m->handle);
            return nullptr;
        }

        if (! m->entry->init (file.getFullPathName().toRawUTF8()))
        {
            clapUnloadModule (m->handle);
            return nullptr;
        }

        return m;
    }

    ~CLAPModule()
    {
        if (entry != nullptr && entry->deinit != nullptr)
            entry->deinit();

        if (handle != nullptr)
            clapUnloadModule (handle);
    }
};

//==============================================================================
// Minimal clap_host_t implementation supplied to each plugin instance.
struct YUPCLAPHost
{
    clap_host_t host {};
    clap_host_note_ports_t notePorts {};
    clap_host_latency_t latency {};
    clap_host_gui_t gui {};
    std::function<void()> latencyChanged;
    std::function<bool (uint32_t, uint32_t)> guiResizeRequested;
    std::function<bool()> guiShowRequested;
    std::function<bool()> guiHideRequested;
    std::function<void (bool)> guiClosed;
    String hostName;
    String hostVendor;
    String hostVersion;

    YUPCLAPHost (const AudioPluginHostContext& ctx)
        : hostName (ctx.hostName)
        , hostVendor (ctx.hostVendor)
        , hostVersion (ctx.hostVersion)
    {
        host.clap_version = CLAP_VERSION;
        host.host_data = this;
        host.name = hostName.toRawUTF8();
        host.vendor = hostVendor.toRawUTF8();
        host.url = "";
        host.version = hostVersion.toRawUTF8();
        notePorts.supported_dialects = [] (const clap_host_t*) -> uint32_t
        {
            return CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
        };
        notePorts.rescan = [] (const clap_host_t*, uint32_t) {};
        latency.changed = [] (const clap_host_t* host)
        {
            auto* self = static_cast<YUPCLAPHost*> (host->host_data);
            if (self->latencyChanged != nullptr)
                self->latencyChanged();
        };
        gui.resize_hints_changed = [] (const clap_host_t*) {};
        gui.request_resize = [] (const clap_host_t* host, uint32_t width, uint32_t height) -> bool
        {
            if (! MessageManager::existsAndIsCurrentThread())
                return false;

            auto* self = static_cast<YUPCLAPHost*> (host->host_data);
            return self->guiResizeRequested != nullptr && self->guiResizeRequested (width, height);
        };
        gui.request_show = [] (const clap_host_t* host) -> bool
        {
            if (! MessageManager::existsAndIsCurrentThread())
                return false;

            auto* self = static_cast<YUPCLAPHost*> (host->host_data);
            return self->guiShowRequested != nullptr && self->guiShowRequested();
        };
        gui.request_hide = [] (const clap_host_t* host) -> bool
        {
            if (! MessageManager::existsAndIsCurrentThread())
                return false;

            auto* self = static_cast<YUPCLAPHost*> (host->host_data);
            return self->guiHideRequested != nullptr && self->guiHideRequested();
        };
        gui.closed = [] (const clap_host_t* host, bool wasDestroyed)
        {
            if (! MessageManager::existsAndIsCurrentThread())
                return;

            auto* self = static_cast<YUPCLAPHost*> (host->host_data);
            if (self->guiClosed != nullptr)
                self->guiClosed (wasDestroyed);
        };

        host.get_extension = [] (const clap_host_t* host, const char* extensionId) -> const void*
        {
            auto* self = static_cast<YUPCLAPHost*> (host->host_data);
            if (std::strcmp (extensionId, CLAP_EXT_NOTE_PORTS) == 0)
                return &self->notePorts;

            if (std::strcmp (extensionId, CLAP_EXT_LATENCY) == 0)
                return &self->latency;

            if (std::strcmp (extensionId, CLAP_EXT_GUI) == 0)
                return &self->gui;

            return nullptr;
        };
        host.request_restart = [] (const clap_host_t*) {};
        host.request_process = [] (const clap_host_t*) {};
        host.request_callback = [] (const clap_host_t*) {};
    }
};

//==============================================================================
#if YUP_MAC
void* getCLAPParentViewFromNativeHandle (void* nativeHandle)
{
    if (nativeHandle == nullptr)
        return nullptr;

    id nativeObject = (__bridge id) nativeHandle;
    if ([nativeObject isKindOfClass:[NSWindow class]])
        return (__bridge void*) [(NSWindow*) nativeObject contentView];

    if ([nativeObject isKindOfClass:[NSView class]])
        return nativeHandle;

    return nullptr;
}
#endif

const char* getCLAPWindowApi()
{
#if YUP_MAC
    return CLAP_WINDOW_API_COCOA;
#elif YUP_WINDOWS
    return CLAP_WINDOW_API_WIN32;
#elif YUP_LINUX
    return CLAP_WINDOW_API_X11;
#else
    return nullptr;
#endif
}

bool initialiseCLAPWindow (clap_window_t& window, void* nativeHandle)
{
    if (nativeHandle == nullptr)
        return false;

    window.api = getCLAPWindowApi();
    if (window.api == nullptr)
        return false;

#if YUP_MAC
    window.cocoa = getCLAPParentViewFromNativeHandle (nativeHandle);
    return window.cocoa != nullptr;
#elif YUP_WINDOWS
    window.win32 = nativeHandle;
    return true;
#elif YUP_LINUX
    window.x11 = static_cast<clap_xwnd> (reinterpret_cast<std::uintptr_t> (nativeHandle));
    return window.x11 != 0;
#else
    ignoreUnused (window);
    return false;
#endif
}

bool canCreateCLAPEditor (const clap_plugin_t* plugin)
{
    if (plugin == nullptr)
        return false;

    const auto* gui = reinterpret_cast<const clap_plugin_gui_t*> (
        plugin->get_extension (plugin, CLAP_EXT_GUI));

    const auto* windowApi = getCLAPWindowApi();
    return gui != nullptr
        && windowApi != nullptr
        && gui->is_api_supported != nullptr
        && gui->is_api_supported (plugin, windowApi, false);
}

class CLAPEditor final : public AudioProcessorEditor
{
public:
    static std::unique_ptr<CLAPEditor> create (const clap_plugin_t* plugin, YUPCLAPHost& host)
    {
        if (! canCreateCLAPEditor (plugin))
            return nullptr;

        const auto* gui = reinterpret_cast<const clap_plugin_gui_t*> (
            plugin->get_extension (plugin, CLAP_EXT_GUI));

        if (gui == nullptr || gui->create == nullptr || ! gui->create (plugin, getCLAPWindowApi(), false))
            return nullptr;

        return std::unique_ptr<CLAPEditor> (new CLAPEditor (plugin, gui, host));
    }

    ~CLAPEditor() override
    {
        detachPlugView();

        host.guiResizeRequested = nullptr;
        host.guiShowRequested = nullptr;
        host.guiHideRequested = nullptr;
        host.guiClosed = nullptr;

        if (gui != nullptr && gui->destroy != nullptr)
            gui->destroy (clapPlugin);
    }

    bool isResizable() const override
    {
        return gui != nullptr && gui->can_resize != nullptr && gui->can_resize (clapPlugin);
    }

    Size<int> getPreferredSize() const override { return preferredSize; }

    void paint (Graphics& g) override
    {
        g.setFillColor (Color (0xff101417));
        g.fillAll();
    }

    void resized() override
    {
        resizePlugViewToBounds();
    }

    void attachedToNative() override
    {
        attachPlugView();
    }

    void detachedFromNative() override
    {
        detachPlugView();
    }

private:
    CLAPEditor (const clap_plugin_t* plugin, const clap_plugin_gui_t* guiExtension, YUPCLAPHost& hostToUse)
        : clapPlugin (plugin)
        , gui (guiExtension)
        , host (hostToUse)
    {
        uint32_t width = 0;
        uint32_t height = 0;

        if (gui != nullptr
            && gui->get_size != nullptr
            && gui->get_size (clapPlugin, &width, &height)
            && width > 0
            && height > 0)
        {
            preferredSize = {
                jmax (320, static_cast<int> (width)),
                jmax (240, static_cast<int> (height))
            };
        }

        setSize (preferredSize.to<float>());

        host.guiResizeRequested = [this] (uint32_t widthToUse, uint32_t heightToUse)
        {
            return handleResizeRequest (widthToUse, heightToUse);
        };
        host.guiShowRequested = [this]
        {
            return handleShowRequest();
        };
        host.guiHideRequested = [this]
        {
            return handleHideRequest();
        };
        host.guiClosed = [this] (bool)
        {
            shown = false;
            attached = false;
        };
    }

    bool handleResizeRequest (uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
            return false;

        preferredSize = {
            jmax (1, static_cast<int> (width)),
            jmax (1, static_cast<int> (height))
        };

        if (auto* topLevel = getTopLevelComponent())
            topLevel->setSize (preferredSize.to<float>());
        else
            setSize (preferredSize.to<float>());

        return true;
    }

    bool handleShowRequest()
    {
        if (auto* topLevel = getTopLevelComponent())
        {
            topLevel->setVisible (true);
            return true;
        }

        return false;
    }

    bool handleHideRequest()
    {
        if (auto* topLevel = getTopLevelComponent())
        {
            topLevel->setVisible (false);
            return true;
        }

        return false;
    }

    void attachPlugView()
    {
        if (gui == nullptr || clapPlugin == nullptr || attached)
            return;

        auto* nativeComponent = getNativeComponent();
        if (nativeComponent == nullptr)
            return;

        clap_window_t parentWindow {};
        if (! initialiseCLAPWindow (parentWindow, nativeComponent->getNativeHandle()))
            return;

        if (gui->set_parent == nullptr || ! gui->set_parent (clapPlugin, &parentWindow))
            return;

        attached = true;
        resizePlugViewToBounds();

        if (! shown && gui->show != nullptr)
            shown = gui->show (clapPlugin);
    }

    void detachPlugView()
    {
        if (gui == nullptr || clapPlugin == nullptr)
            return;

        if (shown && gui->hide != nullptr)
            gui->hide (clapPlugin);

        shown = false;
        attached = false;
    }

    void resizePlugViewToBounds()
    {
        if (gui == nullptr || clapPlugin == nullptr || ! attached || gui->set_size == nullptr)
            return;

        const auto bounds = getBoundsRelativeToTopLevelComponent();
        uint32_t width = static_cast<uint32_t> (jmax (1.0f, bounds.getWidth()));
        uint32_t height = static_cast<uint32_t> (jmax (1.0f, bounds.getHeight()));

        if (gui->adjust_size != nullptr)
            gui->adjust_size (clapPlugin, &width, &height);

        if (width > 0 && height > 0)
            gui->set_size (clapPlugin, width, height);
    }

    const clap_plugin_t* clapPlugin = nullptr;
    const clap_plugin_gui_t* gui = nullptr;
    YUPCLAPHost& host;
    Size<int> preferredSize { 640, 480 };
    bool attached = false;
    bool shown = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CLAPEditor)
};

struct CLAPInputEvents
{
    std::deque<clap_event_param_value_t> parameterEvents;
    std::deque<clap_event_note_t> noteEvents;
    std::deque<clap_event_midi_t> midiEvents;
    std::deque<clap_event_midi_sysex_t> sysexEvents;
    std::vector<const clap_event_header_t*> eventHeaders;
    clap_input_events_t inputEvents {};

    CLAPInputEvents()
    {
        inputEvents.ctx = this;
        inputEvents.size = [] (const clap_input_events_t* events) -> uint32_t
        {
            auto* self = static_cast<const CLAPInputEvents*> (events->ctx);
            return static_cast<uint32_t> (self->eventHeaders.size());
        };
        inputEvents.get = [] (const clap_input_events_t* events, uint32_t index) -> const clap_event_header_t*
        {
            auto* self = static_cast<const CLAPInputEvents*> (events->ctx);
            if (index >= self->eventHeaders.size())
                return nullptr;

            return self->eventHeaders[static_cast<std::size_t> (index)];
        };
    }

    void clear()
    {
        parameterEvents.clear();
        noteEvents.clear();
        midiEvents.clear();
        sysexEvents.clear();
        eventHeaders.clear();
    }

    void addParameterValue (clap_id id, double value)
    {
        clap_event_param_value_t event {};
        event.header.size = sizeof (event);
        event.header.time = 0;
        event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        event.header.type = CLAP_EVENT_PARAM_VALUE;
        event.param_id = id;
        event.note_id = -1;
        event.port_index = -1;
        event.channel = -1;
        event.key = -1;
        event.value = value;

        parameterEvents.push_back (event);
        eventHeaders.push_back (&parameterEvents.back().header);
    }

    void addMidiMessage (const MidiMessageMetadata& metadata)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn() || message.isNoteOff())
        {
            clap_event_note_t event {};
            event.header.size = sizeof (event);
            event.header.time = static_cast<uint32_t> (jmax (0, metadata.samplePosition));
            event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            event.header.type = message.isNoteOn() ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF;
            event.note_id = -1;
            event.port_index = 0;
            event.channel = static_cast<int16_t> (message.getChannel() - 1);
            event.key = static_cast<int16_t> (message.getNoteNumber());
            event.velocity = static_cast<double> (message.getFloatVelocity());

            noteEvents.push_back (event);
            eventHeaders.push_back (&noteEvents.back().header);
            return;
        }

        if (message.isSysEx())
        {
            clap_event_midi_sysex_t event {};
            event.header.size = sizeof (event);
            event.header.time = static_cast<uint32_t> (jmax (0, metadata.samplePosition));
            event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            event.header.type = CLAP_EVENT_MIDI_SYSEX;
            event.port_index = 0;
            event.buffer = message.getSysExData();
            event.size = static_cast<uint32_t> (message.getSysExDataSize());

            sysexEvents.push_back (event);
            eventHeaders.push_back (&sysexEvents.back().header);
            return;
        }

        if (metadata.numBytes > 0 && metadata.numBytes <= 3)
        {
            clap_event_midi_t event {};
            event.header.size = sizeof (event);
            event.header.time = static_cast<uint32_t> (jmax (0, metadata.samplePosition));
            event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            event.header.type = CLAP_EVENT_MIDI;
            event.port_index = 0;
            std::memcpy (event.data, metadata.data, static_cast<std::size_t> (metadata.numBytes));

            midiEvents.push_back (event);
            eventHeaders.push_back (&midiEvents.back().header);
        }
    }
};

struct CLAPOutputEvents
{
    clap_output_events_t outputEvents {};
    MidiBuffer* midiOutput = nullptr;

    CLAPOutputEvents()
    {
        outputEvents.ctx = this;
        outputEvents.try_push = [] (const clap_output_events_t* events, const clap_event_header_t* event) -> bool
        {
            auto* self = static_cast<CLAPOutputEvents*> (events->ctx);
            if (self == nullptr || self->midiOutput == nullptr || event == nullptr)
                return false;

            self->addEventToMidiBuffer (*event);
            return true;
        };
    }

    void addEventToMidiBuffer (const clap_event_header_t& event)
    {
        if (event.space_id != CLAP_CORE_EVENT_SPACE_ID)
            return;

        const int samplePosition = static_cast<int> (event.time);

        if (event.type == CLAP_EVENT_NOTE_ON
            || event.type == CLAP_EVENT_NOTE_OFF
            || event.type == CLAP_EVENT_NOTE_CHOKE
            || event.type == CLAP_EVENT_NOTE_END)
        {
            const auto& note = *reinterpret_cast<const clap_event_note_t*> (&event);
            const int channel = note.channel >= 0 ? note.channel + 1 : 1;
            const int key = note.key >= 0 ? note.key : 0;
            const auto velocity = static_cast<float> (note.velocity);

            midiOutput->addEvent (event.type == CLAP_EVENT_NOTE_ON
                                      ? MidiMessage::noteOn (channel, key, velocity)
                                      : MidiMessage::noteOff (channel, key, velocity),
                                  samplePosition);
        }
        else if (event.type == CLAP_EVENT_MIDI)
        {
            const auto& midi = *reinterpret_cast<const clap_event_midi_t*> (&event);
            const int numBytes = MidiMessage::getMessageLengthFromFirstByte (midi.data[0]);
            midiOutput->addEvent (midi.data, numBytes, samplePosition);
        }
        else if (event.type == CLAP_EVENT_MIDI_SYSEX)
        {
            const auto& sysex = *reinterpret_cast<const clap_event_midi_sysex_t*> (&event);
            midiOutput->addEvent (MidiMessage::createSysExMessage (sysex.buffer, static_cast<int> (sysex.size)),
                                  samplePosition);
        }
    }
};

} // namespace

//==============================================================================

class CLAPInstance : public AudioPluginInstance
{
public:
    CLAPInstance (const AudioPluginDescription& desc,
                  const AudioPluginHostContext& context,
                  std::unique_ptr<CLAPModule> module,
                  std::unique_ptr<YUPCLAPHost> clapHost,
                  const clap_plugin_t* plugin)
        : AudioPluginInstance (desc, buildBusLayout (plugin))
        , hostContext (context)
        , clapModule (std::move (module))
        , yupHost (std::move (clapHost))
        , clapPlugin (plugin)
    {
        yupHost->latencyChanged = [this]
        {
            setLatencySamples (getLatencySamples());
        };

        buildParameterList();
        setNonRealtime (context.isNonRealtime);
    }

    ~CLAPInstance() override
    {
        releaseResources();
        yupHost->latencyChanged = nullptr;

        if (clapPlugin != nullptr)
        {
            clapPlugin->destroy (clapPlugin);
            clapPlugin = nullptr;
        }
    }

    //==============================================================================

    void prepareToPlay (float sampleRate, int maxBlockSize) override
    {
        const int numChannels = jmax (2, pluginDescription.numInputChannels, pluginDescription.numOutputChannels);
        preparedInPtrs.resize (static_cast<std::size_t> (numChannels));
        preparedOutPtrs.resize (static_cast<std::size_t> (numChannels));

        updateRenderMode();
        clapPlugin->activate (clapPlugin, sampleRate, 1, static_cast<uint32_t> (jmax (1, maxBlockSize)));
        updateRenderMode();

        clapPlugin->start_processing (clapPlugin);
    }

    void releaseResources() override
    {
        if (clapPlugin != nullptr)
        {
            clapPlugin->stop_processing (clapPlugin);
            clapPlugin->deactivate (clapPlugin);
            currentRenderMode = -1;
        }
    }

    void processBlock (AudioProcessContext<float>& context) override
    {
        ScopedNoDenormals noDenormals;

        if (isBypassed())
        {
            processBlockBypassed (context);
            return;
        }

        auto& audioBuffer = context.audio;
        auto& midiBuffer = context.midi;

        const int numSamples = audioBuffer.getNumSamples();
        const int numChannels = audioBuffer.getNumChannels();

        if (static_cast<int> (preparedInPtrs.size()) < numChannels && numChannels > 0)
        {
            jassertfalse;
            return;
        }

        for (int c = 0; c < numChannels; ++c)
        {
            preparedInPtrs[static_cast<std::size_t> (c)] = audioBuffer.getReadPointer (c);
            preparedOutPtrs[static_cast<std::size_t> (c)] = audioBuffer.getWritePointer (c);
        }

        clap_audio_buffer_t inputBuf {};
        if (pluginDescription.numInputChannels > 0 && numChannels > 0)
        {
            inputBuf.data32 = const_cast<float**> (preparedInPtrs.data());
            inputBuf.channel_count = static_cast<uint32_t> (numChannels);
        }
        inputBuf.latency = 0;
        inputBuf.constant_mask = 0;

        clap_audio_buffer_t outputBuf {};
        if (pluginDescription.numOutputChannels > 0 && numChannels > 0)
        {
            outputBuf.data32 = preparedOutPtrs.data();
            outputBuf.channel_count = static_cast<uint32_t> (numChannels);
        }

        clap_process_t process {};
        process.steady_time = -1;
        process.frames_count = static_cast<uint32_t> (numSamples);
        process.audio_inputs = inputBuf.channel_count > 0 ? &inputBuf : nullptr;
        process.audio_inputs_count = inputBuf.channel_count > 0 ? 1 : 0;
        process.audio_outputs = outputBuf.channel_count > 0 ? &outputBuf : nullptr;
        process.audio_outputs_count = outputBuf.channel_count > 0 ? 1 : 0;

        clapInputEvents.clear();
        const auto params = getParameters();
        const auto numParams = yup::jmin (params.size(), clapParameterIds.size());

        for (std::size_t i = 0; i < numParams; ++i)
            clapInputEvents.addParameterValue (clapParameterIds[i], static_cast<double> (params[i]->getValue()));

        for (const auto& metadata : midiBuffer)
            clapInputEvents.addMidiMessage (metadata);

        process.in_events = &clapInputEvents.inputEvents;
        process.out_events = &clapOutputEvents.outputEvents;

        outputMidiBuffer.clear();
        clapOutputEvents.midiOutput = &outputMidiBuffer;

        clapPlugin->process (clapPlugin, &process);

        clapOutputEvents.midiOutput = nullptr;
        midiBuffer.clear();
        midiBuffer.addEvents (outputMidiBuffer, 0, -1, 0);
    }

    //==============================================================================

    int getCurrentPreset() const noexcept override { return currentPreset; }

    void setCurrentPreset (int index) noexcept override
    {
        currentPreset = index;
    }

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    //==============================================================================

    Result loadStateFromMemory (const MemoryBlock& memoryBlock) override
    {
        auto* stateExt = reinterpret_cast<const clap_plugin_state_t*> (
            clapPlugin->get_extension (clapPlugin, CLAP_EXT_STATE));

        if (stateExt == nullptr)
            return Result::fail ("Plugin does not support CLAP state extension");

        struct MemStream
        {
            const MemoryBlock* block;
            std::size_t position = 0;
        };

        MemStream ms { &memoryBlock };

        clap_istream_t stream {};
        stream.ctx = &ms;
        stream.read = [] (const clap_istream_t* s, void* buf, uint64_t size) -> int64_t
        {
            auto* ms = static_cast<MemStream*> (s->ctx);
            const std::size_t remaining = ms->block->getSize() - ms->position;
            const std::size_t toRead = std::min (static_cast<std::size_t> (size), remaining);

            std::memcpy (buf,
                         static_cast<const char*> (ms->block->getData()) + ms->position,
                         toRead);
            ms->position += toRead;
            return static_cast<int64_t> (toRead);
        };

        return stateExt->load (clapPlugin, &stream)
                 ? Result::ok()
                 : Result::fail ("CLAP state load failed");
    }

    Result saveStateIntoMemory (MemoryBlock& memoryBlock) override
    {
        auto* stateExt = reinterpret_cast<const clap_plugin_state_t*> (
            clapPlugin->get_extension (clapPlugin, CLAP_EXT_STATE));

        if (stateExt == nullptr)
            return Result::fail ("Plugin does not support CLAP state extension");

        memoryBlock.reset();

        clap_ostream_t stream {};
        stream.ctx = &memoryBlock;
        stream.write = [] (const clap_ostream_t* s, const void* buf, uint64_t size) -> int64_t
        {
            auto* block = static_cast<MemoryBlock*> (s->ctx);
            block->append (buf, static_cast<std::size_t> (size));
            return static_cast<int64_t> (size);
        };

        return stateExt->save (clapPlugin, &stream)
                 ? Result::ok()
                 : Result::fail ("CLAP state save failed");
    }

    //==============================================================================

    bool hasEditor() const override
    {
        return canCreateCLAPEditor (clapPlugin);
    }

    AudioProcessorEditor* createEditor() override
    {
        if (auto editor = CLAPEditor::create (clapPlugin, *yupHost))
            return editor.release();

        return nullptr;
    }

    int getLatencySamples() override
    {
        auto* latencyExt = reinterpret_cast<const clap_plugin_latency_t*> (
            clapPlugin->get_extension (clapPlugin, CLAP_EXT_LATENCY));

        return latencyExt != nullptr ? static_cast<int> (latencyExt->get (clapPlugin)) : 0;
    }

    //==============================================================================

    static std::unique_ptr<CLAPInstance> create (const AudioPluginDescription& desc,
                                                 const AudioPluginHostContext& context)
    {
        auto mod = CLAPModule::load (File (desc.fileOrBundlePath));
        if (mod == nullptr)
            return nullptr;

        const clap_plugin_factory_t* factory = reinterpret_cast<const clap_plugin_factory_t*> (
            mod->entry->get_factory (CLAP_PLUGIN_FACTORY_ID));

        if (factory == nullptr)
            return nullptr;

        auto yupHost = std::make_unique<YUPCLAPHost> (context);

        const uint32_t count = factory->get_plugin_count (factory);
        for (uint32_t i = 0; i < count; ++i)
        {
            const clap_plugin_descriptor_t* plugDesc = factory->get_plugin_descriptor (factory, i);
            if (plugDesc == nullptr)
                continue;

            if (String (plugDesc->id) != desc.identifier)
                continue;

            const clap_plugin_t* plugin = factory->create_plugin (factory,
                                                                  &yupHost->host,
                                                                  plugDesc->id);
            if (plugin == nullptr)
                continue;

            if (! plugin->init (plugin))
            {
                plugin->destroy (plugin);
                continue;
            }

            return std::make_unique<CLAPInstance> (desc, context, std::move (mod), std::move (yupHost), plugin);
        }

        return nullptr;
    }

private:
    static AudioBusLayout buildBusLayout (const clap_plugin_t* plugin)
    {
        std::vector<AudioBus> inputs, outputs;

        auto* portsExt = reinterpret_cast<const clap_plugin_audio_ports_t*> (
            plugin->get_extension (plugin, CLAP_EXT_AUDIO_PORTS));

        if (portsExt != nullptr)
        {
            const uint32_t numInputs = portsExt->count (plugin, true);
            for (uint32_t i = 0; i < numInputs; ++i)
            {
                clap_audio_port_info_t info {};
                portsExt->get (plugin, i, true, &info);
                inputs.emplace_back (String (info.name), AudioBus::Type::Audio, AudioBus::Direction::Input, static_cast<int> (info.channel_count));
            }

            const uint32_t numOutputs = portsExt->count (plugin, false);
            for (uint32_t i = 0; i < numOutputs; ++i)
            {
                clap_audio_port_info_t info {};
                portsExt->get (plugin, i, false, &info);
                outputs.emplace_back (String (info.name), AudioBus::Type::Audio, AudioBus::Direction::Output, static_cast<int> (info.channel_count));
            }
        }

        auto* notePortsExt = reinterpret_cast<const clap_plugin_note_ports_t*> (
            plugin->get_extension (plugin, CLAP_EXT_NOTE_PORTS));

        if (notePortsExt != nullptr)
        {
            const uint32_t numInputs = notePortsExt->count (plugin, true);
            for (uint32_t i = 0; i < numInputs; ++i)
            {
                clap_note_port_info_t info {};
                if (notePortsExt->get (plugin, i, true, &info))
                    inputs.emplace_back (String (info.name), AudioBus::Type::MIDI, AudioBus::Direction::Input, 1);
            }

            const uint32_t numOutputs = notePortsExt->count (plugin, false);
            for (uint32_t i = 0; i < numOutputs; ++i)
            {
                clap_note_port_info_t info {};
                if (notePortsExt->get (plugin, i, false, &info))
                    outputs.emplace_back (String (info.name), AudioBus::Type::MIDI, AudioBus::Direction::Output, 1);
            }
        }

        return AudioBusLayout (std::move (inputs), std::move (outputs));
    }

    void buildParameterList()
    {
        auto* paramsExt = reinterpret_cast<const clap_plugin_params_t*> (
            clapPlugin->get_extension (clapPlugin, CLAP_EXT_PARAMS));

        if (paramsExt == nullptr)
            return;

        const uint32_t count = paramsExt->count (clapPlugin);
        for (uint32_t i = 0; i < count; ++i)
        {
            clap_param_info_t info {};
            if (! paramsExt->get_info (clapPlugin, i, &info))
                continue;

            auto param = AudioParameterBuilder()
                             .withID (String (static_cast<int64> (info.id)))
                             .withName (String (info.name))
                             .withRange (static_cast<float> (info.min_value),
                                         static_cast<float> (info.max_value))
                             .withDefault (static_cast<float> (info.default_value))
                             .build();

            clapParameterIds.push_back (info.id);
            addParameter (std::move (param));
        }
    }

    void updateRenderMode()
    {
        if (clapPlugin == nullptr)
            return;

        const auto mode = isNonRealtime() ? CLAP_RENDER_OFFLINE : CLAP_RENDER_REALTIME;
        if (currentRenderMode == mode)
            return;

        auto* renderExt = reinterpret_cast<const clap_plugin_render_t*> (
            clapPlugin->get_extension (clapPlugin, CLAP_EXT_RENDER));

        if (renderExt != nullptr && renderExt->set != nullptr)
        {
            if (mode == CLAP_RENDER_OFFLINE
                && renderExt->has_hard_realtime_requirement != nullptr
                && renderExt->has_hard_realtime_requirement (clapPlugin))
            {
                return;
            }

            if (renderExt->set (clapPlugin, mode))
                currentRenderMode = mode;
        }
    }

    void nonRealtimeStateChanged() override
    {
        updateRenderMode();
    }

    AudioPluginHostContext hostContext;
    std::unique_ptr<CLAPModule> clapModule;
    std::unique_ptr<YUPCLAPHost> yupHost;
    const clap_plugin_t* clapPlugin = nullptr;
    CLAPInputEvents clapInputEvents;
    CLAPOutputEvents clapOutputEvents;
    std::vector<const float*> preparedInPtrs;
    std::vector<float*> preparedOutPtrs;
    MidiBuffer outputMidiBuffer;
    std::vector<clap_id> clapParameterIds;
    clap_plugin_render_mode currentRenderMode = -1;
    int currentPreset = 0;
};

//==============================================================================

CLAPFormat::CLAPFormat() = default;
CLAPFormat::~CLAPFormat() = default;

AudioPluginFormatType CLAPFormat::getFormatType() const
{
    return AudioPluginFormatType::clap;
}

String CLAPFormat::getFormatName() const
{
    return "CLAP";
}

StringArray CLAPFormat::getFileExtensions() const
{
    return { ".clap" };
}

FileSearchPath CLAPFormat::getDefaultSearchPaths() const
{
    FileSearchPath paths;

#if YUP_MAC
    paths.add (File ("/Library/Audio/Plug-Ins/CLAP"));
    paths.add (File::getSpecialLocation (File::userHomeDirectory).getChildFile ("Library/Audio/Plug-Ins/CLAP"));
#elif YUP_WINDOWS
    if (const char* pf = getenv ("CommonProgramFiles"))
        paths.add (File (String (pf) + "\\CLAP"));
    if (const char* appdata = getenv ("APPDATA"))
        paths.add (File (String (appdata) + "\\CLAP"));
#elif YUP_LINUX
    paths.add (File ("/usr/lib/clap"));
    paths.add (File ("/usr/local/lib/clap"));
    paths.add (File::getSpecialLocation (File::userHomeDirectory).getChildFile (".clap"));
#endif

    return paths;
}

ResultValue<std::vector<AudioPluginDescription>> CLAPFormat::scanFile (const File& file)
{
    if (file.getFileExtension().toLowerCase() != ".clap")
        return makeResultValueFail ("Not a CLAP file");

    auto mod = CLAPModule::load (file);
    if (mod == nullptr)
        return makeResultValueFail ("Failed to load CLAP module: " + file.getFullPathName());

    const clap_plugin_factory_t* factory = reinterpret_cast<const clap_plugin_factory_t*> (
        mod->entry->get_factory (CLAP_PLUGIN_FACTORY_ID));

    if (factory == nullptr)
        return makeResultValueFail ("No plugin factory in: " + file.getFullPathName());

    std::vector<AudioPluginDescription> results;
    const uint32_t count = factory->get_plugin_count (factory);

    for (uint32_t i = 0; i < count; ++i)
    {
        const clap_plugin_descriptor_t* plugDesc = factory->get_plugin_descriptor (factory, i);
        if (plugDesc == nullptr)
            continue;

        AudioPluginDescription desc;
        desc.formatType = AudioPluginFormatType::clap;
        desc.fileOrBundlePath = file.getFullPathName();
        desc.name = String (plugDesc->name);
        desc.vendor = String (plugDesc->vendor);
        desc.version = String (plugDesc->version);
        desc.identifier = String (plugDesc->id);

        if (plugDesc->features != nullptr)
        {
            for (int j = 0; plugDesc->features[j] != nullptr; ++j)
            {
                const String feature (plugDesc->features[j]);
                if (feature == CLAP_PLUGIN_FEATURE_INSTRUMENT)
                    desc.isInstrument = true;
                if (feature == CLAP_PLUGIN_FEATURE_AUDIO_EFFECT)
                    desc.isEffect = true;
            }
        }

        // Briefly instantiate the plugin to collect channel counts
        {
            clap_host_t host {};
            host.clap_version = CLAP_VERSION;
            host.host_data = nullptr;
            host.name = "YUP Scanner";
            host.vendor = "yup";
            host.url = "";
            host.version = "1.0.0";
            host.get_extension = [] (const clap_host_t*, const char*) -> const void*
            {
                return nullptr;
            };
            host.request_restart = [] (const clap_host_t*) {};
            host.request_process = [] (const clap_host_t*) {};
            host.request_callback = [] (const clap_host_t*) {};

            const clap_plugin_t* plugin = factory->create_plugin (factory, &host, plugDesc->id);
            if (plugin != nullptr && plugin->init (plugin))
            {
                auto* portsExt = reinterpret_cast<const clap_plugin_audio_ports_t*> (
                    plugin->get_extension (plugin, CLAP_EXT_AUDIO_PORTS));

                if (portsExt != nullptr)
                {
                    const uint32_t numInputs = portsExt->count (plugin, true);
                    for (uint32_t p = 0; p < numInputs; ++p)
                    {
                        clap_audio_port_info_t info {};
                        if (portsExt->get (plugin, p, true, &info))
                            desc.numInputChannels += static_cast<int> (info.channel_count);
                    }

                    const uint32_t numOutputs = portsExt->count (plugin, false);
                    for (uint32_t p = 0; p < numOutputs; ++p)
                    {
                        clap_audio_port_info_t info {};
                        if (portsExt->get (plugin, p, false, &info))
                            desc.numOutputChannels += static_cast<int> (info.channel_count);
                    }
                }

                auto* notePortsExt = reinterpret_cast<const clap_plugin_note_ports_t*> (
                    plugin->get_extension (plugin, CLAP_EXT_NOTE_PORTS));

                if (notePortsExt != nullptr)
                {
                    desc.numMidiInputPorts = static_cast<int> (notePortsExt->count (plugin, true));
                    desc.numMidiOutputPorts = static_cast<int> (notePortsExt->count (plugin, false));
                }

                plugin->destroy (plugin);
            }
        }

        results.push_back (std::move (desc));
    }

    if (results.empty())
        return makeResultValueFail ("No plugins found in: " + file.getFullPathName());

    return makeResultValueOk (std::move (results));
}

ResultValue<std::unique_ptr<AudioPluginInstance>> CLAPFormat::loadPlugin (
    const AudioPluginDescription& description,
    const AudioPluginHostContext& context)
{
    auto instance = CLAPInstance::create (description, context);

    if (instance == nullptr)
        return makeResultValueFail ("Failed to load CLAP plugin: " + description.name);

    return makeResultValueOk (std::move (instance));
}

} // namespace yup

#endif // YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP
