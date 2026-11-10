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
#include <optional>

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

/** Converts editor sizes between YUP points and the units hosts use for plugin views.

    Cocoa hosts size views in points like YUP. Win32 and X11 hosts size them in physical pixels, which the
    editor window maps to points with its display scale. Until the editor window exists, the scale suggested
    by the host (VST3 setContentScaleFactor, CLAP set_scale) or the primary screen scale is used.
*/
class WrapperViewScale
{
public:
    /** Stores the scale suggested by the host, returns true if it changed. */
    bool setHostScale (float newScale)
    {
        if (newScale <= 0.0f || hostScale == newScale)
            return false;

        hostScale = newScale;
        return true;
    }

    /** Returns the host view units per editor point. */
    float get ([[maybe_unused]] const Component& editor) const
    {
#if YUP_MAC
        return 1.0f;
#else
        if (editor.isOnDesktop())
            return editor.getScaleDpi();

        if (hostScale.has_value())
            return *hostScale;

        if (auto screen = Desktop::getInstance()->getPrimaryScreen())
            return screen->contentScaleX > 0.0f ? screen->contentScaleX : 1.0f;

        return 1.0f;
#endif
    }

    /** Converts an editor size to host view units. */
    Size<int> toHost (Size<int> size, const Component& editor) const
    {
        const auto scale = get (editor);
        return { jmax (1, roundToInt (size.getWidth() * scale)), jmax (1, roundToInt (size.getHeight() * scale)) };
    }

    /** Converts a host view size to editor points. */
    Size<int> fromHost (Size<int> size, const Component& editor) const
    {
        const auto scale = get (editor);
        return { jmax (1, roundToInt (size.getWidth() / scale)), jmax (1, roundToInt (size.getHeight() / scale)) };
    }

private:
    std::optional<float> hostScale;
};

#if YUP_LINUX
/** Interval of the host timer that services YUP messages on Linux, where the host owns the event loop. */
inline constexpr int hostMessageTimerIntervalMs = 16;

/** Services pending YUP messages from a host GUI thread timer without blocking it. */
inline void dispatchPendingMessagesFromHostTimer()
{
    // Windowing is initialised lazily, there is nothing to service before that
    if (auto* messageManager = MessageManager::getInstanceWithoutCreating(); messageManager != nullptr && messageManager->isThisTheMessageThread())
        messageManager->runDispatchLoopUntil (0);
}
#endif

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

/** Starts the windowing subsystem for a plugin instance the first time it opens an editor.

    Hosts may create plugin instances off the main thread (scanners often do), while windowing
    can only start on the main thread, so it is not started on construction. Once started it
    runs until the instance is destroyed, so it is never shut down while a host closes an editor
    and immediately opens a new one.
*/
class WrapperWindowing
{
public:
    /** Starts windowing if needed. Call on the message thread before creating an editor. */
    void ensureInitialised()
    {
        if (! initialiser.has_value())
            initialiser.emplace();
    }

private:
    std::optional<ScopedYupInitialiser_Windowing> initialiser;
};

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

/** Collects the host's per-note expressions for one process call.

    Processors that opt in (AudioProcessor::supportsNoteExpression) receive the events through
    AudioProcessContext::noteExpressions. For the others the events are folded into the MIDI
    buffer, as MPE when AudioProcessor::supportsMpe() is true.

    clear(), add(), reset() and apply() are allocation-free and safe on the audio thread.
*/
class WrapperNoteExpressions
{
public:
    /** Configures the delivery mode and pre-allocates storage, call while not processing. */
    void prepare (const AudioProcessor& processor, int maxEventsPerBlock = 1024)
    {
        passThrough = processor.supportsNoteExpression();
        expressions.reserve (maxEventsPerBlock);

        converter.setMode (processor.supportsMpe() ? NoteExpressionMidiConverter::Mode::mpe : NoteExpressionMidiConverter::Mode::channelWide);
        converter.prepare (4096 + maxEventsPerBlock * 4);
    }

    /** Forgets active notes, e.g. when the host resets the plugin. */
    void reset() noexcept { converter.reset(); }

