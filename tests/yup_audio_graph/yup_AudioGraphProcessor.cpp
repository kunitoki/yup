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

#include <atomic>
#include <cmath>
#include <thread>

#include <yup_audio_graph/yup_audio_graph.h>

using namespace yup;

namespace
{
AudioBusLayout stereoLayout()
{
    return AudioBusLayout ({ AudioBus ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                           { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) });
}

AudioBusLayout monoLayout()
{
    return AudioBusLayout ({ AudioBus ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 1) },
                           { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 1) });
}

AudioBusLayout midiLayout()
{
    return AudioBusLayout ({ AudioBus ("MIDI In", AudioBus::Type::MIDI, AudioBus::Direction::Input, 0) },
                           { AudioBus ("MIDI Out", AudioBus::Type::MIDI, AudioBus::Direction::Output, 0) });
}

class TestProcessor : public AudioProcessor
{
public:
    explicit TestProcessor (float gainToUse = 1.0f, int latencyToUse = 0)
        : AudioProcessor ("Test", stereoLayout())
        , gain (gainToUse)
        , latency (latencyToUse)
    {
    }

    void prepareToPlay (float sampleRate, int maxBlockSize) override
    {
        preparedSampleRate = sampleRate;
        preparedBlockSize = maxBlockSize;
        prepared = true;
    }

    void releaseResources() override { prepared = false; }

    void processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer&) override
    {
        for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
            audioBuffer.applyGain (channel, 0, audioBuffer.getNumSamples(), gain);
    }

    int getLatencySamples() override { return latency; }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

    Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

    bool hasEditor() const override { return false; }

    float preparedSampleRate = 0.0f;
    int preparedBlockSize = 0;
    bool prepared = false;

private:
    float gain = 1.0f;
    int latency = 0;
};

class MidiPassthroughProcessor : public AudioProcessor
{
public:
    explicit MidiPassthroughProcessor (int latencyToUse = 0)
        : AudioProcessor ("MIDI Passthrough", midiLayout())
        , latency (latencyToUse)
    {
    }

    void prepareToPlay (float, int) override {}

    void releaseResources() override {}

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override {}

    int getLatencySamples() override { return latency; }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

    Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

    bool hasEditor() const override { return false; }

private:
    int latency = 0;
};

class MidiDelayingProcessor : public AudioProcessor
{
public:
    explicit MidiDelayingProcessor (int latencyToUse)
        : AudioProcessor ("MIDI Delay", midiLayout())
        , latency (latencyToUse)
    {
        pendingMidi.ensureSize (4096);
        nextPendingMidi.ensureSize (4096);
    }

    void prepareToPlay (float, int) override
    {
        pendingMidi.clear();
        nextPendingMidi.clear();
    }

    void releaseResources() override
    {
        pendingMidi.clear();
        nextPendingMidi.clear();
    }

    void flush() override
    {
        pendingMidi.clear();
        nextPendingMidi.clear();
    }

    void processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer& midiBuffer) override
    {
        const int numSamples = audioBuffer.getNumSamples();
        const MidiBuffer inputMidi = midiBuffer;

        midiBuffer.clear();
        nextPendingMidi.clear();

        for (const auto metadata : pendingMidi)
        {
            if (metadata.samplePosition < numSamples)
            {
                midiBuffer.addEvent (metadata.data, metadata.numBytes, metadata.samplePosition);
            }
            else
            {
                nextPendingMidi.addEvent (metadata.data, metadata.numBytes, metadata.samplePosition - numSamples);
            }
        }

        for (const auto metadata : inputMidi)
        {
            if (metadata.samplePosition < 0 || metadata.samplePosition >= numSamples)
            {
                continue;
            }

            const int delayedPosition = metadata.samplePosition + latency;

            if (delayedPosition < numSamples)
            {
                midiBuffer.addEvent (metadata.data, metadata.numBytes, delayedPosition);
            }
            else
            {
                nextPendingMidi.addEvent (metadata.data, metadata.numBytes, delayedPosition - numSamples);
            }
        }

        pendingMidi.swapWith (nextPendingMidi);
    }

    int getLatencySamples() override { return latency; }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

    Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

    bool hasEditor() const override { return false; }

private:
    int latency = 0;
    MidiBuffer pendingMidi;
    MidiBuffer nextPendingMidi;
};

class MonoLayoutProcessor : public AudioProcessor
{
public:
    MonoLayoutProcessor()
        : AudioProcessor ("Mono", monoLayout())
    {
    }

    void prepareToPlay (float, int) override {}

    void releaseResources() override {}

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override {}

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

    Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

    bool hasEditor() const override { return false; }
};

class DelayingProcessor : public TestProcessor
{
public:
    explicit DelayingProcessor (int latencyToUse)
        : TestProcessor (1.0f, latencyToUse)
        , delaySamples (latencyToUse)
        , history (2, latencyToUse + 32)
    {
        history.clear();
    }

    void processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer&) override
    {
        const int ringSize = history.getNumSamples();

        for (int sample = 0; sample < audioBuffer.getNumSamples(); ++sample)
        {
            const int readPosition = (writePosition + ringSize - delaySamples) % ringSize;

            for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
            {
                const float input = audioBuffer.getReadPointer (channel)[sample];
                audioBuffer.getWritePointer (channel)[sample] = history.getReadPointer (channel)[readPosition];
                history.getWritePointer (channel)[writePosition] = input;
            }

            writePosition = (writePosition + 1) % ringSize;
        }
    }

private:
    int delaySamples = 0;
    int writePosition = 0;
    AudioBuffer<float> history;
};

class BlockingLatencyProcessor : public TestProcessor
{
public:
    BlockingLatencyProcessor (std::atomic<bool>& enteredFlag, std::atomic<bool>& continueFlag)
        : TestProcessor()
        , entered (enteredFlag)
        , shouldContinue (continueFlag)
    {
    }

    int getLatencySamples() override
    {
        entered.store (true);

        while (! shouldContinue.load())
            std::this_thread::yield();

        return 0;
    }

private:
    std::atomic<bool>& entered;
    std::atomic<bool>& shouldContinue;
};

class StatefulGainProcessor : public AudioProcessor
{
public:
    explicit StatefulGainProcessor (float initialGain)
        : AudioProcessor ("Stateful Gain", stereoLayout())
        , gain (initialGain)
    {
    }

    void prepareToPlay (float, int) override {}

    void releaseResources() override {}

    void processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer&) override
    {
        for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
            audioBuffer.applyGain (channel, 0, audioBuffer.getNumSamples(), gain);
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock& memoryBlock) override
    {
        if (memoryBlock.getSize() != sizeof (float))
            return Result::fail ("Invalid gain state");

        MemoryInputStream stream (memoryBlock, false);
        gain = stream.readFloat();
        return Result::ok();
    }

    Result saveStateIntoMemory (MemoryBlock& memoryBlock) override
    {
        MemoryOutputStream stream (memoryBlock, false);
        stream.writeFloat (gain);
        stream.flush();
        return Result::ok();
    }

    bool hasEditor() const override { return false; }

    void setGain (float newGain) noexcept { gain = newGain; }

private:
    float gain = 1.0f;
};

AudioGraphNodeProperties statefulGainProperties (float positionX = 0.0f, float positionY = 0.0f)
{
    AudioGraphNodeProperties properties;
    properties.identifier = "statefulGain";
    properties.name = "Stateful Gain";
    properties.positionX = positionX;
    properties.positionY = positionY;
    return properties;
}

AudioGraphProcessor::NodeFactory statefulGainFactory()
{
    return [] (const AudioGraphNodeProperties& properties) -> ResultValue<std::unique_ptr<AudioProcessor>>
    {
        if (properties.identifier != "statefulGain")
            return makeResultValueFail ("Unknown node type");

        return makeResultValueOk (std::make_unique<StatefulGainProcessor> (1.0f));
    };
}

class StatefulExternalProcessor : public AudioProcessor
{
public:
    explicit StatefulExternalProcessor (String processorName, float initialGain = 1.0f)
        : AudioProcessor (std::move (processorName), stereoLayout())
        , gain (initialGain)
    {
    }

    void prepareToPlay (float, int) override {}

    void releaseResources() override {}

    void processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer&) override
    {
        for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
            audioBuffer.applyGain (channel, 0, audioBuffer.getNumSamples(), gain);
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock& memoryBlock) override
    {
        if (memoryBlock.getSize() != sizeof (float))
            return Result::fail ("Invalid external processor gain state");

        MemoryInputStream stream (memoryBlock, false);
        gain = stream.readFloat();
        return Result::ok();
    }

    Result saveStateIntoMemory (MemoryBlock& memoryBlock) override
    {
        MemoryOutputStream stream (memoryBlock, false);
        stream.writeFloat (gain);
        stream.flush();
        return Result::ok();
    }

    bool hasEditor() const override { return false; }

private:
    float gain = 1.0f;
};

class SaveFailingProcessor : public AudioProcessor
{
public:
    SaveFailingProcessor()
        : AudioProcessor ("Save Failing", stereoLayout())
    {
    }

    void prepareToPlay (float, int) override {}

    void releaseResources() override {}

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override {}

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

    Result saveStateIntoMemory (MemoryBlock&) override { return Result::fail ("Save failed"); }

    bool hasEditor() const override { return false; }
};

AudioGraphNodeProperties externalProcessorProperties (float positionX = 0.0f, float positionY = 0.0f)
{
    AudioGraphNodeProperties properties;
    properties.identifier = "externalPlugin";
    properties.name = "External Plugin";
    properties.positionX = positionX;
    properties.positionY = positionY;

    MemoryOutputStream stream (properties.creationData, false);
    stream.writeString ("Fake Plugin");
    stream.writeString ("fake.plugin");
    stream.flush();

    return properties;
}

AudioGraphProcessor::NodeFactory statefulGainAndExternalFactory (int& externalLoadCount, String& externalIdentifier)
{
    return [&externalLoadCount, &externalIdentifier] (const AudioGraphNodeProperties& properties) -> ResultValue<std::unique_ptr<AudioProcessor>>
    {
        if (properties.identifier == "statefulGain")
            return makeResultValueOk (std::make_unique<StatefulGainProcessor> (1.0f));

        if (properties.identifier != "externalPlugin")
            return makeResultValueFail ("Unknown node type");

        MemoryInputStream stream (properties.creationData, false);
        const auto pluginName = stream.readString();
        externalIdentifier = stream.readString();
        ++externalLoadCount;

        return makeResultValueOk (std::make_unique<StatefulExternalProcessor> (pluginName));
    };
}

