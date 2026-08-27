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

// DspJitExecutionReport

const std::vector<DspJitKernelReport>& DspJitExecutionReport::getKernels() const noexcept
{
    return kernels;
}

std::vector<DspJitKernelReport>& DspJitExecutionReport::getKernels() noexcept
{
    return kernels;
}

int DspJitExecutionReport::getTotalBoundedIterations() const noexcept
{
    int total = 0;

    for (const auto& kernel : kernels)
        total += kernel.boundedIterationCount;

    return total;
}

bool DspJitExecutionReport::isProvenRealtimeSafe() const noexcept
{
    for (const auto& kernel : kernels)
        if (! kernel.provenRealtimeSafe)
            return false;

    return true;
}

//==============================================================================

#if YUP_EMSCRIPTEN
extern "C" void EMSCRIPTEN_KEEPALIVE ydspCommitOutputEvent (YdspOutputEventQueue* queue, int64_t shapeTag, int32_t sampleOffset, int32_t endpointIndex)
#else
extern "C" void ydspCommitOutputEvent (YdspOutputEventQueue* queue, int64_t shapeTag, int32_t sampleOffset, int32_t endpointIndex)
#endif
{
    if (queue->entries.size() < queue->entries.capacity())
    {
        queue->entries.push_back ({ sampleOffset, endpointIndex, shapeTag, queue->staging });
        return;
    }

    queue->droppedCount.fetch_add (1, std::memory_order_relaxed);
}

#if YUP_EMSCRIPTEN
extern "C" void EMSCRIPTEN_KEEPALIVE ydspCommitOutputEventWasm (YdspOutputEventQueue* queue, int32_t shapeTag, int32_t sampleOffset, int32_t endpointIndex)
{
    ydspCommitOutputEvent (queue, static_cast<int64_t> (shapeTag), sampleOffset, endpointIndex);
}
#endif

//==============================================================================

// DspJitGraph

DspJitGraph::DspJitGraph() = default;

DspJitGraph::~DspJitGraph() = default;

DspJitGraph::DspJitGraph (DspJitGraph&&) noexcept = default;
DspJitGraph& DspJitGraph::operator= (DspJitGraph&&) noexcept = default;

//==============================================================================

bool DspJitGraph::isValid() const noexcept
{
    return pimpl != nullptr && pimpl->valid;
}

//==============================================================================

