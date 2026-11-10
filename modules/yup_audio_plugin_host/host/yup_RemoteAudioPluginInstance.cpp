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

struct RemoteAudioPluginInstance::PluginInfo
{
    using Protocol = AudioPluginBridgeProtocol;

    explicit PluginInfo (InputStream& stream)
        : busLayout (Protocol::readBusLayout (stream))
    {
        const auto numParameters = stream.readInt();
        for (int i = 0; i < numParameters && ! stream.isExhausted(); ++i)
            parameters.push_back (Protocol::readParameter (stream));

        latencySamples = stream.readInt();
        tailSamples = stream.readInt();

        for (auto* flag : { &hasEditor, &acceptsMidi, &producesMidi, &supportsNoteExpression, &supportsMpe })
            *flag = stream.readBool();

        std::tie (currentPreset, presetNames) = Protocol::readPresets (stream);
    }

    AudioBusLayout busLayout;
    std::vector<std::tuple<String, AudioParameter::Metadata, float>> parameters;
    int latencySamples = 0, tailSamples = 0, currentPreset = 0;
    bool hasEditor = false, acceptsMidi = false, producesMidi = false, supportsNoteExpression = false, supportsMpe = false;
    StringArray presetNames;
};

//==============================================================================

/** The control channel to a bridge server process: one request at a time, plus notifications. */
class RemoteAudioPluginInstance::Connection final : private ChildProcessCoordinator
{
public:
    using Protocol = AudioPluginBridgeProtocol;
    using NotificationHandler = std::function<void (const MemoryBlock&)>;
    using LostHandler = std::function<void()>;

    ~Connection() override
    {
        detach();
        terminateWorkerProcess();
    }

    Result launch (const Options& options)
    {
        if (! options.executable.existsAsFile())
            return Result::fail ("The plugin bridge server executable doesn't exist: " + options.executable.getFullPathName());

        if (! launchWorkerProcess (options.executable, options.commandLineUniqueID, 0, 0))
            return Result::fail ("Unable to launch the plugin bridge server");

        return Result::ok();
    }

    void attach (NotificationHandler notificationHandler, LostHandler lostHandler)
    {
        const ScopedLock sl (handlerLock);
        onNotification = std::move (notificationHandler);
        onLost = std::move (lostHandler);
    }

    void detach()
    {
        const ScopedLock sl (handlerLock);
        onNotification = nullptr;
        onLost = nullptr;
    }

    void terminate()
    {
        lost = true;
        terminateWorkerProcess();
    }

    bool isServerRunning() const
    {
        return ! lost && isWorkerProcessRunning();
    }

    /** Sends a request without waiting for its completion. */
    void post (Protocol::MessageType type, const MemoryBlock& body = {})
    {
        if (! lost)
            sendMessageToWorker (makeMessage (type, 0, body));
    }

    /** Sends a request and waits for its reply payload, losing the server when it doesn't answer. */
    ResultValue<MemoryBlock> request (Protocol::MessageType type, const MemoryBlock& body, int timeoutMs)
    {
        const ScopedLock sl (requestLock);

        if (lost)
            return makeResultValueFail ("The plugin bridge server is not running");

        const auto requestId = ++lastRequestId > 0 ? lastRequestId : (lastRequestId = 1);

        {
            const ScopedLock rl (replyLock);
            expectedRequestId = requestId;
            reply.reset();
        }

        replyReceived.reset();

        if (! sendMessageToWorker (makeMessage (type, requestId, body)))
            return makeResultValueFail (markLost ("Lost the connection to the plugin bridge server"));

        const auto deadline = Time::getMillisecondCounterHiRes() + timeoutMs;

        while (! replyReceived.wait (50))
        {
            if (! isWorkerProcessRunning())
                return makeResultValueFail (markLost ("The plugin crashed the plugin bridge server"));

            if (Time::getMillisecondCounterHiRes() >= deadline)
                return makeResultValueFail (markLost ("The plugin bridge server didn't answer after " + String (timeoutMs) + " ms"));
        }

        std::optional<MemoryBlock> received;

        {
            const ScopedLock rl (replyLock);
            received = std::move (reply);
        }

        if (! received.has_value())
            return makeResultValueFail (markLost ("The plugin crashed the plugin bridge server"));

        MemoryInputStream input (*received, false);
        Protocol::readHeader (input);

        const auto ok = input.readBool();
        const auto error = input.readString();

        if (! ok)
            return makeResultValueFail (error);

        MemoryBlock payload;
        input.readIntoMemoryBlock (payload);
        return makeResultValueOk (std::move (payload));
    }

private:
    static MemoryBlock makeMessage (Protocol::MessageType type, int32 requestId, const MemoryBlock& body)
    {
        MemoryOutputStream stream;
        Protocol::writeHeader (stream, type, requestId);
        stream << body;
        return stream.getMemoryBlock();
    }

