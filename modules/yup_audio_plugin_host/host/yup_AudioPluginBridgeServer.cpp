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


namespace yup
{

//==============================================================================
/** The plugin loaded by a bridge server, with its audio thread and editor window. */
class AudioPluginBridgeServer::Session final
    : private AudioParameter::Listener
    , private AudioProcessorBase::Listener
    , private Timer
    , private Thread
{
public:
    using Protocol = AudioPluginBridgeProtocol;

    Session (AudioPluginBridgeServer& serverToUse, std::unique_ptr<AudioPluginInstance> instanceToUse)
        : Thread ("yup_PluginBridgeAudio")
        , server (serverToUse)
        , instance (std::move (instanceToUse))
    {
        const auto parameters = instance->getParameters();
        toHost.resize (static_cast<int> (parameters.size()));

        for (const auto& parameter : parameters)
            parameter->addListener (this);

        instance->addListener (this);
        startTimerHz (30);
    }

    ~Session() override
    {
        stopTimer();
        closeEditor();
        release();

        instance->removeListener (this);

        for (const auto& parameter : instance->getParameters())
            parameter->removeListener (this);
    }

    AudioPluginInstance& getInstance() noexcept { return *instance; }

    //==============================================================================

    void writeInfo (OutputStream& stream)
    {
        Protocol::writeBusLayout (stream, instance->getBusLayout());

        const auto parameters = instance->getParameters();
        stream.writeInt (static_cast<int> (parameters.size()));

        for (const auto& parameter : parameters)
            Protocol::writeParameter (stream, *parameter);

        stream.writeInt (instance->getLatencySamples());
        stream.writeInt (instance->getTailSamples());

        for (const auto flag : { instance->hasEditor(), instance->acceptsMidi(), instance->producesMidi(), instance->supportsNoteExpression(), instance->supportsMpe() })
            stream.writeBool (flag);

        Protocol::writePresets (stream, *instance);
    }

    //==============================================================================

    Result prepare (float sampleRate, int maxBlockSize, const String& sharedMemoryName, int numInputChannels, int numWorkChannels)
    {
        release();

        const Protocol::SharedLayout layout (maxBlockSize, numInputChannels, numWorkChannels);

        if (auto result = sharedMemory.open (sharedMemoryName, layout.totalSize); result.failed())
            return result;

        if (auto result = requestSemaphore.open (Protocol::getRequestSemaphoreName (sharedMemoryName)); result.failed())
            return result;

        if (auto result = doneSemaphore.open (Protocol::getDoneSemaphoreName (sharedMemoryName)); result.failed())
            return result;

        block = Protocol::SharedBlock (sharedMemory.getData(), layout);
        blockSize = layout.blockSize;

        midi.ensureSize (static_cast<std::size_t> (Protocol::midiCapacityBytes));
        parameterChanges.reserve (Protocol::maxParameterChanges);
        noteExpressions.reserve (Protocol::maxNoteExpressions);
        buildBusViews();

        instance->setPlaybackConfiguration (sampleRate, layout.blockSize);
        instance->prepareToPlay (AudioSpec (sampleRate, layout.blockSize, jmax (numInputChannels, numWorkChannels)));
        prepared = true;

        startThread (Priority::highest);
        return Result::ok();
    }

    void release()
    {
        if (isThreadRunning())
        {
            signalThreadShouldExit();
            requestSemaphore.signal();
            stopThread (-1);
        }

        if (prepared)
        {
            instance->releaseResources();
            prepared = false;
        }

        block = {};
        requestSemaphore.close();
        doneSemaphore.close();
        sharedMemory.close();
    }

    //==============================================================================

    void openEditor (uint64 ownerWindowId, int ownerProcessId)
    {
        if (editorWindow == nullptr)
        {
            auto editor = std::unique_ptr<AudioProcessorEditor> (instance->hasEditor() ? instance->createEditor() : nullptr);
            if (editor == nullptr)
            {
                sendEditorClosed();
                return;
            }

            const auto preferredSize = editor->getPreferredSize();

            editorWindow = std::make_unique<EditorWindow> (instance->getName(), std::move (editor), [this]
            {
                closeEditor();
                sendEditorClosed();
            });

            editorWindow->centreWithSize (preferredSize);
            editorWindow->setVisible (true);
        }

        if (auto* native = editorWindow->getNativeComponent())
            native->setForeignOwner (ownerWindowId, ownerProcessId);

        editorWindow->toFront (true);
    }

    void closeEditor()
    {
        editorWindow.reset();
    }

private:
    //==============================================================================

    class EditorWindow final : public DocumentWindow
    {
    public:
        EditorWindow (StringRef title, std::unique_ptr<AudioProcessorEditor> editorToOwn, std::function<void()> onCloseCallback)
            : DocumentWindow (ComponentNative::Options()
                                  .withResizableWindow (editorToOwn->isResizable())
                                  .withFramerateRedraw (0.0f)
                                  .withRenderContinuous (false),
                              Color (0xff101417))
            , editor (std::move (editorToOwn))
            , onClose (std::move (onCloseCallback))
        {
            setTitle (title);
            addAndMakeVisible (*editor);
            editor->attachedToNative();
        }

        void resized() override
        {
            editor->setBounds (getLocalBounds());
        }

        void userTriedToCloseWindow() override
        {
            MessageManager::callAsync (onClose);
        }

    private:
        std::unique_ptr<AudioProcessorEditor> editor;
        std::function<void()> onClose;
    };

    struct FixedPlayHead final : AudioPlayHead
    {
        std::optional<PositionInfo> getPosition() const override { return position; }

        PositionInfo position;
    };

    //==============================================================================

    void buildBusViews()
    {
        inputViews.clear();
        outputViews.clear();

        int channel = 0;
        for (const auto& bus : instance->getBusLayout().getInputBuses())
        {
            if (bus.getType() != AudioBus::Type::Audio)
                continue;

            const auto numChannels = jlimit (0, static_cast<int> (block.inputChannels.size()) - channel, bus.getNumChannels());
            inputViews.emplace_back (block.inputChannels.data() + channel, numChannels, bus.getRole());
            channel += numChannels;
        }

        channel = 0;
        for (const auto& bus : instance->getBusLayout().getOutputBuses())
        {
            if (bus.getType() != AudioBus::Type::Audio)
                continue;

            const auto numChannels = jlimit (0, static_cast<int> (block.workChannels.size()) - channel, bus.getNumChannels());
            outputViews.emplace_back (block.workChannels.data() + channel, numChannels, bus.getRole());
            channel += numChannels;
        }
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            if (! requestSemaphore.wait (100) || threadShouldExit())
                continue;

            processSharedBlock();

            block.header->doneSequence.store (block.header->requestSequence.load());
            doneSemaphore.signal();
        }
    }

