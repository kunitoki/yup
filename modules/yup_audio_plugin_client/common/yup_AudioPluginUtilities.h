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

#include <limits>

//==============================================================================
namespace yup
{

/** Returns a hex string describing a pointer, for debug logging. */
inline String describePointer (const void* value)
{
    return "0x" + String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (value)));
}

/** Applies an editor's size constraint once the user stops resizing the host window.

    While the host window edges are dragged, wrappers fill the host area and fit the editor inside it,
    because constraining every intermediate size makes the host window jump between the dragged and the
    constrained size. Once the mouse button is released, onSnap is called so the wrapper can resize the
    host window to the constrained size in a single step.
*/
class HostResizeSnapper final : private Timer
{
public:
    explicit HostResizeSnapper (std::function<void()> onSnap)
        : onSnap (std::move (onSnap))
    {
    }

    /** Call whenever the host resizes the editor area. */
    void hostResized() { startTimer (100); }

    /** Cancels a pending snap. */
    void cancel() { stopTimer(); }

private:
    void timerCallback() override
    {
        if (Desktop::getInstance()->isMouseButtonDown())
            return;

        stopTimer();

        if (onSnap)
            onSnap();
    }

    std::function<void()> onSnap;
};

/** Returns the first host-facing parameter ID not used by the processor.

    This is useful for wrapper-owned synthetic parameters, such as bypass, that
    must not collide with plugin-authored stable automation IDs.
*/
inline uint32 findFirstUnusedHostParameterID (const AudioProcessor& processor, uint32 preferredParameterID)
{
    auto parameterID = preferredParameterID;

    while (processor.getParameterByHostID (parameterID) != nullptr
           && parameterID < AudioParameter::maximumHostParameterID)
    {
        ++parameterID;
    }

    jassert (parameterID <= AudioParameter::maximumHostParameterID);
    jassert (processor.getParameterByHostID (parameterID) == nullptr);

    return parameterID;
}

/** Returns a host-facing parameter ID suitable for a wrapper-owned bypass parameter. */
inline uint32 getBypassHostParameterID (const AudioProcessor& processor)
{
    return findFirstUnusedHostParameterID (processor, static_cast<uint32> (processor.getParameters().size()));
}

/** The host-facing bypass control of a plugin wrapper.

    When the processor designates a bypass parameter (AudioParameterBuilder::withBypass) that
    parameter is the bypass control and holds the state. Otherwise the wrapper exposes a synthetic
    parameter, with an ID from getBypassHostParameterID(), and the state lives here.

    Reads and writes are lock-free and safe on the audio thread.
*/
class WrapperBypass
{
public:
    /** Binds to the processor, call again when its parameters change. */
    void attach (const AudioProcessor& processor)
    {
        parameter = processor.getBypassParameter();
        hostParameterID = parameter != nullptr ? parameter->getHostParameterID() : getBypassHostParameterID (processor);
    }

    /** Returns true when the processor owns the bypass parameter. */
    bool isProcessorOwned() const noexcept { return parameter != nullptr; }

    /** Returns the host-facing ID of the bypass control. */
    uint32 getHostParameterID() const noexcept { return hostParameterID; }

    /** Returns true when the ID refers to the wrapper's synthetic bypass parameter. */
    bool isSyntheticParameter (uint32 parameterID) const noexcept { return parameter == nullptr && parameterID == hostParameterID; }

    /** Returns true when the bypass is engaged. */
    bool isBypassed() const noexcept
    {
        if (parameter != nullptr)
            return parameter->getNormalizedValue() >= 0.5f;

        return bypassed.load (std::memory_order_relaxed);
    }

    /** Engages or releases the bypass without notifying listeners. */
    void setBypassed (bool shouldBeBypassed) noexcept
    {
        bypassed.store (shouldBeBypassed, std::memory_order_relaxed);

        if (parameter != nullptr)
            parameter->setNormalizedValue (shouldBeBypassed ? 1.0f : 0.0f);
    }

private:
    AudioParameter::Ptr parameter;
    uint32 hostParameterID = 0;
    std::atomic<bool> bypassed { false };
};

/** Returns the total number of output audio channels across all output audio buses. */
inline int getTotalAudioOutputChannels (const AudioProcessor& processor)
{
    int count = 0;

    for (const auto& bus : processor.getBusLayout().getOutputBuses())
        if (bus.getType() == AudioBus::Type::Audio)
            count += bus.getNumChannels();

    return count;
}

/** Returns the total number of input audio channels across all input audio buses. */
inline int getTotalAudioInputChannels (const AudioProcessor& processor)
{
    int count = 0;

    for (const auto& bus : processor.getBusLayout().getInputBuses())
        if (bus.getType() == AudioBus::Type::Audio)
            count += bus.getNumChannels();

    return count;
}

/** Detects the host application from its reported name (when the format provides one) and
    the current process, and declares it through AudioProcessor::setHostType().

    A host detected earlier is kept when the new detection finds nothing.
*/
inline void updatePluginHostType (StringRef hostName = {})
{
    const auto hostType = PluginHostType::detectFromCurrentProcess (hostName);

    if (hostType.getHost() != PluginHostType::Host::unknown)
        AudioProcessor::setHostType (hostType);
}

