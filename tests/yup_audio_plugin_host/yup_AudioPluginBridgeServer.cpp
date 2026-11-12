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


#include <gtest/gtest.h>

#include <yup_audio_plugin_host/yup_audio_plugin_host.h>

#include <yup_audio_plugin_host/host/yup_AudioPluginParameterMailbox.h>
#include <yup_audio_plugin_host/host/yup_AudioPluginBridgeProtocol.h>

#if YUP_MAC || YUP_WINDOWS || YUP_LINUX

using namespace yup;

namespace yup
{
std::function<bool (const String&)>& getGoogleTestChildProcessEntryPoint();
} // namespace yup

class AudioPluginBridgeServerTests : public ::testing::Test
{
public:
    using Protocol = AudioPluginBridgeProtocol;
    using MessageType = Protocol::MessageType;

    static constexpr const char* serverID = "yupBridgeServerDirectTests";

    /** A stereo gain echoing MIDI, whose saved state reports what the server forwarded to it. */
    class ProbeInstance : public AudioPluginInstance
    {
    public:
        ProbeInstance (const AudioPluginDescription& description, const AudioPluginHostContext& contextToUse)
            : AudioPluginInstance (description,
                                   AudioBusLayout ({ AudioBus ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                                                   { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) }))
            , context (contextToUse)
        {
            level = AudioParameterBuilder()
                        .withID ("level")
                        .withName ("Level")
                        .withRange (0.0f, 2.0f)
                        .withDefault (1.0f)
                        .withValueToString ([] (float value)
            {
                return String (value, 2) + "x";
            })
                        .build();

            addParameter (level);
            setLatencySamples (3);
        }

        void prepareToPlay (const AudioSpec&) override {}

        void releaseResources() override {}

        void processBlock (AudioProcessContext<float>& processContext) override
        {
            for (const auto& change : processContext.params)
            {
                if (change.parameterIndex == 0)
                    level->setNormalizedValue (change.normalizedValue);
            }

            processContext.audio.applyGain (level->getValue());
        }

        bool acceptsMidi() const noexcept override { return true; }

        bool producesMidi() const noexcept override { return false; }

        int getCurrentPreset() const noexcept override { return currentPreset; }

        void setCurrentPreset (int index) noexcept override
        {
            currentPreset = index;

            level->beginChangeGesture();
            level->setValueNotifyingHost (index == 1 ? 0.5f : 1.0f);
            level->endChangeGesture();
        }

        int getNumPresets() const override { return 2; }

        String getPresetName (int index) const override { return index == 0 ? "Unity" : "Half"; }

        void setPresetName (int, StringRef) override {}

        /** Any non empty state sets the latency to its size. */
        Result loadStateFromMemory (const MemoryBlock& memoryBlock) override
        {
            if (memoryBlock.isEmpty())
                return Result::fail ("empty state");

            setLatencySamples (static_cast<int> (memoryBlock.getSize()));
            return Result::ok();
        }

        Result saveStateIntoMemory (MemoryBlock& memoryBlock) override
        {
            MemoryOutputStream stream (memoryBlock, false);
            stream.writeString (context.hostName);
            stream.writeFloat (context.sampleRate);
            stream.writeString (getHostInfo().trackName);
            stream.writeInt (static_cast<int> (getHostInfo().trackColor.value_or (0)));
            stream.writeBool (isNonRealtime());
            return Result::ok();
        }

        bool hasEditor() const override { return false; }

    private:
        AudioPluginHostContext context;
        AudioParameter::Ptr level;
        int currentPreset = 0;
    };

    class ProbeFormat : public AudioPluginFormat
    {
    public:
        ProbeFormat (AudioPluginFormatType typeToUse, bool shouldFailLoading)
            : type (typeToUse)
            , failLoading (shouldFailLoading)
        {
        }

        AudioPluginFormatType getFormatType() const override { return type; }

        String getFormatName() const override { return "Probe"; }

        StringArray getFileExtensions() const override { return { ".probeplugin" }; }

        FileSearchPath getDefaultSearchPaths() const override { return {}; }