void DspJitGraph::prepare (double sampleRate, int maxBlockSize, int maxEventsPerVoicePerBlock, int maxAutomationPerNodePerBlock, int maxOutputEventsPerBlock)
{
    if (pimpl == nullptr)
        return;

    static constexpr int maxAllSoundOffsPerNodePerBlock = 16;

    pimpl->sampleRate = sampleRate;
    pimpl->maxBlockSize = maxBlockSize;
    pimpl->ensureEventInputs();

    const auto prepareConnectionDelay = [maxBlockSize] (Pimpl::StreamConnection& connection)
    {
        connection.delayWritePos = 0;

        if (connection.delaySamples <= 0)
        {
            connection.delayData.clear();
            connection.delayOutput.clear();
            return;
        }

        connection.delayData.assign (static_cast<size_t> (connection.delaySamples + maxBlockSize), 0.0f);
        connection.delayOutput.assign (static_cast<size_t> (maxBlockSize), 0.0f);
    };

    size_t stateBytes = 0;

    for (auto& node : pimpl->nodes)
    {
        node.stateOffset = static_cast<int> (stateBytes);
        stateBytes += node.stateSize * static_cast<size_t> (node.voiceCount);

        jassert (! (node.isEventDriven && (node.rateMultiplier > 1 || node.rateDivider > 1)));

        for (auto& calls : node.voicePendingCalls)
            calls.reserve (static_cast<size_t> (maxEventsPerVoicePerBlock));

        node.pendingAutomation.reserve (static_cast<size_t> (maxAutomationPerNodePerBlock));
        node.automationParamSnapshot.reserve (static_cast<size_t> (maxAutomationPerNodePerBlock));
        node.outputEventQueue.entries.reserve (static_cast<size_t> (maxOutputEventsPerBlock));
        node.carryQueue.reserve (static_cast<size_t> (maxOutputEventsPerBlock));
        node.pendingAllSoundOffOffsets.reserve (static_cast<size_t> (maxAllSoundOffsPerNodePerBlock));
        node.splitPointScratch.reserve (static_cast<size_t> (maxEventsPerVoicePerBlock + maxAutomationPerNodePerBlock + maxAllSoundOffsPerNodePerBlock));
        node.subBlockInputs.resize (static_cast<size_t> (node.numInputs));
        node.subBlockOutputs.resize (static_cast<size_t> (node.numOutputs));

        if (node.voiceCount > 1)
        {
            node.voiceScratch.assign (static_cast<size_t> (node.numOutputs * maxBlockSize), 0.0f);
            node.voiceScratchPtrs.resize (static_cast<size_t> (node.numOutputs));

            for (int ch = 0; ch < node.numOutputs; ++ch)
                node.voiceScratchPtrs[static_cast<size_t> (ch)] = node.voiceScratch.data() + static_cast<size_t> (ch) * static_cast<size_t> (maxBlockSize);
        }

        if (node.rateMultiplier > 1 && node.numOutputs > 0 && (node.numInputs == node.numOutputs || node.numInputs == 0))
        {
            const auto factor = node.rateMultiplier;
            const auto osBlockSize = maxBlockSize * factor;
            const auto channels = jmax (node.numInputs, node.numOutputs);

            node.oversampleInPtrs.resize (static_cast<size_t> (node.numInputs));
            node.oversampleOutPtrs.resize (static_cast<size_t> (node.numOutputs));

            if (node.numInputs > 0)
            {
                node.oversampleInputBuf.assign (static_cast<size_t> (node.numInputs * osBlockSize), 0.0f);
            }
            else
            {
                node.oversampleZeroBuf.assign (static_cast<size_t> (node.numOutputs * osBlockSize), 0.0f);
                node.oversampleZeroInPtrs.resize (static_cast<size_t> (node.numOutputs));

                for (int ch = 0; ch < node.numOutputs; ++ch)
                    node.oversampleZeroInPtrs[static_cast<size_t> (ch)] = node.oversampleZeroBuf.data() + static_cast<size_t> (ch) * static_cast<size_t> (osBlockSize);
            }

            switch (factor)
            {
                case 2:
                    node.oversampler2x = std::make_unique<yup::Oversampler<float, 2, ydspOversamplerSincRadius>>();
                    node.oversampler2x->prepare (sampleRate, channels, maxBlockSize);
                    break;

                case 4:
                    node.oversampler4x = std::make_unique<yup::Oversampler<float, 4, ydspOversamplerSincRadius>>();
                    node.oversampler4x->prepare (sampleRate, channels, maxBlockSize);
                    break;

                case 8:
                    node.oversampler8x = std::make_unique<yup::Oversampler<float, 8, ydspOversamplerSincRadius>>();
                    node.oversampler8x->prepare (sampleRate, channels, maxBlockSize);
                    break;

                default:
                    jassertfalse; // unsupported oversampling factor: the node runs at 1x
                    break;
            }
        }
        else if (node.rateMultiplier > 1)
        {
            jassertfalse; // oversampling requires equal input/output stream counts (or no inputs)
        }

        if (node.rateDivider > 1 && node.numOutputs > 0 && (node.numInputs == node.numOutputs || node.numInputs == 0))
        {
            const auto factor = node.rateDivider;
            const auto channels = jmax (node.numInputs, node.numOutputs);
            const auto baseBlockSize = (maxBlockSize + factor - 1) / factor;

            node.decimInputBuf.assign (static_cast<size_t> (channels * baseBlockSize), 0.0f);
            node.decimOutputBuf.assign (static_cast<size_t> (channels * baseBlockSize), 0.0f);
            node.decimInPtrs.resize (static_cast<size_t> (node.numInputs));
            node.decimOutPtrs.resize (static_cast<size_t> (node.numOutputs));

            for (int ch = 0; ch < node.numInputs; ++ch)
                node.decimInPtrs[static_cast<size_t> (ch)] = node.decimInputBuf.data() + static_cast<size_t> (ch) * static_cast<size_t> (baseBlockSize);

            for (int ch = 0; ch < node.numOutputs; ++ch)
                node.decimOutPtrs[static_cast<size_t> (ch)] = node.decimOutputBuf.data() + static_cast<size_t> (ch) * static_cast<size_t> (baseBlockSize);

            node.pendingInBuf.assign (static_cast<size_t> (node.numInputs * (factor - 1)), 0.0f);
            node.pendingOutBuf.assign (static_cast<size_t> (node.numOutputs * (factor - 1)), 0.0f);
            node.pendingInCount = 0;
            node.pendingOutCount = factor - 1;

            const auto prepareBoth = [&] (auto& decimator, auto& interpolator)
            {
                using ResamplerType = typename std::remove_reference_t<decltype (decimator)>::element_type;

                decimator = std::make_unique<ResamplerType>();
                decimator->prepare (sampleRate / factor, channels, baseBlockSize);

                interpolator = std::make_unique<ResamplerType>();
                interpolator->prepare (sampleRate / factor, channels, baseBlockSize);
            };

            switch (factor)
            {
                case 2:
                    prepareBoth (node.decimator2x, node.interpolator2x);
                    break;

                case 4:
                    prepareBoth (node.decimator4x, node.interpolator4x);
                    break;

                case 8:
                    prepareBoth (node.decimator8x, node.interpolator8x);
                    break;

                default:
                    jassertfalse; // unsupported undersampling factor: the node runs at 1x
                    break;
            }
        }
        else if (node.rateDivider > 1)
        {
            jassertfalse; // undersampling requires equal input/output stream counts (or no inputs)
        }

        for (auto& connection : node.inputConnections)
            prepareConnectionDelay (connection);
    }

    for (auto& source : pimpl->graphOutputSources)
        prepareConnectionDelay (source);

    pimpl->state.assign (stateBytes, 0);

    const auto slotIsMixed = [] (const Pimpl::Node& node, int slot)
    {
        return node.inputConnectionStart[static_cast<size_t> (slot) + 1]
                 - node.inputConnectionStart[static_cast<size_t> (slot)]
             > 1;
    };

    size_t scratchBytes = 0;

    for (const auto& node : pimpl->nodes)
    {
        for (int s = 0; s < node.numOutputs; ++s)
            if (node.outputSlotBuffer[static_cast<size_t> (s)] == -2)
                scratchBytes += static_cast<size_t> (maxBlockSize) * static_cast<size_t> (node.outputElemSizes[static_cast<size_t> (s)]);

        for (int s = 0; s < node.numInputs; ++s)
            if (slotIsMixed (node, s))
                scratchBytes += static_cast<size_t> (maxBlockSize) * static_cast<size_t> (node.inputElemSizes[static_cast<size_t> (s)]);
    }

    pimpl->scratch.assign (scratchBytes, 0);

    size_t scratchCursor = 0;

    for (auto& node : pimpl->nodes)
    {
        node.runtimeInputs.assign (static_cast<size_t> (node.numInputs), nullptr);
        node.runtimeOutputs.assign (static_cast<size_t> (node.numOutputs), nullptr);
        node.inputMixPtrs.assign (static_cast<size_t> (node.numInputs), nullptr);

        for (int s = 0; s < node.numOutputs; ++s)
        {
            if (node.outputSlotBuffer[static_cast<size_t> (s)] != -2)
                continue;

            node.outputSlotScratch[static_cast<size_t> (s)] = static_cast<int> (scratchCursor);
            node.runtimeOutputs[static_cast<size_t> (s)] = pimpl->scratch.data() + scratchCursor;
            scratchCursor += static_cast<size_t> (maxBlockSize) * static_cast<size_t> (node.outputElemSizes[static_cast<size_t> (s)]);
        }

        for (int s = 0; s < node.numInputs; ++s)
        {
            if (! slotIsMixed (node, s))
                continue;

            node.inputMixPtrs[static_cast<size_t> (s)] = pimpl->scratch.data() + scratchCursor;
            scratchCursor += static_cast<size_t> (maxBlockSize) * static_cast<size_t> (node.inputElemSizes[static_cast<size_t> (s)]);
        }
    }

    jassert (scratchCursor == scratchBytes);

    pimpl->runInitKernels();
}

