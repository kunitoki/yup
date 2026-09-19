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

size_t YdspAudioGraph::getScratchMemorySizeBytes() const noexcept
{
    return pimpl != nullptr ? pimpl->scratch.size() : 0;
}

YdspElementType YdspAudioGraph::getInputStreamType (int index) const noexcept
{
    if (pimpl == nullptr || index < 0 || static_cast<size_t> (index) >= pimpl->inputStreamTypes.size())
        return YdspElementType::float32;

    return detail::toElementType (pimpl->inputStreamTypes[static_cast<size_t> (index)]);
}

YdspElementType YdspAudioGraph::getOutputStreamType (int index) const noexcept
{
    if (pimpl == nullptr || index < 0 || static_cast<size_t> (index) >= pimpl->outputStreamTypes.size())
        return YdspElementType::float32;

    return detail::toElementType (pimpl->outputStreamTypes[static_cast<size_t> (index)]);
}

int YdspAudioGraph::getInputStreamCount() const noexcept
{
    return pimpl != nullptr ? static_cast<int> (pimpl->inputStreamTypes.size()) : 0;
}

int YdspAudioGraph::getOutputStreamCount() const noexcept
{
    return pimpl != nullptr ? static_cast<int> (pimpl->outputStreamTypes.size()) : 0;
}

int YdspAudioGraph::getEventInputCount() const noexcept
{
    return pimpl != nullptr ? static_cast<int> (pimpl->eventInputNames.size()) : 0;
}

String YdspAudioGraph::getEventInputName (int index) const noexcept
{
    if (pimpl == nullptr || index < 0 || static_cast<size_t> (index) >= pimpl->eventInputNames.size())
        return {};

    return pimpl->eventInputNames[static_cast<size_t> (index)];
}

int YdspAudioGraph::getLatencySamples() const noexcept
{
    return pimpl != nullptr ? pimpl->latencySamples : 0;
}

int YdspAudioGraph::getParameterCount() const noexcept
{
    return pimpl != nullptr ? static_cast<int> (pimpl->paramInfos.size()) : 0;
}

const YdspParameterInfo& YdspAudioGraph::getParameterInfo (int slot) const noexcept
{
    static const YdspParameterInfo empty;

    if (pimpl == nullptr || slot < 0 || static_cast<size_t> (slot) >= pimpl->paramInfos.size())
        return empty;

    return pimpl->paramInfos[static_cast<size_t> (slot)];
}

YdspElementType YdspAudioGraph::getParameterType (StringRef qualifiedName) const noexcept
{
    if (pimpl == nullptr)
        return YdspElementType::float32;

    const auto slot = pimpl->paramSlot (qualifiedName);

    if (slot < 0 || static_cast<size_t> (slot) >= pimpl->paramSlotTypes.size())
        return YdspElementType::float32;

    return detail::toElementType (pimpl->paramSlotTypes[static_cast<size_t> (slot)]);
}

bool YdspAudioGraph::hasParameter (StringRef qualifiedName) const noexcept
{
    return pimpl != nullptr && pimpl->paramSlot (qualifiedName) >= 0;
}

float YdspAudioGraph::getParameter (StringRef qualifiedName) const noexcept
{
    return getParameterBySlot (getParameterSlot (qualifiedName));
}

float YdspAudioGraph::getParameterBySlot (int slot) const noexcept
{
    if (pimpl == nullptr)
        return 0.0f;

    if (slot < 0 || static_cast<size_t> (slot) >= pimpl->paramSlotTypes.size()
        || pimpl->paramSlotTypes[static_cast<size_t> (slot)] != YdspValueType::float32Type)
        return 0.0f;

    float value = 0.0f;
    const auto bits = pimpl->parameterSnapshots[static_cast<size_t> (slot)].load (std::memory_order_relaxed);
    std::memcpy (&value, &bits, sizeof (value));
    return value;
}

void YdspAudioGraph::setParameter (StringRef qualifiedName, float value)
{
    setParameterBySlot (getParameterSlot (qualifiedName), value);
}

void YdspAudioGraph::setParameterBySlot (int slot, float value)
{
    if (pimpl == nullptr)
        return;

    if (slot < 0 || static_cast<size_t> (slot) >= pimpl->paramSlotTypes.size()
        || pimpl->paramSlotTypes[static_cast<size_t> (slot)] != YdspValueType::float32Type)
        return;

    uint64_t bits = 0;
    std::memcpy (&bits, &value, sizeof (value));

    if (! pimpl->pushHostParam (slot, bits))
        pimpl->droppedEventCount.fetch_add (1, std::memory_order_relaxed);
}

double YdspAudioGraph::getDoubleParameter (StringRef qualifiedName) const noexcept
{
    return getDoubleParameterBySlot (getParameterSlot (qualifiedName));
}

