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

//==============================================================================

namespace detail
{

int elementSize (YdspValueType type) noexcept
{
    return (type == YdspValueType::float64Type || type == YdspValueType::int64Type) ? 8 : 4;
}

DspJitElementType toElementType (YdspValueType type) noexcept
{
    switch (type)
    {
        case YdspValueType::float32Type:
            return DspJitElementType::float32;
        case YdspValueType::float64Type:
            return DspJitElementType::float64;
        case YdspValueType::int32Type:
            return DspJitElementType::int32;
        case YdspValueType::int64Type:
            return DspJitElementType::int64;
        case YdspValueType::boolType:
            return DspJitElementType::boolean;
    }

    return DspJitElementType::float32;
}

void writeConstValue (uint8_t* dst, const YdspConstValue& value)
{
    switch (value.type)
    {
        case YdspValueType::float32Type:
        {
            const auto v = static_cast<float> (value.asDouble);
            std::memcpy (dst, &v, sizeof (v));
            break;
        }

        case YdspValueType::float64Type:
        {
            std::memcpy (dst, &value.asDouble, sizeof (value.asDouble));
            break;
        }

        case YdspValueType::int32Type:
        {
            const auto v = static_cast<int32_t> (value.asInt);
            std::memcpy (dst, &v, sizeof (v));
            break;
        }

        case YdspValueType::int64Type:
        {
            std::memcpy (dst, &value.asInt, sizeof (value.asInt));
            break;
        }

        case YdspValueType::boolType:
        {
            const int32_t v = value.asBool ? 1 : 0;
            std::memcpy (dst, &v, sizeof (v));
            break;
        }
    }
}

double annotationValue (const YdspEndpointDecl& endpoint, StringRef key, double fallback)
{
    for (const auto& [k, v] : endpoint.annotations)
        if (k == key)
            return v.getDoubleValue();

    return fallback;
}

String annotationString (const YdspEndpointDecl& endpoint, StringRef key, String fallback)
{
    for (const auto& [k, v] : endpoint.annotations)
        if (k == key)
            return v;

    return fallback;
}

StringArray annotationValues (const YdspEndpointDecl& endpoint, StringRef key)
{
    for (const auto& [k, v] : endpoint.annotations)
    {
        if (k != key)
            continue;

        StringArray values;

        for (const auto& token : StringArray::fromTokens (v, ",", ""))
        {
            const auto trimmed = token.trim();

            if (trimmed.isNotEmpty())
                values.add (trimmed);
        }

        return values;
    }

    return {};
}

double constDefaultAsDouble (const YdspConstValue& value)
{
    return isFloatValueType (value.type) ? value.asDouble : static_cast<double> (value.asInt);
}

//==============================================================================
struct DspJitBufferView
{
    const void* data = nullptr;
    DspJitElementType type = DspJitElementType::float32;
    size_t numElements = 0;
};

template <typename BufferVariant>
DspJitBufferView getStreamBufferView (const BufferVariant& buffer)
{
    return std::visit ([] (const auto& span) -> DspJitBufferView
    {
        using Element = std::remove_cv_t<std::remove_pointer_t<decltype (span.data())>>;

        DspJitBufferView view;
        view.data = span.data();
        view.numElements = span.size();

        if constexpr (std::is_same_v<Element, float>)
            view.type = DspJitElementType::float32;
        else if constexpr (std::is_same_v<Element, double>)
            view.type = DspJitElementType::float64;
        else if constexpr (std::is_same_v<Element, int32_t>)
            view.type = DspJitElementType::int32;
        else
            view.type = DspJitElementType::int64;

        return view;
    },
                       buffer);
}

} // namespace detail

//==============================================================================
struct YdspEventPayload
{
    float pitch = 0.0f;
    float velocity = 0.0f;
    float pressure = 0.0f;
    float slide = 0.0f;
    float bend = 0.0f;
    float value = 0.0f;
    int32_t index = 0;
    int32_t flags = 0;
    int32_t channel = 0;
};