        ResultValue<std::vector<AudioPluginDescription>> scanFile (const File&) override
        {
            return makeResultValueFail ("not scannable");
        }

        ResultValue<std::unique_ptr<AudioPluginInstance>> loadPlugin (const AudioPluginDescription& description,
                                                                      const AudioPluginHostContext& context) override
        {
            if (failLoading)
                return makeResultValueFail ("stale format");

            if (description.name == "broken")
                return makeResultValueFail ("cannot open " + description.name);

            return makeResultValueOk (std::unique_ptr<AudioPluginInstance> (std::make_unique<ProbeInstance> (description, context)));
        }

    private:
        AudioPluginFormatType type;
        bool failLoading;
    };

    static bool runServer (const String& commandLine)
    {
        static std::unique_ptr<AudioPluginBridgeServer> server;

        auto newServer = std::make_unique<AudioPluginBridgeServer>();
        newServer->addFormat (std::make_unique<ProbeFormat> (AudioPluginFormatType::clap, false));
        newServer->addFormat (std::make_unique<ProbeFormat> (AudioPluginFormatType::vst3, true));
        newServer->addFormat (std::make_unique<ProbeFormat> (AudioPluginFormatType::vst3, false));

        if (! newServer->initialiseFromCommandLine (commandLine, serverID))
            return false;

        server = std::move (newServer);
        return true;
    }

protected:
    struct Reply
    {
        bool ok = false;
        String error;
        MemoryBlock payload;
    };

    /** Talks the raw bridge protocol to a server process, collecting everything it sends. */
    class Host : public ChildProcessCoordinator
    {
    public:
        ~Host() override
        {
            terminateWorkerProcess();
        }

        bool launch()
        {
            return launchWorkerProcess (File::getSpecialLocation (File::currentExecutableFile), serverID, 0, 0);
        }

        void post (MessageType type, const MemoryBlock& body = {})
        {
            sendMessageToWorker (makeMessage (type, 0, body));
        }

        std::optional<Reply> request (MessageType type, const MemoryBlock& body = {})
        {
            const auto requestId = ++lastRequestId;

            if (! sendMessageToWorker (makeMessage (type, requestId, body)))
                return {};

            const auto message = waitForMessage (MessageType::reply, requestId);
            if (! message.has_value())
                return {};

            MemoryInputStream input (*message, false);

            Reply reply;
            reply.ok = input.readBool();
            reply.error = input.readString();
            input.readIntoMemoryBlock (reply.payload);
            return reply;
        }

        /** Returns the payload of the first unconsumed message of a type, waiting for it to arrive. */
        std::optional<MemoryBlock> waitForMessage (MessageType type, int32 requestId = 0)
        {
            for (int i = 0; i < 500; ++i)
            {
                {
                    const ScopedLock sl (lock);

                    for (auto it = received.begin(); it != received.end(); ++it)
                    {
                        MemoryInputStream input (*it, false);
                        const auto [messageType, messageRequestId] = Protocol::readHeader (input);

                        if (messageType == type && messageRequestId == requestId)
                        {
                            MemoryBlock payload;
                            input.readIntoMemoryBlock (payload);
                            received.erase (it);
                            return payload;
                        }
                    }
                }

                messageArrived.wait (10);
            }

            return {};
        }

    private:
        static MemoryBlock makeMessage (MessageType type, int32 requestId, const MemoryBlock& body)
        {
            MemoryOutputStream stream;
            Protocol::writeHeader (stream, type, requestId);
            stream << body;
            return stream.getMemoryBlock();
        }

        void handleMessageFromWorker (const MemoryBlock& message) override
        {
            {
                const ScopedLock sl (lock);
                received.push_back (message);
            }

            messageArrived.signal();
        }

        CriticalSection lock;
        std::vector<MemoryBlock> received;
        WaitableEvent messageArrived;
        int32 lastRequestId = 0;
    };

    void SetUp() override
    {
        ASSERT_TRUE (host.launch());
    }