//==============================================================================

void DspJitGraph::reset()
{
    if (pimpl == nullptr || ! pimpl->valid)
        return;

    for (auto& instrument : pimpl->mpeInstruments)
        instrument->releaseAllNotes();

    pimpl->state.assign (pimpl->state.size(), 0);

    for (auto& node : pimpl->nodes)
    {
        node.voiceSlots.assign (node.voiceSlots.size(), {});
        node.numMonoHeldNotes = 0;
        node.voiceTriggerCounter = 0;
        node.voiceReleaseCounter = 0;
        node.pendingAllSoundOffOffsets.clear();
        node.carryQueue.clear();

        for (auto& calls : node.voicePendingCalls)
            calls.clear();

        if (node.oversampler2x != nullptr)
            node.oversampler2x->reset();

        if (node.oversampler4x != nullptr)
            node.oversampler4x->reset();

        if (node.oversampler8x != nullptr)
            node.oversampler8x->reset();

        const auto resetResampler = [] (auto& resampler)
        {
            if (resampler != nullptr)
                resampler->reset();
        };

        resetResampler (node.decimator2x);
        resetResampler (node.interpolator2x);
        resetResampler (node.decimator4x);
        resetResampler (node.interpolator4x);
        resetResampler (node.decimator8x);
        resetResampler (node.interpolator8x);

        if (node.rateDivider > 1)
        {
            std::fill (node.pendingInBuf.begin(), node.pendingInBuf.end(), 0.0f);
            std::fill (node.pendingOutBuf.begin(), node.pendingOutBuf.end(), 0.0f);
            node.pendingInCount = 0;
            node.pendingOutCount = node.rateDivider - 1;
        }

        for (auto& connection : node.inputConnections)
            Pimpl::clearConnectionDelay (connection);
    }

    for (auto& source : pimpl->graphOutputSources)
        Pimpl::clearConnectionDelay (source);

    pimpl->runInitKernels();
}