//==============================================================================

struct DspJitGraph::Pimpl
{
    Pimpl();
    ~Pimpl();

    //==========================================================================
    // One source feeding one destination stream slot.

    struct StreamConnection
    {
        int srcNode = -1;               // node index, or -1 for a graph input
        int srcIndex = 0;               // graph input index, or the source node's output stream
        int delaySamples = 0;           // inline delay on this edge
        std::vector<float> delayData;   // the ring (empty when delaySamples == 0)
        std::vector<float> delayOutput; // one block of delayed samples
        int delayWritePos = 0;          // write position in the ring (0 <= delayWritePos < delaySamples)
    };

    //==========================================================================
    // A compiled node (one kernel plus its wiring).

    struct Node
    {
        String instanceName;
        String processorName;

        YdspCompiledKernel kernel;
        YdspCompiledKernel initKernel;

        int voiceCount = 1;
        bool isEventDriven = false;

        bool isMidiOnly = false;

        struct EventInputBinding
        {
            // Position of this node's own declared `input event` endpoint,
            // in declaration order - local to the node, not an index into
            // Pimpl::eventInputNames. It is the same number whether the event
            // arrived through an explicit graph-input connection (via
            // Pimpl::graphInputRouting) or from another node's `emit`.
            int eventInputSlot = -1;
            std::array<YdspCompiledKernel, numProcessorEventShapes> handlers;
        };

        std::vector<EventInputBinding> eventInputs;

        YdspVoiceMode voiceMode = YdspVoiceMode::poly;
        YdspVoiceStealing stealing = YdspVoiceStealing::oldest;
        YdspMonoPriority monoPriority = YdspMonoPriority::last;

        struct VoiceSlot
        {
            uint16_t noteId = 0;       // MPENote::noteID of the owning note (0 = none)
            int currentPitch = -1;     // for the note-off dispatched on a steal
            int eventInputIndex = -1;  // the node's own input-event endpoint slot that triggered the note
            bool held = false;         // the note is sounding (key down or sustained)
            uint64_t triggerOrder = 0; // monotonically increasing trigger stamp
            uint64_t releaseOrder = 0; // monotonically increasing release stamp
        };

        std::vector<VoiceSlot> voiceSlots;
        uint64_t voiceTriggerCounter = 0;
        uint64_t voiceReleaseCounter = 0;

        struct HeldNote
        {
            uint16_t noteId = 0;
            int pitch = 0;
            float velocity = 0.0f;
            float bend = 0.0f;
            int eventInputIndex = -1;
        };

        static constexpr int maxMonoHeldNotes = 16;

        std::array<HeldNote, maxMonoHeldNotes> monoHeldNotes {};
        int numMonoHeldNotes = 0;

        struct PendingHandlerCall
        {
            int sampleOffset = 0;
            int eventInputIndex = 0;
            YdspEventShape shape = YdspEventShape::noteOn;
            YdspEventPayload payload;
        };

        std::vector<std::vector<PendingHandlerCall>> voicePendingCalls;

        YdspOutputEventQueue outputEventQueue; // per node, not per voice (Phase 2 concern)

        struct RoutedEventEdge
        {
            int dstNode = -1;            // -1 = graph boundary (deliver to host midiOut)
            int dstEventInputIndex = -1; // valid only when dstNode >= 0 (see below)
            int compensationSamples = 0;
        };

        std::vector<std::vector<RoutedEventEdge>> outputRouting; // outer index = this node's own srcEndpoint (matches Entry::endpointIndex)

        struct CarriedEventEntry
        {
            int sampleOffset = 0;        // already includes compensationSamples; relative to "one block from now"
            int dstNode = -1;            // -1 = graph boundary (deliver to host midiOut)
            int dstEventInputIndex = -1; // valid only when dstNode >= 0 (see RoutedEventEdge above)
            int64_t shapeTag = 0;
            YdspEventContext fields {}; // snapshot of the emitted record's fields, same as YdspOutputEventQueue::Entry::fields
        };