    static AudioPluginDescription makeDescription (const String& name, AudioPluginFormatType type = AudioPluginFormatType::clap)
    {
        AudioPluginDescription description;
        description.formatType = type;
        description.name = name;
        description.identifier = "test." + name;
        return description;
    }

    static MemoryBlock makeLoadRequest (const AudioPluginDescription& description,
                                        const AudioPluginHostContext& context = {},
                                        int32 version = Protocol::version)
    {
        MemoryOutputStream stream;
        stream.writeInt (version);
        Protocol::writeDescription (stream, description);
        Protocol::writeHostContext (stream, context);
        return stream.getMemoryBlock();
    }

    Reply load (const String& name = "probe", AudioPluginFormatType type = AudioPluginFormatType::clap, const AudioPluginHostContext& context = {})
    {
        return requestOrFail (MessageType::load, makeLoadRequest (makeDescription (name, type), context));
    }

    Reply requestOrFail (MessageType type, const MemoryBlock& body = {})
    {
        auto reply = host.request (type, body);
        EXPECT_TRUE (reply.has_value()) << "no reply from the bridge server";
        return reply.value_or (Reply { false, "no reply", {} });
    }

    /** Reads back what the probe plugin received, from its saved state. */
    struct ProbeState
    {
        String hostName;
        float sampleRate = 0.0f;
        String trackName;
        uint32 trackColor = 0;
        bool nonRealtime = false;
    };

    ProbeState getProbeState()
    {
        const auto reply = requestOrFail (MessageType::getState);
        EXPECT_TRUE (reply.ok) << reply.error;

        MemoryInputStream input (reply.payload, false);
        MemoryBlock state;
        input.readIntoMemoryBlock (state, static_cast<ssize_t> (input.readInt64()));

        MemoryInputStream stateInput (state, false);
        ProbeState probe;
        probe.hostName = stateInput.readString();
        probe.sampleRate = stateInput.readFloat();
        probe.trackName = stateInput.readString();
        probe.trackColor = static_cast<uint32> (stateInput.readInt());
        probe.nonRealtime = stateInput.readBool();
        return probe;
    }

    Host host;
};

namespace
{
const bool bridgeServerDirectEntryPointRegistered = []
{
    auto& entryPoint = yup::getGoogleTestChildProcessEntryPoint();

    entryPoint = [previous = entryPoint] (const String& commandLine)
    {
        return AudioPluginBridgeServerTests::runServer (commandLine) || (previous && previous (commandLine));
    };

    return true;
}();
} // namespace

TEST (AudioPluginBridgeServerCommandLineTests, IgnoresCommandLinesNotMeantForIt)
{
    const String serverID = AudioPluginBridgeServerTests::serverID;
    AudioPluginBridgeServer server;

    EXPECT_FALSE (server.initialiseFromCommandLine ("", serverID));
    EXPECT_FALSE (server.initialiseFromCommandLine ("--someOtherID:p1234", serverID));
    EXPECT_FALSE (server.initialiseFromCommandLine ("--" + String (serverID) + ":", serverID));
}

TEST_F (AudioPluginBridgeServerTests, LoadRepliesWithPluginInfo)
{
    const auto reply = load();
    ASSERT_TRUE (reply.ok) << reply.error;

    MemoryInputStream input (reply.payload, false);

    const auto layout = Protocol::readBusLayout (input);
    ASSERT_EQ (1u, layout.getInputBuses().size());
    ASSERT_EQ (1u, layout.getOutputBuses().size());
    EXPECT_EQ (2, layout.getOutputBuses()[0].getNumChannels());

    ASSERT_EQ (1, input.readInt());
    const auto [id, metadata, value] = Protocol::readParameter (input);
    EXPECT_EQ ("level", id);
    EXPECT_EQ ("Level", metadata.name);
    EXPECT_FLOAT_EQ (2.0f, metadata.valueRange.end);
    EXPECT_FLOAT_EQ (1.0f, value);

    EXPECT_EQ (3, input.readInt());
    EXPECT_EQ (0, input.readInt());

    EXPECT_FALSE (input.readBool());
    EXPECT_TRUE (input.readBool());
    EXPECT_FALSE (input.readBool());
    EXPECT_FALSE (input.readBool());
    EXPECT_FALSE (input.readBool());

    const auto [currentPreset, presetNames] = Protocol::readPresets (input);
    EXPECT_EQ (0, currentPreset);
    EXPECT_EQ (StringArray ({ "Unity", "Half" }), presetNames);
}