double YdspAudioGraph::getDoubleParameterBySlot (int slot) const noexcept
{
    if (pimpl == nullptr)
        return 0.0;

    if (slot < 0 || static_cast<size_t> (slot) >= pimpl->paramSlotTypes.size()
        || pimpl->paramSlotTypes[static_cast<size_t> (slot)] != YdspValueType::float64Type)
        return 0.0;

    double value = 0.0;
    const auto bits = pimpl->parameterSnapshots[static_cast<size_t> (slot)].load (std::memory_order_relaxed);
    std::memcpy (&value, &bits, sizeof (value));
    return value;
}

void YdspAudioGraph::setDoubleParameter (StringRef qualifiedName, double value)
{
    setDoubleParameterBySlot (getParameterSlot (qualifiedName), value);
}

void YdspAudioGraph::setDoubleParameterBySlot (int slot, double value)
{
    if (pimpl == nullptr)
        return;

    if (slot < 0 || static_cast<size_t> (slot) >= pimpl->paramSlotTypes.size()
        || pimpl->paramSlotTypes[static_cast<size_t> (slot)] != YdspValueType::float64Type)
        return;

    uint64_t bits = 0;
    std::memcpy (&bits, &value, sizeof (value));

    if (! pimpl->pushHostParam (slot, bits))
        pimpl->droppedEventCount.fetch_add (1, std::memory_order_relaxed);
}

int64_t YdspAudioGraph::getIntParameter (StringRef qualifiedName) const noexcept
{
    return getIntParameterBySlot (getParameterSlot (qualifiedName));
}

int64_t YdspAudioGraph::getIntParameterBySlot (int slot) const noexcept
{
    if (pimpl == nullptr)
        return 0;

    if (slot < 0 || static_cast<size_t> (slot) >= pimpl->paramSlotTypes.size()
        || pimpl->paramSlotTypes[static_cast<size_t> (slot)] != YdspValueType::int64Type)
        return 0;

    int64_t value = 0;
    const auto bits = pimpl->parameterSnapshots[static_cast<size_t> (slot)].load (std::memory_order_relaxed);
    std::memcpy (&value, &bits, sizeof (value));
    return value;
}

void YdspAudioGraph::setIntParameter (StringRef qualifiedName, int64_t value)
{
    setIntParameterBySlot (getParameterSlot (qualifiedName), value);
}

void YdspAudioGraph::setIntParameterBySlot (int slot, int64_t value)
{
    if (pimpl == nullptr)
        return;

    if (slot < 0 || static_cast<size_t> (slot) >= pimpl->paramSlotTypes.size()
        || pimpl->paramSlotTypes[static_cast<size_t> (slot)] != YdspValueType::int64Type)
        return;

    uint64_t bits = 0;
    std::memcpy (&bits, &value, sizeof (value));

    if (! pimpl->pushHostParam (slot, bits))
        pimpl->droppedEventCount.fetch_add (1, std::memory_order_relaxed);
}

int YdspAudioGraph::getParameterSlot (StringRef qualifiedName) const noexcept
{
    return pimpl != nullptr ? pimpl->paramSlot (qualifiedName) : -1;
}

bool YdspAudioGraph::supportsSampleAccurateAutomation (int slot) const noexcept
{
    if (pimpl == nullptr || slot < 0
        || static_cast<size_t> (slot) >= pimpl->paramSlotTypes.size()
        || pimpl->paramSlotTypes[static_cast<size_t> (slot)] != YdspValueType::float32Type)
        return false;

    for (const auto& [nodeIndex, localIndex] : pimpl->paramSlotToNode[static_cast<size_t> (slot)])
    {
        if (nodeIndex < 0 || localIndex < 0)
            continue;

        const auto& node = pimpl->nodes[static_cast<size_t> (nodeIndex)];
        if (node.rateMultiplier != 1 || node.rateDivider != 1)
            return false;
    }

    return true;
}

uint64_t YdspAudioGraph::getDroppedEventCount() const noexcept
{
    return pimpl != nullptr ? pimpl->droppedEventCount.load (std::memory_order_relaxed) : 0;
}

uint64_t YdspAudioGraph::getDroppedOutputEventCount() const noexcept
{
    if (pimpl == nullptr)
        return 0;

    uint64_t total = 0;

    for (const auto& node : pimpl->nodes)
        total += node.outputEventQueue.droppedCount.load (std::memory_order_relaxed);

    return total;
}

int YdspAudioGraph::getOutputValueCount() const noexcept
{
    return pimpl != nullptr ? static_cast<int> (pimpl->meterSlotNames.size()) : 0;
}

int YdspAudioGraph::getOutputValueSlot (StringRef qualifiedName) const noexcept
{
    return pimpl != nullptr ? pimpl->meterSlot (qualifiedName) : -1;
}