    String markLost (const String& reason)
    {
        terminate();
        notifyLost();
        return reason;
    }

    void notifyLost()
    {
        const ScopedLock sl (handlerLock);
        if (onLost != nullptr)
            onLost();
    }

    void handleMessageFromWorker (const MemoryBlock& message) override
    {
        MemoryInputStream input (message, false);
        const auto [type, requestId] = Protocol::readHeader (input);

        if (type == Protocol::MessageType::reply)
        {
            {
                const ScopedLock rl (replyLock);
                if (requestId != expectedRequestId)
                    return;

                reply = message;
            }

            replyReceived.signal();
            return;
        }

        const ScopedLock sl (handlerLock);
        if (onNotification != nullptr)
            onNotification (message);
    }

    void handleConnectionLost() override
    {
        lost = true;
        replyReceived.signal();
        notifyLost();
    }

    CriticalSection requestLock, replyLock, handlerLock;
    int32 lastRequestId = 0;
    int32 expectedRequestId = 0;
    std::optional<MemoryBlock> reply;
    WaitableEvent replyReceived;
    std::atomic<bool> lost { false };
    NotificationHandler onNotification;
    LostHandler onLost;
};

//==============================================================================

/** The shared memory block and semaphores used to process audio in the bridge server. */
class RemoteAudioPluginInstance::AudioChannel
{
public:
    using Protocol = AudioPluginBridgeProtocol;

    AudioChannel (const AudioBusLayout& busLayout, const AudioSpec& spec)
        : layout (spec.maxBlockSize, countChannels (busLayout.getInputBuses()), jmax (countChannels (busLayout.getInputBuses()), countChannels (busLayout.getOutputBuses())))
        , sampleRate (spec.sampleRate)
    {
        for (const auto& bus : busLayout.getInputBuses())
            if (bus.getType() == AudioBus::Type::Audio)
                inputBusChannels.push_back (bus.getNumChannels());

        for (const auto& bus : busLayout.getOutputBuses())
            if (bus.getType() == AudioBus::Type::Audio)
                outputBusChannels.push_back (bus.getNumChannels());
    }

    Result open()
    {
        name = SharedMemory::createUniqueName ("yupbr");

        if (auto result = memory.create (name, layout.totalSize); result.failed())
            return result;

        if (auto result = requestSemaphore.create (Protocol::getRequestSemaphoreName (name)); result.failed())
            return result;

        if (auto result = doneSemaphore.create (Protocol::getDoneSemaphoreName (name)); result.failed())
            return result;

        block = Protocol::SharedBlock (memory.getData(), layout);
        return Result::ok();
    }

    MemoryBlock makePrepareRequest() const
    {
        MemoryOutputStream stream;
        stream.writeFloat (sampleRate);
        stream.writeInt (layout.blockSize);
        stream.writeString (name);
        stream.writeInt (layout.numInputChannels);
        stream.writeInt (layout.numWorkChannels);
        return stream.getMemoryBlock();
    }

    enum class Outcome
    {
        processed,
        late,
        timedOut
    };

    /** Returns false while the server still owns the shared memory, until it completes a block abandoned after a timeout. */
    bool isReady() noexcept
    {
        if (! waitingForLateBlock)
            return true;

        if (block.header->doneSequence.load() != sequence)
            return false;

        doneSemaphore.wait (0);
        waitingForLateBlock = false;
        return true;
    }