TEST_F (AudioPluginBridgeServerTests, LoadForwardsTheHostContext)
{
    AudioPluginHostContext context;
    context.hostName = "Bridge Test Host";
    context.sampleRate = 96000.0f;

    ASSERT_TRUE (load ("probe", AudioPluginFormatType::clap, context).ok);

    const auto probe = getProbeState();
    EXPECT_EQ ("Bridge Test Host", probe.hostName);
    EXPECT_FLOAT_EQ (96000.0f, probe.sampleRate);
}

TEST_F (AudioPluginBridgeServerTests, AddedFormatReplacesOneOfTheSameType)
{
    const auto reply = load ("probe", AudioPluginFormatType::vst3);

    EXPECT_TRUE (reply.ok) << reply.error;
}

TEST_F (AudioPluginBridgeServerTests, LoadFailsWithMismatchingProtocolVersion)
{
    const auto reply = requestOrFail (MessageType::load, makeLoadRequest (makeDescription ("probe"), {}, Protocol::version + 1));

    EXPECT_FALSE (reply.ok);
    EXPECT_EQ ("Mismatching plugin bridge protocol version", reply.error);
}

TEST_F (AudioPluginBridgeServerTests, LoadFailsForUnsupportedFormat)
{
    const auto reply = load ("probe", AudioPluginFormatType::lv2);

    EXPECT_FALSE (reply.ok);
    EXPECT_EQ ("Format not supported by the plugin bridge server", reply.error);
}

TEST_F (AudioPluginBridgeServerTests, LoadReportsPluginErrorsAndAllowsRetrying)
{
    const auto failed = load ("broken");
    EXPECT_FALSE (failed.ok);
    EXPECT_EQ ("cannot open broken", failed.error);

    EXPECT_TRUE (load().ok);
}

TEST_F (AudioPluginBridgeServerTests, OnlyOnePluginCanBeLoaded)
{
    ASSERT_TRUE (load().ok);

    const auto reply = load();
    EXPECT_FALSE (reply.ok);
    EXPECT_EQ ("The plugin bridge server already hosts a plugin", reply.error);
}

TEST_F (AudioPluginBridgeServerTests, RequestsFailBeforeAPluginIsLoaded)
{
    for (const auto type : { MessageType::getState, MessageType::release, MessageType::closeEditor })
    {
        const auto reply = requestOrFail (type);
        EXPECT_FALSE (reply.ok);
        EXPECT_EQ ("No plugin is loaded in the plugin bridge server", reply.error);
    }
}

TEST_F (AudioPluginBridgeServerTests, UnknownRequestFails)
{
    ASSERT_TRUE (load().ok);

    const auto reply = requestOrFail (MessageType::parameterEvents);
    EXPECT_FALSE (reply.ok);
    EXPECT_EQ ("Unknown plugin bridge request", reply.error);
}

TEST_F (AudioPluginBridgeServerTests, ConvertsParameterValuesAndText)
{
    ASSERT_TRUE (load().ok);

    MemoryOutputStream toText;
    toText.writeInt (0);
    toText.writeFloat (0.5f);
    const auto text = requestOrFail (MessageType::valueToText, toText.getMemoryBlock());
    ASSERT_TRUE (text.ok) << text.error;
    EXPECT_EQ ("0.50x", MemoryInputStream (text.payload, false).readString());

    MemoryOutputStream toValue;
    toValue.writeInt (0);
    toValue.writeString ("1.5");
    const auto value = requestOrFail (MessageType::textToValue, toValue.getMemoryBlock());
    ASSERT_TRUE (value.ok) << value.error;
    EXPECT_FLOAT_EQ (1.5f, MemoryInputStream (value.payload, false).readFloat());
}

