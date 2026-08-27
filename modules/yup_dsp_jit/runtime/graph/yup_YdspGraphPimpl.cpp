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

namespace detail
{

YdspEventPayload payloadFromNote (const yup::MPENote& note)
{
    YdspEventPayload payload;
    payload.pitch = static_cast<float> (note.initialNote);
    payload.velocity = note.noteOnVelocity.asUnsignedFloat();
    payload.pressure = note.pressure.asUnsignedFloat();
    payload.slide = note.timbre.asUnsignedFloat();
    payload.bend = static_cast<float> (note.totalPitchbendInSemitones);
    payload.channel = note.midiChannel - 1;

    return payload;
}

/** Synthesizes a `noteId` for a note-on/note-off routed from one node's output
    event endpoint to another node's input event endpoint.

    Node-to-node routed notes are keyed by `(sourceNodeIndex, quantizedPitch)`
    rather than pitch alone, so two source nodes emitting the same pitch onto a
    shared destination voice bank cannot collide on the same synthetic id.

    Bit layout of the returned value: bit 15 is always set (see below); bits
    7-14 (8 bits) hold `sourceNodeIndex & 0xFF`, supporting up to 256 distinct
    source nodes; bits 0-6 (7 bits) hold `pitch & 0x7F`, which is lossless
    since MIDI pitch is always 0-127. A graph with more than 256 nodes aliases
    source node indices in this encoding rather than failing outright - an
    acceptable degradation for a synthetic id, not a correctness-critical
    value.

    Bit 15 disjoints this id space from `yup::MPENote::generateNoteID()`'s
    own `(midiChannel << 7) + midiNoteNumber` encoding (channel 1-16), whose
    largest possible value is `(16 << 7) + 127 = 2175` - well under `0x8000`.
    Without this tag, a routed note from a low-numbered source node could
    alias a genuinely host-triggered note sharing the same `eventInputIndex`
    (`findVoiceForNote` keys only on `(noteId, eventInputIndex)`), letting a
    routed noteOff silently release the wrong, host-triggered voice.
*/
uint16_t makeSyntheticNoteId (int sourceNodeIndex, int pitch) noexcept
{
    return static_cast<uint16_t> (0x8000 | ((sourceNodeIndex & 0xFF) << 7) | (pitch & 0x7F));
}

} // namespace detail

//==============================================================================

DspJitGraph::Pimpl::Pimpl()
{
    // The per-event-input MPE instruments are created in ensureEventInputs()
    // (called from prepare() and the control-thread MIDI mode setters), so the
    // constructor has nothing to set up here. Plain MIDI is the default
    // tracking mode, matching the historical single-stream behaviour.
}

void DspJitGraph::Pimpl::setExpressionTrackingMode (yup::MPEInstrument::TrackingMode mode)
{
    expressionTrackingMode = mode;

    for (auto& instrument : mpeInstruments)
    {
        instrument->setPitchbendTrackingMode (mode);
        instrument->setPressureTrackingMode (mode);
        instrument->setTimbreTrackingMode (mode);
    }
}

void DspJitGraph::Pimpl::ensureEventInputs()
{
    const auto count = eventInputNames.size();

    if (mpeInstruments.size() == count)
        return;

    mpeInstruments.clear();
    eventIngests.clear();

    mpeInstruments.reserve (count);
    eventIngests.reserve (count);

    for (size_t i = 0; i < count; ++i)
    {
        eventIngests.emplace_back (*this, static_cast<int> (i));
        mpeInstruments.emplace_back (std::make_unique<yup::MPEInstrument>());
        auto& instrument = *mpeInstruments.back();

        instrument.addListener (&eventIngests.back());
        instrument.reserveNotes (maxTrackedNotes);

        // Plain MIDI is the default, so a host that knows nothing about MPE
        // keeps working. This also warms the listener list's iteration
        // storage, which grows once on its first call and would otherwise do
        // so on the audio thread.
        instrument.enableLegacyMode();
        instrument.setPitchbendTrackingMode (expressionTrackingMode);
        instrument.setPressureTrackingMode (expressionTrackingMode);
        instrument.setTimbreTrackingMode (expressionTrackingMode);
    }
}

DspJitGraph::Pimpl::~Pimpl()
{
    for (size_t i = 0; i < mpeInstruments.size(); ++i)
        mpeInstruments[i]->removeListener (&eventIngests[i]);

#if YUP_WASM
    for (const auto handle : wasmHandles)
        YdspWasmRuntime::freeKernel (handle);
#endif
}

//==============================================================================

void DspJitGraph::Pimpl::EventIngest::noteAdded (yup::MPENote note)
{
    owner.routeEvent (YdspEventShape::noteOn, note.noteID, detail::payloadFromNote (note), owner.currentSampleOffset, eventInputIndex);
}