    void processSharedBlock()
    {
        ScopedNoDenormals noDenormals;

        auto& header = *block.header;
        const auto numSamples = jlimit (0, blockSize, header.numSamples);
        const auto numInputs = static_cast<int> (block.inputChannels.size());

        for (int channel = 0; channel < static_cast<int> (block.workChannels.size()); ++channel)
        {
            if (channel < numInputs)
                FloatVectorOperations::copy (block.workChannels[static_cast<std::size_t> (channel)], block.inputChannels[static_cast<std::size_t> (channel)], numSamples);
            else
                FloatVectorOperations::clear (block.workChannels[static_cast<std::size_t> (channel)], numSamples);
        }

        midi.clear();
        Protocol::readMidi (block.midiInput, jlimit (0, Protocol::midiCapacityBytes, header.numMidiInputBytes), midi);

        parameterChanges.clear();
        for (int i = 0; i < jlimit (0, Protocol::maxParameterChanges, header.numParameterChanges); ++i)
        {
            const auto& change = block.parameterChanges[i];
            parameterChanges.addChange (change.parameterIndex, change.normalizedValue, jlimit (0, jmax (0, numSamples - 1), change.sampleOffset));
        }

        noteExpressions.clear();
        for (int i = 0; i < jlimit (0, Protocol::maxNoteExpressions, header.numNoteExpressions); ++i)
            noteExpressions.add (block.noteExpressions[i]);

        playHead.position = header.position;

        AudioBuffer<float> audio (block.workChannels.data(), static_cast<int> (block.workChannels.size()), numSamples);

        AudioProcessContext<float> context {
            audio,
            midi,
            parameterChanges,
            (header.flags & Protocol::SharedBlockHeader::hasPosition) != 0 ? &playHead : nullptr,
            { inputViews.data(), inputViews.size() },
            { outputViews.data(), outputViews.size() }
        };

        context.noteExpressions = noteExpressions.isEmpty() ? nullptr : &noteExpressions;

        instance->processBlock (context);

        header.numMidiOutputBytes = Protocol::writeMidi (midi, block.midiOutput, Protocol::midiCapacityBytes);
    }

    //==============================================================================