        std::vector<CarriedEventEntry> carryQueue;

        std::vector<int> pendingAllSoundOffOffsets;

        struct PendingAutomation
        {
            int sampleOffset = 0;
            int localParamIndex = 0;  // index into this node's own param block
            int globalParamSlot = -1; // the host-facing slot (for persistence)
            float value = 0.0f;
        };

        std::vector<PendingAutomation> pendingAutomation;

        std::vector<std::pair<int, float>> automationParamSnapshot;
        std::vector<float> voiceScratch;
        std::vector<float*> voiceScratchPtrs;
        std::vector<int> splitPointScratch;
        std::vector<void*> subBlockInputs;
        std::vector<void*> subBlockOutputs;

        int numInputs = 0;
        int numOutputs = 0;

        int paramOffset = 0; // into params
        int numParams = 0;
        int paramOutOffset = 0; // into paramOut
        int numParamOuts = 0;

        int stateOffset = 0;        // byte offset into state
        size_t stateSize = 0;       // bytes
        size_t stateScalarSize = 0; // bytes; stateArrays = state + this

        int activityByteOffset = -1;

        std::vector<int> paramByteOffsets;
        std::vector<int> paramGlobalSlots;

        std::vector<StreamConnection> inputConnections;
        std::vector<int> inputConnectionStart;
        std::vector<void*> inputMixPtrs;

        std::vector<int> outputSlotBuffer;   // -2 internal, -1 graph output
        std::vector<int> outputSlotScratch;  // scratch byte offset for internal outputs
        std::vector<int> outputSlotGraphOut; // graph output index when outputSlotBuffer == -1

        std::vector<int> inputElemSizes;
        std::vector<int> outputElemSizes;
        std::vector<YdspValueType> inputElemTypes;
        std::vector<YdspValueType> outputElemTypes;

        std::vector<void*> runtimeInputs;
        std::vector<void*> runtimeOutputs;

        std::vector<std::pair<int, int>> paramCopies;

        int rateMultiplier = 1;
        int rateDivider = 1;
        std::unique_ptr<yup::Oversampler<float, 2, ydspOversamplerSincRadius>> oversampler2x;
        std::unique_ptr<yup::Oversampler<float, 4, ydspOversamplerSincRadius>> oversampler4x;
        std::unique_ptr<yup::Oversampler<float, 8, ydspOversamplerSincRadius>> oversampler8x;
        std::vector<float> oversampleInputBuf;
        std::vector<float*> oversampleInPtrs;
        std::vector<float*> oversampleOutPtrs;
        std::vector<float> oversampleZeroBuf; // inputless generators only
        std::vector<const float*> oversampleZeroInPtrs;

        std::unique_ptr<yup::Oversampler<float, 2, ydspOversamplerSincRadius>> decimator2x, interpolator2x;
        std::unique_ptr<yup::Oversampler<float, 4, ydspOversamplerSincRadius>> decimator4x, interpolator4x;
        std::unique_ptr<yup::Oversampler<float, 8, ydspOversamplerSincRadius>> decimator8x, interpolator8x;
        std::vector<float> decimInputBuf;
        std::vector<float*> decimInPtrs;
        std::vector<float> decimOutputBuf;
        std::vector<float*> decimOutPtrs;
        std::vector<float> pendingInBuf;  // numInputs * (N - 1)
        std::vector<float> pendingOutBuf; // numOutputs * (N - 1)
        int pendingInCount = 0;
        int pendingOutCount = 0;
    };

    //==========================================================================
    // MIDI and MPE ingestion.