/** Declares the wrapper type of the current binary and the host application, then creates the plugin processor.

    Wrappers must create processors through this, so processor constructors can query
    AudioProcessor::getWrapperType() and AudioProcessor::getHostType().
*/
template <class CreateFunction>
AudioProcessor* createPluginProcessorForWrapper (AudioPluginWrapperType wrapperType, CreateFunction&& createFunction, StringRef hostName = {})
{
    AudioProcessor::setWrapperType (wrapperType);

    if (wrapperType != AudioPluginWrapperType::standalone)
        updatePluginHostType (hostName);

    return createFunction();
}

/** Returns the default automation-event capacity used by plugin wrappers. */
inline int getDefaultParameterChangeCapacity (const AudioProcessor& processor)
{
    return static_cast<int> (processor.getParameters().size()) * 4 + 32;
}

/** Calls the processor's normal or bypass processing path for a prepared context. */
template <typename Context>
void processAudioBlock (AudioProcessor& processor, Context& context, bool isBypassed)
{
    if (isBypassed)
        processor.processBlockBypassed (context);
    else
        processor.processBlock (context);
}

/** Wrapper-owned state that can be stored alongside processor state. */
struct WrapperBypassState
{
    bool hasWrapperState = false;
    bool isBypassed = false;
    bool hasProcessorState = false;
    MemoryBlock processorState;
};

/** Writes wrapper bypass state and optional processor state using a format-specific magic. */
inline MemoryBlock writeWrapperBypassState (int magic,
                                            int version,
                                            bool isBypassed,
                                            const MemoryBlock& processorState,
                                            bool hasProcessorState)
{
    MemoryBlock data;
    MemoryOutputStream output (data, false);
    output.writeInt (magic);
    output.writeInt (version);
    output.writeBool (isBypassed);
    output.writeBool (hasProcessorState);
    output.writeInt64 (hasProcessorState ? static_cast<int64> (processorState.getSize()) : 0);

    if (hasProcessorState && ! processorState.isEmpty())
        output.write (processorState.getData(), processorState.getSize());

    output.flush();
    return data;
}

/** Reads wrapper bypass state, falling back to legacy raw processor state. */
inline WrapperBypassState readWrapperBypassState (const MemoryBlock& data, int magic, int version)
{
    WrapperBypassState result;

    MemoryInputStream input (data, false);

    if (input.readInt() != magic
        || input.readInt() != version)
    {
        result.processorState = data;
        return result;
    }

    result.hasWrapperState = true;
    result.isBypassed = input.readBool();
    result.hasProcessorState = input.readBool();

    const auto processorStateSize = input.readInt64();
    if (processorStateSize < 0
        || processorStateSize > input.getNumBytesRemaining()
        || processorStateSize > static_cast<int64> (std::numeric_limits<int>::max()))
    {
        result.hasWrapperState = false;
        result.hasProcessorState = false;
        result.processorState = data;
        return result;
    }

    result.processorState.setSize (static_cast<size_t> (processorStateSize));

    const auto bytesToRead = static_cast<int> (processorStateSize);
    if (bytesToRead > 0
        && input.read (result.processorState.getData(), bytesToRead) != bytesToRead)
    {
        result.hasWrapperState = false;
        result.hasProcessorState = false;
        result.processorState = data;
    }

    return result;
}

/** Adds a normalized automation change for a host-facing parameter ID.

    @returns true if the host parameter ID mapped to a processor parameter and
             the change was added to the buffer.
*/
inline bool addParameterChangeByHostParameterID (AudioProcessor& processor,
                                                 ParameterChangeBuffer& changes,
                                                 uint32 hostParameterID,
                                                 float normalizedValue,
                                                 int sampleOffset)
{
    const auto parameters = processor.getParameters();
    const auto parameterIndex = processor.getParameterIndexByHostID (hostParameterID);

    if (! isPositiveAndBelow (parameterIndex, static_cast<int> (parameters.size())))
        return false;

    return changes.addChange (parameterIndex, normalizedValue, sampleOffset);
}

/** Applies the last known normalized values in a parameter change buffer.

    Wrappers use this after collecting sample-accurate changes so processors that
    still read AudioParameter atomics directly see the latest host value.
*/
inline void applyParameterChangesToProcessor (AudioProcessor& processor, const ParameterChangeBuffer& changes)
{
    const auto parameters = processor.getParameters();

    for (const auto& change : changes)
    {
        if (isPositiveAndBelow (change.parameterIndex, static_cast<int> (parameters.size())))
            parameters[change.parameterIndex]->setNormalizedValue (change.normalizedValue);
    }
}

/** Ends any active parameter gestures before a plugin wrapper tears down its processor. */
inline void endActiveParameterGestures (AudioProcessor* processor)
{
    if (processor == nullptr)
        return;

    for (auto& parameter : processor->getParameters())
    {
        while (parameter->isPerformingChangeGesture())
            parameter->endChangeGesture();
    }
}

} // namespace yup