    /** Writes the inputs, lets the server process them and reads back the outputs. */
    template <class IsServerLost>
    Outcome process (AudioProcessContext<float>& context, int timeoutMs, IsServerLost&& isServerLost)
    {
        auto& header = *block.header;
        const auto numSamples = jmin (context.audio.getNumSamples(), layout.blockSize);

        if (! isReady())
            return Outcome::late;

        writeInputs (context, numSamples);

        header.numSamples = numSamples;
        header.numMidiInputBytes = Protocol::writeMidi (context.midi, block.midiInput, Protocol::midiCapacityBytes);
        header.numMidiOutputBytes = 0;

        header.numNoteExpressions = 0;
        if (context.noteExpressions != nullptr)
        {
            for (const auto& expression : *context.noteExpressions)
            {
                if (header.numNoteExpressions >= Protocol::maxNoteExpressions)
                    break;

                block.noteExpressions[header.numNoteExpressions++] = expression;
            }
        }

        header.flags = 0;
        if (context.playHead != nullptr)
        {
            if (const auto position = context.playHead->getPosition(); position.has_value())
            {
                header.position = *position;
                header.flags |= Protocol::SharedBlockHeader::hasPosition;
            }
        }

        header.requestSequence.store (++sequence);
        requestSemaphore.signal();

        if (! waitForServer (timeoutMs, isServerLost) || header.doneSequence.load() != sequence)
        {
            waitingForLateBlock = true;
            return Outcome::timedOut;
        }

        readOutputs (context, numSamples);

        context.midi.clear();
        Protocol::readMidi (block.midiOutput, jlimit (0, Protocol::midiCapacityBytes, header.numMidiOutputBytes), context.midi);
        return Outcome::processed;
    }

    /** Parameter changes for the next block, written before process(). */
    void clearParameterChanges() noexcept { block.header->numParameterChanges = 0; }

    void addParameterChange (int index, float normalizedValue, int sampleOffset) noexcept
    {
        auto& count = block.header->numParameterChanges;
        if (count < Protocol::maxParameterChanges)
            block.parameterChanges[count++] = { index, normalizedValue, sampleOffset };
    }

    int getRealtimeTimeoutMs (int numSamples) const noexcept
    {
        return jmax (10, roundToInt (2000.0 * numSamples / jmax (1.0f, sampleRate)));
    }

private:
    template <class IsServerLost>
    bool waitForServer (int timeoutMs, IsServerLost&& isServerLost)
    {
        constexpr int sliceMs = 50;

        for (int remainingMs = timeoutMs; remainingMs > 0 && ! isServerLost(); remainingMs -= sliceMs)
        {
            if (doneSemaphore.wait (jmin (sliceMs, remainingMs)))
                return true;
        }

        return false;
    }

    static int countChannels (Span<const AudioBus> buses)
    {
        int numChannels = 0;
        for (const auto& bus : buses)
            if (bus.getType() == AudioBus::Type::Audio)
                numChannels += bus.getNumChannels();

        return numChannels;
    }

    void writeInputs (const AudioProcessContext<float>& context, int numSamples)
    {
        int channel = 0;

        for (std::size_t bus = 0; bus < inputBusChannels.size(); ++bus)
        {
            for (int busChannel = 0; busChannel < inputBusChannels[bus] && channel < layout.numInputChannels; ++busChannel, ++channel)
            {
                const float* source = nullptr;

                if (! context.inputs.empty())
                {
                    if (bus < context.inputs.size() && busChannel < context.inputs[bus].getNumChannels())
                        source = context.inputs[bus].getReadPointer (busChannel);
                }
                else if (channel < context.audio.getNumChannels())
                {
                    source = context.audio.getReadPointer (channel);
                }

                auto* destination = block.inputChannels[static_cast<std::size_t> (channel)];

                if (source != nullptr)
                    FloatVectorOperations::copy (destination, source, numSamples);
                else
                    FloatVectorOperations::clear (destination, numSamples);
            }
        }
    }

    void readOutputs (AudioProcessContext<float>& context, int numSamples)
    {
        if (context.outputs.empty())
        {
            for (int channel = 0; channel < context.audio.getNumChannels(); ++channel)
            {
                if (channel < layout.numWorkChannels)
                    context.audio.copyFrom (channel, 0, block.workChannels[static_cast<std::size_t> (channel)], numSamples);
                else
                    context.audio.clear (channel, 0, numSamples);
            }

            return;
        }

        int channel = 0;

        for (std::size_t bus = 0; bus < context.outputs.size(); ++bus)
        {
            auto& view = context.outputs[bus];
            const auto numBusChannels = bus < outputBusChannels.size() ? outputBusChannels[bus] : 0;

            for (int busChannel = 0; busChannel < view.getNumChannels(); ++busChannel)
            {
                const auto sourceChannel = channel + busChannel;

                if (busChannel < numBusChannels && sourceChannel < layout.numWorkChannels)
                    FloatVectorOperations::copy (view.getWritePointer (busChannel), block.workChannels[static_cast<std::size_t> (sourceChannel)], numSamples);
                else
                    FloatVectorOperations::clear (view.getWritePointer (busChannel), numSamples);
            }

            channel += numBusChannels;
        }
    }