//==============================================================================

void DspJitGraph::setMpeZoneLayout (const yup::MPEZoneLayout& layout)
{
    if (pimpl == nullptr)
        return;

    pimpl->ensureEventInputs();

    for (auto& instrument : pimpl->mpeInstruments)
        instrument->setZoneLayout (layout);

    pimpl->setExpressionTrackingMode (yup::MPEInstrument::lastNotePlayedOnChannel);
}

void DspJitGraph::setLegacyMidiMode (int pitchbendRangeSemitones)
{
    if (pimpl == nullptr)
        return;

    pimpl->ensureEventInputs();

    for (auto& instrument : pimpl->mpeInstruments)
    {
        instrument->enableLegacyMode (pitchbendRangeSemitones);
        instrument->setLegacyModePitchbendRange (pitchbendRangeSemitones);
    }

    pimpl->setExpressionTrackingMode (yup::MPEInstrument::allNotesOnChannel);
}

//==============================================================================

void DspJitGraph::prewarmKernels()
{
#if YUP_WASM
    if (pimpl == nullptr || pimpl->wasmModules.empty())
        return;

    YdspWasmRuntime::prewarmInCurrentRealm (pimpl->wasmHandles, pimpl->wasmModules);
#endif
}

//==============================================================================

DspJitProcessResult DspJitGraph::process (yup::Span<const DspJitInputBuffer> inputs, yup::Span<DspJitOutputBuffer> outputs, int numSamples)
{
    return process (inputs, outputs, numSamples, nullptr, nullptr, 0);
}