void fillImpulse (AudioBuffer<float>& buffer)
{
    buffer.clear();
    buffer.getWritePointer (0)[0] = 1.0f;
    buffer.getWritePointer (1)[0] = 1.0f;
}

void fillImpulseAt (AudioBuffer<float>& buffer, int samplePosition)
{
    buffer.clear();
    buffer.getWritePointer (0)[samplePosition] = 1.0f;
    buffer.getWritePointer (1)[samplePosition] = 1.0f;
}

int countMidiEventsAt (const MidiBuffer& midi, int samplePosition)
{
    int count = 0;

    for (const auto metadata : midi)
        if (metadata.samplePosition == samplePosition)
            ++count;

    return count;
}

int countMidiEvents (const MidiBuffer& midi)
{
    int count = 0;

    for (const auto metadata : midi)
        if (metadata.data != nullptr)
            ++count;

    return count;
}

std::unique_ptr<XmlElement> parseMemoryBlockAsXml (const MemoryBlock& memoryBlock)
{
    MemoryInputStream stream (memoryBlock, false);
    return parseXML (stream.readEntireStreamAsString());
}

MemoryBlock memoryBlockFromString (const String& text)
{
    MemoryBlock result;
    MemoryOutputStream stream (result, false);
    stream << text;
    stream.flush();
    return result;
}

MemoryBlock memoryBlockFromXml (const XmlElement& xml)
{
    MemoryBlock result;
    MemoryOutputStream stream (result, false);
    xml.writeTo (stream);
    stream.flush();
    return result;
}

String toBase64Text (const MemoryBlock& block)
{
    return Base64::toBase64 (block.getData(), block.getSize());
}

bool resetGraphToSingleGainPath (AudioGraphProcessor& graph, float gain)
{
    graph.clear();

    const auto node = graph.addNode (std::make_unique<TestProcessor> (gain));
    if (! node.isValid())
        return false;

    if (graph.addConnection ({ AudioGraphEndpoint::graphInput (0),
                               AudioGraphEndpoint::nodeInput (node, 0) })
            .failed())
        return false;

    if (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0),
                               AudioGraphEndpoint::graphOutput (0) })
            .failed())
        return false;

    return graph.commitChanges().wasOk();
}

bool isExpectedGain (float sample)
{
    return std::abs (sample - 0.25f) < 0.000001f
        || std::abs (sample - 0.75f) < 0.000001f;
}

class DenormalCheckProcessor : public AudioProcessor
{
public:
    DenormalCheckProcessor()
        : AudioProcessor ("DenormalCheck", stereoLayout())
    {
    }

    void prepareToPlay (float, int) override {}

    void releaseResources() override {}

    void processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer&) override
    {
        for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
            audioBuffer.applyGain (channel, 0, audioBuffer.getNumSamples(), 1.0f);

        denormalsWereDisabled = FloatVectorOperations::areDenormalsDisabled();
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

    Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

    bool hasEditor() const override { return false; }

    bool denormalsWereDisabled = false;
};

AudioBusLayout mixedLayout()
{
    return AudioBusLayout ({ AudioBus ("Audio In", AudioBus::Type::Audio, AudioBus::Direction::Input, 2),
                             AudioBus ("MIDI In", AudioBus::Type::MIDI, AudioBus::Direction::Input, 0) },
                           { AudioBus ("Audio Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2),
                             AudioBus ("MIDI Out", AudioBus::Type::MIDI, AudioBus::Direction::Output, 0) });
}

class MixedProcessor : public AudioProcessor
{
public:
    explicit MixedProcessor (float gainToUse = 1.0f, int latencyToUse = 0)
        : AudioProcessor ("Mixed", mixedLayout())
        , gain (gainToUse)
        , latency (latencyToUse)
    {
    }

    void prepareToPlay (float, int) override {}

    void releaseResources() override {}

    void processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer&) override
    {
        for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
            audioBuffer.applyGain (channel, 0, audioBuffer.getNumSamples(), gain);
    }

    int getLatencySamples() override { return latency; }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

    Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

    bool hasEditor() const override { return false; }

private:
    float gain = 1.0f;
    int latency = 0;
};

class MixedDelayingProcessor : public AudioProcessor
{
public:
    explicit MixedDelayingProcessor (int latencyToUse)
        : AudioProcessor ("MixedDelay", mixedLayout())
        , delaySamples (latencyToUse)
        , history (2, latencyToUse + 32)
    {
        history.clear();
    }

    void prepareToPlay (float, int) override
    {
        history.clear();
        writePosition = 0;
    }

    void releaseResources() override {}

    void flush() override
    {
        history.clear();
        writePosition = 0;
    }

    void processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer&) override
    {
        const int ringSize = history.getNumSamples();

        for (int sample = 0; sample < audioBuffer.getNumSamples(); ++sample)
        {
            const int readPosition = (writePosition + ringSize - delaySamples) % ringSize;

            for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
            {
                const float input = audioBuffer.getReadPointer (channel)[sample];
                audioBuffer.getWritePointer (channel)[sample] = history.getReadPointer (channel)[readPosition];
                history.getWritePointer (channel)[writePosition] = input;
            }

            writePosition = (writePosition + 1) % ringSize;
        }
    }

    int getLatencySamples() override { return delaySamples; }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

    Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

    bool hasEditor() const override { return false; }

private:
    int delaySamples = 0;
    int writePosition = 0;
    AudioBuffer<float> history;
};
} // namespace

TEST (AudioGraphProcessorTests, CommitPreparesNodes)
{
    AudioGraphProcessor graph;
    auto processor = std::make_unique<TestProcessor>();
    auto* processorPtr = processor.get();
    const auto node = graph.addNode (std::move (processor));

    EXPECT_TRUE (node.isValid());

    graph.prepareToPlay (48000.0f, 128);
    const auto result = graph.commitChanges();

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (processorPtr->prepared);
    EXPECT_FLOAT_EQ (48000.0f, processorPtr->preparedSampleRate);
    EXPECT_EQ (128, processorPtr->preparedBlockSize);
}

TEST (AudioGraphProcessorTests, RejectsMissingNodeConnection)
{
    AudioGraphProcessor graph;

    const auto result = graph.addConnection ({ AudioGraphEndpoint::nodeOutput (AudioGraphNodeID::invalid(), 0),
                                               AudioGraphEndpoint::graphOutput (0) });

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (graph.commitChanges().failed());
}

TEST (AudioGraphProcessorTests, RejectsCycles)
{
    AudioGraphProcessor graph;
    const auto first = graph.addNode (std::make_unique<TestProcessor>());
    const auto second = graph.addNode (std::make_unique<TestProcessor>());

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (first, 0),
                                        AudioGraphEndpoint::nodeInput (second, 0) })
                     .wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (second, 0),
                                        AudioGraphEndpoint::nodeInput (first, 0) })
                     .wasOk());

    EXPECT_TRUE (graph.commitChanges().failed());
}

TEST (AudioGraphProcessorTests, ProcessesSerialAudioChain)
{
    AudioGraphProcessor graph;
    const auto first = graph.addNode (std::make_unique<TestProcessor> (2.0f));
    const auto second = graph.addNode (std::make_unique<TestProcessor> (0.5f));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (first, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (first, 0), AudioGraphEndpoint::nodeInput (second, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (second, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (1.0f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (1.0f, audio.getReadPointer (1)[0]);
}

TEST (AudioGraphProcessorTests, ProcessesBlocksLargerThanPreparedMaximumInChunks)
{
    AudioGraphProcessor graph;
    graph.prepareToPlay (48000.0f, 16);

    const auto node = graph.addNode (std::make_unique<TestProcessor> (0.5f));
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 40);
    MidiBuffer midi;

    for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        for (int sample = 0; sample < audio.getNumSamples(); ++sample)
            audio.getWritePointer (channel)[sample] = 1.0f;

    graph.processBlock (audio, midi);

    for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        for (int sample = 0; sample < audio.getNumSamples(); ++sample)
            EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (channel)[sample]) << "channel " << channel << " sample " << sample;
}

TEST (AudioGraphProcessorTests, PreservesMidiEventsInBlocksLargerThanPreparedMaximum)
{
    AudioGraphProcessor graph (midiLayout());
    graph.prepareToPlay (48000.0f, 16);

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0),
                                        AudioGraphEndpoint::graphOutput (0) })
                     .wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (0, 40);
    MidiBuffer midi;
    const uint8 noteOn[] = { 0x90, 60, 100 };

    midi.addEvent (noteOn, 3, 7);
    midi.addEvent (noteOn, 3, 17);
    midi.addEvent (noteOn, 3, 35);

    graph.processBlock (audio, midi);

    EXPECT_EQ (1, countMidiEventsAt (midi, 7));
    EXPECT_EQ (1, countMidiEventsAt (midi, 17));
    EXPECT_EQ (1, countMidiEventsAt (midi, 35));
    EXPECT_EQ (3, countMidiEvents (midi));
}