    const Protocol::SharedLayout layout;
    const float sampleRate;
    std::vector<int> inputBusChannels, outputBusChannels;

    String name;
    SharedMemory memory;
    InterProcessSemaphore requestSemaphore, doneSemaphore;
    Protocol::SharedBlock block;
    uint32 sequence = 0;
    bool waitingForLateBlock = false;
};

//==============================================================================

/** Stands in for the editor, which opens in a window of the bridge server process. */
class RemoteAudioPluginInstance::RemoteEditor final : public AudioProcessorEditor
{
public:
    explicit RemoteEditor (RemoteAudioPluginInstance& ownerToUse)
        : owner (ownerToUse)
    {
        label.setText ("The plugin editor opens in a separate window. Click to show it.", dontSendNotification);
        label.setJustification (Justification::center);
        addAndMakeVisible (label);

        label.setWantsMouseEvents (false, false);
        setSize (getPreferredSize().to<float>());

        owner.sendEditorRequest (true);
    }

    ~RemoteEditor() override
    {
        owner.sendEditorRequest (false);
    }

    bool isResizable() const override { return false; }

    Size<int> getPreferredSize() const override { return { 360, 80 }; }

    void paint (Graphics& g) override
    {
        g.setFillColor (Color (0xff101417));
        g.fillAll();
    }

    void resized() override
    {
        label.setBounds (getLocalBounds().reduced (8));
    }

    void mouseDown (const MouseEvent&) override
    {
        owner.sendEditorRequest (true, getTopLevelComponent());
    }

private:
    RemoteAudioPluginInstance& owner;
    Label label;
};

//==============================================================================

ResultValue<std::unique_ptr<RemoteAudioPluginInstance>> RemoteAudioPluginInstance::load (const AudioPluginDescription& description,
                                                                                         const AudioPluginHostContext& context,
                                                                                         const Options& options)
{
    using Protocol = AudioPluginBridgeProtocol;
    using ResultType = std::unique_ptr<RemoteAudioPluginInstance>;

    auto connection = std::make_unique<Connection>();

    if (auto result = connection->launch (options); result.failed())
        return makeResultValueFail (result.getErrorMessage());

    MemoryOutputStream request;
    request.writeInt (Protocol::version);
    Protocol::writeDescription (request, description);
    Protocol::writeHostContext (request, context);

    auto reply = connection->request (Protocol::MessageType::load, request.getMemoryBlock(), options.timeoutMs);
    if (reply.failed())
        return makeResultValueFail (reply.getErrorMessage());

    MemoryInputStream input (reply.getReference(), false);
    const PluginInfo info (input);

    return makeResultValueOk (ResultType (new RemoteAudioPluginInstance (description, context, options, std::move (connection), info)));
}

RemoteAudioPluginInstance::RemoteAudioPluginInstance (const AudioPluginDescription& description,
                                                      const AudioPluginHostContext& context,
                                                      const Options& optionsToUse,
                                                      std::unique_ptr<Connection> connectionToUse,
                                                      const PluginInfo& info)
    : AudioPluginInstance (description, info.busLayout)
    , hostContext (context)
    , options (optionsToUse)
    , connection (std::move (connectionToUse))
    , editorAvailable (info.hasEditor)
    , midiInput (info.acceptsMidi)
    , midiOutput (info.producesMidi)
    , noteExpressions (info.supportsNoteExpression)
    , mpe (info.supportsMpe)
    , tailSamples (info.tailSamples)
    , currentPreset (info.currentPreset)
    , presetNames (info.presetNames)
{
    const auto numParameters = static_cast<int> (info.parameters.size());

    for (int index = 0; index < numParameters; ++index)
    {
        auto [id, metadata, value] = info.parameters[static_cast<std::size_t> (index)];

        if (metadata.isBypass())
        {
            if (bypassParameterIndex < 0)
                bypassParameterIndex = index;
            else
                metadata.setBypass (false);
        }

        AudioParameter::Ptr parameter = new AudioParameter (
            id,
            std::move (metadata),
            [this, index] (float v)
        {
            return convertValueToText (index, v);
        },
            [this, index] (const String& text)
        {
            return convertTextToValue (index, text);
        });

        parameter->setValue (value);
        addParameter (parameter);
    }

    lastSentValues = std::make_unique<std::atomic<float>[]> (static_cast<std::size_t> (numParameters));
    for (int index = 0; index < numParameters; ++index)
        lastSentValues[static_cast<std::size_t> (index)].store (getParameters()[static_cast<std::size_t> (index)]->getValue());

    fromServer = std::make_unique<AudioPluginParameterMailbox>();
    fromServer->resize (numParameters);
    setLatencySamples (info.latencySamples);

    attach (*connection);
    startTimer (250);
}