//==============================================================================

DspJitProcessResult DspJitGraph::process (yup::Span<const DspJitInputBuffer> inputs,
                                          yup::Span<DspJitOutputBuffer> outputs,
                                          int numSamples,
                                          const yup::MidiBuffer* midiIn,
                                          const DspJitAutomationEvent* automation,
                                          int numAutomationEvents)
{
    const yup::MidiBuffer* buffers[] = { midiIn };
    const auto numEventInputs = (pimpl != nullptr && pimpl->valid) ? static_cast<int> (pimpl->eventInputNames.size()) : 0;

    return process (inputs, outputs, numSamples, yup::Span<const yup::MidiBuffer*> (buffers, numEventInputs > 0 ? 1 : 0), automation, numAutomationEvents);
}

DspJitProcessResult DspJitGraph::process (yup::Span<const DspJitInputBuffer> inputs,
                                          yup::Span<DspJitOutputBuffer> outputs,
                                          int numSamples,
                                          yup::Span<const yup::MidiBuffer*> eventInputs,
                                          const DspJitAutomationEvent* automation,
                                          int numAutomationEvents)
{
    return process (inputs, outputs, numSamples, eventInputs, automation, numAutomationEvents, nullptr);
}

DspJitProcessResult DspJitGraph::process (yup::Span<const DspJitInputBuffer> inputs,
                                          yup::Span<DspJitOutputBuffer> outputs,
                                          int numSamples,
                                          const yup::MidiBuffer* midiIn,
                                          const DspJitAutomationEvent* automation,
                                          int numAutomationEvents,
                                          yup::MidiBuffer* midiOut)
{
    const yup::MidiBuffer* buffers[] = { midiIn };
    const auto numEventInputs = (pimpl != nullptr && pimpl->valid) ? static_cast<int> (pimpl->eventInputNames.size()) : 0;

    return process (inputs, outputs, numSamples, yup::Span<const yup::MidiBuffer*> (buffers, numEventInputs > 0 ? 1 : 0), automation, numAutomationEvents, midiOut);
}