TEST (AudioGraphProcessorTests, MixesFanIn)
{
    AudioGraphProcessor graph;
    const auto left = graph.addNode (std::make_unique<TestProcessor> (0.25f));
    const auto right = graph.addNode (std::make_unique<TestProcessor> (0.75f));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (left, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (right, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (left, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (right, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (1.0f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (1.0f, audio.getReadPointer (1)[0]);
}

TEST (AudioGraphProcessorTests, PreservesMidiTimestamps)
{
    AudioBusLayout midiLayout ({ AudioBus ("MIDI In", AudioBus::Type::MIDI, AudioBus::Direction::Input, 0) },
                               { AudioBus ("MIDI Out", AudioBus::Type::MIDI, AudioBus::Direction::Output, 0) });
    AudioGraphProcessor graph (midiLayout);

    const auto node = graph.addNode (std::make_unique<MidiPassthroughProcessor>());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (0, 32);
    MidiBuffer midi;
    const uint8 noteOn[] = { 0x90, 60, 100 };
    midi.addEvent (noteOn, 3, 7);

    graph.processBlock (audio, midi);

    EXPECT_EQ (1, countMidiEventsAt (midi, 7));
}

TEST (AudioGraphProcessorTests, CompensatesShorterParallelPaths)
{
    AudioGraphProcessor graph;
    const auto dry = graph.addNode (std::make_unique<TestProcessor> (1.0f, 0));
    const auto latent = graph.addNode (std::make_unique<DelayingProcessor> (4));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (dry, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (latent, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (dry, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (latent, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (2.0f, audio.getReadPointer (0)[4]);
    EXPECT_EQ (4, graph.getLatencySamples());
    EXPECT_EQ (4, graph.getAllocationStats().totalCompensationSamples);
}

TEST (AudioGraphProcessorTests, NullNodeIsRejected)
{
    AudioGraphProcessor graph;

    EXPECT_FALSE (graph.addNode (nullptr).isValid());
    EXPECT_TRUE (graph.hasUncommittedChanges());
}

TEST (AudioGraphProcessorTests, RejectsInvalidEndpointDirectionImmediately)
{
    AudioGraphProcessor graph;
    const auto node = graph.addNode (std::make_unique<TestProcessor>());

    const auto badSource = graph.addConnection ({ AudioGraphEndpoint::nodeInput (node, 0),
                                                  AudioGraphEndpoint::graphOutput (0) });
    const auto badDestination = graph.addConnection ({ AudioGraphEndpoint::graphInput (0),
                                                       AudioGraphEndpoint::nodeOutput (node, 0) });

    EXPECT_TRUE (badSource.failed());
    EXPECT_TRUE (badDestination.failed());
}

TEST (AudioGraphProcessorTests, RejectsDuplicateConnectionsImmediately)
{
    AudioGraphProcessor graph;
    const auto node = graph.addNode (std::make_unique<TestProcessor>());
    const AudioGraphConnection connection { AudioGraphEndpoint::graphInput (0),
                                            AudioGraphEndpoint::nodeInput (node, 0) };

    EXPECT_TRUE (graph.addConnection (connection).wasOk());
    EXPECT_TRUE (graph.addConnection (connection).failed());
}

TEST (AudioGraphProcessorTests, RejectsInvalidBusIndexAtCommit)
{
    AudioGraphProcessor graph;
    const auto node = graph.addNode (std::make_unique<TestProcessor>());

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0),
                                        AudioGraphEndpoint::nodeInput (node, 99) })
                     .wasOk());
    EXPECT_TRUE (graph.commitChanges().failed());
}

TEST (AudioGraphProcessorTests, RejectsAudioMidiTypeMismatchAtCommit)
{
    AudioGraphProcessor graph;
    const auto audioNode = graph.addNode (std::make_unique<TestProcessor>());
    const auto midiNode = graph.addNode (std::make_unique<MidiPassthroughProcessor>());

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (audioNode, 0),
                                        AudioGraphEndpoint::nodeInput (midiNode, 0) })
                     .wasOk());
    EXPECT_TRUE (graph.commitChanges().failed());
}

TEST (AudioGraphProcessorTests, RejectsAudioChannelMismatchAtCommit)
{
    AudioGraphProcessor graph;
    const auto stereoNode = graph.addNode (std::make_unique<TestProcessor>());
    const auto monoNode = graph.addNode (std::make_unique<MonoLayoutProcessor>());

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (stereoNode, 0),
                                        AudioGraphEndpoint::nodeInput (monoNode, 0) })
                     .wasOk());
    EXPECT_TRUE (graph.commitChanges().failed());
}