void DspJitGraph::Pimpl::EventIngest::noteReleased (yup::MPENote note)
{
    auto payload = detail::payloadFromNote (note);
    payload.velocity = note.noteOffVelocity.asUnsignedFloat();

    owner.routeEvent (YdspEventShape::noteOff, note.noteID, payload, owner.currentSampleOffset, eventInputIndex);
}

void DspJitGraph::Pimpl::EventIngest::notePitchbendChanged (yup::MPENote note)
{
    owner.routeEvent (YdspEventShape::pitchBend, note.noteID, detail::payloadFromNote (note), owner.currentSampleOffset, eventInputIndex);
}

void DspJitGraph::Pimpl::EventIngest::notePressureChanged (yup::MPENote note)
{
    owner.routeEvent (YdspEventShape::pressure, note.noteID, detail::payloadFromNote (note), owner.currentSampleOffset, eventInputIndex);
}

void DspJitGraph::Pimpl::EventIngest::noteTimbreChanged (yup::MPENote note)
{
    owner.routeEvent (YdspEventShape::slide, note.noteID, detail::payloadFromNote (note), owner.currentSampleOffset, eventInputIndex);
}

//==============================================================================

void DspJitGraph::Pimpl::ingestChannelMessage (const yup::MidiMessage& message, int eventInputIndex)
{
    if (message.isProgramChange())
    {
        YdspEventPayload payload;
        payload.index = message.getProgramChangeNumber();
        payload.channel = message.getChannel() - 1;

        routeEvent (YdspEventShape::programChange, 0, payload, currentSampleOffset, eventInputIndex);
        return;
    }

    if (message.isAftertouch())
    {
        auto& instrument = *mpeInstruments[static_cast<size_t> (eventInputIndex)];

        if (instrument.isLegacyModeEnabled())
            instrument.polyAftertouch (message.getChannel(), message.getNoteNumber(), yup::MPEValue::from7BitInt (message.getAfterTouchValue()));

        return;
    }

    if (! message.isController())
        return;

    if (message.getControllerNumber() == 120)
        scheduleAllSoundOff (eventInputIndex);

    YdspEventPayload payload;
    payload.index = message.getControllerNumber();
    payload.value = static_cast<float> (message.getControllerValue()) / 127.0f;
    payload.channel = message.getChannel() - 1;

    routeEvent (YdspEventShape::controlChange, 0, payload, currentSampleOffset, eventInputIndex);
}

void DspJitGraph::Pimpl::scheduleAllSoundOff (int eventInputIndex)
{
    for (const auto& route : graphInputRouting[static_cast<size_t> (eventInputIndex)])
    {
        if (route.dstNode < 0)
            continue;

        auto& node = nodes[static_cast<size_t> (route.dstNode)];

        if (! node.isEventDriven || ! subscribesTo (node, route.dstEventInputIndex))
            continue;

        if (node.pendingAllSoundOffOffsets.size() < node.pendingAllSoundOffOffsets.capacity())
        {
            node.pendingAllSoundOffOffsets.push_back (currentSampleOffset);
            node.voiceSlots.assign (node.voiceSlots.size(), {});
            node.numMonoHeldNotes = 0;
        }
        else
        {
            droppedEventCount.fetch_add (1, std::memory_order_relaxed);
        }

        for (auto& calls : node.voicePendingCalls)
        {
            calls.erase (std::remove_if (calls.begin(),
                                         calls.end(),
                                         [this] (const Node::PendingHandlerCall& call)
            {
                return call.sampleOffset >= currentSampleOffset;
            }),
                         calls.end());
        }
    }

    mpeInstruments[static_cast<size_t> (eventInputIndex)]->releaseAllNotes();
}

//==============================================================================

void DspJitGraph::Pimpl::routeEvent (YdspEventShape shape, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex)
{
    // eventInputIndex here is the graph input port the host event arrived on;
    // translated below to each explicitly-connected node's own local
    // input-event slot, which is what dispatchEventToNode expects.
    for (const auto& route : graphInputRouting[static_cast<size_t> (eventInputIndex)])
    {
        if (route.dstNode < 0)
            continue;

        auto& node = nodes[static_cast<size_t> (route.dstNode)];

        if (! node.isEventDriven)
            continue;

        dispatchEventToNode (node, shape, noteId, payload, sampleOffset, route.dstEventInputIndex);
    }
}