TEST_F (AudioPluginBridgeServerTests, ConversionsRejectInvalidParameterIndices)
{
    ASSERT_TRUE (load().ok);

    for (const auto index : { -1, 1 })
    {
        MemoryOutputStream toText;
        toText.writeInt (index);
        toText.writeFloat (0.5f);
        const auto text = requestOrFail (MessageType::valueToText, toText.getMemoryBlock());
        EXPECT_FALSE (text.ok);
        EXPECT_EQ ("Invalid parameter index", text.error);

        MemoryOutputStream toValue;
        toValue.writeInt (index);
        toValue.writeString ("1.0");
        const auto value = requestOrFail (MessageType::textToValue, toValue.getMemoryBlock());
        EXPECT_FALSE (value.ok);
        EXPECT_EQ ("Invalid parameter index", value.error);
    }
}

TEST_F (AudioPluginBridgeServerTests, ForwardsHostInfoAndNonRealtimeToThePlugin)
{
    ASSERT_TRUE (load().ok);

    MemoryOutputStream hostInfo;
    hostInfo.writeString ("Drums");
    hostInfo.writeBool (true);
    hostInfo.writeInt (static_cast<int> (0xff336699));
    ASSERT_TRUE (requestOrFail (MessageType::setHostInfo, hostInfo.getMemoryBlock()).ok);

    MemoryOutputStream nonRealtime;
    nonRealtime.writeBool (true);
    host.post (MessageType::setNonRealtime, nonRealtime.getMemoryBlock());

    const auto probe = getProbeState();
    EXPECT_EQ ("Drums", probe.trackName);
    EXPECT_EQ (0xff336699u, probe.trackColor);
    EXPECT_TRUE (probe.nonRealtime);
}

TEST_F (AudioPluginBridgeServerTests, SetStateReportsPluginResultAndLatencyChange)
{
    ASSERT_TRUE (load().ok);

    MemoryOutputStream empty;
    empty.writeInt64 (0);
    const auto failed = requestOrFail (MessageType::setState, empty.getMemoryBlock());
    EXPECT_FALSE (failed.ok);
    EXPECT_EQ ("empty state", failed.error);

    MemoryOutputStream state;
    state.writeInt64 (11);
    state << MemoryBlock (11, true);
    ASSERT_TRUE (requestOrFail (MessageType::setState, state.getMemoryBlock()).ok);

    const auto changed = host.waitForMessage (MessageType::processorChanged);
    ASSERT_TRUE (changed.has_value());

    MemoryInputStream input (*changed, false);
    EXPECT_NE (0, input.readInt() & 1);
    EXPECT_EQ (11, input.readInt());
}

TEST_F (AudioPluginBridgeServerTests, SetPresetNotifiesProcessorAndParameterChanges)
{
    ASSERT_TRUE (load().ok);

    MemoryOutputStream preset;
    preset.writeInt (1);
    ASSERT_TRUE (requestOrFail (MessageType::setPreset, preset.getMemoryBlock()).ok);

    const auto changed = host.waitForMessage (MessageType::processorChanged);
    ASSERT_TRUE (changed.has_value());

    MemoryInputStream changedInput (*changed, false);
    EXPECT_EQ (0, changedInput.readInt());
    EXPECT_EQ (3, changedInput.readInt());
    EXPECT_EQ (0, changedInput.readInt());
    EXPECT_EQ (1, Protocol::readPresets (changedInput).first);

    uint8 flags = 0;
    double value = 0.0;

    while (flags != (AudioPluginParameterMailbox::valueChanged | AudioPluginParameterMailbox::gestureBegan | AudioPluginParameterMailbox::gestureEnded))
    {
        const auto events = host.waitForMessage (MessageType::parameterEvents);
        ASSERT_TRUE (events.has_value());

        MemoryInputStream input (*events, false);
        for (int i = input.readInt(); i > 0; --i)
        {
            EXPECT_EQ (0, input.readInt());
            value = input.readDouble();
            flags |= static_cast<uint8> (input.readByte());
        }
    }

    EXPECT_DOUBLE_EQ (0.5, value);
}