TEST (AudioGraphProcessorTests, MissingCommitKeepsPreviousPlan)
{
    AudioGraphProcessor graph;
    const auto first = graph.addNode (std::make_unique<TestProcessor> (0.5f));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (first, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (first, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    const auto second = graph.addNode (std::make_unique<TestProcessor> (0.25f));
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (first, 0), AudioGraphEndpoint::nodeInput (second, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (second, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.hasUncommittedChanges());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (0)[0]);
}

TEST (AudioGraphProcessorTests, CommitKeepsDirtyWhenModelChangesDuringCompilation)
{
    AudioGraphProcessor graph;

    std::atomic<bool> latencyEntered { false };
    std::atomic<bool> allowLatencyQueryToContinue { false };

    const auto blockingNode = graph.addNode (std::make_unique<BlockingLatencyProcessor> (latencyEntered, allowLatencyQueryToContinue));
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (blockingNode, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (blockingNode, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());

    std::atomic<bool> commitSucceeded { false };
    std::thread commitThread ([&]
    {
        commitSucceeded.store (graph.commitChanges().wasOk());
    });

    for (int spinCount = 0; spinCount < 10000 && ! latencyEntered.load(); ++spinCount)
        std::this_thread::yield();

    const bool didEnterLatencyQuery = latencyEntered.load();
    EXPECT_TRUE (didEnterLatencyQuery);

    if (! didEnterLatencyQuery)
    {
        allowLatencyQueryToContinue.store (true);
        commitThread.join();
        return;
    }

    const auto addedDuringCommit = graph.addNode (std::make_unique<TestProcessor>());
    EXPECT_TRUE (addedDuringCommit.isValid());

    allowLatencyQueryToContinue.store (true);
    commitThread.join();

    EXPECT_TRUE (commitSucceeded.load());
    EXPECT_TRUE (graph.hasUncommittedChanges());

    EXPECT_TRUE (graph.commitChanges().wasOk());
    EXPECT_FALSE (graph.hasUncommittedChanges());
}

TEST (AudioGraphProcessorTests, SaveAndLoadRestoresConnectionsAndNodeState)
{
    AudioGraphProcessor graph;
    auto processor = std::make_unique<StatefulGainProcessor> (0.25f);
    auto* processorPtr = processor.get();
    const auto node = graph.addNode (std::move (processor), statefulGainProperties (12.0f, 34.0f));

    const AudioGraphConnection inputConnection { AudioGraphEndpoint::graphInput (0),
                                                 AudioGraphEndpoint::nodeInput (node, 0) };
    const AudioGraphConnection outputConnection { AudioGraphEndpoint::nodeOutput (node, 0),
                                                  AudioGraphEndpoint::graphOutput (0) };
    const AudioGraphConnection bypassConnection { AudioGraphEndpoint::graphInput (0),
                                                  AudioGraphEndpoint::graphOutput (0) };

    ASSERT_TRUE (graph.addConnection (inputConnection).wasOk());
    ASSERT_TRUE (graph.addConnection (outputConnection).wasOk());
    ASSERT_TRUE (graph.commitChanges().wasOk());

    MemoryBlock savedState;
    EXPECT_TRUE (graph.saveStateIntoMemory (savedState).wasOk());

    auto xml = parseMemoryBlockAsXml (savedState);
    ASSERT_NE (nullptr, xml.get());
    EXPECT_TRUE (xml->hasTagName ("YUPAudioGraphState"));

    auto* savedNodeElement = xml->getChildByName ("nodes")->getChildByName ("node");
    ASSERT_NE (nullptr, savedNodeElement);
    auto* savedNodeStateElement = savedNodeElement->getChildByName ("state");
    ASSERT_NE (nullptr, savedNodeStateElement);
    EXPECT_EQ (String ("base64"), savedNodeStateElement->getStringAttribute ("encoding"));
    EXPECT_FALSE (savedNodeStateElement->getAllSubText().trim().isEmpty());

    processorPtr->setGain (0.75f);
    EXPECT_TRUE (graph.removeConnection (inputConnection));
    EXPECT_TRUE (graph.removeConnection (outputConnection));
    EXPECT_TRUE (graph.addConnection (bypassConnection).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    graph.setNodeFactory (statefulGainFactory());
    EXPECT_TRUE (graph.loadStateFromMemory (savedState).wasOk());
    EXPECT_FALSE (graph.hasUncommittedChanges());

    const auto properties = graph.getNodeProperties (node);
    ASSERT_TRUE (properties.has_value());
    EXPECT_EQ (String ("statefulGain"), properties->identifier);
    EXPECT_FLOAT_EQ (12.0f, properties->positionX);
    EXPECT_FLOAT_EQ (34.0f, properties->positionY);

    const auto connections = graph.getConnections();
    ASSERT_EQ (2u, connections.size());
    EXPECT_EQ (inputConnection, connections[0]);
    EXPECT_EQ (outputConnection, connections[1]);

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.25f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (0.25f, audio.getReadPointer (1)[0]);
}

TEST (AudioGraphProcessorTests, LoadStateRecreatesProcessorNodesWithFactory)
{
    AudioGraphProcessor source;
    const auto node = source.addNode (std::make_unique<StatefulGainProcessor> (0.25f),
                                      statefulGainProperties (64.0f, 128.0f));

    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    ASSERT_TRUE (source.commitChanges().wasOk());

    MemoryBlock savedState;
    ASSERT_TRUE (source.saveStateIntoMemory (savedState).wasOk());

    AudioGraphProcessor destination;
    destination.setNodeFactory (statefulGainFactory());

    EXPECT_TRUE (destination.loadStateFromMemory (savedState).wasOk());

    const auto properties = destination.getNodeProperties (node);
    ASSERT_TRUE (properties.has_value());
    EXPECT_EQ (String ("statefulGain"), properties->identifier);
    EXPECT_FLOAT_EQ (64.0f, properties->positionX);
    EXPECT_FLOAT_EQ (128.0f, properties->positionY);

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    destination.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.25f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (0.25f, audio.getReadPointer (1)[0]);
}

TEST (AudioGraphProcessorTests, CreateXmlAndRestoreFromXmlCanBeUsedDirectly)
{
    AudioGraphProcessor source;
    const auto node = source.addNode (std::make_unique<StatefulGainProcessor> (0.25f),
                                      statefulGainProperties (96.0f, 192.0f));

    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    ASSERT_TRUE (source.commitChanges().wasOk());

    const auto xml = source.createXml();
    ASSERT_NE (nullptr, xml.get());
    EXPECT_TRUE (xml->hasTagName ("YUPAudioGraphState"));

    AudioGraphProcessor destination;
    destination.setNodeFactory (statefulGainFactory());

    ASSERT_TRUE (destination.restoreFromXml (*xml).wasOk());
    EXPECT_FALSE (destination.hasUncommittedChanges());

    const auto properties = destination.getNodeProperties (node);
    ASSERT_TRUE (properties.has_value());
    EXPECT_EQ (String ("statefulGain"), properties->identifier);
    EXPECT_FLOAT_EQ (96.0f, properties->positionX);
    EXPECT_FLOAT_EQ (192.0f, properties->positionY);

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    destination.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.25f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (0.25f, audio.getReadPointer (1)[0]);
}

TEST (AudioGraphProcessorTests, LoadStateFailsWhenFactoryIsMissing)
{
    AudioGraphProcessor source;
    const auto node = source.addNode (std::make_unique<StatefulGainProcessor> (0.25f),
                                      statefulGainProperties());

    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    ASSERT_TRUE (source.commitChanges().wasOk());

    MemoryBlock savedState;
    ASSERT_TRUE (source.saveStateIntoMemory (savedState).wasOk());

    AudioGraphProcessor destination;
    EXPECT_TRUE (destination.loadStateFromMemory (savedState).failed());
}

TEST (AudioGraphProcessorTests, LoadStateRecreatesExternalNodesWithOpaqueCreationData)
{
    AudioGraphProcessor source;
    const auto node = source.addNode (std::make_unique<StatefulExternalProcessor> ("Fake Plugin", 0.25f),
                                      externalProcessorProperties (4.0f, 8.0f));

    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    ASSERT_TRUE (source.commitChanges().wasOk());

    MemoryBlock savedState;
    ASSERT_TRUE (source.saveStateIntoMemory (savedState).wasOk());

    auto xml = parseMemoryBlockAsXml (savedState);
    ASSERT_NE (nullptr, xml.get());
    auto* savedNodeElement = xml->getChildByName ("nodes")->getChildByName ("node");
    ASSERT_NE (nullptr, savedNodeElement);
    auto* creationDataElement = savedNodeElement->getChildByName ("creationData");
    ASSERT_NE (nullptr, creationDataElement);
    EXPECT_EQ (String ("base64"), creationDataElement->getStringAttribute ("encoding"));
    EXPECT_FALSE (creationDataElement->getAllSubText().trim().isEmpty());

    int externalLoadCount = 0;
    String externalIdentifier;

    AudioGraphProcessor destination;
    destination.setNodeFactory (statefulGainAndExternalFactory (externalLoadCount, externalIdentifier));

    EXPECT_TRUE (destination.loadStateFromMemory (savedState).wasOk());
    EXPECT_EQ (1, externalLoadCount);
    EXPECT_EQ (String ("fake.plugin"), externalIdentifier);

    const auto properties = destination.getNodeProperties (node);
    ASSERT_TRUE (properties.has_value());
    EXPECT_EQ (String ("externalPlugin"), properties->identifier);
    EXPECT_FALSE (properties->creationData.isEmpty());
    EXPECT_FLOAT_EQ (4.0f, properties->positionX);
    EXPECT_FLOAT_EQ (8.0f, properties->positionY);

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    destination.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.25f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (0.25f, audio.getReadPointer (1)[0]);
}

TEST (AudioGraphProcessorTests, LoadStateFailureRestoresPreviousGraphModel)
{
    AudioGraphProcessor invalidSource;
    ASSERT_TRUE (invalidSource.addConnection ({ AudioGraphEndpoint::nodeOutput (AudioGraphNodeID (999), 0),
                                                AudioGraphEndpoint::graphOutput (0) })
                     .wasOk());

    MemoryBlock invalidState;
    ASSERT_TRUE (invalidSource.saveStateIntoMemory (invalidState).wasOk());

    AudioGraphProcessor destination;
    ASSERT_TRUE (resetGraphToSingleGainPath (destination, 0.5f));
    EXPECT_FALSE (destination.hasUncommittedChanges());

    EXPECT_TRUE (destination.loadStateFromMemory (invalidState).failed());
    EXPECT_FALSE (destination.hasUncommittedChanges());

    const auto connections = destination.getConnections();
    ASSERT_EQ (2u, connections.size());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    destination.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (1)[0]);
}

TEST (AudioGraphProcessorTests, SaveStateWritesCompleteXmlTopology)
{
    AudioGraphProcessor graph;
    const auto gainNode = graph.addNode (std::make_unique<StatefulGainProcessor> (0.5f),
                                         statefulGainProperties (11.0f, 22.0f));
    const auto externalNode = graph.addNode (std::make_unique<StatefulExternalProcessor> ("External", 0.25f),
                                             externalProcessorProperties (33.0f, 44.0f));

    ASSERT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (gainNode, 0) }).wasOk());
    ASSERT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (gainNode, 0), AudioGraphEndpoint::nodeInput (externalNode, 0) }).wasOk());
    ASSERT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (externalNode, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    ASSERT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    ASSERT_TRUE (graph.commitChanges().wasOk());

    MemoryBlock savedState;
    ASSERT_TRUE (graph.saveStateIntoMemory (savedState).wasOk());

    const auto xml = parseMemoryBlockAsXml (savedState);
    ASSERT_NE (nullptr, xml.get());
    EXPECT_TRUE (xml->hasTagName ("YUPAudioGraphState"));
    EXPECT_EQ (1, xml->getIntAttribute ("version"));
    EXPECT_EQ (String (static_cast<int64> (externalNode.getRawID())), xml->getStringAttribute ("nextNodeID"));

    auto* nodesElement = xml->getChildByName ("nodes");
    ASSERT_NE (nullptr, nodesElement);
    EXPECT_EQ (2, nodesElement->getNumChildElements());

    auto* gainElement = nodesElement->getChildByAttribute ("id", String (static_cast<int64> (gainNode.getRawID())));
    ASSERT_NE (nullptr, gainElement);
    EXPECT_EQ (String ("statefulGain"), gainElement->getStringAttribute ("identifier"));
    EXPECT_EQ (String ("Stateful Gain"), gainElement->getStringAttribute ("name"));
    EXPECT_DOUBLE_EQ (11.0, gainElement->getDoubleAttribute ("positionX"));
    EXPECT_DOUBLE_EQ (22.0, gainElement->getDoubleAttribute ("positionY"));
    ASSERT_NE (nullptr, gainElement->getChildByName ("state"));
    ASSERT_NE (nullptr, gainElement->getChildByName ("creationData"));

    auto* externalElement = nodesElement->getChildByAttribute ("id", String (static_cast<int64> (externalNode.getRawID())));
    ASSERT_NE (nullptr, externalElement);
    EXPECT_EQ (String ("externalPlugin"), externalElement->getStringAttribute ("identifier"));
    EXPECT_EQ (String ("External Plugin"), externalElement->getStringAttribute ("name"));
    EXPECT_DOUBLE_EQ (33.0, externalElement->getDoubleAttribute ("positionX"));
    EXPECT_DOUBLE_EQ (44.0, externalElement->getDoubleAttribute ("positionY"));
    EXPECT_FALSE (externalElement->getChildByName ("creationData")->getAllSubText().trim().isEmpty());

    auto* connectionsElement = xml->getChildByName ("connections");
    ASSERT_NE (nullptr, connectionsElement);
    EXPECT_EQ (4, connectionsElement->getNumChildElements());

    auto* firstConnection = connectionsElement->getChildByName ("connection");
    ASSERT_NE (nullptr, firstConnection);
    ASSERT_NE (nullptr, firstConnection->getChildByName ("source"));
    ASSERT_NE (nullptr, firstConnection->getChildByName ("destination"));
    EXPECT_EQ (String ("graphInput"), firstConnection->getChildByName ("source")->getStringAttribute ("kind"));
    EXPECT_EQ (String ("nodeInput"), firstConnection->getChildByName ("destination")->getStringAttribute ("kind"));
}

TEST (AudioGraphProcessorTests, LoadStateRestoresMultiNodeXmlGraphAndNextNodeID)
{
    AudioGraphProcessor source;
    const auto firstNode = source.addNode (std::make_unique<StatefulGainProcessor> (0.5f),
                                           statefulGainProperties (10.0f, 20.0f));
    const auto secondNode = source.addNode (std::make_unique<StatefulGainProcessor> (0.25f),
                                            statefulGainProperties (30.0f, 40.0f));

    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (firstNode, 0) }).wasOk());
    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::nodeOutput (firstNode, 0), AudioGraphEndpoint::nodeInput (secondNode, 0) }).wasOk());
    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::nodeOutput (secondNode, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    ASSERT_TRUE (source.commitChanges().wasOk());

    MemoryBlock savedState;
    ASSERT_TRUE (source.saveStateIntoMemory (savedState).wasOk());

    AudioGraphProcessor destination;
    destination.setNodeFactory (statefulGainFactory());

    ASSERT_TRUE (destination.loadStateFromMemory (savedState).wasOk());
    EXPECT_FALSE (destination.hasUncommittedChanges());

    const auto firstProperties = destination.getNodeProperties (firstNode);
    ASSERT_TRUE (firstProperties.has_value());
    EXPECT_FLOAT_EQ (10.0f, firstProperties->positionX);
    EXPECT_FLOAT_EQ (20.0f, firstProperties->positionY);

    const auto secondProperties = destination.getNodeProperties (secondNode);
    ASSERT_TRUE (secondProperties.has_value());
    EXPECT_FLOAT_EQ (30.0f, secondProperties->positionX);
    EXPECT_FLOAT_EQ (40.0f, secondProperties->positionY);

    const auto connections = destination.getConnections();
    ASSERT_EQ (3u, connections.size());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    destination.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.125f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (0.125f, audio.getReadPointer (1)[0]);

    const auto nextNode = destination.addNode (std::make_unique<TestProcessor>());
    EXPECT_GT (nextNode.getRawID(), secondNode.getRawID());
}

TEST (AudioGraphProcessorTests, SaveStateFailsWhenNodeStateSaveFails)
{
    AudioGraphProcessor graph;
    const auto node = graph.addNode (std::make_unique<SaveFailingProcessor>());

    ASSERT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    ASSERT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());

    MemoryBlock savedState;
    EXPECT_TRUE (graph.saveStateIntoMemory (savedState).failed());
}

TEST (AudioGraphProcessorTests, CreateXmlReturnsNullWhenNodeStateSaveFails)
{
    AudioGraphProcessor graph;
    const auto node = graph.addNode (std::make_unique<SaveFailingProcessor>());

    ASSERT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    ASSERT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());

    auto xml = graph.createXml();
    EXPECT_EQ (nullptr, xml.get());
}

