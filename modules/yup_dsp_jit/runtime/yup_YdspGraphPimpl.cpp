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

uint16_t makeSyntheticNoteId (int sourceNodeIndex, int pitch) noexcept
{
    return static_cast<uint16_t> (0x8000 | ((sourceNodeIndex & 0xFF) << 7) | (pitch & 0x7F));
}

} // namespace detail

//==============================================================================

YdspAudioGraph::Pimpl::Pimpl()
{
}

void YdspAudioGraph::Pimpl::setExpressionTrackingMode (yup::MPEInstrument::TrackingMode mode)
{
    expressionTrackingMode = mode;

    for (auto& instrument : mpeInstruments)
    {
        instrument->setPitchbendTrackingMode (mode);
        instrument->setPressureTrackingMode (mode);
        instrument->setTimbreTrackingMode (mode);
    }
}

void YdspAudioGraph::Pimpl::ensureEventInputs()
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

        instrument.enableLegacyMode();
        instrument.setPitchbendTrackingMode (expressionTrackingMode);
        instrument.setPressureTrackingMode (expressionTrackingMode);
        instrument.setTimbreTrackingMode (expressionTrackingMode);
    }
}

YdspAudioGraph::Pimpl::~Pimpl()
{
    for (size_t i = 0; i < mpeInstruments.size(); ++i)
        mpeInstruments[i]->removeListener (&eventIngests[i]);

#if YUP_WASM
    for (const auto handle : wasmHandles)
        YdspWasmRuntime::freeKernel (handle);
#endif
}

//==============================================================================

void YdspAudioGraph::Pimpl::EventIngest::noteAdded (yup::MPENote note)
{
    owner.routeEvent (YdspEventShape::noteOn, note.noteID, detail::payloadFromNote (note), owner.currentSampleOffset, eventInputIndex);
}

void YdspAudioGraph::Pimpl::EventIngest::noteReleased (yup::MPENote note)
{
    auto payload = detail::payloadFromNote (note);
    payload.velocity = note.noteOffVelocity.asUnsignedFloat();

    owner.routeEvent (YdspEventShape::noteOff, note.noteID, payload, owner.currentSampleOffset, eventInputIndex);
}

void YdspAudioGraph::Pimpl::EventIngest::notePitchbendChanged (yup::MPENote note)
{
    owner.routeEvent (YdspEventShape::pitchBend, note.noteID, detail::payloadFromNote (note), owner.currentSampleOffset, eventInputIndex);
}

void YdspAudioGraph::Pimpl::EventIngest::notePressureChanged (yup::MPENote note)
{
    owner.routeEvent (YdspEventShape::pressure, note.noteID, detail::payloadFromNote (note), owner.currentSampleOffset, eventInputIndex);
}

void YdspAudioGraph::Pimpl::EventIngest::noteTimbreChanged (yup::MPENote note)
{
    owner.routeEvent (YdspEventShape::slide, note.noteID, detail::payloadFromNote (note), owner.currentSampleOffset, eventInputIndex);
}

//==============================================================================

void YdspAudioGraph::Pimpl::ingestChannelMessage (const yup::MidiMessage& message, int eventInputIndex)
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