    struct EventIngest : public yup::MPEInstrument::Listener
    {
        EventIngest (Pimpl& owner, int eventInputIndex) noexcept
            : owner (owner)
            , eventInputIndex (eventInputIndex)
        {
        }

        void noteAdded (yup::MPENote note) override;
        void noteReleased (yup::MPENote note) override;
        void notePitchbendChanged (yup::MPENote note) override;
        void notePressureChanged (yup::MPENote note) override;
        void noteTimbreChanged (yup::MPENote note) override;

        Pimpl& owner;
        int eventInputIndex = 0; // the graph event input this instrument feeds
    };

    void ensureEventInputs();

    void setExpressionTrackingMode (yup::MPEInstrument::TrackingMode mode);

    /** The number of simultaneously playing notes tracked without allocating. */
    static constexpr int maxTrackedNotes = 128;

    std::vector<String> eventInputNames;
    std::vector<EventIngest> eventIngests;
    std::vector<std::unique_ptr<yup::MPEInstrument>> mpeInstruments;

    // Outer index = graph input event port (matches eventInputNames). Built
    // from explicit `graphInput -> node.inputEvent;` connections only - a
    // graph input event has no implicit broadcast to same-named node inputs.
    std::vector<std::vector<Node::RoutedEventEdge>> graphInputRouting;
    yup::MPEInstrument::TrackingMode expressionTrackingMode = yup::MPEInstrument::allNotesOnChannel;

    int currentSampleOffset = 0;

    //==========================================================================

    bool valid = false;
    double sampleRate = 44100.0;
    int maxBlockSize = 0;
    int latencySamples = 0;

#if YUP_WASM
    std::vector<std::vector<uint8_t>> wasmModules;
    std::vector<YdspWasmKernelHandle> wasmHandles;
#else
    asmjit::JitRuntime runtime;
#endif

    DspJitExecutionReport report;
    DspJitDiagnostics diagnostics;

    std::vector<Node> nodes;
    std::vector<int> topoOrder;

    std::vector<uint8_t> params;
    std::vector<uint8_t> paramOut;
    std::unordered_map<String, int> paramSlotByName;
    std::unordered_map<String, int> meterSlotByName;

    std::vector<String> meterSlotNames;

    std::vector<DspJitParameterInfo> paramInfos;
    std::vector<int> paramOffsets;
    std::vector<YdspValueType> paramSlotTypes;
    std::vector<YdspValueType> meterSlotTypes;

    std::vector<YdspValueType> inputStreamTypes;
    std::vector<YdspValueType> outputStreamTypes;

    std::vector<std::vector<std::pair<int, int>>> paramSlotToNode;

    std::vector<StreamConnection> graphOutputSources;
    std::vector<int> graphOutputSourceStart;
    std::vector<bool> graphOutputDirect;

    std::vector<uint8_t> state;
    std::vector<uint8_t> scratch;

    std::atomic<uint64_t> droppedEventCount { 0 };

    int paramSlot (StringRef name) const noexcept
    {
        const auto it = paramSlotByName.find (String (name));
        return it == paramSlotByName.end() ? -1 : it->second;
    }

    int meterSlot (StringRef name) const noexcept
    {
        const auto it = meterSlotByName.find (String (name));
        return it == meterSlotByName.end() ? -1 : it->second;
    }

    void runInitKernels();

    //==========================================================================
    // Event ingestion and routing.

    void ingestChannelMessage (const yup::MidiMessage& message, int eventInputIndex);
    void scheduleAllSoundOff (int eventInputIndex);
    void routeEvent (YdspEventShape shape, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex);
    void dispatchEventToNode (Node& node, YdspEventShape shape, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex);

    //==========================================================================
    // Mid-loop drain: routes a node's committed output events to their fan-out
    // destinations (another node, the graph boundary, or next block's carry
    // queue), once per node per block.