RemoteAudioPluginInstance::~RemoteAudioPluginInstance()
{
    stopTimer();
    connection->detach();
    cancelPendingUpdate();

    {
        const ScopedLock sl (getProcessLock());
        audioChannel.reset();
    }

    connection.reset();
}

void RemoteAudioPluginInstance::attach (Connection& connectionToAttach)
{
    connectionToAttach.attach ([this] (const MemoryBlock& message)
    {
        handleNotification (message);
    },
                               [this]
    {
        crashed = true;
        triggerAsyncUpdate();
    });
}

//==============================================================================

bool RemoteAudioPluginInstance::hasCrashed() const noexcept
{
    return crashed.load();
}

Result RemoteAudioPluginInstance::reload()
{
    using Protocol = AudioPluginBridgeProtocol;

    if (! crashed.load())
        return Result::ok();

    auto newConnection = std::make_unique<Connection>();

    if (auto result = newConnection->launch (options); result.failed())
        return result;

    MemoryOutputStream request;
    request.writeInt (Protocol::version);
    Protocol::writeDescription (request, pluginDescription);
    Protocol::writeHostContext (request, hostContext);

    auto reply = newConnection->request (Protocol::MessageType::load, request.getMemoryBlock(), options.timeoutMs);
    if (reply.failed())
        return Result::fail (reply.getErrorMessage());

    MemoryInputStream input (reply.getReference(), false);
    const PluginInfo info (input);
    const auto parameters = getParameters();

    if (info.parameters.size() != parameters.size())
        return Result::fail ("The reloaded plugin reports different parameters");

    for (std::size_t index = 0; index < parameters.size(); ++index)
    {
        if (std::get<0> (info.parameters[index]) != parameters[index]->getID())
            return Result::fail ("The reloaded plugin reports different parameters");
    }

    const auto spec = preparedSpec;
    releaseResources();

    connection->detach();
    connection = std::move (newConnection);
    attach (*connection);

    cancelPendingUpdate();
    crashed = false;
    crashReported = false;

    std::optional<MemoryBlock> state;

    {
        const ScopedLock sl (stateLock);
        state = lastState;
    }

    if (state.has_value())
    {
        if (auto result = loadStateFromMemory (*state); result.failed())
            return result;
    }

    // Sends every parameter value with the next block
    for (std::size_t index = 0; index < parameters.size(); ++index)
        lastSentValues[index].store (std::numeric_limits<float>::quiet_NaN());

    if (isNonRealtime())
        nonRealtimeStateChanged();

    hostInfoChanged (getHostInfo());

    if (spec.has_value())
        prepareToPlay (*spec);

    return Result::ok();
}

//==============================================================================

void RemoteAudioPluginInstance::prepareToPlay (const AudioSpec& spec)
{
    releaseResources();
    preparedSpec = spec;

    if (crashed.load())
        return;

    auto channel = std::make_unique<AudioChannel> (getBusLayout(), spec);

    if (auto result = channel->open(); result.failed())
    {
        YUP_DBG ("Unable to create the plugin bridge audio channel: " << result.getErrorMessage());
        return;
    }

    if (auto reply = connection->request (AudioPluginBridgeProtocol::MessageType::prepare, channel->makePrepareRequest(), options.timeoutMs); reply.failed())
    {
        YUP_DBG ("Unable to prepare the remote plugin: " << reply.getErrorMessage());
        return;
    }

    const ScopedLock sl (getProcessLock());
    audioChannel = std::move (channel);
    lateSinceMs = 0;
}

void RemoteAudioPluginInstance::releaseResources()
{
    preparedSpec.reset();

    std::unique_ptr<AudioChannel> channel;

    {
        const ScopedLock sl (getProcessLock());
        channel = std::move (audioChannel);
    }

    if (channel != nullptr && ! crashed.load())
        connection->request (AudioPluginBridgeProtocol::MessageType::release, {}, options.timeoutMs);
}