    void parameterValueChanged (const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        toHost.set (indexInContainer, parameter->getValue());
    }

    void parameterGestureBegin (const AudioParameter::Ptr&, int indexInContainer) override
    {
        toHost.post (indexInContainer, AudioPluginParameterMailbox::gestureBegan);
    }

    void parameterGestureEnd (const AudioParameter::Ptr&, int indexInContainer) override
    {
        toHost.post (indexInContainer, AudioPluginParameterMailbox::gestureEnded);
    }

    void audioProcessorChanged (AudioProcessorBase*, const AudioProcessorBase::ChangeDetails& details) override
    {
        int32 flags = 0;
        int32 bit = 1;

        for (const auto changed : { details.latencyChanged, details.tailChanged, details.parameterValuesChanged, details.parameterInfoChanged, details.nonParameterStateChanged, details.programChanged, details.busLayoutChanged })
        {
            if (changed)
                flags |= bit;

            bit <<= 1;
        }

        pendingChanges.fetch_or (flags);
    }

    void timerCallback() override
    {
        MemoryOutputStream events;
        int numEvents = 0;

        toHost.flush ([&] (int index, double value, uint8 flags)
        {
            events.writeInt (index);
            events.writeDouble (value);
            events.writeByte (static_cast<char> (flags));
            ++numEvents;
        });

        if (numEvents > 0)
        {
            MemoryOutputStream stream;
            Protocol::writeHeader (stream, Protocol::MessageType::parameterEvents, 0);
            stream.writeInt (numEvents);
            stream << events.getMemoryBlock();
            server.sendToHost (stream.getMemoryBlock());
        }

        if (const auto flags = pendingChanges.exchange (0); flags != 0)
            sendProcessorChanged (flags);
    }

    void sendEditorClosed()
    {
        MemoryOutputStream stream;
        Protocol::writeHeader (stream, Protocol::MessageType::editorClosed, 0);
        server.sendToHost (stream.getMemoryBlock());
    }

public:
    void sendProcessorChanged (int32 flags)
    {
        MemoryOutputStream stream;
        Protocol::writeHeader (stream, Protocol::MessageType::processorChanged, 0);
        stream.writeInt (flags);
        stream.writeInt (instance->getLatencySamples());
        stream.writeInt (instance->getTailSamples());
        Protocol::writePresets (stream, *instance);
        server.sendToHost (stream.getMemoryBlock());
    }

private:
    AudioPluginBridgeServer& server;
    std::unique_ptr<AudioPluginInstance> instance;
    std::unique_ptr<EditorWindow> editorWindow;

    AudioPluginParameterMailbox toHost;
    std::atomic<int32> pendingChanges { 0 };

    SharedMemory sharedMemory;
    InterProcessSemaphore requestSemaphore, doneSemaphore;
    Protocol::SharedBlock block;
    int blockSize = 0;
    bool prepared = false;

    MidiBuffer midi;
    ParameterChangeBuffer parameterChanges;
    NoteExpressionBuffer noteExpressions;
    FixedPlayHead playHead;
    std::vector<AudioBusBufferView<const float>> inputViews;
    std::vector<AudioBusBufferView<float>> outputViews;
};

//==============================================================================

AudioPluginBridgeServer::AudioPluginBridgeServer() = default;

AudioPluginBridgeServer::~AudioPluginBridgeServer()
{
    session.reset();
}

void AudioPluginBridgeServer::addFormat (std::unique_ptr<AudioPluginFormat> format)
{
    jassert (format != nullptr);
    if (format == nullptr)
        return;

    for (auto& existing : formats)
    {
        if (existing->getFormatType() == format->getFormatType())
        {
            existing = std::move (format);
            return;
        }
    }

    formats.push_back (std::move (format));
}

bool AudioPluginBridgeServer::initialiseFromCommandLine (const String& commandLine, const String& commandLineUniqueID)
{
    return ChildProcessWorker::initialiseFromCommandLine (commandLine, commandLineUniqueID);
}

void AudioPluginBridgeServer::handleMessageFromCoordinator (const MemoryBlock& message)
{
    MessageManager::callAsync ([this, message]
    {
        handleRequest (message);
    });
}

void AudioPluginBridgeServer::handleConnectionLost()
{
    // The host is gone or gave up on a hanging plugin: the message thread may be stuck in it
    Process::terminate();
}

void AudioPluginBridgeServer::sendToHost (const MemoryBlock& message)
{
    sendMessageToCoordinator (message);
}