TEST (AudioGraphProcessorTests, LoadStateRejectsMalformedXmlHeaderAndUnsupportedVersion)
{
    AudioGraphProcessor graph;
    const String unsupportedVersionXml = "<YUPAudioGraphState version=\"99\" nextNodeID=\"0\">"
                                         "<nodes />"
                                         "<connections />"
                                         "</YUPAudioGraphState>";

    EXPECT_TRUE (graph.loadStateFromMemory (memoryBlockFromString ("not xml")).failed());
    EXPECT_TRUE (graph.loadStateFromMemory (memoryBlockFromString ("<WrongRoot />")).failed());
    EXPECT_TRUE (graph.loadStateFromMemory (memoryBlockFromString (unsupportedVersionXml)).failed());
}

TEST (AudioGraphProcessorTests, LoadStateRejectsMissingRequiredXmlSections)
{
    AudioGraphProcessor graph;
    const String missingNodesXml = "<YUPAudioGraphState version=\"1\" nextNodeID=\"0\">"
                                   "<connections />"
                                   "</YUPAudioGraphState>";
    const String missingConnectionsXml = "<YUPAudioGraphState version=\"1\" nextNodeID=\"0\">"
                                         "<nodes />"
                                         "</YUPAudioGraphState>";

    EXPECT_TRUE (graph.loadStateFromMemory (memoryBlockFromString (missingNodesXml)).failed());
    EXPECT_TRUE (graph.loadStateFromMemory (memoryBlockFromString (missingConnectionsXml)).failed());
}

TEST (AudioGraphProcessorTests, LoadStateRejectsInvalidBase64Payloads)
{
    AudioGraphProcessor source;
    const auto node = source.addNode (std::make_unique<StatefulExternalProcessor> ("External", 0.25f),
                                      externalProcessorProperties());

    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    ASSERT_TRUE (source.commitChanges().wasOk());

    MemoryBlock savedState;
    ASSERT_TRUE (source.saveStateIntoMemory (savedState).wasOk());

    auto invalidNodeState = parseMemoryBlockAsXml (savedState);
    ASSERT_NE (nullptr, invalidNodeState.get());
    auto* stateElement = invalidNodeState->getChildByName ("nodes")->getChildByName ("node")->getChildByName ("state");
    ASSERT_NE (nullptr, stateElement);
    stateElement->deleteAllChildElements();
    stateElement->addTextElement ("not-base64");

    int externalLoadCount = 0;
    String externalIdentifier;

    AudioGraphProcessor destination;
    destination.setNodeFactory (statefulGainAndExternalFactory (externalLoadCount, externalIdentifier));
    EXPECT_TRUE (destination.loadStateFromMemory (memoryBlockFromXml (*invalidNodeState)).failed());
    EXPECT_EQ (0, externalLoadCount);

    auto invalidCreationData = parseMemoryBlockAsXml (savedState);
    ASSERT_NE (nullptr, invalidCreationData.get());
    auto* creationDataElement = invalidCreationData->getChildByName ("nodes")->getChildByName ("node")->getChildByName ("creationData");
    ASSERT_NE (nullptr, creationDataElement);
    creationDataElement->deleteAllChildElements();
    creationDataElement->addTextElement ("not-base64");

    EXPECT_TRUE (destination.loadStateFromMemory (memoryBlockFromXml (*invalidCreationData)).failed());
    EXPECT_EQ (0, externalLoadCount);
}

TEST (AudioGraphProcessorTests, LoadStateRejectsInvalidConnectionXml)
{
    AudioGraphProcessor source;
    const auto node = source.addNode (std::make_unique<StatefulGainProcessor> (0.5f),
                                      statefulGainProperties());

    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    ASSERT_TRUE (source.commitChanges().wasOk());

    MemoryBlock savedState;
    ASSERT_TRUE (source.saveStateIntoMemory (savedState).wasOk());

    auto invalidKind = parseMemoryBlockAsXml (savedState);
    ASSERT_NE (nullptr, invalidKind.get());
    auto* firstConnection = invalidKind->getChildByName ("connections")->getChildByName ("connection");
    ASSERT_NE (nullptr, firstConnection);
    firstConnection->getChildByName ("source")->setAttribute ("kind", "invalidKind");

    AudioGraphProcessor destination;
    destination.setNodeFactory (statefulGainFactory());
    EXPECT_TRUE (destination.loadStateFromMemory (memoryBlockFromXml (*invalidKind)).failed());

    auto missingDestination = parseMemoryBlockAsXml (savedState);
    ASSERT_NE (nullptr, missingDestination.get());
    firstConnection = missingDestination->getChildByName ("connections")->getChildByName ("connection");
    ASSERT_NE (nullptr, firstConnection);
    firstConnection->removeChildElement (firstConnection->getChildByName ("destination"), true);

    EXPECT_TRUE (destination.loadStateFromMemory (memoryBlockFromXml (*missingDestination)).failed());
}

TEST (AudioGraphProcessorTests, LoadStateFailsWhenFactoryReturnsNullProcessor)
{
    AudioGraphProcessor source;
    const auto node = source.addNode (std::make_unique<StatefulGainProcessor> (0.25f),
                                      statefulGainProperties());

    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    ASSERT_TRUE (source.commitChanges().wasOk());

    MemoryBlock savedState;
    ASSERT_TRUE (source.saveStateIntoMemory (savedState).wasOk());

    AudioGraphProcessor destination;
    destination.setNodeFactory ([] (const AudioGraphNodeProperties&) -> ResultValue<std::unique_ptr<AudioProcessor>>
    {
        return makeResultValueOk (std::unique_ptr<AudioProcessor>());
    });

    EXPECT_TRUE (destination.loadStateFromMemory (savedState).failed());
}

TEST (AudioGraphProcessorTests, LoadStateFailureFromNodeStateCallbackRestoresPreviousGraph)
{
    AudioGraphProcessor source;
    const auto node = source.addNode (std::make_unique<StatefulGainProcessor> (0.25f),
                                      statefulGainProperties());

    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    ASSERT_TRUE (source.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    ASSERT_TRUE (source.commitChanges().wasOk());

    MemoryBlock savedState;
    ASSERT_TRUE (source.saveStateIntoMemory (savedState).wasOk());

    auto xml = parseMemoryBlockAsXml (savedState);
    ASSERT_NE (nullptr, xml.get());
    auto* stateElement = xml->getChildByName ("nodes")->getChildByName ("node")->getChildByName ("state");
    ASSERT_NE (nullptr, stateElement);
    stateElement->deleteAllChildElements();

    const MemoryBlock invalidState ("bad", 3);
    stateElement->addTextElement (toBase64Text (invalidState));

    AudioGraphProcessor destination;
    destination.setNodeFactory (statefulGainFactory());
    ASSERT_TRUE (resetGraphToSingleGainPath (destination, 0.5f));
    EXPECT_FALSE (destination.hasUncommittedChanges());

    EXPECT_TRUE (destination.loadStateFromMemory (memoryBlockFromXml (*xml)).failed());
    EXPECT_FALSE (destination.hasUncommittedChanges());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    destination.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (1)[0]);
}

TEST (AudioGraphProcessorTests, RemoveConnectionStopsRoutingAfterCommit)
{
    AudioGraphProcessor graph;
    const auto node = graph.addNode (std::make_unique<TestProcessor>());
    const AudioGraphConnection inputConnection { AudioGraphEndpoint::graphInput (0),
                                                 AudioGraphEndpoint::nodeInput (node, 0) };
    const AudioGraphConnection outputConnection { AudioGraphEndpoint::nodeOutput (node, 0),
                                                  AudioGraphEndpoint::graphOutput (0) };

    EXPECT_TRUE (graph.addConnection (inputConnection).wasOk());
    EXPECT_TRUE (graph.addConnection (outputConnection).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());
    EXPECT_TRUE (graph.removeConnection (outputConnection));
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[0]);
    EXPECT_FALSE (graph.removeConnection (outputConnection));
}

TEST (AudioGraphProcessorTests, RemoveNodePrunesConnections)
{
    AudioGraphProcessor graph;
    const auto node = graph.addNode (std::make_unique<TestProcessor>());

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.removeNode (node));
    EXPECT_EQ (nullptr, graph.getNodeProcessor (node));
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[0]);
    EXPECT_FALSE (graph.removeNode (node));
}

TEST (AudioGraphProcessorTests, ClearRemovesAllRouting)
{
    AudioGraphProcessor graph;
    const auto node = graph.addNode (std::make_unique<TestProcessor>());

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    graph.clear();
    EXPECT_TRUE (graph.hasUncommittedChanges());
    EXPECT_TRUE (graph.commitChanges().wasOk());
    EXPECT_FALSE (graph.hasUncommittedChanges());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[0]);
}

TEST (AudioGraphProcessorTests, ReleaseResourcesMarksPreparedNodesUnprepared)
{
    AudioGraphProcessor graph;
    auto processor = std::make_unique<TestProcessor>();
    auto* processorPtr = processor.get();
    const auto node = graph.addNode (std::move (processor));

    EXPECT_TRUE (node.isValid());
    graph.prepareToPlay (44100.0f, 64);
    EXPECT_TRUE (processorPtr->prepared);

    graph.releaseResources();

    EXPECT_FALSE (processorPtr->prepared);
}