void RemoteAudioPluginInstance::processBlock (AudioProcessContext<float>& context)
{
    ScopedNoDenormals noDenormals;

    const ScopedTryLock lock (getProcessLock());
    if (! lock.isLocked() || isSuspended() || crashed.load() || audioChannel == nullptr)
    {
        context.audio.clear();
        context.midi.clear();
        return;
    }

    if (isBypassed() && bypassParameterIndex < 0)
    {
        processBlockBypassed (context);
        return;
    }

    const auto ready = audioChannel->isReady();

    if (ready)
    {
        writeParameterChanges (context.params);
    }
    else
    {
        const auto parameters = getParameters();

        for (const auto& change : context.params)
        {
            if (isPositiveAndBelow (change.parameterIndex, static_cast<int> (parameters.size())))
                parameters[static_cast<std::size_t> (change.parameterIndex)]->setNormalizedValue (change.normalizedValue);
        }
    }

    const auto nonRealtime = isNonRealtime();
    const auto timeoutMs = nonRealtime ? options.timeoutMs : audioChannel->getRealtimeTimeoutMs (context.audio.getNumSamples());

    // Offline waits can be long: also notice a dead server without waiting for the watchdog
    const auto isServerLost = [this, nonRealtime]
    {
        return crashed.load() || (nonRealtime && ! connection->isServerRunning());
    };

    if (ready && audioChannel->process (context, timeoutMs, isServerLost) == AudioChannel::Outcome::processed)
    {
        lateSinceMs = 0;
        return;
    }

    context.audio.clear();
    context.midi.clear();

    // A server stuck for longer than a control request is considered hung
    const auto now = Time::getMillisecondCounter();
    if (lateSinceMs == 0)
        lateSinceMs = jmax (1u, now);

    if (nonRealtime || now - lateSinceMs >= static_cast<uint32> (options.timeoutMs))
    {
        crashed = true;
        triggerAsyncUpdate();
    }
}

void RemoteAudioPluginInstance::writeParameterChanges (const ParameterChangeBuffer& changes)
{
    audioChannel->clearParameterChanges();

    const auto parameters = getParameters();

    // Values changed outside the plugin since they were last sent (host edits, bypass, reloads)
    for (std::size_t index = 0; index < parameters.size(); ++index)
    {
        const auto value = parameters[index]->getValue();

        if (value != lastSentValues[index].exchange (value, std::memory_order_relaxed))
            audioChannel->addParameterChange (static_cast<int> (index), parameters[index]->getNormalizedValue(), 0);
    }

    for (const auto& change : changes)
    {
        if (! isPositiveAndBelow (change.parameterIndex, static_cast<int> (parameters.size())))
            continue;

        auto& parameter = parameters[static_cast<std::size_t> (change.parameterIndex)];
        parameter->setNormalizedValue (change.normalizedValue);
        lastSentValues[static_cast<std::size_t> (change.parameterIndex)].store (parameter->getValue(), std::memory_order_relaxed);

        audioChannel->addParameterChange (change.parameterIndex, change.normalizedValue, change.sampleOffset);
    }
}

//==============================================================================

bool RemoteAudioPluginInstance::acceptsMidi() const noexcept { return midiInput; }

bool RemoteAudioPluginInstance::producesMidi() const noexcept { return midiOutput; }

bool RemoteAudioPluginInstance::supportsNoteExpression() const { return noteExpressions; }

bool RemoteAudioPluginInstance::supportsMpe() const { return mpe; }

int RemoteAudioPluginInstance::getTailSamples() { return tailSamples.load(); }

//==============================================================================

bool RemoteAudioPluginInstance::hasEditor() const { return editorAvailable; }

AudioProcessorEditor* RemoteAudioPluginInstance::createEditor()
{
    if (! editorAvailable || crashed.load())
        return nullptr;

    return new RemoteEditor (*this);
}

void RemoteAudioPluginInstance::showEditor (Component* ownerWindow)
{
    if (editorAvailable)
        sendEditorRequest (true, ownerWindow);
}

void RemoteAudioPluginInstance::hideEditor()
{
    sendEditorRequest (false);
}

bool RemoteAudioPluginInstance::isEditorShowing() const noexcept
{
    return editorShowing.load() && ! crashed.load();
}