void YdspAudioGraph::Pimpl::scheduleAllSoundOff (int eventInputIndex)
{
    for (const auto& route : singleEventRoutes[static_cast<size_t> (eventInputIndex)])
    {
        if (route.dstNode < 0)
            continue;

        auto& node = nodes[static_cast<size_t> (route.dstNode)];

        if (! node.isEventDriven || ! subscribesTo (node, route.dstEventInputIndex))
            continue;

        auto& group = voiceGroups[static_cast<size_t> (node.groupIndex)];

        if (node.pendingAllSoundOffOffsets.size() < node.pendingAllSoundOffOffsets.capacity())
        {
            node.pendingAllSoundOffOffsets.push_back (currentSampleOffset);
            group.voiceSlots.assign (group.voiceSlots.size(), {});
            group.numMonoHeldNotes = 0;
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

    for (const auto& bucket : groupEventBuckets[static_cast<size_t> (eventInputIndex)])
    {
        auto& group = voiceGroups[static_cast<size_t> (bucket.groupIndex)];
        bool accepted = true;

        for (const int member : group.members)
        {
            auto& node = nodes[static_cast<size_t> (member)];

            if (node.pendingAllSoundOffOffsets.size() >= node.pendingAllSoundOffOffsets.capacity())
            {
                accepted = false;
                break;
            }
        }

        if (! accepted)
        {
            droppedEventCount.fetch_add (1, std::memory_order_relaxed);
            continue;
        }

        group.voiceSlots.assign (group.voiceSlots.size(), {});
        group.numMonoHeldNotes = 0;

        for (const int member : group.members)
        {
            auto& node = nodes[static_cast<size_t> (member)];
            node.pendingAllSoundOffOffsets.push_back (currentSampleOffset);

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
    }

    mpeInstruments[static_cast<size_t> (eventInputIndex)]->releaseAllNotes();
}

//==============================================================================

void YdspAudioGraph::Pimpl::routeEvent (YdspEventShape shape, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex)
{
    for (const auto& route : singleEventRoutes[static_cast<size_t> (eventInputIndex)])
    {
        if (route.dstNode < 0)
            continue;

        auto& node = nodes[static_cast<size_t> (route.dstNode)];

        if (! node.isEventDriven)
            continue;

        dispatchEventToNode (node, shape, noteId, payload, sampleOffset, route.dstEventInputIndex);
    }

    for (const auto& bucket : groupEventBuckets[static_cast<size_t> (eventInputIndex)])
    {
        if (bucket.targets.empty())
            continue;

        dispatchEventToGroup (bucket, voiceGroups[static_cast<size_t> (bucket.groupIndex)], shape, noteId, payload, sampleOffset, eventInputIndex);
    }
}

void YdspAudioGraph::Pimpl::buildEventRouteBuckets()
{
    groupEventBuckets.assign (eventInputNames.size(), {});
    singleEventRoutes.assign (eventInputNames.size(), {});

    for (size_t i = 0; i < eventInputNames.size(); ++i)
    {
        auto& groups = groupEventBuckets[i];
        auto& singles = singleEventRoutes[i];

        for (const auto& route : graphInputRouting[i])
        {
            if (route.dstNode < 0)
                continue;

            const auto& node = nodes[static_cast<size_t> (route.dstNode)];
            const auto& group = voiceGroups[static_cast<size_t> (node.groupIndex)];

            if (group.members.size() <= 1 || group.voiceCount <= 1)
            {
                singles.push_back (route);
                continue;
            }

            int bucketIndex = -1;

            for (size_t b = 0; b < groups.size(); ++b)
            {
                if (groups[b].groupIndex == node.groupIndex)
                {
                    bucketIndex = static_cast<int> (b);
                    break;
                }
            }

            if (bucketIndex < 0)
            {
                groups.emplace_back();
                groups.back().groupIndex = node.groupIndex;
                bucketIndex = static_cast<int> (groups.size()) - 1;
            }

            groups[static_cast<size_t> (bucketIndex)].targets.push_back ({ route.dstNode, route.dstEventInputIndex });
        }
    }
}

//==============================================================================

void YdspAudioGraph::Pimpl::dispatchEventToGroup (const GroupEventBucket& bucket, VoiceGroup& group, YdspEventShape shape, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int origin)
{
    const auto pushToTargets = [&] (int voice, YdspEventShape shape, const YdspEventPayload& p, int offset)
    {
        for (const auto& target : bucket.targets)
        {
            auto& node = nodes[static_cast<size_t> (target.nodeIndex)];
            pushPendingCall (node, voice, offset, shape, p, target.localSlot);
        }
    };

    switch (shape)
    {
        case YdspEventShape::noteOn:
        {
            int chosen = -1;
            uint64_t oldestRelease = std::numeric_limits<uint64_t>::max();

            for (int v = 0; v < group.voiceCount; ++v)
            {
                const auto& slot = group.voiceSlots[static_cast<size_t> (v)];

                if (! slot.held && slot.releaseOrder < oldestRelease)
                {
                    oldestRelease = slot.releaseOrder;
                    chosen = v;
                }
            }

            if (chosen < 0 && group.stealing != YdspVoiceStealing::none)
            {
                const bool stealOldest = group.stealing == YdspVoiceStealing::oldest;
                uint64_t bestTrigger = stealOldest ? std::numeric_limits<uint64_t>::max() : 0;

                for (int v = 0; v < group.voiceCount; ++v)
                {
                    const auto& slot = group.voiceSlots[static_cast<size_t> (v)];

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
                break;

            auto& slot = group.voiceSlots[static_cast<size_t> (chosen)];

            if (slot.held)
            {
                YdspEventPayload stolen;
                stolen.pitch = static_cast<float> (slot.currentPitch);

                pushToTargets (chosen, YdspEventShape::noteOff, stolen, sampleOffset);
                slot.held = false;
            }

            slot.noteId = noteId;
            slot.currentPitch = static_cast<int> (payload.pitch);
            slot.eventInputIndex = origin;
            slot.held = true;
            slot.triggerOrder = group.voiceTriggerCounter++;

            pushToTargets (chosen, YdspEventShape::noteOn, payload, sampleOffset);
            break;
        }

        case YdspEventShape::noteOff:
        {
            int chosen = -1;

            for (int v = 0; v < group.voiceCount; ++v)
            {
                const auto& slot = group.voiceSlots[static_cast<size_t> (v)];

                if (slot.held && slot.noteId == noteId && slot.eventInputIndex == origin)
                {
                    chosen = v;
                    break;
                }
            }

            if (chosen < 0)
                break;

            auto& slot = group.voiceSlots[static_cast<size_t> (chosen)];
            slot.held = false;
            slot.releaseOrder = group.voiceReleaseCounter++;

            pushToTargets (chosen, YdspEventShape::noteOff, payload, sampleOffset);
            break;
        }

        case YdspEventShape::pitchBend:
        case YdspEventShape::pressure:
        case YdspEventShape::slide:
        {
            int chosen = -1;

            for (int v = 0; v < group.voiceCount; ++v)
            {
                const auto& slot = group.voiceSlots[static_cast<size_t> (v)];

                if (slot.held && slot.noteId == noteId && slot.eventInputIndex == origin)
                {
                    chosen = v;
                    break;
                }
            }

            if (chosen < 0)
            {
                droppedEventCount.fetch_add (1, std::memory_order_relaxed);
                break;
            }

            pushToTargets (chosen, shape, payload, sampleOffset);
            break;
        }

        case YdspEventShape::controlChange:
        case YdspEventShape::programChange:
            for (int v = 0; v < group.voiceCount; ++v)
                pushToTargets (v, shape, payload, sampleOffset);
            break;

        case YdspEventShape::midi:
            break;
    }
}

void YdspAudioGraph::Pimpl::dispatchEventToNode (Node& node, YdspEventShape shape, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex)
{
    jassert (node.groupIndex >= 0);
    auto& group = voiceGroups[static_cast<size_t> (node.groupIndex)];
    jassert (group.voiceCount == node.voiceCount);

    if (node.isMidiOnly)
    {
        pushPendingCall (node, 0, sampleOffset, shape, payload, eventInputIndex);
        return;
    }

    switch (shape)
    {
        case YdspEventShape::noteOn:
            if (group.voiceMode == YdspVoiceMode::mono)
                resolveMonoNoteOn (node, group, noteId, payload, sampleOffset, eventInputIndex);
            else
                resolveNoteOn (node, group, noteId, payload, sampleOffset, eventInputIndex);
            break;

        case YdspEventShape::noteOff:
            if (group.voiceMode == YdspVoiceMode::mono)
                resolveMonoNoteOff (node, group, noteId, payload, sampleOffset, eventInputIndex);
            else
                resolveNoteOff (node, group, noteId, payload, sampleOffset, eventInputIndex);
            break;

        case YdspEventShape::pitchBend:
        case YdspEventShape::pressure:
        case YdspEventShape::slide:
        {
            if (! handlesShape (node, eventInputIndex, shape))
                break;

            const auto voice = findVoiceForNote (node, group, noteId, eventInputIndex);

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

void YdspAudioGraph::Pimpl::drainOutputEvents (Node& node, int srcNodeIndex, int blockSize, yup::MidiBuffer* midiOut)
{
    for (const auto& carried : node.carryQueue)
        deliverResolvedEvent (carried.dstNode, carried.dstEventInputIndex, srcNodeIndex, carried.shapeTag, carried.fields, carried.sampleOffset, blockSize, midiOut);

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
                    node.carryQueue.push_back ({ offset - blockSize, dest.dstNode, dest.dstEventInputIndex, entry.shapeTag, entry.fields });
                else
                    node.outputEventQueue.droppedCount.fetch_add (1, std::memory_order_relaxed);

                continue;
            }

            deliverResolvedEvent (dest.dstNode, dest.dstEventInputIndex, srcNodeIndex, entry.shapeTag, entry.fields, offset, blockSize, midiOut);
        }
    }
}

void YdspAudioGraph::Pimpl::deliverResolvedEvent (int dstNode, int dstEventInputIndex, int srcNodeIndex, int64_t shapeTag, const YdspEventContext& fields, int sampleOffset, int blockSize, yup::MidiBuffer* midiOut)
{
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

void YdspAudioGraph::Pimpl::resolveNodeInputs (Node& node, Span<const YdspInputBuffer> inputs, int blockSize)
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

void YdspAudioGraph::Pimpl::resolveGroupMemberInputs (Node& node, Span<const YdspInputBuffer> inputs, int blockSize, int voice)
{
    for (int s = 0; s < node.numInputs; ++s)
    {
        const auto first = node.inputConnectionStart[static_cast<size_t> (s)];
        const auto last = node.inputConnectionStart[static_cast<size_t> (s) + 1];
        jassert (last > first);

        bool hasIntra = false;

        for (int c = first; c < last; ++c)
        {
            if (node.inputConnections[static_cast<size_t> (c)].readsVoiceScratch)
            {
                hasIntra = true;
                break;
            }
        }

        const auto elemSize = static_cast<size_t> (node.inputElemSizes[static_cast<size_t> (s)]);
        const auto type = node.inputElemTypes[static_cast<size_t> (s)];

        if (voice < 0)
        {
            if (hasIntra)
            {
                for (int c = first; c < last; ++c)
                {
                    auto& connection = node.inputConnections[static_cast<size_t> (c)];

                    if (! connection.readsVoiceScratch)
                        connection.blockSource = applyConnectionDelay (connection, connectionSourceData (connection, inputs), blockSize);
                }

                continue;
            }

            if (last - first == 1)
            {
                auto& connection = node.inputConnections[static_cast<size_t> (first)];
                connection.blockSource = applyConnectionDelay (connection, connectionSourceData (connection, inputs), blockSize);
                node.runtimeInputs[static_cast<size_t> (s)] = const_cast<void*> (connection.blockSource);
                continue;
            }

            auto* mix = node.inputMixPtrs[static_cast<size_t> (s)];
            jassert (mix != nullptr);

            for (int c = first; c < last; ++c)
            {
                auto& connection = node.inputConnections[static_cast<size_t> (c)];
                connection.blockSource = applyConnectionDelay (connection, connectionSourceData (connection, inputs), blockSize);

                if (c == first)
                    std::memcpy (mix, connection.blockSource, static_cast<size_t> (blockSize) * elemSize);
                else
                    accumulateStream (mix, connection.blockSource, type, blockSize);
            }

            node.runtimeInputs[static_cast<size_t> (s)] = mix;
            continue;
        }

        if (! hasIntra)
            continue; // resolved whole-block in the prologue

        if (last - first == 1)
        {
            auto& connection = node.inputConnections[static_cast<size_t> (first)];

            const auto* data = connection.readsVoiceScratch
                                 ? applyConnectionDelay (connection, connectionSourceAtVoice (connection, inputs, voice), blockSize, voice)
                                 : connection.blockSource;

            node.runtimeInputs[static_cast<size_t> (s)] = const_cast<void*> (data);
            continue;
        }

        auto* mix = node.inputMixPtrs[static_cast<size_t> (s)];
        jassert (mix != nullptr);

        for (int c = first; c < last; ++c)
        {
            auto& connection = node.inputConnections[static_cast<size_t> (c)];

            const auto* data = connection.readsVoiceScratch
                                 ? applyConnectionDelay (connection, connectionSourceAtVoice (connection, inputs, voice), blockSize, voice)
                                 : connection.blockSource;

            if (c == first)
                std::memcpy (mix, data, static_cast<size_t> (blockSize) * elemSize);
            else
                accumulateStream (mix, data, type, blockSize);
        }

        node.runtimeInputs[static_cast<size_t> (s)] = mix;
    }
}

//==============================================================================

void YdspAudioGraph::Pimpl::processPolyphonicGroup (VoiceGroup& group,
                                                    int topoStart,
                                                    YdspKernelContext& ctx,
                                                    Span<const YdspInputBuffer> inputs,
                                                    Span<YdspOutputBuffer> outputs,
                                                    int blockSize,
                                                    yup::MidiBuffer* midiOut)
{
    const int memberCount = static_cast<int> (group.members.size());

    jassert (memberCount > 1);
    jassert (group.voiceCount > 1);

    // ---- Per-member prologue, once per block ----
    for (int m = 0; m < memberCount; ++m)
    {
        auto& node = nodes[static_cast<size_t> (topoOrder[static_cast<size_t> (topoStart + m)])];

        for (int s = 0; s < node.numOutputs; ++s)
        {
            if (node.outputSlotBuffer[static_cast<size_t> (s)] == -1)
                node.runtimeOutputs[static_cast<size_t> (s)] = const_cast<void*> (detail::getStreamBufferView (outputs[node.outputSlotGraphOut[static_cast<size_t> (s)]]).data);
        }

        ctx.params = params.data() + node.paramOffset;
        ctx.paramOut = paramOut.data() + node.paramOutOffset;
        ctx.outputEvents = &node.outputEventQueue;

        for (const auto& [graphSlot, localIndex] : node.paramCopies)
        {
            const auto localByteOffset = node.paramByteOffsets[static_cast<size_t> (localIndex)];
            const auto size = static_cast<size_t> (elementSizeBytes (paramSlotTypes[static_cast<size_t> (graphSlot)]));
            std::memcpy (static_cast<uint8_t*> (ctx.params) + static_cast<size_t> (localByteOffset),
                         params.data() + static_cast<size_t> (paramOffsets[static_cast<size_t> (graphSlot)]),
                         size);
        }

        if (! node.pendingAutomation.empty())
            snapshotAutomationParams (node, ctx);

        resolveGroupMemberInputs (node, inputs, blockSize, -1);

        if (node.accumulatesToRuntimeOutputs)
        {
            for (int ch = 0; ch < node.numOutputs; ++ch)
            {
                auto* dst = static_cast<float*> (node.runtimeOutputs[static_cast<size_t> (ch)]);
                std::fill (dst, dst + blockSize, 0.0f);
            }
        }
    }

    // ---- Voice-major schedule ----
    const bool hasBlockWork = [&]
    {
        for (int m = 0; m < memberCount; ++m)
        {
            const auto& node = nodes[static_cast<size_t> (topoOrder[static_cast<size_t> (topoStart + m)])];

            if (! node.pendingAllSoundOffOffsets.empty() || ! node.pendingAutomation.empty())
                return true;
        }

        return false;
    }();

    for (int v = 0; v < group.voiceCount; ++v)
    {
        for (int m = 0; m < memberCount; ++m)
        {
            auto& node = nodes[static_cast<size_t> (topoOrder[static_cast<size_t> (topoStart + m)])];

            for (int ch = 0; ch < node.numOutputs; ++ch)
            {
                auto* dst = node.voiceScratchPtrs[static_cast<size_t> (ch)];
                std::fill (dst, dst + blockSize, 0.0f);
            }
        }

        bool voiceRuns = hasBlockWork;

        if (! voiceRuns)
        {
            for (int m = 0; m < memberCount && ! voiceRuns; ++m)
            {
                const auto& node = nodes[static_cast<size_t> (topoOrder[static_cast<size_t> (topoStart + m)])];

                if (! voiceIsIdle (node, v) || ! node.voicePendingCalls[static_cast<size_t> (v)].empty())
                    voiceRuns = true;
            }
        }

        if (! voiceRuns)
            continue;

        for (int m = 0; m < memberCount; ++m)
        {
            auto& node = nodes[static_cast<size_t> (topoOrder[static_cast<size_t> (topoStart + m)])];

            ctx.params = params.data() + node.paramOffset;
            ctx.paramOut = paramOut.data() + node.paramOutOffset;
            ctx.outputEvents = &node.outputEventQueue;

            if (v > 0 && ! node.pendingAutomation.empty())
                restoreAutomationParams (node, ctx);

            resolveGroupMemberInputs (node, inputs, blockSize, v);
            runNodeVoice (node, ctx, v, blockSize);
        }
    }

    // ---- Member teardown ----
    for (int m = 0; m < memberCount; ++m)
    {
        const auto nodeIndex = topoOrder[static_cast<size_t> (topoStart + m)];
        auto& node = nodes[static_cast<size_t> (nodeIndex)];

        drainOutputEvents (node, nodeIndex, blockSize, midiOut);
    }

    ctx.numSamples = blockSize;
}

//==============================================================================

void YdspAudioGraph::Pimpl::mixGraphOutputs (Span<const YdspInputBuffer> inputs, Span<YdspOutputBuffer> outputs, int blockSize)
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
        const auto elemSize = static_cast<size_t> (elementSizeBytes (type));

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

void YdspAudioGraph::Pimpl::runInitKernels()
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

void YdspAudioGraph::Pimpl::pushPendingCall (Node& node, int voice, int sampleOffset, YdspEventShape shape, const YdspEventPayload& payload, int eventInputIndex)
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

int YdspAudioGraph::Pimpl::findVoiceForNote (const Node& node, const VoiceGroup& group, uint16_t noteId, int eventInputIndex) const
{
    jassert (group.voiceCount == node.voiceCount);

    for (int v = 0; v < group.voiceCount; ++v)
    {
        const auto& slot = group.voiceSlots[static_cast<size_t> (v)];

        if (slot.held && slot.noteId == noteId && slot.eventInputIndex == eventInputIndex)
            return v;
    }

    return -1;
}

//==============================================================================

void YdspAudioGraph::Pimpl::resolveNoteOn (Node& node, VoiceGroup& group, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex)
{
    jassert (group.voiceCount == node.voiceCount);

    int chosen = -1;
    uint64_t oldestRelease = std::numeric_limits<uint64_t>::max();

    for (int v = 0; v < group.voiceCount; ++v)
    {
        const auto& slot = group.voiceSlots[static_cast<size_t> (v)];

        if (! slot.held && slot.releaseOrder < oldestRelease)
        {
            oldestRelease = slot.releaseOrder;
            chosen = v;
        }
    }

    if (chosen < 0 && group.stealing != YdspVoiceStealing::none)
    {
        const bool stealOldest = group.stealing == YdspVoiceStealing::oldest;
        uint64_t bestTrigger = stealOldest ? std::numeric_limits<uint64_t>::max() : 0;

        for (int v = 0; v < group.voiceCount; ++v)
        {
            const auto& slot = group.voiceSlots[static_cast<size_t> (v)];

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

    auto& slot = group.voiceSlots[static_cast<size_t> (chosen)];

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
    slot.triggerOrder = group.voiceTriggerCounter++;

    pushPendingCall (node, chosen, sampleOffset, YdspEventShape::noteOn, payload, eventInputIndex);
}

//==============================================================================

void YdspAudioGraph::Pimpl::resolveNoteOff (Node& node, VoiceGroup& group, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex)
{
    const auto chosen = findVoiceForNote (node, group, noteId, eventInputIndex);

    if (chosen < 0)
        return;

    auto& slot = group.voiceSlots[static_cast<size_t> (chosen)];
    slot.held = false;
    slot.releaseOrder = group.voiceReleaseCounter++;

    pushPendingCall (node, chosen, sampleOffset, YdspEventShape::noteOff, payload, eventInputIndex);
}

//==============================================================================

int YdspAudioGraph::Pimpl::chooseMonoNote (const Node& node, const VoiceGroup& group) const
{
    jassert (node.voiceCount == 1);

    if (group.numMonoHeldNotes == 0)
        return -1;

    if (group.monoPriority == YdspMonoPriority::last)
        return group.numMonoHeldNotes - 1;

    const bool wantLowest = group.monoPriority == YdspMonoPriority::low;
    int chosen = 0;

    for (int i = 1; i < group.numMonoHeldNotes; ++i)
    {
        const auto pitch = group.monoHeldNotes[static_cast<size_t> (i)].pitch;
        const auto bestPitch = group.monoHeldNotes[static_cast<size_t> (chosen)].pitch;

        if (wantLowest ? pitch < bestPitch : pitch > bestPitch)
            chosen = i;
    }

    return chosen;
}

void YdspAudioGraph::Pimpl::soundMonoNote (Node& node, VoiceGroup& group, int heldIndex, int sampleOffset, bool isLegato)
{
    const auto& held = group.monoHeldNotes[static_cast<size_t> (heldIndex)];

    auto& slot = group.voiceSlots.front();
    slot.noteId = held.noteId;
    slot.currentPitch = held.pitch;
    slot.eventInputIndex = held.eventInputIndex;
    slot.held = true;
    slot.triggerOrder = group.voiceTriggerCounter++;

    YdspEventPayload payload;
    payload.pitch = static_cast<float> (held.pitch);
    payload.velocity = held.velocity;
    payload.bend = held.bend;
    payload.flags = isLegato ? ydspEventFlagLegato : 0;

    pushPendingCall (node, 0, sampleOffset, YdspEventShape::noteOn, payload, held.eventInputIndex);
}

void YdspAudioGraph::Pimpl::resolveMonoNoteOn (Node& node, VoiceGroup& group, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex)
{
    if (group.numMonoHeldNotes >= VoiceGroup::maxMonoHeldNotes)
    {
        droppedEventCount.fetch_add (1, std::memory_order_relaxed);
        return;
    }

    group.monoHeldNotes[static_cast<size_t> (group.numMonoHeldNotes++)] = { noteId, static_cast<int> (payload.pitch), payload.velocity, payload.bend, eventInputIndex };

    const auto& slot = group.voiceSlots.front();
    const bool wasSounding = slot.held;

    const auto chosen = chooseMonoNote (node, group);

    if (chosen < 0)
        return;

    if (wasSounding && slot.noteId == group.monoHeldNotes[static_cast<size_t> (chosen)].noteId)
        return;

    soundMonoNote (node, group, chosen, sampleOffset, wasSounding);
}

void YdspAudioGraph::Pimpl::resolveMonoNoteOff (Node& node, VoiceGroup& group, uint16_t noteId, const YdspEventPayload& payload, int sampleOffset, int eventInputIndex)
{
    int index = -1;

    for (int i = 0; i < group.numMonoHeldNotes; ++i)
    {
        const auto& held = group.monoHeldNotes[static_cast<size_t> (i)];

        if (held.noteId == noteId && held.eventInputIndex == eventInputIndex)
        {
            index = i;
            break;
        }
    }

    if (index < 0)
        return;

    for (int i = index; i + 1 < group.numMonoHeldNotes; ++i)
        group.monoHeldNotes[static_cast<size_t> (i)] = group.monoHeldNotes[static_cast<size_t> (i + 1)];

    --group.numMonoHeldNotes;

    auto& slot = group.voiceSlots.front();

    if (! slot.held || slot.noteId != noteId)
        return;

    const auto next = chooseMonoNote (node, group);

    if (next < 0)
    {
        slot.held = false;
        slot.releaseOrder = group.voiceReleaseCounter++;

        pushPendingCall (node, 0, sampleOffset, YdspEventShape::noteOff, payload, eventInputIndex);
        return;
    }

    soundMonoNote (node, group, next, sampleOffset, true);
}

//==============================================================================

void YdspAudioGraph::Pimpl::runKernelSubBlock (Node& node, YdspKernelContext& ctx, int offset, int length, bool polyphonic)
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

void YdspAudioGraph::Pimpl::applyAutomation (Node& node, const Node::PendingAutomation& autoEvent, const YdspKernelContext& ctx)
{
    const auto byteOffset = node.paramByteOffsets[static_cast<size_t> (autoEvent.localParamIndex)];
    std::memcpy (static_cast<uint8_t*> (ctx.params) + byteOffset, &autoEvent.value, sizeof (autoEvent.value));

    if (autoEvent.globalParamSlot >= 0 && static_cast<size_t> (autoEvent.globalParamSlot) < paramOffsets.size())
        std::memcpy (params.data() + static_cast<size_t> (paramOffsets[static_cast<size_t> (autoEvent.globalParamSlot)]), &autoEvent.value, sizeof (autoEvent.value));
}

//==============================================================================

void YdspAudioGraph::Pimpl::snapshotAutomationParams (Node& node, const YdspKernelContext& ctx)
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

void YdspAudioGraph::Pimpl::restoreAutomationParams (Node& node, const YdspKernelContext& ctx)
{
    for (const auto& [localParamIndex, value] : node.automationParamSnapshot)
    {
        const auto byteOffset = node.paramByteOffsets[static_cast<size_t> (localParamIndex)];
        std::memcpy (static_cast<uint8_t*> (ctx.params) + byteOffset, &value, sizeof (value));
    }
}

//==============================================================================

void YdspAudioGraph::Pimpl::invokeEventHandler (Node& node, const Node::PendingHandlerCall& call, const YdspKernelContext& ctx)
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

void YdspAudioGraph::Pimpl::silenceVoice (Node& node, YdspKernelContext& ctx)
{
    std::memset (ctx.state, 0, node.stateSize);

    if (node.initKernel.isValid())
        node.initKernel (&ctx);
}

//==============================================================================

void YdspAudioGraph::Pimpl::processNodeWithSplits (Node& node, YdspKernelContext& ctx, int blockSize)
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
    }

    const bool rewindAutomation = polyphonic && ! node.pendingAutomation.empty();
    if (rewindAutomation)
        snapshotAutomationParams (node, ctx);

    for (int v = 0; v < node.voiceCount; ++v)
    {
        if (rewindAutomation && v > 0)
            restoreAutomationParams (node, ctx);

        runNodeVoice (node, ctx, v, blockSize);
    }
}

//==============================================================================

void YdspAudioGraph::Pimpl::runNodeVoice (Node& node, YdspKernelContext& ctx, int voice, int blockSize)
{
    const bool polyphonic = node.voiceCount > 1;

    const auto& calls = node.voicePendingCalls[static_cast<size_t> (voice)];

    bool awake = ! voiceIsIdle (node, voice) || ! node.isEventDriven;
    if (node.isEventDriven
        && ! awake
        && calls.empty()
        && node.pendingAllSoundOffOffsets.empty()
        && node.pendingAutomation.empty())
    {
        if (! polyphonic)
            clearVoiceSpan (node, 0, blockSize, polyphonic);

        return;
    }

    if (polyphonic)
    {
        for (int ch = 0; ch < node.numOutputs; ++ch)
        {
            auto* dst = node.voiceScratchPtrs[static_cast<size_t> (ch)];
            std::fill (dst, dst + blockSize, 0.0f);
        }
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

    ctx.state = state.data() + node.stateOffset + static_cast<size_t> (voice) * node.stateSize;
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
            awake = ! voiceIsIdle (node, voice);

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

    if (polyphonic && rendered && node.accumulatesToRuntimeOutputs)
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

} // namespace yup