    void drainOutputEvents (Node& node, int srcNodeIndex, int blockSize, yup::MidiBuffer* midiOut);
    void deliverResolvedEvent (int dstNode, int dstEventInputIndex, int srcNodeIndex, int64_t shapeTag, const YdspEventContext& fields, int sampleOffset, int blockSize, yup::MidiBuffer* midiOut);

    //==========================================================================
    // Voice allocation (resolve note events into per-voice pending calls).

    static bool handlesShape (const Node& node, int eventInputIndex, YdspEventShape shape) noexcept
    {
        const auto index = eventShapeIndex (shape);

        if (index < 0)
            return false;

        for (const auto& binding : node.eventInputs)
            if (binding.eventInputSlot == eventInputIndex)
                return binding.handlers[static_cast<size_t> (index)].isValid();

        return false;
    }

    static bool subscribesTo (const Node& node, int eventInputIndex) noexcept
    {
        for (const auto& binding : node.eventInputs)
            if (binding.eventInputSlot == eventInputIndex)
                return true;

        return false;
    }

    void pushPendingCall (Node& node, int voice, int sampleOffset, YdspEventShape shape, const YdspEventPayload& payload, int eventInputIndex);
    int findVoiceForNote (const Node& node, uint16_t noteId, int eventInputIndex) const;
    void resolveNoteOn (Node& node, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex);
    void resolveNoteOff (Node& node, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex);
    void resolveMonoNoteOn (Node& node, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex);
    void resolveMonoNoteOff (Node& node, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex);
    int chooseMonoNote (const Node& node) const;
    void soundMonoNote (Node& node, int heldIndex, int sampleOffset, bool isLegato);

    //==========================================================================
    // Stream wiring resolution (per block, before/after the kernel calls).

    static void accumulateStream (void* dst, const void* src, YdspValueType type, int blockSize) noexcept
    {
        if (type == YdspValueType::float64Type)
        {
            auto* out = static_cast<double*> (dst);
            const auto* in = static_cast<const double*> (src);

            for (int i = 0; i < blockSize; ++i)
                out[i] += in[i];

            return;
        }

        jassert (type == YdspValueType::float32Type);

        auto* out = static_cast<float*> (dst);
        const auto* in = static_cast<const float*> (src);

        for (int i = 0; i < blockSize; ++i)
            out[i] += in[i];
    }

    static void clearConnectionDelay (StreamConnection& connection) noexcept
    {
        std::fill (connection.delayData.begin(), connection.delayData.end(), 0.0f);
        std::fill (connection.delayOutput.begin(), connection.delayOutput.end(), 0.0f);
        connection.delayWritePos = 0;
    }

    const void* applyConnectionDelay (StreamConnection& connection, const void* source, int blockSize) noexcept
    {
        if (connection.delaySamples <= 0 || connection.delayData.empty())
            return source;

        const auto ringSize = static_cast<int> (connection.delayData.size());
        const auto* src = static_cast<const float*> (source);
        auto* delayed = connection.delayOutput.data();

        for (int i = 0; i < blockSize; ++i)
        {
            const auto readPos = (connection.delayWritePos - connection.delaySamples + ringSize) % ringSize;
            connection.delayData[static_cast<size_t> (connection.delayWritePos)] = src[i];
            delayed[i] = connection.delayData[static_cast<size_t> (readPos)];
            connection.delayWritePos = (connection.delayWritePos + 1) % ringSize;
        }

        return delayed;
    }

    const void* connectionSourceData (const StreamConnection& connection, Span<const DspJitInputBuffer> inputs) const noexcept
    {
        if (connection.srcNode < 0)
            return detail::getStreamBufferView (inputs[static_cast<size_t> (connection.srcIndex)]).data;

        return nodes[static_cast<size_t> (connection.srcNode)].runtimeOutputs[static_cast<size_t> (connection.srcIndex)];
    }

    void resolveNodeInputs (Node& node, Span<const DspJitInputBuffer> inputs, int blockSize);