void RemoteAudioPluginInstance::sendEditorRequest (bool shouldOpen, Component* ownerWindow)
{
    using Protocol = AudioPluginBridgeProtocol;

    if (crashed.load())
        return;

    editorShowing = shouldOpen;

    if (! shouldOpen)
    {
        connection->post (Protocol::MessageType::closeEditor);
        return;
    }

    uint64 ownerWindowId = 0;

    if (ownerWindow != nullptr)
    {
        if (auto* native = ownerWindow->getTopLevelComponent()->getNativeComponent())
            ownerWindowId = native->getSystemWindowId();
    }

#if YUP_WINDOWS
    const auto processId = static_cast<int> (GetCurrentProcessId());
#elif YUP_MAC || YUP_LINUX
    const auto processId = static_cast<int> (getpid());
#else
    const auto processId = 0;
#endif

    MemoryOutputStream stream;
    stream.writeInt64 (static_cast<int64> (ownerWindowId));
    stream.writeInt (processId);
    connection->post (Protocol::MessageType::openEditor, stream.getMemoryBlock());
}

//==============================================================================

int RemoteAudioPluginInstance::getCurrentPreset() const noexcept { return currentPreset.load(); }

void RemoteAudioPluginInstance::setCurrentPreset (int index) noexcept
{
    if (! isPositiveAndBelow (index, getNumPresets()))
        return;

    currentPreset = index;

    MemoryOutputStream stream;
    stream.writeInt (index);
    connection->post (AudioPluginBridgeProtocol::MessageType::setPreset, stream.getMemoryBlock());
}

int RemoteAudioPluginInstance::getNumPresets() const
{
    const ScopedLock sl (presetLock);
    return presetNames.size();
}

String RemoteAudioPluginInstance::getPresetName (int index) const
{
    const ScopedLock sl (presetLock);
    return presetNames[index];
}

void RemoteAudioPluginInstance::setPresetName (int, StringRef)
{
}

//==============================================================================

Result RemoteAudioPluginInstance::saveStateIntoMemory (MemoryBlock& memoryBlock)
{
    if (! crashed.load())
    {
        auto reply = connection->request (AudioPluginBridgeProtocol::MessageType::getState, {}, options.timeoutMs);

        if (reply.wasOk())
        {
            MemoryInputStream input (reply.getReference(), false);
            const auto size = input.readInt64();

            MemoryBlock state;
            input.readIntoMemoryBlock (state, static_cast<ssize_t> (size));

            const ScopedLock sl (stateLock);
            lastState = state;
            memoryBlock = std::move (state);
            return Result::ok();
        }

        if (! crashed.load())
            return Result::fail (reply.getErrorMessage());
    }

    const ScopedLock sl (stateLock);

    if (! lastState.has_value())
        return Result::fail ("The plugin bridge server is not running");

    memoryBlock = *lastState;
    return Result::ok();
}

Result RemoteAudioPluginInstance::loadStateFromMemory (const MemoryBlock& memoryBlock)
{
    auto result = Result::fail ("The plugin bridge server is not running");

    if (! crashed.load())
    {
        MemoryOutputStream stream;
        stream.writeInt64 (static_cast<int64> (memoryBlock.getSize()));
        stream << memoryBlock;

        auto reply = connection->request (AudioPluginBridgeProtocol::MessageType::setState, stream.getMemoryBlock(), options.timeoutMs);
        result = reply.wasOk() ? Result::ok() : Result::fail (reply.getErrorMessage());
    }

    // Restored by reload() when the plugin crashed before or while loading it
    if (result.wasOk() || crashed.load())
    {
        const ScopedLock sl (stateLock);
        lastState = memoryBlock;
    }

    return result;
}

//==============================================================================

void RemoteAudioPluginInstance::hostInfoChanged (const HostInfo& newInfo)
{
    MemoryOutputStream stream;
    stream.writeString (newInfo.trackName);
    stream.writeBool (newInfo.trackColor.has_value());
    stream.writeInt (static_cast<int> (newInfo.trackColor.value_or (0)));
    connection->post (AudioPluginBridgeProtocol::MessageType::setHostInfo, stream.getMemoryBlock());
}

void RemoteAudioPluginInstance::nonRealtimeStateChanged()
{
    MemoryOutputStream stream;
    stream.writeBool (isNonRealtime());
    connection->post (AudioPluginBridgeProtocol::MessageType::setNonRealtime, stream.getMemoryBlock());
}

void RemoteAudioPluginInstance::bypassStateChanged()
{
    if (bypassParameterIndex < 0)
        return;

    // The changed value reaches the plugin with the next block
    getParameters()[static_cast<std::size_t> (bypassParameterIndex)]->setNormalizedValue (isBypassed() ? 1.0f : 0.0f);
}

//==============================================================================