void DspJitGraph::Pimpl::dispatchEventToNode (Node& node, YdspEventShape shape, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex)
{
    if (node.isMidiOnly)
    {
        pushPendingCall (node, 0, sampleOffset, shape, payload, eventInputIndex);
        return;
    }

    switch (shape)
    {
        case YdspEventShape::noteOn:
            if (node.voiceMode == YdspVoiceMode::mono)
                resolveMonoNoteOn (node, noteId, payload, sampleOffset, eventInputIndex);
            else
                resolveNoteOn (node, noteId, payload, sampleOffset, eventInputIndex);
            break;

        case YdspEventShape::noteOff:
            if (node.voiceMode == YdspVoiceMode::mono)
                resolveMonoNoteOff (node, noteId, payload, sampleOffset, eventInputIndex);
            else
                resolveNoteOff (node, noteId, payload, sampleOffset, eventInputIndex);
            break;

        case YdspEventShape::pitchBend:
        case YdspEventShape::pressure:
        case YdspEventShape::slide:
        {
            if (! handlesShape (node, eventInputIndex, shape))
                break;

            const auto voice = findVoiceForNote (node, noteId, eventInputIndex);

            if (voice < 0)
            {
                droppedEventCount.fetch_add (1, std::memory_order_relaxed);
                break;
            }

            pushPendingCall (node, voice, sampleOffset, shape, payload, eventInputIndex);
            break;
        }

        case YdspEventShape::controlChange:
        case YdspEventShape::programChange:
            for (int v = 0; v < node.voiceCount; ++v)
                pushPendingCall (node, v, sampleOffset, shape, payload, eventInputIndex);
            break;

        case YdspEventShape::midi:
            break;
    }
}

//==============================================================================

void DspJitGraph::Pimpl::drainOutputEvents (Node& node, int srcNodeIndex, int blockSize, yup::MidiBuffer* midiOut)
{
    for (const auto& carried : node.carryQueue)
        deliverResolvedEvent (carried.dstNode, carried.dstEventInputIndex, srcNodeIndex, carried.shapeTag, carried.fields, carried.sampleOffset - blockSize, blockSize, midiOut);

    node.carryQueue.clear();

    for (const auto& entry : node.outputEventQueue.entries)
    {
        if (static_cast<size_t> (entry.endpointIndex) >= node.outputRouting.size())
            continue;

        for (const auto& dest : node.outputRouting[static_cast<size_t> (entry.endpointIndex)])
        {
            const auto offset = entry.sampleOffset + dest.compensationSamples;

            if (offset > blockSize)
            {
                if (node.carryQueue.size() < node.carryQueue.capacity())
                    node.carryQueue.push_back ({ offset, dest.dstNode, dest.dstEventInputIndex, entry.shapeTag, entry.fields });
                else
                    node.outputEventQueue.droppedCount.fetch_add (1, std::memory_order_relaxed);

                continue;
            }

            deliverResolvedEvent (dest.dstNode, dest.dstEventInputIndex, srcNodeIndex, entry.shapeTag, entry.fields, offset, blockSize, midiOut);
        }
    }
}