    void mixGraphOutputs (Span<const DspJitInputBuffer> inputs, Span<DspJitOutputBuffer> outputs, int blockSize);

    //==========================================================================
    // Sample-accurate sub-block execution.

    void runKernelSubBlock (Node& node, YdspKernelContext& ctx, int offset, int length, bool polyphonic);

    bool voiceIsIdle (const Node& node, int voice) const noexcept
    {
        if (node.activityByteOffset < 0)
            return false;

        if (node.voiceSlots[static_cast<size_t> (voice)].held)
            return false;

        const auto offset = static_cast<size_t> (node.stateOffset)
                          + static_cast<size_t> (voice) * node.stateSize
                          + static_cast<size_t> (node.activityByteOffset);

        if (offset + sizeof (int32_t) > state.size())
            return false;

        int32_t flag = 0;
        std::memcpy (&flag, state.data() + offset, sizeof (flag));

        return flag == 0;
    }

    void clearVoiceSpan (Node& node, int offset, int length, bool polyphonic)
    {
        for (int ch = 0; ch < node.numOutputs; ++ch)
        {
            auto* dst = polyphonic
                          ? node.voiceScratchPtrs[static_cast<size_t> (ch)]
                          : static_cast<float*> (node.runtimeOutputs[static_cast<size_t> (ch)]);

            std::fill (dst + offset, dst + offset + length, 0.0f);
        }
    }

    void silenceVoice (Node& node, YdspKernelContext& ctx);

    void applyAutomation (Node& node, const Node::PendingAutomation& autoEvent, const YdspKernelContext& ctx);
    void snapshotAutomationParams (Node& node, const YdspKernelContext& ctx);
    void restoreAutomationParams (Node& node, const YdspKernelContext& ctx);

    void processNodeWithSplits (Node& node, YdspKernelContext& ctx, int blockSize);
    void invokeEventHandler (Node& node, const Node::PendingHandlerCall& call, const YdspKernelContext& ctx);

    //==========================================================================
    // Oversampled node execution

    template <int Factor>
    void runOversampledKernel (yup::Oversampler<float, Factor, ydspOversamplerSincRadius>& oversampler, Node& node, YdspKernelContext& ctx, int blockSize)
    {
        const auto osBlockSize = blockSize * Factor;

        if (node.numInputs > 0)
        {
            oversampler.upsample (reinterpret_cast<const float* const*> (ctx.inputs), node.numInputs, blockSize);

            for (int ch = 0; ch < node.numInputs; ++ch)
            {
                const auto* src = oversampler.getOversampledChannelData (ch);
                auto* dst = node.oversampleInputBuf.data() + static_cast<size_t> (ch) * static_cast<size_t> (osBlockSize);
                std::copy (src, src + osBlockSize, dst);
                node.oversampleInPtrs[static_cast<size_t> (ch)] = dst;
            }
        }
        else
        {
            oversampler.upsample (node.oversampleZeroInPtrs.data(), node.numOutputs, blockSize);
        }

        for (int ch = 0; ch < node.numOutputs; ++ch)
            node.oversampleOutPtrs[static_cast<size_t> (ch)] = oversampler.getOversampledChannelData (ch);

        auto savedInputs = ctx.inputs;
        auto savedOutputs = ctx.outputs;
        auto savedNumSamples = ctx.numSamples;
        auto savedSampleRate = ctx.sampleRate;

        ctx.inputs = reinterpret_cast<void* const*> (node.oversampleInPtrs.data());
        ctx.outputs = reinterpret_cast<void* const*> (node.oversampleOutPtrs.data());
        ctx.numSamples = osBlockSize;

        ctx.sampleRate = savedSampleRate * static_cast<float> (Factor);

        node.kernel (&ctx);

        ctx.inputs = savedInputs;
        ctx.outputs = savedOutputs;
        ctx.numSamples = savedNumSamples;
        ctx.sampleRate = savedSampleRate;

        oversampler.downsample (reinterpret_cast<float* const*> (node.runtimeOutputs.data()), node.numOutputs, blockSize);
    }