DspJitProcessResult DspJitGraph::process (yup::Span<const DspJitInputBuffer> inputs,
                                          yup::Span<DspJitOutputBuffer> outputs,
                                          int numSamples,
                                          yup::Span<const yup::MidiBuffer*> eventInputs,
                                          const DspJitAutomationEvent* automation,
                                          int numAutomationEvents,
                                          yup::MidiBuffer* midiOut)
{
    if (pimpl == nullptr || ! pimpl->valid)
        return DspJitProcessResult::invalidGraph;

    const int blockSize = std::min (numSamples, pimpl->maxBlockSize);

    if (blockSize == 0)
        return DspJitProcessResult::ok;

    if (inputs.size() != pimpl->inputStreamTypes.size()
        || outputs.size() != pimpl->outputStreamTypes.size())
    {
        return DspJitProcessResult::invalidBufferCount;
    }

    for (size_t i = 0; i < inputs.size(); ++i)
    {
        const auto view = detail::getStreamBufferView (inputs[i]);

        if (view.type != detail::toElementType (pimpl->inputStreamTypes[i]))
            return DspJitProcessResult::bufferTypeMismatch;

        if (view.numElements < static_cast<size_t> (blockSize))
            return DspJitProcessResult::bufferTooShort;
    }

    for (size_t i = 0; i < outputs.size(); ++i)
    {
        const auto view = detail::getStreamBufferView (outputs[i]);

        if (view.type != detail::toElementType (pimpl->outputStreamTypes[i]))
            return DspJitProcessResult::bufferTypeMismatch;

        if (view.numElements < static_cast<size_t> (blockSize))
            return DspJitProcessResult::bufferTooShort;
    }

    for (auto& node : pimpl->nodes)
    {
        for (auto& calls : node.voicePendingCalls)
            calls.clear();

        node.pendingAutomation.clear();
        node.pendingAllSoundOffOffsets.clear();
        node.outputEventQueue.entries.clear();
    }

    if (automation != nullptr)
    {
        for (int i = 0; i < numAutomationEvents; ++i)
        {
            const auto& event = automation[i];

            if (event.parameterSlot < 0 || static_cast<size_t> (event.parameterSlot) >= pimpl->paramSlotToNode.size())
            {
                pimpl->droppedEventCount.fetch_add (1, std::memory_order_relaxed);
                continue;
            }

            if (static_cast<size_t> (event.parameterSlot) >= pimpl->paramSlotTypes.size()
                || pimpl->paramSlotTypes[static_cast<size_t> (event.parameterSlot)] != YdspValueType::float32Type)
            {
                pimpl->droppedEventCount.fetch_add (1, std::memory_order_relaxed);
                continue;
            }

            const auto sampleOffset = std::clamp (event.sampleOffset, 0, blockSize);

            for (const auto& [nodeIndex, localParamIndex] : pimpl->paramSlotToNode[static_cast<size_t> (event.parameterSlot)])
            {
                if (nodeIndex < 0 || localParamIndex < 0)
                    continue;

                auto& node = pimpl->nodes[static_cast<size_t> (nodeIndex)];

                if (node.pendingAutomation.size() >= node.pendingAutomation.capacity())
                {
                    pimpl->droppedEventCount.fetch_add (1, std::memory_order_relaxed);
                    continue;
                }

                node.pendingAutomation.push_back ({ sampleOffset, localParamIndex, event.parameterSlot, event.value });
            }
        }
    }

    const auto numEventInputs = pimpl->eventInputNames.size();

    if (eventInputs.size() > numEventInputs)
        return DspJitProcessResult::invalidBufferCount;

    for (size_t s = 0; s < eventInputs.size(); ++s)
    {
        const auto* midiIn = eventInputs[s];

        if (midiIn == nullptr)
            continue;

        const auto eventInputIndex = static_cast<int> (s);
        auto& instrument = *pimpl->mpeInstruments[static_cast<size_t> (eventInputIndex)];

        for (const yup::MidiMessageMetadata metadata : *midiIn)
        {
            const auto message = metadata.getMessage();

            pimpl->currentSampleOffset = std::min (metadata.samplePosition, blockSize);

            instrument.processNextMidiEvent (message);
            pimpl->ingestChannelMessage (message, eventInputIndex);
        }
    }

    YdspKernelContext ctx;
    ctx.sampleRate = static_cast<float> (pimpl->sampleRate);
    ctx.numSamples = blockSize;

    for (const int nodeIndex : pimpl->topoOrder)
    {
        auto& node = pimpl->nodes[static_cast<size_t> (nodeIndex)];

        for (int s = 0; s < node.numOutputs; ++s)
        {
            if (node.outputSlotBuffer[static_cast<size_t> (s)] == -1)
                node.runtimeOutputs[static_cast<size_t> (s)] = const_cast<void*> (detail::getStreamBufferView (outputs[node.outputSlotGraphOut[static_cast<size_t> (s)]]).data);
        }

        pimpl->resolveNodeInputs (node, inputs, blockSize);

#if YUP_DEBUG
        for (int s = 0; s < node.numInputs; ++s)
            jassert (node.runtimeInputs[static_cast<size_t> (s)] != nullptr);

        for (int s = 0; s < node.numOutputs; ++s)
            jassert (node.runtimeOutputs[static_cast<size_t> (s)] != nullptr);
#endif

        ctx.inputs = node.runtimeInputs.data();
        ctx.outputs = node.runtimeOutputs.data();
        ctx.params = pimpl->params.data() + node.paramOffset;
        ctx.paramOut = pimpl->paramOut.data() + node.paramOutOffset;
        ctx.state = pimpl->state.data() + node.stateOffset;
        ctx.stateArrays = static_cast<char*> (ctx.state) + node.stateScalarSize;
        ctx.outputEvents = &node.outputEventQueue;

        for (const auto& [graphSlot, localIndex] : node.paramCopies)
        {
            const auto localByteOffset = node.paramByteOffsets[static_cast<size_t> (localIndex)];
            const auto size = static_cast<size_t> (detail::elementSize (pimpl->paramSlotTypes[static_cast<size_t> (graphSlot)]));
            std::memcpy (static_cast<uint8_t*> (ctx.params) + static_cast<size_t> (localByteOffset),
                         pimpl->params.data() + static_cast<size_t> (pimpl->paramOffsets[static_cast<size_t> (graphSlot)]),
                         size);
        }

        bool hasSplits = ! node.pendingAutomation.empty() || ! node.pendingAllSoundOffOffsets.empty();
        if (! hasSplits && node.isEventDriven)
        {
            for (const auto& calls : node.voicePendingCalls)
            {
                if (! calls.empty())
                {
                    hasSplits = true;
                    break;
                }
            }
        }

        if (hasSplits && (node.rateMultiplier > 1 || node.rateDivider > 1))
        {
            jassert (node.voiceCount == 1);

            for (const auto& autoEvent : node.pendingAutomation)
                pimpl->applyAutomation (node, autoEvent, ctx);

            hasSplits = false;
        }

        if (! hasSplits && node.voiceCount == 1 && ! node.isEventDriven)
        {
            if (node.rateMultiplier > 1)
            {
                if (node.numOutputs > 0 && (node.numInputs == node.numOutputs || node.numInputs == 0))
                {
                    switch (node.rateMultiplier)
                    {
                        case 2:
                            pimpl->runOversampledKernel (*node.oversampler2x, node, ctx, blockSize);
                            break;

                        case 4:
                            pimpl->runOversampledKernel (*node.oversampler4x, node, ctx, blockSize);
                            break;

                        case 8:
                            pimpl->runOversampledKernel (*node.oversampler8x, node, ctx, blockSize);
                            break;

                        default:
                            jassertfalse; // unsupported oversampling factor: the node runs at 1x
                            node.kernel (&ctx);
                            break;
                    }
                }
                else
                {
                    jassertfalse; // oversampling requires equal input/output stream counts (or no inputs)
                    node.kernel (&ctx);
                }
            }
            else if (node.rateDivider > 1)
            {
                if (node.numOutputs > 0 && (node.numInputs == node.numOutputs || node.numInputs == 0))
                {
                    switch (node.rateDivider)
                    {
                        case 2:
                            pimpl->runUndersampledKernel (*node.decimator2x, *node.interpolator2x, node, ctx, blockSize);
                            break;

                        case 4:
                            pimpl->runUndersampledKernel (*node.decimator4x, *node.interpolator4x, node, ctx, blockSize);
                            break;

                        case 8:
                            pimpl->runUndersampledKernel (*node.decimator8x, *node.interpolator8x, node, ctx, blockSize);
                            break;

                        default:
                            jassertfalse; // unsupported undersampling factor: the node runs at 1x
                            node.kernel (&ctx);
                            break;
                    }
                }
                else
                {
                    jassertfalse; // undersampling requires equal input/output stream counts (or no inputs)
                    node.kernel (&ctx);
                }
            }
            else
            {
                node.kernel (&ctx);
            }

            pimpl->drainOutputEvents (node, nodeIndex, blockSize, midiOut);

            continue;
        }

        pimpl->processNodeWithSplits (node, ctx, blockSize);
        pimpl->drainOutputEvents (node, nodeIndex, blockSize, midiOut);

        ctx.numSamples = blockSize;
    }

    pimpl->mixGraphOutputs (inputs, outputs, blockSize);

    return DspJitProcessResult::ok;
}

} // namespace yup