TEST (AudioGraphProcessorTests, ReportsAllocationStats)
{
    AudioGraphProcessor graph;
    const auto first = graph.addNode (std::make_unique<TestProcessor>());
    const auto second = graph.addNode (std::make_unique<TestProcessor>());

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (first, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (second, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (first, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (second, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    const auto stats = graph.getAllocationStats();

    EXPECT_EQ (2, stats.scratchAudioBuffers);
    EXPECT_EQ (2, stats.midiBuffers);
    EXPECT_GE (stats.maxPreallocatedChannels, 2);
    EXPECT_GE (stats.maxPreallocatedBlockSize, 1);
}

TEST (AudioGraphProcessorTests, WorkerThreadOutputMatchesSingleThreadOutput)
{
    AudioGraphProcessor singleThreaded;
    const auto singleLeft = singleThreaded.addNode (std::make_unique<TestProcessor> (0.25f));
    const auto singleRight = singleThreaded.addNode (std::make_unique<TestProcessor> (0.75f));

    EXPECT_TRUE (singleThreaded.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (singleLeft, 0) }).wasOk());
    EXPECT_TRUE (singleThreaded.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (singleRight, 0) }).wasOk());
    EXPECT_TRUE (singleThreaded.addConnection ({ AudioGraphEndpoint::nodeOutput (singleLeft, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (singleThreaded.addConnection ({ AudioGraphEndpoint::nodeOutput (singleRight, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (singleThreaded.commitChanges().wasOk());

    AudioGraphProcessor threaded;
    threaded.setNumWorkerThreads (2);
    const auto threadedLeft = threaded.addNode (std::make_unique<TestProcessor> (0.25f));
    const auto threadedRight = threaded.addNode (std::make_unique<TestProcessor> (0.75f));

    EXPECT_TRUE (threaded.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (threadedLeft, 0) }).wasOk());
    EXPECT_TRUE (threaded.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (threadedRight, 0) }).wasOk());
    EXPECT_TRUE (threaded.addConnection ({ AudioGraphEndpoint::nodeOutput (threadedLeft, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (threaded.addConnection ({ AudioGraphEndpoint::nodeOutput (threadedRight, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (threaded.commitChanges().wasOk());

    AudioBuffer<float> singleAudio (2, 16);
    AudioBuffer<float> threadedAudio (2, 16);
    MidiBuffer singleMidi;
    MidiBuffer threadedMidi;
    fillImpulse (singleAudio);
    fillImpulse (threadedAudio);

    singleThreaded.processBlock (singleAudio, singleMidi);
    threaded.processBlock (threadedAudio, threadedMidi);

    for (int channel = 0; channel < 2; ++channel)
    {
        for (int sample = 0; sample < 16; ++sample)
        {
            EXPECT_FLOAT_EQ (singleAudio.getReadPointer (channel)[sample],
                             threadedAudio.getReadPointer (channel)[sample]);
        }
    }
}

TEST (AudioGraphProcessorTests, WorkerThreadsMatchSingleThreadOutputUnderLoad)
{
    constexpr int numBranches = 16;
    constexpr int numSamples = 64;
    constexpr int numBlocks = 128;

    AudioGraphProcessor singleThreaded;
    AudioGraphProcessor threaded;
    threaded.setNumWorkerThreads (4);

    for (int branch = 0; branch < numBranches; ++branch)
    {
        const float gain = 1.0f / static_cast<float> (numBranches);
        const auto singleNode = singleThreaded.addNode (std::make_unique<TestProcessor> (gain));
        const auto threadedNode = threaded.addNode (std::make_unique<TestProcessor> (gain));

        EXPECT_TRUE (singleNode.isValid());
        EXPECT_TRUE (threadedNode.isValid());
        EXPECT_TRUE (singleThreaded.addConnection ({ AudioGraphEndpoint::graphInput (0),
                                                     AudioGraphEndpoint::nodeInput (singleNode, 0) })
                         .wasOk());
        EXPECT_TRUE (singleThreaded.addConnection ({ AudioGraphEndpoint::nodeOutput (singleNode, 0),
                                                     AudioGraphEndpoint::graphOutput (0) })
                         .wasOk());
        EXPECT_TRUE (threaded.addConnection ({ AudioGraphEndpoint::graphInput (0),
                                               AudioGraphEndpoint::nodeInput (threadedNode, 0) })
                         .wasOk());
        EXPECT_TRUE (threaded.addConnection ({ AudioGraphEndpoint::nodeOutput (threadedNode, 0),
                                               AudioGraphEndpoint::graphOutput (0) })
                         .wasOk());
    }

    EXPECT_TRUE (singleThreaded.commitChanges().wasOk());
    EXPECT_TRUE (threaded.commitChanges().wasOk());

    AudioBuffer<float> singleAudio (2, numSamples);
    AudioBuffer<float> threadedAudio (2, numSamples);
    MidiBuffer singleMidi;
    MidiBuffer threadedMidi;

    for (int block = 0; block < numBlocks; ++block)
    {
        singleAudio.clear();
        threadedAudio.clear();
        singleMidi.clear();
        threadedMidi.clear();

        for (int channel = 0; channel < 2; ++channel)
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                const float input = static_cast<float> ((block + 1) * (sample + 1)) * 0.0001f;
                singleAudio.getWritePointer (channel)[sample] = input;
                threadedAudio.getWritePointer (channel)[sample] = input;
            }
        }

        singleThreaded.processBlock (singleAudio, singleMidi);
        threaded.processBlock (threadedAudio, threadedMidi);

        for (int channel = 0; channel < 2; ++channel)
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                EXPECT_FLOAT_EQ (singleAudio.getReadPointer (channel)[sample],
                                 threadedAudio.getReadPointer (channel)[sample]);
            }
        }
    }
}

TEST (AudioGraphProcessorTests, SwitchingWorkerThreadsBetweenBlocksPreservesOutput)
{
    constexpr int numBranches = 8;
    constexpr int numSamples = 48;
    constexpr int numBlocks = 64;
    constexpr int threadCounts[] = { 0, 1, 4, 2, 0, 3 };
    constexpr int numThreadCounts = sizeof (threadCounts) / sizeof (threadCounts[0]);

    AudioGraphProcessor singleThreaded;
    AudioGraphProcessor threaded;

    for (int branch = 0; branch < numBranches; ++branch)
    {
        const float gain = 1.0f / static_cast<float> (numBranches);
        const auto singleNode = singleThreaded.addNode (std::make_unique<TestProcessor> (gain));
        const auto threadedNode = threaded.addNode (std::make_unique<TestProcessor> (gain));

        EXPECT_TRUE (singleNode.isValid());
        EXPECT_TRUE (threadedNode.isValid());
        EXPECT_TRUE (singleThreaded.addConnection ({ AudioGraphEndpoint::graphInput (0),
                                                     AudioGraphEndpoint::nodeInput (singleNode, 0) })
                         .wasOk());
        EXPECT_TRUE (singleThreaded.addConnection ({ AudioGraphEndpoint::nodeOutput (singleNode, 0),
                                                     AudioGraphEndpoint::graphOutput (0) })
                         .wasOk());
        EXPECT_TRUE (threaded.addConnection ({ AudioGraphEndpoint::graphInput (0),
                                               AudioGraphEndpoint::nodeInput (threadedNode, 0) })
                         .wasOk());
        EXPECT_TRUE (threaded.addConnection ({ AudioGraphEndpoint::nodeOutput (threadedNode, 0),
                                               AudioGraphEndpoint::graphOutput (0) })
                         .wasOk());
    }

    EXPECT_TRUE (singleThreaded.commitChanges().wasOk());
    EXPECT_TRUE (threaded.commitChanges().wasOk());

    AudioBuffer<float> singleAudio (2, numSamples);
    AudioBuffer<float> threadedAudio (2, numSamples);
    MidiBuffer singleMidi;
    MidiBuffer threadedMidi;

    for (int block = 0; block < numBlocks; ++block)
    {
        threaded.setNumWorkerThreads (threadCounts[static_cast<size_t> (block) % numThreadCounts]);

        singleAudio.clear();
        threadedAudio.clear();
        singleMidi.clear();
        threadedMidi.clear();

        for (int channel = 0; channel < 2; ++channel)
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                const float input = static_cast<float> ((block + channel + 1) * (sample + 3)) * 0.0002f;
                singleAudio.getWritePointer (channel)[sample] = input;
                threadedAudio.getWritePointer (channel)[sample] = input;
            }
        }

        singleThreaded.processBlock (singleAudio, singleMidi);
        threaded.processBlock (threadedAudio, threadedMidi);

        for (int channel = 0; channel < 2; ++channel)
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                EXPECT_FLOAT_EQ (singleAudio.getReadPointer (channel)[sample],
                                 threadedAudio.getReadPointer (channel)[sample]);
            }
        }
    }
}

TEST (AudioGraphProcessorTests, ConcurrentCommitsDoNotInvalidateAudioThreadPlan)
{
    AudioGraphProcessor graph;
    graph.setNumWorkerThreads (2);
    graph.prepareToPlay (48000.0f, 32);

    ASSERT_TRUE (resetGraphToSingleGainPath (graph, 0.25f));

    std::atomic<bool> keepProcessing { true };
    std::atomic<bool> startProcessing { false };
    std::atomic<int> invalidBlocks { 0 };
    std::atomic<int> commitFailures { 0 };
    std::atomic<int> processedBlocks { 0 };

    std::thread audioThread ([&]
    {
        AudioBuffer<float> audio (2, 32);
        MidiBuffer midi;

        while (! startProcessing.load())
            std::this_thread::yield();

        while (keepProcessing.load())
        {
            audio.clear();
            midi.clear();

            for (int channel = 0; channel < audio.getNumChannels(); ++channel)
                for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                    audio.getWritePointer (channel)[sample] = 1.0f;

            graph.processBlock (audio, midi);

            for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            {
                for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                {
                    if (! isExpectedGain (audio.getReadPointer (channel)[sample]))
                    {
                        ++invalidBlocks;
                        break;
                    }
                }
            }

            ++processedBlocks;
            std::this_thread::yield();
        }
    });

    std::thread controlThread ([&]
    {
        startProcessing.store (true);

        for (int waitCount = 0; waitCount < 10000 && processedBlocks.load() == 0; ++waitCount)
            std::this_thread::yield();

        for (int iteration = 0; iteration < 250; ++iteration)
        {
            const float gain = (iteration % 2) == 0 ? 0.75f : 0.25f;

            if (! resetGraphToSingleGainPath (graph, gain))
                ++commitFailures;

            std::this_thread::yield();
        }

        keepProcessing.store (false);
    });

    controlThread.join();
    audioThread.join();

    EXPECT_EQ (0, commitFailures.load());
    EXPECT_EQ (0, invalidBlocks.load());
    EXPECT_GT (processedBlocks.load(), 0);
}

TEST (AudioGraphProcessorTests, MidiCompensationCanSpillIntoNextBlock)
{
    AudioBusLayout layout = midiLayout();
    AudioGraphProcessor graph (layout);
    const auto latent = graph.addNode (std::make_unique<MidiPassthroughProcessor> (6));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (latent, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (latent, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (0, 8);
    MidiBuffer midi;
    const uint8 noteOn[] = { 0x90, 64, 100 };
    midi.addEvent (noteOn, 3, 5);

    graph.processBlock (audio, midi);

    EXPECT_EQ (1, countMidiEventsAt (midi, 5));
    EXPECT_EQ (1, countMidiEvents (midi));

    midi.clear();
    graph.processBlock (audio, midi);

    EXPECT_EQ (1, countMidiEventsAt (midi, 3));
}

TEST (AudioGraphProcessorTests, PdcAccumulatesSerialPathLatencyAtGraphOutput)
{
    AudioGraphProcessor graph;
    const auto dry = graph.addNode (std::make_unique<TestProcessor>());
    const auto firstDelay = graph.addNode (std::make_unique<DelayingProcessor> (3));
    const auto secondDelay = graph.addNode (std::make_unique<DelayingProcessor> (4));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (dry, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (dry, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (firstDelay, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (firstDelay, 0), AudioGraphEndpoint::nodeInput (secondDelay, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (secondDelay, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (2.0f, audio.getReadPointer (0)[7]);
    EXPECT_EQ (7, graph.getLatencySamples());
    EXPECT_EQ (7, graph.getAllocationStats().totalCompensationSamples);
    EXPECT_EQ (1, graph.getAllocationStats().delayLines);
}

TEST (AudioGraphProcessorTests, PdcCompensatesFanInAtProcessorInput)
{
    AudioGraphProcessor graph;
    const auto dry = graph.addNode (std::make_unique<TestProcessor>());
    const auto delayed = graph.addNode (std::make_unique<DelayingProcessor> (5));
    const auto summer = graph.addNode (std::make_unique<TestProcessor>());

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (dry, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (delayed, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (dry, 0), AudioGraphEndpoint::nodeInput (summer, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (delayed, 0), AudioGraphEndpoint::nodeInput (summer, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (summer, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (2.0f, audio.getReadPointer (0)[5]);
    EXPECT_EQ (5, graph.getLatencySamples());
    EXPECT_EQ (1, graph.getAllocationStats().delayLines);
}

TEST (AudioGraphProcessorTests, PdcDelaysDirectAudioOutputToMatchLatentAudioPath)
{
    AudioGraphProcessor graph;
    const auto delayed = graph.addNode (std::make_unique<DelayingProcessor> (5));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (delayed, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (delayed, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulseAt (audio, 3);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[3]);
    EXPECT_FLOAT_EQ (2.0f, audio.getReadPointer (0)[8]);
    EXPECT_EQ (5, graph.getLatencySamples());
    EXPECT_EQ (1, graph.getAllocationStats().delayLines);
}

TEST (AudioGraphProcessorTests, PdcAudioDelayLongerThanBlockSpillsAcrossBlocks)
{
    AudioGraphProcessor graph;
    const auto dry = graph.addNode (std::make_unique<TestProcessor>());
    const auto delayed = graph.addNode (std::make_unique<DelayingProcessor> (10));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (dry, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (delayed, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (dry, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (delayed, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 4);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);
    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[0]);

    audio.clear();
    graph.processBlock (audio, midi);
    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[0]);

    audio.clear();
    graph.processBlock (audio, midi);
    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[1]);
    EXPECT_FLOAT_EQ (2.0f, audio.getReadPointer (0)[2]);
    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[3]);
}

TEST (AudioGraphProcessorTests, PdcDirectAudioOutputCompensationSpillsAcrossBlocks)
{
    AudioGraphProcessor graph;
    const auto delayed = graph.addNode (std::make_unique<DelayingProcessor> (6));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (delayed, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (delayed, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 4);
    MidiBuffer midi;
    fillImpulseAt (audio, 1);

    graph.processBlock (audio, midi);
    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[1]);

    audio.clear();
    graph.processBlock (audio, midi);
    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[2]);
    EXPECT_FLOAT_EQ (2.0f, audio.getReadPointer (0)[3]);
}

TEST (AudioGraphProcessorTests, PdcFlushClearsPendingAudioCompensation)
{
    AudioGraphProcessor graph;
    const auto latencyReporter = graph.addNode (std::make_unique<TestProcessor> (0.0f, 6));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (latencyReporter, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (latencyReporter, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 4);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);
    graph.flush();

    audio.clear();
    graph.processBlock (audio, midi);
    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[0]);

    audio.clear();
    graph.processBlock (audio, midi);
    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[2]);
}

TEST (AudioGraphProcessorTests, PdcMidiDelayLongerThanOneBlockAlignsWithDelayedProcessor)
{
    AudioGraphProcessor graph (midiLayout());
    const auto delayed = graph.addNode (std::make_unique<MidiDelayingProcessor> (18));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (delayed, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (delayed, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (0, 8);
    MidiBuffer midi;
    const uint8 noteOn[] = { 0x90, 67, 100 };
    midi.addEvent (noteOn, 3, 6);

    graph.processBlock (audio, midi);
    EXPECT_EQ (0, countMidiEvents (midi));

    midi.clear();
    graph.processBlock (audio, midi);
    EXPECT_EQ (0, countMidiEvents (midi));

    midi.clear();
    graph.processBlock (audio, midi);
    EXPECT_EQ (0, countMidiEvents (midi));

    midi.clear();
    graph.processBlock (audio, midi);
    EXPECT_EQ (2, countMidiEventsAt (midi, 0));
    EXPECT_EQ (2, countMidiEvents (midi));
}

TEST (AudioGraphProcessorTests, PdcFlushClearsPendingMidiCompensation)
{
    AudioGraphProcessor graph (midiLayout());
    const auto latencyReporter = graph.addNode (std::make_unique<MidiPassthroughProcessor> (10));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (latencyReporter, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (latencyReporter, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (0, 8);
    MidiBuffer midi;
    const uint8 noteOn[] = { 0x90, 69, 100 };
    midi.addEvent (noteOn, 3, 7);

    graph.processBlock (audio, midi);
    EXPECT_EQ (1, countMidiEventsAt (midi, 7));

    graph.flush();

    midi.clear();
    graph.processBlock (audio, midi);
    EXPECT_EQ (0, countMidiEvents (midi));

    midi.clear();
    graph.processBlock (audio, midi);
    EXPECT_EQ (0, countMidiEvents (midi));
}

TEST (AudioGraphProcessorTests, PdcRecompileAfterRemovingLatencyPathClearsCompensation)
{
    AudioGraphProcessor graph;
    const auto delayed = graph.addNode (std::make_unique<TestProcessor> (0.0f, 12));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (delayed, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (delayed, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());
    EXPECT_EQ (12, graph.getLatencySamples());
    EXPECT_EQ (1, graph.getAllocationStats().delayLines);

    EXPECT_TRUE (graph.removeNode (delayed));
    EXPECT_TRUE (graph.commitChanges().wasOk());
    EXPECT_EQ (0, graph.getLatencySamples());
    EXPECT_EQ (0, graph.getAllocationStats().delayLines);

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (1.0f, audio.getReadPointer (0)[0]);
}

TEST (AudioGraphProcessorTests, WorkerThreadsProcessManyBlocksCorrectly)
{
    AudioGraphProcessor graph;
    graph.setNumWorkerThreads (2);

    const auto left = graph.addNode (std::make_unique<TestProcessor> (0.5f));
    const auto right = graph.addNode (std::make_unique<TestProcessor> (0.5f));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (left, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (right, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (left, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (right, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;

    for (int block = 0; block < 1000; ++block)
    {
        fillImpulse (audio);
        graph.processBlock (audio, midi);
        EXPECT_FLOAT_EQ (1.0f, audio.getReadPointer (0)[0]) << "block " << block;
    }
}

TEST (AudioGraphProcessorTests, WorkerThreadCountCanBeChangedAfterProcessing)
{
    AudioGraphProcessor graph;

    const auto node = graph.addNode (std::make_unique<TestProcessor> (0.5f));
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;

    graph.setNumWorkerThreads (2);
    fillImpulse (audio);
    graph.processBlock (audio, midi);
    EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (0)[0]);

    graph.setNumWorkerThreads (0);
    fillImpulse (audio);
    graph.processBlock (audio, midi);
    EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (0)[0]);

    graph.setNumWorkerThreads (4);
    fillImpulse (audio);
    graph.processBlock (audio, midi);
    EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (0)[0]);

    graph.setNumWorkerThreads (0);
}

TEST (AudioGraphProcessorTests, WorkerThreadsContinueProcessingAfterIdleReset)
{
    AudioGraphProcessor graph;
    graph.setNumWorkerThreads (2);

    const auto node = graph.addNode (std::make_unique<TestProcessor> (0.5f));
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;

    fillImpulse (audio);
    graph.processBlock (audio, midi);
    EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (0)[0]);

    for (int i = 0; i < 100; ++i)
        std::this_thread::yield();

    fillImpulse (audio);
    graph.processBlock (audio, midi);
    EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (0)[0]);

    graph.setNumWorkerThreads (0);
}

TEST (AudioGraphProcessorTests, ZeroWorkerThreadsProcessesCorrectly)
{
    AudioGraphProcessor graph;
    graph.setNumWorkerThreads (0);

    const auto node = graph.addNode (std::make_unique<TestProcessor> (0.5f));
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;

    for (int block = 0; block < 10; ++block)
    {
        fillImpulse (audio);
        graph.processBlock (audio, midi);
        EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (0)[0]) << "block " << block;
    }
}

TEST (AudioGraphProcessorTests, WorkerThreadOutputMatchesSingleThreadOutputManyBlocks)
{
    auto runGraph = [] (int numWorkers) -> std::vector<float>
    {
        AudioGraphProcessor graph;
        graph.setNumWorkerThreads (numWorkers);

        const auto left = graph.addNode (std::make_unique<TestProcessor> (0.25f));
        const auto right = graph.addNode (std::make_unique<TestProcessor> (0.25f));

        graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (left, 0) });
        graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (right, 0) });
        graph.addConnection ({ AudioGraphEndpoint::nodeOutput (left, 0), AudioGraphEndpoint::graphOutput (0) });
        graph.addConnection ({ AudioGraphEndpoint::nodeOutput (right, 0), AudioGraphEndpoint::graphOutput (0) });
        graph.commitChanges();

        AudioBuffer<float> audio (2, 16);
        MidiBuffer midi;

        std::vector<float> results;
        for (int block = 0; block < 100; ++block)
        {
            fillImpulse (audio);
            graph.processBlock (audio, midi);
            results.push_back (audio.getReadPointer (0)[0]);
        }

        return results;
    };

    const auto singleResults = runGraph (0);
    const auto multiResults = runGraph (2);

    ASSERT_EQ (singleResults.size(), multiResults.size());
    for (size_t i = 0; i < singleResults.size(); ++i)
        EXPECT_FLOAT_EQ (singleResults[i], multiResults[i]) << "block " << i;
}

TEST (AudioGraphProcessorTests, RapidWorkerThreadResizeDoesNotCrash)
{
    AudioGraphProcessor graph;

    const auto node = graph.addNode (std::make_unique<TestProcessor> (1.0f));
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;

    const int threadCounts[] = { 0, 1, 2, 4, 2, 1, 0, 3, 0 };

    for (int count : threadCounts)
    {
        graph.setNumWorkerThreads (count);
        fillImpulse (audio);
        graph.processBlock (audio, midi);
        EXPECT_FLOAT_EQ (1.0f, audio.getReadPointer (0)[0]);
    }
}

#if ! defined(YUP_WASM)
TEST (AudioGraphProcessorTests, ProcessBlockDisablesDenormals)
{
    AudioGraphProcessor graph;

    auto proc = std::make_unique<DenormalCheckProcessor>();
    auto* procPtr = proc.get();
    const auto node = graph.addNode (std::move (proc));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);

    EXPECT_TRUE (procPtr->denormalsWereDisabled);
}
#endif

TEST (AudioGraphProcessorTests, ManyMidiEventsPassThroughGraphCorrectly)
{
    AudioBusLayout layout = midiLayout();
    AudioGraphProcessor graph (layout);

    const auto node = graph.addNode (std::make_unique<MidiPassthroughProcessor>());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (0, 128);
    MidiBuffer midi;

    const int numEvents = 64;
    for (int i = 0; i < numEvents; ++i)
    {
        const uint8 noteOn[] = { static_cast<uint8> (0x90), static_cast<uint8> (i % 128), 100 };
        midi.addEvent (noteOn, 3, i % 128);
    }

    graph.processBlock (audio, midi);

    EXPECT_EQ (numEvents, countMidiEvents (midi));
}

TEST (AudioGraphProcessorTests, WorkerThreadsPdcCompensatesParallelPaths)
{
    // Mirror of CompensatesShorterParallelPaths with 2 worker threads.
    // Verifies that PDC delay-line insertion still aligns a zero-latency branch
    // with a 4-sample latent branch under parallel scheduling.
    AudioGraphProcessor graph;
    graph.setNumWorkerThreads (2);

    const auto dry = graph.addNode (std::make_unique<TestProcessor> (1.0f, 0));
    const auto latent = graph.addNode (std::make_unique<DelayingProcessor> (4));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (dry, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (latent, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (dry, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (latent, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (2.0f, audio.getReadPointer (0)[4]);
    EXPECT_EQ (4, graph.getLatencySamples());
    EXPECT_EQ (4, graph.getAllocationStats().totalCompensationSamples);
}

TEST (AudioGraphProcessorTests, WorkerThreadsPdcMatchesSingleThreadOutput)
{
    // Build the same graph (dry path + serial 3+4 delay path) in both single-threaded
    // and multi-threaded configurations, then verify sample-exact output over several
    // blocks including the initial PDC fill period.
    auto buildGraph = [] (int numWorkers) -> std::unique_ptr<AudioGraphProcessor>
    {
        auto g = std::make_unique<AudioGraphProcessor>();
        g->setNumWorkerThreads (numWorkers);

        const auto dry = g->addNode (std::make_unique<TestProcessor> (1.0f, 0));
        const auto firstDelay = g->addNode (std::make_unique<DelayingProcessor> (3));
        const auto secondDelay = g->addNode (std::make_unique<DelayingProcessor> (4));

        g->addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (dry, 0) });
        g->addConnection ({ AudioGraphEndpoint::nodeOutput (dry, 0), AudioGraphEndpoint::graphOutput (0) });
        g->addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (firstDelay, 0) });
        g->addConnection ({ AudioGraphEndpoint::nodeOutput (firstDelay, 0), AudioGraphEndpoint::nodeInput (secondDelay, 0) });
        g->addConnection ({ AudioGraphEndpoint::nodeOutput (secondDelay, 0), AudioGraphEndpoint::graphOutput (0) });
        g->commitChanges();

        return g;
    };

    const auto single = buildGraph (0);
    const auto multi = buildGraph (3);

    EXPECT_EQ (single->getLatencySamples(), multi->getLatencySamples());

    AudioBuffer<float> singleAudio (2, 16);
    AudioBuffer<float> multiAudio (2, 16);
    MidiBuffer singleMidi;
    MidiBuffer multiMidi;

    for (int block = 0; block < 8; ++block)
    {
        fillImpulse (singleAudio);
        fillImpulse (multiAudio);

        single->processBlock (singleAudio, singleMidi);
        multi->processBlock (multiAudio, multiMidi);

        for (int channel = 0; channel < 2; ++channel)
        {
            for (int sample = 0; sample < 16; ++sample)
            {
                EXPECT_FLOAT_EQ (singleAudio.getReadPointer (channel)[sample],
                                 multiAudio.getReadPointer (channel)[sample])
                    << "block=" << block << " ch=" << channel << " sample=" << sample;
            }
        }
    }
}

TEST (AudioGraphProcessorTests, MixedAudioMidiProcessorPassesThroughBothSignals)
{
    // A single mixed-bus node (audio bus 0, MIDI bus 1) applies gain to audio
    // and passes MIDI through unchanged. Both signals must arrive at the graph
    // output with the correct values and timestamps.
    AudioGraphProcessor graph (mixedLayout());
    const auto node = graph.addNode (std::make_unique<MixedProcessor> (0.5f));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (node, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (1), AudioGraphEndpoint::nodeInput (node, 1) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (node, 1), AudioGraphEndpoint::graphOutput (1) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);
    const uint8 noteOn[] = { 0x90, 60, 100 };
    midi.addEvent (noteOn, 3, 5);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (0.5f, audio.getReadPointer (1)[0]);
    EXPECT_EQ (1, countMidiEventsAt (midi, 5));
    EXPECT_EQ (1, countMidiEvents (midi));
}

TEST (AudioGraphProcessorTests, MixedAudioMidiSerialChainProcessesBothSignals)
{
    // Two mixed-bus nodes in series: audio gain compounds (0.5 * 0.5 = 0.25),
    // and MIDI events pass through both nodes unchanged.
    AudioGraphProcessor graph (mixedLayout());
    const auto first = graph.addNode (std::make_unique<MixedProcessor> (0.5f));
    const auto second = graph.addNode (std::make_unique<MixedProcessor> (0.5f));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (first, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (1), AudioGraphEndpoint::nodeInput (first, 1) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (first, 0), AudioGraphEndpoint::nodeInput (second, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (first, 1), AudioGraphEndpoint::nodeInput (second, 1) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (second, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (second, 1), AudioGraphEndpoint::graphOutput (1) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulse (audio);
    const uint8 noteOn[] = { 0x90, 60, 100 };
    midi.addEvent (noteOn, 3, 3);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.25f, audio.getReadPointer (0)[0]);
    EXPECT_FLOAT_EQ (0.25f, audio.getReadPointer (1)[0]);
    EXPECT_EQ (1, countMidiEventsAt (midi, 3));
    EXPECT_EQ (1, countMidiEvents (midi));
}

TEST (AudioGraphProcessorTests, MixedAudioMidiProcessorPdcCompensatesDirectPaths)
{
    // A mixed-bus node with a 5-sample ring-buffer audio delay sits in parallel
    // with direct graph input → output connections on both buses.
    //
    // PDC must insert:
    //   - a 5-sample audio delay line on the direct audio path so both audio
    //     paths arrive at graphOutput(0) aligned (sum = 2.0f at sample 7)
    //   - a 5-sample MIDI delay line on the direct MIDI path so the direct event
    //     arrives at graphOutput(1) at position 7
    //
    // The node passes MIDI through without physical delay, so its MIDI output
    // arrives at graphOutput(1) at the original position (2), giving two total
    // events at different timestamps.
    AudioGraphProcessor graph (mixedLayout());
    const auto latentNode = graph.addNode (std::make_unique<MixedDelayingProcessor> (5));

    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (0), AudioGraphEndpoint::nodeInput (latentNode, 0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (latentNode, 0), AudioGraphEndpoint::graphOutput (0) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (1), AudioGraphEndpoint::graphOutput (1) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::graphInput (1), AudioGraphEndpoint::nodeInput (latentNode, 1) }).wasOk());
    EXPECT_TRUE (graph.addConnection ({ AudioGraphEndpoint::nodeOutput (latentNode, 1), AudioGraphEndpoint::graphOutput (1) }).wasOk());
    EXPECT_TRUE (graph.commitChanges().wasOk());

    EXPECT_EQ (5, graph.getLatencySamples());
    EXPECT_EQ (2, graph.getAllocationStats().delayLines);

    AudioBuffer<float> audio (2, 16);
    MidiBuffer midi;
    fillImpulseAt (audio, 2);
    const uint8 noteOn[] = { 0x90, 60, 100 };
    midi.addEvent (noteOn, 3, 2);

    graph.processBlock (audio, midi);

    EXPECT_FLOAT_EQ (0.0f, audio.getReadPointer (0)[2]);
    EXPECT_FLOAT_EQ (2.0f, audio.getReadPointer (0)[7]);
    EXPECT_EQ (1, countMidiEventsAt (midi, 2));
    EXPECT_EQ (1, countMidiEventsAt (midi, 7));
    EXPECT_EQ (2, countMidiEvents (midi));
}

TEST (AudioGraphProcessorTests, GetNodeIDsReturnsAllAddedNodes)
{
    AudioGraphProcessor graph;

    EXPECT_TRUE (graph.getNodeIDs().empty());

    const auto idA = graph.addNode (std::make_unique<TestProcessor>());
    const auto idB = graph.addNode (std::make_unique<TestProcessor>());

    const auto ids = graph.getNodeIDs();
    EXPECT_EQ (2u, ids.size());
    EXPECT_NE (ids.end(), std::find (ids.begin(), ids.end(), idA));
    EXPECT_NE (ids.end(), std::find (ids.begin(), ids.end(), idB));

    EXPECT_TRUE (graph.removeNode (idA));
    const auto idsAfter = graph.getNodeIDs();
    EXPECT_EQ (1u, idsAfter.size());
    EXPECT_EQ (idsAfter.end(), std::find (idsAfter.begin(), idsAfter.end(), idA));
}