void DspJitGraph::Pimpl::deliverResolvedEvent (int dstNode, int dstEventInputIndex, int srcNodeIndex, int64_t shapeTag, const YdspEventContext& fields, int sampleOffset, int blockSize, yup::MidiBuffer* midiOut)
{
    // A carried record more than one block stale resolves in a single carry
    // step rather than re-carrying, so the offset is bounded to this block.
    const auto offset = std::clamp (sampleOffset, 0, blockSize);
    const auto shape = static_cast<YdspEventShape> (shapeTag);

    YdspEventPayload payload;
    payload.pitch = fields.pitch;
    payload.velocity = fields.velocity;
    payload.pressure = fields.pressure;
    payload.slide = fields.slide;
    payload.bend = fields.bend;
    payload.value = fields.value;
    payload.index = fields.index;
    payload.flags = fields.flags;
    payload.channel = fields.channel;

    if (dstNode >= 0)
    {
        auto& node = nodes[static_cast<size_t> (dstNode)];

        if (! node.isEventDriven)
            return;

        switch (shape)
        {
            case YdspEventShape::noteOn:
            case YdspEventShape::noteOff:
            {
                const auto syntheticNoteId = detail::makeSyntheticNoteId (srcNodeIndex, static_cast<int> (std::round (payload.pitch)));
                dispatchEventToNode (node, shape, syntheticNoteId, payload, offset, dstEventInputIndex);
                break;
            }

            case YdspEventShape::controlChange:
            case YdspEventShape::programChange:
                dispatchEventToNode (node, shape, 0, payload, offset, dstEventInputIndex);
                break;

            case YdspEventShape::pitchBend:
            case YdspEventShape::pressure:
            case YdspEventShape::slide:
                // A routed pitchBend/pressure/slide carries no pitch to match a voice
                // against (unlike a host MPE event, already bound to an MPENote), so
                // it is broadcast to every voice instead of targeted - the same
                // pattern dispatchEventToNode's own switch uses for controlChange/
                // programChange.
                for (int v = 0; v < node.voiceCount; ++v)
                    pushPendingCall (node, v, offset, shape, payload, dstEventInputIndex);
                break;

            case YdspEventShape::midi:
                break;
        }

        return;
    }

    if (midiOut == nullptr)
        return;

    // A patch's `emit` fields are script-controlled, so every value crossing into MIDI's fixed ranges is clamped rather than trusted.
    const auto channel = std::clamp (payload.channel, 0, 15) + 1;
    const auto noteNumber = std::clamp (static_cast<int> (std::round (payload.pitch)), 0, 127);
    const auto velocity = std::clamp (payload.velocity, 0.0f, 1.0f);

    switch (shape)
    {
        case YdspEventShape::noteOn:
            midiOut->addEvent (yup::MidiMessage::noteOn (channel, noteNumber, velocity), offset);
            break;

        case YdspEventShape::noteOff:
            midiOut->addEvent (yup::MidiMessage::noteOff (channel, noteNumber, velocity), offset);
            break;

        case YdspEventShape::pitchBend:
        {
            // No per-instrument pitchbend range survives to this delivery path (a
            // routed bend is not bound to any MPE note/instrument), so the MIDI
            // default of 2 semitones (see DspJitGraph::setLegacyMidiMode's own
            // default) is used to encode the 14-bit wheel position.
            constexpr float pitchBendRangeSemitones = 2.0f;
            const auto normalized = std::clamp (payload.bend / pitchBendRangeSemitones, -1.0f, 1.0f);
            const auto position = std::clamp (static_cast<int> (std::round ((normalized + 1.0f) * 0.5f * 16383.0f)), 0, 16383);
            midiOut->addEvent (yup::MidiMessage::pitchWheel (channel, position), offset);
            break;
        }

        case YdspEventShape::pressure:
            midiOut->addEvent (yup::MidiMessage::channelPressureChange (channel, std::clamp (static_cast<int> (std::round (payload.pressure * 127.0f)), 0, 127)), offset);
            break;

        case YdspEventShape::slide:
            midiOut->addEvent (yup::MidiMessage::controllerEvent (channel, 74, std::clamp (static_cast<int> (std::round (payload.slide * 127.0f)), 0, 127)), offset);
            break;

        case YdspEventShape::controlChange:
            midiOut->addEvent (yup::MidiMessage::controllerEvent (channel, std::clamp (payload.index, 0, 127), std::clamp (static_cast<int> (std::round (payload.value * 127.0f)), 0, 127)), offset);
            break;

        case YdspEventShape::programChange:
            midiOut->addEvent (yup::MidiMessage::programChange (channel, std::clamp (payload.index, 0, 127)), offset);
            break;

        case YdspEventShape::midi:
            break;
    }
}

//==============================================================================

void DspJitGraph::Pimpl::resolveNodeInputs (Node& node, Span<const DspJitInputBuffer> inputs, int blockSize)
{
    for (int s = 0; s < node.numInputs; ++s)
    {
        const auto first = node.inputConnectionStart[static_cast<size_t> (s)];
        const auto last = node.inputConnectionStart[static_cast<size_t> (s) + 1];
        jassert (last > first);

        if (last - first == 1)
        {
            auto& connection = node.inputConnections[static_cast<size_t> (first)];
            const auto* data = applyConnectionDelay (connection, connectionSourceData (connection, inputs), blockSize);
            node.runtimeInputs[static_cast<size_t> (s)] = const_cast<void*> (data);
            continue;
        }

        auto* mix = node.inputMixPtrs[static_cast<size_t> (s)];
        jassert (mix != nullptr);

        const auto elemSize = static_cast<size_t> (node.inputElemSizes[static_cast<size_t> (s)]);
        const auto type = node.inputElemTypes[static_cast<size_t> (s)];

        for (int c = first; c < last; ++c)
        {
            auto& connection = node.inputConnections[static_cast<size_t> (c)];
            const auto* data = applyConnectionDelay (connection, connectionSourceData (connection, inputs), blockSize);

            if (c == first)
                std::memcpy (mix, data, static_cast<size_t> (blockSize) * elemSize);
            else
                accumulateStream (mix, data, type, blockSize);
        }

        node.runtimeInputs[static_cast<size_t> (s)] = mix;
    }
}

//==============================================================================