String YdspAudioGraph::getOutputValueName (int slot) const noexcept
{
    if (pimpl == nullptr || slot < 0 || static_cast<size_t> (slot) >= pimpl->meterSlotNames.size())
        return {};

    return pimpl->meterSlotNames[static_cast<size_t> (slot)];
}

float YdspAudioGraph::getOutputValue (StringRef qualifiedName) const noexcept
{
    return getOutputValueBySlot (getOutputValueSlot (qualifiedName));
}

float YdspAudioGraph::getOutputValueBySlot (int slot) const noexcept
{
    if (pimpl == nullptr)
        return 0.0f;

    if (slot < 0 || static_cast<size_t> (slot) >= pimpl->meterSlotTypes.size()
        || static_cast<size_t> (slot) >= pimpl->meterOffsets.size()
        || pimpl->meterSlotTypes[static_cast<size_t> (slot)] != YdspValueType::float32Type)
        return 0.0f;

    float value = 0.0f;
    const auto bits = pimpl->meterSnapshots[static_cast<size_t> (slot)].load (std::memory_order_relaxed);
    std::memcpy (&value, &bits, sizeof (value));
    return value;
}

double YdspAudioGraph::getDoubleOutputValue (StringRef qualifiedName) const noexcept
{
    return getDoubleOutputValueBySlot (getOutputValueSlot (qualifiedName));
}

double YdspAudioGraph::getDoubleOutputValueBySlot (int slot) const noexcept
{
    if (pimpl == nullptr)
        return 0.0;

    if (slot < 0 || static_cast<size_t> (slot) >= pimpl->meterSlotTypes.size()
        || static_cast<size_t> (slot) >= pimpl->meterOffsets.size()
        || pimpl->meterSlotTypes[static_cast<size_t> (slot)] != YdspValueType::float64Type)
        return 0.0;

    double value = 0.0;
    const auto bits = pimpl->meterSnapshots[static_cast<size_t> (slot)].load (std::memory_order_relaxed);
    std::memcpy (&value, &bits, sizeof (value));
    return value;
}

int64_t YdspAudioGraph::getIntOutputValue (StringRef qualifiedName) const noexcept
{
    return getIntOutputValueBySlot (getOutputValueSlot (qualifiedName));
}

int64_t YdspAudioGraph::getIntOutputValueBySlot (int slot) const noexcept
{
    if (pimpl == nullptr)
        return 0;

    if (slot < 0 || static_cast<size_t> (slot) >= pimpl->meterSlotTypes.size()
        || static_cast<size_t> (slot) >= pimpl->meterOffsets.size()
        || pimpl->meterSlotTypes[static_cast<size_t> (slot)] != YdspValueType::int64Type)
        return 0;

    int64_t value = 0;
    const auto bits = pimpl->meterSnapshots[static_cast<size_t> (slot)].load (std::memory_order_relaxed);
    std::memcpy (&value, &bits, sizeof (value));
    return value;
}

int YdspAudioGraph::getActiveVoiceCount (StringRef nodeName) const noexcept
{
    if (pimpl == nullptr)
        return 0;

    const String name (nodeName);

    for (const auto& node : pimpl->nodes)
    {
        if (node.instanceName != name)
            continue;

        const auto& group = pimpl->voiceGroups[static_cast<size_t> (node.groupIndex)];

        int count = 0;

        for (int v = 0; v < group.voiceCount; ++v)
            if (! pimpl->voiceIsIdle (node, v))
                ++count;

        return count;
    }

    for (const auto& group : pimpl->voiceGroups)
    {
        if (group.name != name)
            continue;

        int activityMember = -1;

        for (const int member : group.members)
        {
            if (pimpl->nodes[static_cast<size_t> (member)].activityByteOffset >= 0)
            {
                activityMember = member;
                break;
            }
        }

        if (activityMember < 0)
        {
            int count = 0;

            for (const auto& slot : group.voiceSlots)
                if (slot.held)
                    ++count;

            return count;
        }

        const auto& node = pimpl->nodes[static_cast<size_t> (activityMember)];

        int count = 0;

        for (int v = 0; v < group.voiceCount; ++v)
            if (! pimpl->voiceIsIdle (node, v))
                ++count;

        return count;
    }

    return 0;
}

const YdspExecutionReport& YdspAudioGraph::getExecutionReport() const noexcept
{
    static YdspExecutionReport empty;

    return pimpl != nullptr ? pimpl->report : empty;
}

const YdspDiagnostics& YdspAudioGraph::getDiagnostics() const noexcept
{
    static YdspDiagnostics empty;

    return pimpl != nullptr ? pimpl->diagnostics : empty;
}

} // namespace yup