TEST_F (AudioPluginBridgeServerTests, OpeningAMissingEditorReportsItClosed)
{
    ASSERT_TRUE (load().ok);

    MemoryOutputStream owner;
    owner.writeInt64 (0);
    owner.writeInt (0);
    ASSERT_TRUE (requestOrFail (MessageType::openEditor, owner.getMemoryBlock()).ok);

    EXPECT_TRUE (host.waitForMessage (MessageType::editorClosed).has_value());
    EXPECT_TRUE (requestOrFail (MessageType::closeEditor).ok);
}

TEST_F (AudioPluginBridgeServerTests, PrepareFailsWithoutSharedMemory)
{
    ASSERT_TRUE (load().ok);

    MemoryOutputStream prepare;
    prepare.writeFloat (44100.0f);
    prepare.writeInt (32);
    prepare.writeString (SharedMemory::createUniqueName ("yupbsmissing"));
    prepare.writeInt (2);
    prepare.writeInt (2);

    EXPECT_FALSE (requestOrFail (MessageType::prepare, prepare.getMemoryBlock()).ok);
    EXPECT_TRUE (requestOrFail (MessageType::release).ok);
}

TEST_F (AudioPluginBridgeServerTests, ProcessesSharedMemoryBlocks)
{
    ASSERT_TRUE (load().ok);

    constexpr int blockSize = 32;
    const Protocol::SharedLayout layout (blockSize, 2, 2);
    const auto name = SharedMemory::createUniqueName ("yupbs");

    SharedMemory memory;
    InterProcessSemaphore requestSemaphore, doneSemaphore;
    ASSERT_TRUE (memory.create (name, layout.totalSize).wasOk());
    ASSERT_TRUE (requestSemaphore.create (Protocol::getRequestSemaphoreName (name)).wasOk());
    ASSERT_TRUE (doneSemaphore.create (Protocol::getDoneSemaphoreName (name)).wasOk());

    MemoryOutputStream prepare;
    prepare.writeFloat (44100.0f);
    prepare.writeInt (blockSize);
    prepare.writeString (name);
    prepare.writeInt (layout.numInputChannels);
    prepare.writeInt (layout.numWorkChannels);
    const auto prepared = requestOrFail (MessageType::prepare, prepare.getMemoryBlock());
    ASSERT_TRUE (prepared.ok) << prepared.error;

    const Protocol::SharedBlock block (memory.getData(), layout);
    auto& header = *block.header;

    for (int channel = 0; channel < 2; ++channel)
    {
        FloatVectorOperations::fill (block.inputChannels[static_cast<std::size_t> (channel)], 0.25f, blockSize);
        FloatVectorOperations::fill (block.workChannels[static_cast<std::size_t> (channel)], 9.0f, blockSize);
    }

    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 64, 0.5f), 5);

    header.numSamples = blockSize;
    header.numMidiInputBytes = Protocol::writeMidi (midi, block.midiInput, Protocol::midiCapacityBytes);
    header.numParameterChanges = 1;
    block.parameterChanges[0] = { 0, 1.0f, 0 };
    header.flags = 0;
    header.requestSequence.store (1);
    requestSemaphore.signal();

    ASSERT_TRUE (doneSemaphore.wait (5000));
    EXPECT_EQ (1u, header.doneSequence.load());

    for (int channel = 0; channel < 2; ++channel)
    {
        EXPECT_FLOAT_EQ (0.5f, block.workChannels[static_cast<std::size_t> (channel)][0]);
        EXPECT_FLOAT_EQ (0.5f, block.workChannels[static_cast<std::size_t> (channel)][blockSize - 1]);
    }

    MidiBuffer echoed;
    Protocol::readMidi (block.midiOutput, header.numMidiOutputBytes, echoed);
    ASSERT_EQ (1, echoed.getNumEvents());
    EXPECT_EQ (64, (*echoed.begin()).getMessage().getNoteNumber());
    EXPECT_EQ (5, (*echoed.begin()).samplePosition);

    EXPECT_TRUE (requestOrFail (MessageType::release).ok);
}

#endif