void DspJitGraph::Pimpl::mixGraphOutputs (Span<const DspJitInputBuffer> inputs, Span<DspJitOutputBuffer> outputs, int blockSize)
{
    const auto numOutputs = static_cast<int> (outputStreamTypes.size());

    for (int o = 0; o < numOutputs; ++o)
    {
        if (graphOutputDirect[static_cast<size_t> (o)])
            continue;

        const auto first = graphOutputSourceStart[static_cast<size_t> (o)];
        const auto last = graphOutputSourceStart[static_cast<size_t> (o) + 1];
        jassert (last > first);

        auto* destination = const_cast<void*> (detail::getStreamBufferView (outputs[static_cast<size_t> (o)]).data);
        const auto type = outputStreamTypes[static_cast<size_t> (o)];
        const auto elemSize = static_cast<size_t> (detail::elementSize (type));

        for (int c = first; c < last; ++c)
        {
            auto& source = graphOutputSources[static_cast<size_t> (c)];
            const auto* data = applyConnectionDelay (source, connectionSourceData (source, inputs), blockSize);

            if (c == first)
                std::memmove (destination, data, static_cast<size_t> (blockSize) * elemSize);
            else
                accumulateStream (destination, data, type, blockSize);
        }
    }
}

//==============================================================================

void DspJitGraph::Pimpl::runInitKernels()
{
    YdspKernelContext ctx;
    ctx.sampleRate = static_cast<float> (sampleRate);
    ctx.numSamples = 0;
    ctx.inputs = nullptr;
    ctx.outputs = nullptr;

    for (const int nodeIndex : topoOrder)
    {
        auto& node = nodes[static_cast<size_t> (nodeIndex)];

        if (! node.initKernel.isValid())
            continue;

        ctx.params = params.data() + node.paramOffset;
        ctx.paramOut = paramOut.data() + node.paramOutOffset;
        ctx.outputEvents = &node.outputEventQueue;

        for (int v = 0; v < node.voiceCount; ++v)
        {
            ctx.state = state.data() + node.stateOffset + static_cast<size_t> (v) * node.stateSize;
            ctx.stateArrays = static_cast<char*> (ctx.state) + node.stateScalarSize;
            node.initKernel (&ctx);
        }
    }
}

//==============================================================================

void DspJitGraph::Pimpl::pushPendingCall (Node& node, int voice, int sampleOffset, YdspEventShape shape, const YdspEventPayload& payload, int eventInputIndex)
{
    if (! handlesShape (node, eventInputIndex, shape))
        return;

    auto& calls = node.voicePendingCalls[static_cast<size_t> (voice)];

    if (calls.size() >= calls.capacity())
    {
        droppedEventCount.fetch_add (1, std::memory_order_relaxed);
        return;
    }

    calls.push_back ({ sampleOffset, eventInputIndex, shape, payload });
}

//==============================================================================

int DspJitGraph::Pimpl::findVoiceForNote (const Node& node, uint16_t noteId, int eventInputIndex) const
{
    for (int v = 0; v < node.voiceCount; ++v)
    {
        const auto& slot = node.voiceSlots[static_cast<size_t> (v)];

        if (slot.held && slot.noteId == noteId && slot.eventInputIndex == eventInputIndex)
            return v;
    }

    return -1;
}

//==============================================================================

void DspJitGraph::Pimpl::resolveNoteOn (Node& node, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex)
{
    int chosen = -1;
    uint64_t oldestRelease = std::numeric_limits<uint64_t>::max();

    for (int v = 0; v < node.voiceCount; ++v)
    {
        const auto& slot = node.voiceSlots[static_cast<size_t> (v)];

        if (! slot.held && slot.releaseOrder < oldestRelease)
        {
            oldestRelease = slot.releaseOrder;
            chosen = v;
        }
    }

    if (chosen < 0 && node.stealing != YdspVoiceStealing::none)
    {
        const bool stealOldest = node.stealing == YdspVoiceStealing::oldest;
        uint64_t bestTrigger = stealOldest ? std::numeric_limits<uint64_t>::max() : 0;

        for (int v = 0; v < node.voiceCount; ++v)
        {
            const auto& slot = node.voiceSlots[static_cast<size_t> (v)];

            if (! slot.held)
                continue;

            if (stealOldest ? slot.triggerOrder < bestTrigger : slot.triggerOrder >= bestTrigger)
            {
                bestTrigger = slot.triggerOrder;
                chosen = v;
            }
        }
    }

    if (chosen < 0)
        return;

    auto& slot = node.voiceSlots[static_cast<size_t> (chosen)];

    if (slot.held)
    {
        YdspEventPayload stolen;
        stolen.pitch = static_cast<float> (slot.currentPitch);

        pushPendingCall (node, chosen, sampleOffset, YdspEventShape::noteOff, stolen, eventInputIndex);
        slot.held = false;
    }

    slot.noteId = noteId;
    slot.currentPitch = static_cast<int> (payload.pitch);
    slot.eventInputIndex = eventInputIndex;
    slot.held = true;
    slot.triggerOrder = node.voiceTriggerCounter++;

    pushPendingCall (node, chosen, sampleOffset, YdspEventShape::noteOn, payload, eventInputIndex);
}

