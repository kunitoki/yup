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


#pragma once

namespace yup
{

namespace
{

//==============================================================================
/** Wire format shared by plugin host processes and their scanner and bridge worker processes.

    Every process runs the same binary, so plain data is exchanged with its in-memory layout.
*/
struct AudioPluginBridgeProtocol
{
    static constexpr int32 version = 1;

    //==============================================================================

    enum class MessageType : int32
    {
        load = 1,
        prepare,
        release,
        getState,
        setState,
        setPreset,
        valueToText,
        textToValue,
        setNonRealtime,
        setHostInfo,
        openEditor,
        closeEditor,

        reply = 100,

        parameterEvents = 200,
        processorChanged,
        editorClosed
    };

    /** Writes a message header, followed by the payload written by the caller. */
    static void writeHeader (MemoryOutputStream& stream, MessageType type, int32 requestId)
    {
        stream.writeInt (static_cast<int32> (type));
        stream.writeInt (requestId);
    }

    static std::pair<MessageType, int32> readHeader (MemoryInputStream& stream)
    {
        const auto type = static_cast<MessageType> (stream.readInt());
        return { type, stream.readInt() };
    }

    //==============================================================================

    static void writeDescription (OutputStream& stream, const AudioPluginDescription& description)
    {
        stream.writeInt (static_cast<int> (description.formatType));

        for (const auto* text : { &description.name, &description.vendor, &description.version, &description.category, &description.identifier, &description.fileOrBundlePath })
            stream.writeString (*text);

        for (const auto flag : { description.isV3, description.isInstrument, description.isEffect })
            stream.writeBool (flag);

        for (const auto count : { description.numInputChannels, description.numOutputChannels, description.numMidiInputPorts, description.numMidiOutputPorts })
            stream.writeInt (count);
    }

    static AudioPluginDescription readDescription (InputStream& stream)
    {
        AudioPluginDescription description;
        description.formatType = static_cast<AudioPluginFormatType> (stream.readInt());

        for (auto* text : { &description.name, &description.vendor, &description.version, &description.category, &description.identifier, &description.fileOrBundlePath })
            *text = stream.readString();

        for (auto* flag : { &description.isV3, &description.isInstrument, &description.isEffect })
            *flag = stream.readBool();

        for (auto* count : { &description.numInputChannels, &description.numOutputChannels, &description.numMidiInputPorts, &description.numMidiOutputPorts })
            *count = stream.readInt();

        return description;
    }

    //==============================================================================

    static void writeHostContext (OutputStream& stream, const AudioPluginHostContext& context)
    {
        stream.writeFloat (context.sampleRate);
        stream.writeInt (context.maxBlockSize);
        stream.writeBool (context.preferDoublePrecision);
        stream.writeBool (context.isNonRealtime);
        stream.writeString (context.hostName);
        stream.writeString (context.hostVendor);
        stream.writeString (context.hostVersion);
    }

    static AudioPluginHostContext readHostContext (InputStream& stream)
    {
        AudioPluginHostContext context;
        context.sampleRate = stream.readFloat();
        context.maxBlockSize = stream.readInt();
        context.preferDoublePrecision = stream.readBool();
        context.isNonRealtime = stream.readBool();
        context.hostName = stream.readString();
        context.hostVendor = stream.readString();
        context.hostVersion = stream.readString();
        return context;
    }

    //==============================================================================

    static void writeBusLayout (OutputStream& stream, const AudioBusLayout& layout)
    {
        for (const auto buses : { layout.getInputBuses(), layout.getOutputBuses() })
        {
            stream.writeInt (static_cast<int> (buses.size()));

            for (const auto& bus : buses)
            {
                stream.writeString (bus.getName());
                stream.writeInt (static_cast<int> (bus.getType()));
                stream.writeInt (static_cast<int> (bus.getDirection()));
                stream.writeInt (bus.getNumChannels());
                stream.writeInt (static_cast<int> (bus.getRole()));
                stream.writeBool (bus.isDefaultActive());
            }
        }
    }

    static AudioBusLayout readBusLayout (InputStream& stream)
    {
        std::vector<AudioBus> inputs, outputs;

        for (auto* buses : { &inputs, &outputs })
        {
            const auto numBuses = stream.readInt();

            for (int i = 0; i < numBuses && ! stream.isExhausted(); ++i)
            {
                const auto name = stream.readString();
                const auto type = static_cast<AudioBus::Type> (stream.readInt());
                const auto direction = static_cast<AudioBus::Direction> (stream.readInt());
                const auto numChannels = stream.readInt();
                const auto role = static_cast<AudioBus::Role> (stream.readInt());
                buses->emplace_back (name, type, direction, numChannels, role, stream.readBool());
            }
        }

        return AudioBusLayout (std::move (inputs), std::move (outputs));
    }