void AudioPluginBridgeServer::handleRequest (const MemoryBlock& message)
{
    using Protocol = AudioPluginBridgeProtocol;
    using MessageType = Protocol::MessageType;

    MemoryInputStream input (message, false);
    const auto [type, requestId] = Protocol::readHeader (input);

    auto result = Result::ok();
    MemoryOutputStream payload;

    if (type == MessageType::load)
    {
        if (input.readInt() != Protocol::version)
        {
            result = Result::fail ("Mismatching plugin bridge protocol version");
        }
        else if (session != nullptr)
        {
            result = Result::fail ("The plugin bridge server already hosts a plugin");
        }
        else
        {
            const auto description = Protocol::readDescription (input);
            const auto context = Protocol::readHostContext (input);

            const auto format = std::find_if (formats.begin(), formats.end(), [&] (const auto& f)
            {
                return f->getFormatType() == description.formatType;
            });

            if (format == formats.end())
            {
                result = Result::fail ("Format not supported by the plugin bridge server");
            }
            else if (auto loaded = (*format)->loadPlugin (description, context); loaded.failed())
            {
                result = Result::fail (loaded.getErrorMessage());
            }
            else
            {
                session = std::make_unique<Session> (*this, std::move (loaded).getValue());
                session->writeInfo (payload);
            }
        }
    }
    else if (session == nullptr)
    {
        result = Result::fail ("No plugin is loaded in the plugin bridge server");
    }
    else
    {
        auto& instance = session->getInstance();

        switch (type)
        {
            case MessageType::prepare:
            {
                const auto sampleRate = input.readFloat();
                const auto maxBlockSize = input.readInt();
                const auto sharedMemoryName = input.readString();
                const auto numInputChannels = input.readInt();
                const auto numWorkChannels = input.readInt();
                result = session->prepare (sampleRate, maxBlockSize, sharedMemoryName, numInputChannels, numWorkChannels);
                break;
            }

            case MessageType::release:
                session->release();
                break;

            case MessageType::getState:
            {
                MemoryBlock state;
                result = instance.saveStateIntoMemory (state);
                payload.writeInt64 (static_cast<int64> (state.getSize()));
                payload << state;
                break;
            }

            case MessageType::setState:
            {
                MemoryBlock state;
                input.readIntoMemoryBlock (state, static_cast<ssize_t> (input.readInt64()));
                result = instance.loadStateFromMemory (state);
                break;
            }

            case MessageType::setPreset:
                instance.setCurrentPreset (input.readInt());
                session->sendProcessorChanged (0);
                break;

            case MessageType::valueToText:
            {
                const auto parameters = instance.getParameters();
                const auto index = input.readInt();
                const auto value = input.readFloat();

                if (isPositiveAndBelow (index, static_cast<int> (parameters.size())))
                    payload.writeString (parameters[static_cast<std::size_t> (index)]->convertToString (value));
                else
                    result = Result::fail ("Invalid parameter index");

                break;
            }

            case MessageType::textToValue:
            {
                const auto parameters = instance.getParameters();
                const auto index = input.readInt();
                const auto text = input.readString();

                if (isPositiveAndBelow (index, static_cast<int> (parameters.size())))
                    payload.writeFloat (parameters[static_cast<std::size_t> (index)]->convertFromString (text));
                else
                    result = Result::fail ("Invalid parameter index");

                break;
            }

            case MessageType::setNonRealtime:
                instance.setNonRealtime (input.readBool());
                break;

            case MessageType::setHostInfo:
            {
                AudioProcessorBase::HostInfo info;
                info.trackName = input.readString();

                if (input.readBool())
                    info.trackColor = static_cast<uint32> (input.readInt());
                else
                    input.readInt();

                instance.setHostInfo (info);
                break;
            }

            case MessageType::openEditor:
            {
                const auto ownerWindowId = static_cast<uint64> (input.readInt64());
                const auto ownerProcessId = input.readInt();
                session->openEditor (ownerWindowId, ownerProcessId);
                break;
            }

            case MessageType::closeEditor:
                session->closeEditor();
                break;

            default:
                result = Result::fail ("Unknown plugin bridge request");
                break;
        }
    }

    if (requestId == 0)
        return;

    MemoryOutputStream reply;
    Protocol::writeHeader (reply, MessageType::reply, requestId);
    reply.writeBool (result.wasOk());
    reply.writeString (result.getErrorMessage());
    reply << payload.getMemoryBlock();
    sendToHost (reply.getMemoryBlock());
}

} // namespace yup