String RemoteAudioPluginInstance::convertValueToText (int index, float value)
{
    if (! crashed.load())
    {
        MemoryOutputStream stream;
        stream.writeInt (index);
        stream.writeFloat (value);

        if (auto reply = connection->request (AudioPluginBridgeProtocol::MessageType::valueToText, stream.getMemoryBlock(), options.timeoutMs); reply.wasOk())
            return MemoryInputStream (reply.getReference(), false).readString();
    }

    return String (value, 2);
}

float RemoteAudioPluginInstance::convertTextToValue (int index, const String& text)
{
    if (! crashed.load())
    {
        MemoryOutputStream stream;
        stream.writeInt (index);
        stream.writeString (text);

        if (auto reply = connection->request (AudioPluginBridgeProtocol::MessageType::textToValue, stream.getMemoryBlock(), options.timeoutMs); reply.wasOk())
            return MemoryInputStream (reply.getReference(), false).readFloat();
    }

    return text.getFloatValue();
}

//==============================================================================

void RemoteAudioPluginInstance::timerCallback()
{
    if (! crashed.load() && ! connection->isServerRunning())
    {
        crashed = true;
        handleAsyncUpdate();
    }
}

void RemoteAudioPluginInstance::handleNotification (const MemoryBlock& message)
{
    using Protocol = AudioPluginBridgeProtocol;

    MemoryInputStream input (message, false);
    const auto type = Protocol::readHeader (input).first;

    if (type == Protocol::MessageType::parameterEvents)
    {
        const auto numEvents = input.readInt();

        for (int i = 0; i < numEvents && ! input.isExhausted(); ++i)
        {
            const auto index = input.readInt();
            const auto value = input.readDouble();
            const auto flags = static_cast<uint8> (input.readByte());

            if ((flags & AudioPluginParameterMailbox::valueChanged) != 0)
                fromServer->set (index, value);

            if (const auto gestures = static_cast<uint8> (flags & ~AudioPluginParameterMailbox::valueChanged); gestures != 0)
                fromServer->post (index, gestures);
        }
    }
    else if (type == Protocol::MessageType::processorChanged)
    {
        pendingChanges.fetch_or (input.readInt());
        pendingLatency = input.readInt();
        tailSamples = input.readInt();

        auto [preset, names] = Protocol::readPresets (input);
        currentPreset = preset;

        const ScopedLock sl (presetLock);
        presetNames = std::move (names);
    }
    else if (type == Protocol::MessageType::editorClosed)
    {
        editorShowing = false;
        editorClosedPending = true;
    }
    else
    {
        return;
    }

    triggerAsyncUpdate();
}

void RemoteAudioPluginInstance::handleAsyncUpdate()
{
    if (crashed.load())
    {
        if (crashReported.exchange (true))
            return;

        connection->terminate();

        {
            const ScopedLock sl (getProcessLock());
            audioChannel.reset();
        }

        editorShowing = false;

        if (onCrashed != nullptr)
            onCrashed();

        return;
    }

    if (editorClosedPending.exchange (false) && onEditorClosed != nullptr)
        onEditorClosed();

    const auto parameters = getParameters();

    fromServer->flush ([&] (int index, double value, uint8 flags)
    {
        auto& parameter = parameters[static_cast<std::size_t> (index)];

        if ((flags & AudioPluginParameterMailbox::gestureBegan) != 0)
            parameter->beginChangeGesture();

        if ((flags & AudioPluginParameterMailbox::valueChanged) != 0)
        {
            // Not echoed back to the server with the next block
            const auto plainValue = parameter->getMetadata().valueRange.snapToLegalValue (static_cast<float> (value));
            lastSentValues[static_cast<std::size_t> (index)].store (plainValue, std::memory_order_relaxed);
            parameter->setValueNotifyingHost (plainValue);

            if (index == bypassParameterIndex)
                setBypassed (parameter->getNormalizedValue() >= 0.5f);
        }

        if ((flags & AudioPluginParameterMailbox::gestureEnded) != 0)
            parameter->endChangeGesture();
    });

    if (const auto latency = pendingLatency.exchange (-1); latency >= 0)
        setLatencySamples (latency);

    if (const auto flags = pendingChanges.exchange (0); flags != 0)
    {
        const auto has = [flags] (int bit) { return (flags & (1 << bit)) != 0; };

        updateHostDisplay (ChangeDetails()
                               .withTailChanged (has (1))
                               .withParameterValuesChanged (has (2))
                               .withParameterInfoChanged (has (3))
                               .withNonParameterStateChanged (has (4))
                               .withProgramChanged (has (5))
                               .withBusLayoutChanged (has (6)));
    }
}

} // namespace yup