//==============================================================================

void DspJitGraph::Pimpl::resolveNoteOff (Node& node, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex)
{
    const auto chosen = findVoiceForNote (node, noteId, eventInputIndex);

    if (chosen < 0)
        return;

    auto& slot = node.voiceSlots[static_cast<size_t> (chosen)];
    slot.held = false;
    slot.releaseOrder = node.voiceReleaseCounter++;

    pushPendingCall (node, chosen, sampleOffset, YdspEventShape::noteOff, payload, eventInputIndex);
}

//==============================================================================

int DspJitGraph::Pimpl::chooseMonoNote (const Node& node) const
{
    if (node.numMonoHeldNotes == 0)
        return -1;

    if (node.monoPriority == YdspMonoPriority::last)
        return node.numMonoHeldNotes - 1;

    const bool wantLowest = node.monoPriority == YdspMonoPriority::low;
    int chosen = 0;

    for (int i = 1; i < node.numMonoHeldNotes; ++i)
    {
        const auto pitch = node.monoHeldNotes[static_cast<size_t> (i)].pitch;
        const auto bestPitch = node.monoHeldNotes[static_cast<size_t> (chosen)].pitch;

        if (wantLowest ? pitch < bestPitch : pitch > bestPitch)
            chosen = i;
    }

    return chosen;
}

void DspJitGraph::Pimpl::soundMonoNote (Node& node, int heldIndex, int sampleOffset, bool isLegato)
{
    const auto& held = node.monoHeldNotes[static_cast<size_t> (heldIndex)];

    auto& slot = node.voiceSlots.front();
    slot.noteId = held.noteId;
    slot.currentPitch = held.pitch;
    slot.eventInputIndex = held.eventInputIndex;
    slot.held = true;
    slot.triggerOrder = node.voiceTriggerCounter++;

    YdspEventPayload payload;
    payload.pitch = static_cast<float> (held.pitch);
    payload.velocity = held.velocity;
    payload.bend = held.bend;
    payload.flags = isLegato ? ydspEventFlagLegato : 0;

    pushPendingCall (node, 0, sampleOffset, YdspEventShape::noteOn, payload, held.eventInputIndex);
}

void DspJitGraph::Pimpl::resolveMonoNoteOn (Node& node, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex)
{
    if (node.numMonoHeldNotes >= Node::maxMonoHeldNotes)
    {
        droppedEventCount.fetch_add (1, std::memory_order_relaxed);
        return;
    }

    node.monoHeldNotes[static_cast<size_t> (node.numMonoHeldNotes++)] = { noteId, static_cast<int> (payload.pitch), payload.velocity, payload.bend, eventInputIndex };

    const auto& slot = node.voiceSlots.front();
    const bool wasSounding = slot.held;

    const auto chosen = chooseMonoNote (node);

    if (chosen < 0)
        return;

    if (wasSounding && slot.noteId == node.monoHeldNotes[static_cast<size_t> (chosen)].noteId)
        return;

    soundMonoNote (node, chosen, sampleOffset, wasSounding);
}

void DspJitGraph::Pimpl::resolveMonoNoteOff (Node& node, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex)
{
    int index = -1;

    for (int i = 0; i < node.numMonoHeldNotes; ++i)
    {
        const auto& held = node.monoHeldNotes[static_cast<size_t> (i)];

        if (held.noteId == noteId && held.eventInputIndex == eventInputIndex)
        {
            index = i;
            break;
        }
    }

    if (index < 0)
        return;

    for (int i = index; i + 1 < node.numMonoHeldNotes; ++i)
        node.monoHeldNotes[static_cast<size_t> (i)] = node.monoHeldNotes[static_cast<size_t> (i + 1)];

    --node.numMonoHeldNotes;

    auto& slot = node.voiceSlots.front();

    if (! slot.held || slot.noteId != noteId)
        return;

    const auto next = chooseMonoNote (node);

    if (next < 0)
    {
        slot.held = false;
        slot.releaseOrder = node.voiceReleaseCounter++;

        pushPendingCall (node, 0, sampleOffset, YdspEventShape::noteOff, payload, eventInputIndex);
        return;
    }

    soundMonoNote (node, next, sampleOffset, true);
}

//==============================================================================