    //==========================================================================
    // Undersampled node execution

    template <int Factor>
    void runUndersampledKernel (yup::Oversampler<float, Factor, ydspOversamplerSincRadius>& decimator,
                                yup::Oversampler<float, Factor, ydspOversamplerSincRadius>& interpolator,
                                Node& node,
                                YdspKernelContext& ctx,
                                int blockSize)
    {
        constexpr int carry = Factor - 1;

        const auto available = node.pendingInCount + blockSize;
        const auto baseSamples = available / Factor;
        const auto consumed = baseSamples * Factor;

        for (int ch = 0; ch < node.numInputs; ++ch)
        {
            auto* highRate = decimator.getOversampledChannelData (ch);
            const auto* source = static_cast<const float*> (ctx.inputs[ch]);
            auto* pending = node.pendingInBuf.data() + static_cast<size_t> (ch) * static_cast<size_t> (carry);

            for (int i = 0; i < node.pendingInCount; ++i)
                highRate[i] = pending[i];

            for (int i = node.pendingInCount; i < consumed; ++i)
                highRate[i] = source[i - node.pendingInCount];

            for (int i = consumed; i < available; ++i)
                pending[i - consumed] = source[i - node.pendingInCount];
        }

        if (baseSamples > 0)
        {
            if (node.numInputs > 0)
            {
                decimator.downsample (node.decimInPtrs.data(), node.numInputs, baseSamples);
            }
            else
            {
                // Inputless generator: nothing to decimate, the kernel just runs at the lower rate.
            }

            auto savedInputs = ctx.inputs;
            auto savedOutputs = ctx.outputs;
            auto savedNumSamples = ctx.numSamples;
            auto savedSampleRate = ctx.sampleRate;

            ctx.inputs = reinterpret_cast<void* const*> (node.decimInPtrs.data());
            ctx.outputs = reinterpret_cast<void* const*> (node.decimOutPtrs.data());
            ctx.numSamples = baseSamples;

            ctx.sampleRate = savedSampleRate / static_cast<float> (Factor);

            node.kernel (&ctx);

            ctx.inputs = savedInputs;
            ctx.outputs = savedOutputs;
            ctx.numSamples = savedNumSamples;
            ctx.sampleRate = savedSampleRate;

            interpolator.upsample (const_cast<const float* const*> (node.decimOutPtrs.data()), node.numOutputs, baseSamples);
        }

        for (int ch = 0; ch < node.numOutputs; ++ch)
        {
            auto* destination = static_cast<float*> (node.runtimeOutputs[static_cast<size_t> (ch)]);
            auto* pending = node.pendingOutBuf.data() + static_cast<size_t> (ch) * static_cast<size_t> (carry);
            const auto* fresh = baseSamples > 0 ? interpolator.getOversampledChannelData (ch) : nullptr;

            const auto fromPending = std::min (node.pendingOutCount, blockSize);

            for (int i = 0; i < fromPending; ++i)
                destination[i] = pending[i];

            for (int i = fromPending; i < blockSize; ++i)
                destination[i] = fresh[i - fromPending];

            int kept = 0;

            for (int i = fromPending; i < node.pendingOutCount; ++i)
                pending[kept++] = pending[i];

            for (int i = blockSize - fromPending; i < consumed; ++i)
                pending[kept++] = fresh[i];

            jassert (kept <= carry);
        }

        node.pendingOutCount += consumed - blockSize;
        node.pendingInCount = available - consumed;

        jassert (node.pendingInCount >= 0 && node.pendingInCount <= carry);
        jassert (node.pendingOutCount >= 0 && node.pendingOutCount <= carry);
        jassert (node.pendingInCount + node.pendingOutCount == carry);
    }
};

} // namespace yup