    //==============================================================================

    static void writeParameter (OutputStream& stream, const AudioParameter& parameter)
    {
        const auto& metadata = parameter.getMetadata();

        stream.writeString (parameter.getID());
        stream.writeString (metadata.name);
        stream.writeInt (static_cast<int> (metadata.hostParameterID));
        stream.writeFloat (metadata.valueRange.start);
        stream.writeFloat (metadata.valueRange.end);
        stream.writeFloat (metadata.valueRange.interval);
        stream.writeFloat (metadata.valueRange.skew);
        stream.writeBool (metadata.valueRange.symmetricSkew);
        stream.writeFloat (metadata.defaultValue);
        stream.writeFloat (metadata.smoothingTimeMs);
        stream.writeString (metadata.modulePath);
        stream.writeInt (static_cast<int> (metadata.unit));
        stream.writeString (metadata.unitName);

        for (const auto flag : { metadata.isAutomatable(), metadata.isReadOnly(), metadata.isStepped(), metadata.isEnum(), metadata.isModulatable(), metadata.isPerNoteModulatable(), metadata.isSmoothingEnabled(), metadata.isBypass() })
            stream.writeBool (flag);

        stream.writeFloat (parameter.getValue());
    }

    /** Reads a parameter description and returns the parameter id, its metadata and its current value. */
    static std::tuple<String, AudioParameter::Metadata, float> readParameter (InputStream& stream)
    {
        const auto id = stream.readString();

        AudioParameter::Metadata metadata;
        metadata.name = stream.readString();
        metadata.hostParameterID = static_cast<uint32> (stream.readInt());
        metadata.valueRange.start = stream.readFloat();
        metadata.valueRange.end = stream.readFloat();
        metadata.valueRange.interval = stream.readFloat();
        metadata.valueRange.skew = stream.readFloat();
        metadata.valueRange.symmetricSkew = stream.readBool();
        metadata.defaultValue = stream.readFloat();
        metadata.smoothingTimeMs = stream.readFloat();
        metadata.modulePath = stream.readString();
        metadata.unit = static_cast<AudioParameter::ParameterUnit> (stream.readInt());
        metadata.unitName = stream.readString();

        metadata.setAutomatable (stream.readBool());
        metadata.setReadOnly (stream.readBool());
        metadata.setStepped (stream.readBool());
        metadata.setEnum (stream.readBool());
        metadata.setModulatable (stream.readBool());
        metadata.setPerNoteModulatable (stream.readBool());
        metadata.setSmoothingEnabled (stream.readBool());
        metadata.setBypass (stream.readBool());

        const auto value = stream.readFloat();
        return { id, std::move (metadata), value };
    }

    //==============================================================================

    static void writePresets (OutputStream& stream, const AudioProcessorBase& processor)
    {
        const auto numPresets = jmax (0, processor.getNumPresets());

        stream.writeInt (processor.getCurrentPreset());
        stream.writeInt (numPresets);

        for (int i = 0; i < numPresets; ++i)
            stream.writeString (processor.getPresetName (i));
    }

    static std::pair<int, StringArray> readPresets (InputStream& stream)
    {
        const auto currentPreset = stream.readInt();
        const auto numPresets = stream.readInt();

        StringArray names;
        for (int i = 0; i < numPresets && ! stream.isExhausted(); ++i)
            names.add (stream.readString());

        return { currentPreset, names };
    }

    //==============================================================================
    /** Header of the shared memory block exchanged for every processed audio block.

        The host writes the inputs, bumps requestSequence and signals the request semaphore.
        The server processes, writes the outputs, copies the sequence to doneSequence and
        signals the done semaphore. A host that gave up on a block recognises its late
        completion from the sequence numbers.
    */
    struct SharedBlockHeader
    {
        enum Flags : int32
        {
            hasPosition = 1 << 0,
            processFailed = 1 << 1
        };

        std::atomic<uint32> requestSequence;
        std::atomic<uint32> doneSequence;
        std::atomic<int32> exitRequested;
        int32 numSamples;
        int32 numParameterChanges;
        int32 numNoteExpressions;
        int32 numMidiInputBytes;
        int32 numMidiOutputBytes;
        int32 flags;
        AudioPlayHead::PositionInfo position;
    };

    static_assert (std::is_trivially_copyable_v<AudioPlayHead::PositionInfo>);
    static_assert (std::is_trivially_copyable_v<ParameterChange>);
    static_assert (std::is_trivially_copyable_v<NoteExpression>);
    static_assert (std::atomic<uint32>::is_always_lock_free);

    static constexpr int maxParameterChanges = 4096;
    static constexpr int maxNoteExpressions = 2048;
    static constexpr int midiCapacityBytes = 65536;