void DspJitGraph::Pimpl::runKernelSubBlock (Node& node, YdspKernelContext& ctx, int offset, int length, bool polyphonic)
{
    ctx.numSamples = length;

    for (int ch = 0; ch < node.numInputs; ++ch)
    {
        auto* base = static_cast<uint8_t*> (node.runtimeInputs[static_cast<size_t> (ch)]);
        node.subBlockInputs[static_cast<size_t> (ch)] = base + static_cast<size_t> (offset) * static_cast<size_t> (node.inputElemSizes[static_cast<size_t> (ch)]);
    }

    ctx.inputs = node.subBlockInputs.data();

    for (int ch = 0; ch < node.numOutputs; ++ch)
    {
        auto* base = static_cast<uint8_t*> (polyphonic
                                                ? static_cast<void*> (node.voiceScratchPtrs[static_cast<size_t> (ch)])
                                                : node.runtimeOutputs[static_cast<size_t> (ch)]);

        node.subBlockOutputs[static_cast<size_t> (ch)] = base + static_cast<size_t> (offset) * static_cast<size_t> (node.outputElemSizes[static_cast<size_t> (ch)]);
    }

    ctx.outputs = node.subBlockOutputs.data();

    node.kernel (&ctx);
}

//==============================================================================

void DspJitGraph::Pimpl::applyAutomation (Node& node, const Node::PendingAutomation& autoEvent, const YdspKernelContext& ctx)
{
    const auto byteOffset = node.paramByteOffsets[static_cast<size_t> (autoEvent.localParamIndex)];
    std::memcpy (static_cast<uint8_t*> (ctx.params) + byteOffset, &autoEvent.value, sizeof (autoEvent.value));

    if (autoEvent.globalParamSlot >= 0 && static_cast<size_t> (autoEvent.globalParamSlot) < paramOffsets.size())
        std::memcpy (params.data() + static_cast<size_t> (paramOffsets[static_cast<size_t> (autoEvent.globalParamSlot)]), &autoEvent.value, sizeof (autoEvent.value));
}

//==============================================================================

void DspJitGraph::Pimpl::snapshotAutomationParams (Node& node, const YdspKernelContext& ctx)
{
    node.automationParamSnapshot.clear();

    for (const auto& autoEvent : node.pendingAutomation)
    {
        const auto byteOffset = node.paramByteOffsets[static_cast<size_t> (autoEvent.localParamIndex)];
        float value = 0.0f;
        std::memcpy (&value, static_cast<const uint8_t*> (ctx.params) + byteOffset, sizeof (value));
        node.automationParamSnapshot.emplace_back (autoEvent.localParamIndex, value);
    }
}

void DspJitGraph::Pimpl::restoreAutomationParams (Node& node, const YdspKernelContext& ctx)
{
    for (const auto& [localParamIndex, value] : node.automationParamSnapshot)
    {
        const auto byteOffset = node.paramByteOffsets[static_cast<size_t> (localParamIndex)];
        std::memcpy (static_cast<uint8_t*> (ctx.params) + byteOffset, &value, sizeof (value));
    }
}

//==============================================================================

void DspJitGraph::Pimpl::invokeEventHandler (Node& node, const Node::PendingHandlerCall& call, const YdspKernelContext& ctx)
{
    const auto shapeIndex = eventShapeIndex (call.shape);
    jassert (shapeIndex >= 0);

    const auto binding = std::find_if (node.eventInputs.begin(), node.eventInputs.end(), [&] (const Node::EventInputBinding& b)
    {
        return b.eventInputSlot == call.eventInputIndex;
    });

    jassert (binding != node.eventInputs.end());
    jassert (binding->handlers[static_cast<size_t> (shapeIndex)].isValid());

    YdspEventContext eventCtx;
    eventCtx.state = static_cast<float*> (ctx.state);
    eventCtx.stateArrays = eventCtx.state + node.stateScalarSize / sizeof (float);
    eventCtx.params = static_cast<float*> (ctx.params);
    eventCtx.sampleRate = ctx.sampleRate;
    eventCtx.pitch = call.payload.pitch;
    eventCtx.velocity = call.payload.velocity;
    eventCtx.pressure = call.payload.pressure;
    eventCtx.slide = call.payload.slide;
    eventCtx.bend = call.payload.bend;
    eventCtx.value = call.payload.value;
    eventCtx.index = call.payload.index;
    eventCtx.flags = call.payload.flags;
    eventCtx.channel = call.payload.channel;
    eventCtx.sampleOffset = call.sampleOffset;
    eventCtx.outputEvents = &node.outputEventQueue;

    binding->handlers[static_cast<size_t> (shapeIndex)](&eventCtx);
}

//==============================================================================