    /** Removes the events of the previous block. */
    void clear() noexcept { expressions.clear(); }

    /** Adds an event of the current block. */
    void add (const NoteExpression& expression) noexcept { expressions.add (expression); }

    /** Delivers the collected events, returns the buffer to pass in AudioProcessContext::noteExpressions. */
    const NoteExpressionBuffer* apply (MidiBuffer& midi)
    {
        expressions.sort();

        if (passThrough)
            return &expressions;

        converter.process (midi, expressions);
        return nullptr;
    }

private:
    NoteExpressionBuffer expressions;
    NoteExpressionMidiConverter converter;
    bool passThrough = false;
};

/** Converts a MIDI 2.0 per-note channel voice message (UMP message type 0x4) to a note expression.

    Per-note pitch bend becomes tuning, using the default per-note bend range of 48 semitones.
    Registered per-note controllers map to vibrato (1), volume (7, as linear gain up to unity),
    pan (10), expression (11) and brightness (74). Any other message returns nothing.

    @param words         the message words, at least two for a MIDI 2.0 channel voice message
    @param numWords      the number of valid words
    @param sampleOffset  the event position in the current block
*/
inline std::optional<NoteExpression> umpToNoteExpression (const uint32* words, int numWords, int sampleOffset) noexcept
{
    if (words == nullptr || numWords < 2 || (words[0] >> 28) != 0x4)
        return std::nullopt;

    constexpr double perNotePitchBendRange = 48.0;
    constexpr double center = 2147483648.0;

    const auto status = (words[0] >> 20) & 0xf;
    const auto value = static_cast<double> (words[1]);

    NoteExpression expression;
    expression.sampleOffset = sampleOffset;
    expression.channel = static_cast<int8> (((words[0] >> 16) & 0xf) + 1);
    expression.key = static_cast<int8> ((words[0] >> 8) & 0x7f);

    if (status == 0x6)
    {
        expression.type = NoteExpression::Type::tuning;
        expression.value = static_cast<float> ((value - center) / center * perNotePitchBendRange);
        return expression;
    }

    if (status != 0x0)
        return std::nullopt;

    switch (words[0] & 0xff)
    {
        case 1:
            expression.type = NoteExpression::Type::vibrato;
            break;
        case 7:
            expression.type = NoteExpression::Type::volume;
            break;
        case 10:
            expression.type = NoteExpression::Type::pan;
            break;
        case 11:
            expression.type = NoteExpression::Type::expression;
            break;
        case 74:
            expression.type = NoteExpression::Type::brightness;
            break;
        default:
            return std::nullopt;
    }

    expression.value = static_cast<float> (value / 4294967295.0);
    return expression;
}

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

//==============================================================================

/** Workarounds for hosts that break the plugin format specifications.

    Wrappers query this instead of testing host types inline, so every workaround is listed and
    documented here. Add an entry only for a reproduced misbehavior, naming the host and format.
*/
struct HostQuirks
{
    /** VST3: FL Studio's Patcher calls process() before and during setActive(). */
    bool processMayOverlapActivation = false;

    /** VST3: Adobe Audition CS6 may pass its own corrupted "VC2!E" chunks to setState(). */
    bool mayPassForeignStateChunks = false;

    /** Returns the workarounds needed by a host. */
    static HostQuirks forHost (const PluginHostType& hostType) noexcept
    {
        HostQuirks quirks;
        quirks.processMayOverlapActivation = hostType.isFLStudio();
        quirks.mayPassForeignStateChunks = hostType.getHost() == PluginHostType::Host::adobeAudition;
        return quirks;
    }

    /** Returns the workarounds needed by the host declared through AudioProcessor::setHostType(). */
    static HostQuirks forCurrentHost() noexcept { return forHost (AudioProcessor::getHostType()); }

    /** Returns true when a state chunk must be ignored because the host is known to corrupt it. */
    bool isForeignStateChunk (const MemoryBlock& data) const noexcept
    {
        return mayPassForeignStateChunks && data.getSize() >= 5 && std::memcmp (data.getData(), "VC2!E", 5) == 0;
    }
};

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