    /** Offsets of each region of the shared memory block, for a given configuration. */
    struct SharedLayout
    {
        SharedLayout (int maxBlockSize, int numInputChannelsIn, int numWorkChannelsIn)
            : blockSize (jmax (1, maxBlockSize))
            , numInputChannels (jmax (0, numInputChannelsIn))
            , numWorkChannels (jmax (1, numWorkChannelsIn))
        {
            parameterChanges = align (sizeof (SharedBlockHeader));
            noteExpressions = align (parameterChanges + sizeof (ParameterChange) * maxParameterChanges);
            midiInput = align (noteExpressions + sizeof (NoteExpression) * maxNoteExpressions);
            midiOutput = align (midiInput + midiCapacityBytes);
            inputs = align (midiOutput + midiCapacityBytes);
            work = align (inputs + sizeof (float) * static_cast<std::size_t> (numInputChannels * blockSize));
            totalSize = align (work + sizeof (float) * static_cast<std::size_t> (numWorkChannels * blockSize));
        }

        int blockSize, numInputChannels, numWorkChannels;
        std::size_t parameterChanges = 0, noteExpressions = 0, midiInput = 0, midiOutput = 0, inputs = 0, work = 0, totalSize = 0;

    private:
        static std::size_t align (std::size_t offset) noexcept { return (offset + 63) & ~static_cast<std::size_t> (63); }
    };

    /** Typed views over a mapped shared memory block. */
    struct SharedBlock
    {
        SharedBlock() = default;

        SharedBlock (void* memory, const SharedLayout& layout)
            : header (static_cast<SharedBlockHeader*> (memory))
            , parameterChanges (reinterpret_cast<ParameterChange*> (static_cast<uint8*> (memory) + layout.parameterChanges))
            , noteExpressions (reinterpret_cast<NoteExpression*> (static_cast<uint8*> (memory) + layout.noteExpressions))
            , midiInput (static_cast<uint8*> (memory) + layout.midiInput)
            , midiOutput (static_cast<uint8*> (memory) + layout.midiOutput)
        {
            auto* inputs = reinterpret_cast<float*> (static_cast<uint8*> (memory) + layout.inputs);
            auto* work = reinterpret_cast<float*> (static_cast<uint8*> (memory) + layout.work);

            for (int channel = 0; channel < layout.numInputChannels; ++channel)
                inputChannels.push_back (inputs + channel * layout.blockSize);

            for (int channel = 0; channel < layout.numWorkChannels; ++channel)
                workChannels.push_back (work + channel * layout.blockSize);
        }

        SharedBlockHeader* header = nullptr;
        ParameterChange* parameterChanges = nullptr;
        NoteExpression* noteExpressions = nullptr;
        uint8* midiInput = nullptr;
        uint8* midiOutput = nullptr;
        std::vector<float*> inputChannels;
        std::vector<float*> workChannels;
    };

    /** Names of the semaphores paired with a shared memory block. */
    static String getRequestSemaphoreName (const String& sharedMemoryName) { return sharedMemoryName + "q"; }

    static String getDoneSemaphoreName (const String& sharedMemoryName) { return sharedMemoryName + "d"; }

    //==============================================================================
    /** Packs MIDI events as [int32 position][uint16 size][bytes], dropping what doesn't fit. */
    static int32 writeMidi (const MidiBuffer& midi, uint8* destination, int capacity) noexcept
    {
        int32 written = 0;

        for (const auto metadata : midi)
        {
            const auto eventSize = static_cast<int32> (sizeof (int32) + sizeof (uint16)) + metadata.numBytes;
            if (metadata.numBytes > 0xffff || written + eventSize > capacity)
                break;

            const auto position = static_cast<int32> (metadata.samplePosition);
            const auto numBytes = static_cast<uint16> (metadata.numBytes);

            std::memcpy (destination + written, &position, sizeof (position));
            std::memcpy (destination + written + sizeof (position), &numBytes, sizeof (numBytes));
            std::memcpy (destination + written + sizeof (position) + sizeof (numBytes), metadata.data, static_cast<std::size_t> (metadata.numBytes));
            written += eventSize;
        }

        return written;
    }

    static void readMidi (const uint8* source, int32 numBytes, MidiBuffer& midi) noexcept
    {
        constexpr auto eventHeaderSize = static_cast<int32> (sizeof (int32) + sizeof (uint16));

        for (int32 offset = 0; offset + eventHeaderSize <= numBytes;)
        {
            int32 position = 0;
            uint16 size = 0;
            std::memcpy (&position, source + offset, sizeof (position));
            std::memcpy (&size, source + offset + sizeof (position), sizeof (size));

            offset += eventHeaderSize;
            if (offset + size > numBytes)
                break;

            midi.addEvent (source + offset, size, position);
            offset += size;
        }
    }
};

} // namespace

} // namespace yup