void DspJitGraph::Pimpl::silenceVoice (Node& node, YdspKernelContext& ctx)
{
    std::memset (ctx.state, 0, node.stateSize);

    if (node.initKernel.isValid())
        node.initKernel (&ctx);
}

//==============================================================================

void DspJitGraph::Pimpl::processNodeWithSplits (Node& node, YdspKernelContext& ctx, int blockSize)
{
    const bool polyphonic = node.voiceCount > 1;

    for (int ch = 0; ch < node.numInputs; ++ch)
        node.subBlockInputs[static_cast<size_t> (ch)] = node.runtimeInputs[static_cast<size_t> (ch)];

    for (int ch = 0; ch < node.numOutputs; ++ch)
        node.subBlockOutputs[static_cast<size_t> (ch)] = node.runtimeOutputs[static_cast<size_t> (ch)];

    ctx.inputs = node.subBlockInputs.data();
    ctx.outputs = node.subBlockOutputs.data();

    if (polyphonic)
    {
        for (int ch = 0; ch < node.numOutputs; ++ch)
        {
            auto* dst = static_cast<float*> (node.runtimeOutputs[static_cast<size_t> (ch)]);
            std::fill (dst, dst + blockSize, 0.0f);
        }

        for (int ch = 0; ch < node.numOutputs; ++ch)
        {
            auto* dst = node.voiceScratchPtrs[static_cast<size_t> (ch)];
            std::fill (dst, dst + blockSize, 0.0f);
        }
    }

    const bool rewindAutomation = polyphonic && ! node.pendingAutomation.empty();
    if (rewindAutomation)
        snapshotAutomationParams (node, ctx);

    for (int v = 0; v < node.voiceCount; ++v)
    {
        const auto& calls = node.voicePendingCalls[static_cast<size_t> (v)];

        if (rewindAutomation && v > 0)
            restoreAutomationParams (node, ctx);

        bool awake = ! voiceIsIdle (node, v);
        if (! awake
            && calls.empty()
            && node.pendingAllSoundOffOffsets.empty()
            && node.pendingAutomation.empty())
        {
            if (! polyphonic)
                clearVoiceSpan (node, 0, blockSize, polyphonic);

            continue;
        }

        auto& splitPoints = node.splitPointScratch;
        splitPoints.clear();

        for (const auto& call : calls)
            splitPoints.push_back (call.sampleOffset);

        for (const auto& autoEvent : node.pendingAutomation)
            splitPoints.push_back (autoEvent.sampleOffset);

        for (const auto allSoundOffOffset : node.pendingAllSoundOffOffsets)
            splitPoints.push_back (allSoundOffOffset);

        std::sort (splitPoints.begin(), splitPoints.end());
        splitPoints.erase (std::unique (splitPoints.begin(), splitPoints.end()), splitPoints.end());

        ctx.state = state.data() + node.stateOffset + static_cast<size_t> (v) * node.stateSize;
        ctx.stateArrays = static_cast<char*> (ctx.state) + node.stateScalarSize;

        bool rendered = false;

        int prevOffset = 0;
        for (const int p : splitPoints)
        {
            if (p > prevOffset)
            {
                if (awake)
                {
                    runKernelSubBlock (node, ctx, prevOffset, p - prevOffset, polyphonic);
                    rendered = true;
                }
                else
                {
                    clearVoiceSpan (node, prevOffset, p - prevOffset, polyphonic);
                }
            }

            for (const auto& autoEvent : node.pendingAutomation)
                if (autoEvent.sampleOffset == p)
                    applyAutomation (node, autoEvent, ctx);

            for (const auto allSoundOffOffset : node.pendingAllSoundOffOffsets)
                if (allSoundOffOffset == p)
                    silenceVoice (node, ctx);

            for (const auto& call : calls)
                if (call.sampleOffset == p)
                    invokeEventHandler (node, call, ctx);

            if (! awake)
                awake = ! voiceIsIdle (node, v);

            prevOffset = p;
        }

        if (prevOffset < blockSize)
        {
            if (awake)
            {
                runKernelSubBlock (node, ctx, prevOffset, blockSize - prevOffset, polyphonic);
                rendered = true;
            }
            else
            {
                clearVoiceSpan (node, prevOffset, blockSize - prevOffset, polyphonic);
            }
        }

        if (polyphonic && rendered)
        {
            for (int ch = 0; ch < node.numOutputs; ++ch)
            {
                const auto* src = node.voiceScratchPtrs[static_cast<size_t> (ch)];
                auto* dst = static_cast<float*> (node.runtimeOutputs[static_cast<size_t> (ch)]);

                for (int i = 0; i < blockSize; ++i)
                    dst[i] += src[i];
            }
        }
    }
}

} // namespace yup
