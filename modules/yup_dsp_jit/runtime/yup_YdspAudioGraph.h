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

// Forward declaration (defined in compiler/yup_YdspDiagnostics.h).
class YdspDiagnostics;

//==============================================================================
/** A compiled YDSP patch ready to run in the audio callback.

    Creation (YdspCompiler::compile) and prepare() must happen on the
    control thread. process() is the realtime entry point: it performs no
    heap allocation, takes no locks, and never throws.

    @see YdspCompiler
*/
class YUP_API YdspAudioGraph
{
public:
    /** Constructs an invalid graph (as returned by a failed compile). */
    YdspAudioGraph();

    /** Destructor. */
    ~YdspAudioGraph();

    YdspAudioGraph (YdspAudioGraph&&) noexcept;
    YdspAudioGraph& operator= (YdspAudioGraph&&) noexcept;

    //==============================================================================
    /** Switches MIDI ingestion to MPE, using the given zone layout.

        The patch itself is unaware of the distinction: the same `noteOn`,
        `pitchBend`, `pressure` and `slide` events arrive either way, and note
        expression is always addressed to the voice that owns the note. Under
        MPE the number of concurrently sounding notes is capped by the zone's
        member-channel count, independently of a node's `[N]` voice count.

        Must be called from the control thread, never concurrently with
        process(). Every playing note is discarded and its voice slot freed,
        but voice *state* is left alone - call reset() as well if the mode is
        switched while audio is running and ringing tails are unwanted.

        @see setLegacyMidiMode
    */
    void setMpeZoneLayout (const yup::MPEZoneLayout& layout);

    /** Switches MIDI ingestion to plain (non-MPE) MIDI. This is the default.

        Channel-wide pitch bend and pressure are broadcast to every note on the
        channel, and polyphonic aftertouch folds into the affected note's
        `pressure`.

        Must be called from the control thread, never concurrently with
        process(); same note-discarding caveat as setMpeZoneLayout().

        @param pitchbendRangeSemitones  the channel pitch-bend range, in
               semitones (0 to 96); the MIDI default is 2.

        @see setMpeZoneLayout
    */
    void setLegacyMidiMode (int pitchbendRangeSemitones = 2);

    //==============================================================================
    /** Returns true if this graph was produced by a successful compile. */
    bool isValid() const noexcept;

    /** Preallocates all state, buffer and parameter memory.

        Must be called before process() (once per sample rate / block size).
        Sample rate is stored as a float in generated code.

        @param maxEventsPerVoicePerBlock  fixed capacity of each voice's
            per-block pending-event queue; overflowing events are dropped and
            counted in getDroppedEventCount() (never allocated on the audio
            thread).
        @param maxAutomationPerNodePerBlock  fixed capacity of each node's
            per-block pending-automation queue (same overflow semantics).
        @param maxOutputEventsPerBlock  fixed capacity of each node's
            per-block output-event queue; overflowing `emit`s are dropped and
            counted in getDroppedOutputEventCount() (same overflow semantics).
    */
    void prepare (double sampleRate, int maxBlockSize, int maxEventsPerVoicePerBlock = 64, int maxAutomationPerNodePerBlock = 32, int maxOutputEventsPerBlock = 32);

    /** Re-runs the patch's init/reset sequence.

        Zeroes every node's state memory and re-executes the processors' one-shot
        init kernels (in topological order), restoring the patch to its freshly
        prepared state. Parameter values are left untouched. Safe to call between
        process() blocks (not from the realtime audio thread).
    */
    void reset();

    /** Pre-registers the compiled wasm kernels in the calling JS realm.

        On wasm targets each JS realm (the main thread, and the audio-worklet
        thread, which is its own realm) instantiates its own copy of the
        generated kernels. This call registers every kernel in the current
        realm up front, so the first audio block does not pay the one-time
        instantiation cost. It is idempotent (kernels already registered are
        left untouched) and a no-op on desktop, where kernels are native code
        and are always invoked directly.

        @warning This is a no-op on anything else than Emscripten/WebAssembly.
    */
    void prewarmKernels();

    /** Runs the patch for one block. Realtime-safe (no allocation, no locking).

        Stream buffers are passed as typed spans (one per declared stream, in
        declaration order): the active alternative of each YdspInputBuffer /
        YdspOutputBuffer must match the stream's declared element type
        (getInputStreamType/getOutputStreamType). On a mismatch the call is
        ignored and the problem is reported through the returned error value
        (never through asserts or exceptions, so hosts may probe freely).

        @param inputs   one YdspInputBuffer per graph input stream, in
                        declaration order (empty for a patch with no inputs,
                        e.g. a MIDI-only synth)
        @param outputs  one YdspOutputBuffer per graph output stream, in
                        declaration order
        @param numSamples  number of samples in this block (must be <= the
                        maxBlockSize passed to prepare(); every buffer must
                        hold at least numSamples elements)

        @returns YdspProcessResult::ok when the block was processed;
                 a descriptive error otherwise (the graph was not touched).
    */
    YdspProcessResult process (yup::Span<const YdspInputBuffer> inputs,
                                 yup::Span<YdspOutputBuffer> outputs,
                                 int numSamples);

    /** Runs the patch for one block with MIDI and sample-accurate parameter
        automation. Realtime-safe (no allocation, no locking).

        `midiIn` is decoded into the patch's event shapes: notes are voice
        allocated per the node's `[[ mode / stealing / priority ]]`
        annotations, note expression (pitch bend, pressure, slide) reaches only
        the voice that owns the note, and CC / program change are broadcast to
        every voice. Each event is dispatched at its exact sample offset within
        the block via runtime sub-block splitting. Automation events are
        applied as exact-sample step changes to their params using the same
        mechanism.

        Sustain, sostenuto, reset-all-controllers and all-notes-off are honoured
        as ordinary note lifecycle; all-sound-off (CC120) additionally silences
        and re-initialises every event-driven node at its sample offset.

        This overload feeds `midiIn` to the patch's first `input event` (or
        ignores it when the patch declares none); use the span-based overload
        below to feed each named event input separately.

        Passing null/empty for both `midiIn` and `automation` is equivalent to
        the 3-argument process() overload (and takes the identical fast path).

        @param inputs   one YdspInputBuffer per graph input stream (see the
                        3-argument overload)
        @param outputs  one YdspOutputBuffer per graph output stream
        @param numSamples  number of samples in this block
        @param midiIn      incoming MIDI buffer (may be nullptr); events must
                        be sorted by timestamp, in sample order
        @param automation array of YdspAutomationEvent (may be nullptr)
        @param numAutomationEvents  number of automation events

        @returns YdspProcessResult::ok when the block was processed;
                 a descriptive error otherwise (the graph was not touched).
    */
    YdspProcessResult process (yup::Span<const YdspInputBuffer> inputs,
                                 yup::Span<YdspOutputBuffer> outputs,
                                 int numSamples,
                                 const yup::MidiBuffer* midiIn,
                                 const YdspAutomationEvent* automation,
                                 int numAutomationEvents);

    /** Runs the patch for one block with per-input MIDI and sample-accurate
        parameter automation. Realtime-safe (no allocation, no locking).

        Identical to the single-buffer overload above, except that `eventInputs`
        carries one `MidiBuffer` per graph event input, in declaration order
        (query them with getEventInputCount()/getEventInputName()). Each input
        is decoded and routed independently, reaching only the nodes the patch
        explicitly wired it to (`graphInput -> node.inputEvent;` in its
        `connection { }` block) - there is no implicit delivery by matching
        endpoint names. A shorter span, or a null entry, means no events on
        that input; passing more buffers than the patch declares is an error.

        @param eventInputs  one MidiBuffer per graph event input, in
                            declaration order; may be shorter or contain nulls
    */
    YdspProcessResult process (yup::Span<const YdspInputBuffer> inputs,
                                 yup::Span<YdspOutputBuffer> outputs,
                                 int numSamples,
                                 yup::Span<const yup::MidiBuffer*> eventInputs,
                                 const YdspAutomationEvent* automation,
                                 int numAutomationEvents);

    /** Runs the patch for one block with MIDI, sample-accurate parameter
        automation, and routed output-event collection. Realtime-safe (no
        allocation, no locking).

        Identical to the single-buffer overload above, with one addition:
        `midiOut` may be non-null to collect events reaching any graph-scope
        `output event` endpoint during this block, each at its
        latency-compensated sample offset.

        @param midiIn      incoming MIDI buffer (may be nullptr); see the
                        single-buffer overload above
        @param automation array of YdspAutomationEvent (may be nullptr)
        @param numAutomationEvents  number of automation events
        @param midiOut     buffer to receive routed/boundary output events for
                        this block (may be nullptr to collect nothing)

        @returns YdspProcessResult::ok when the block was processed;
                 a descriptive error otherwise (the graph was not touched).
    */
    YdspProcessResult process (yup::Span<const YdspInputBuffer> inputs,
                                 yup::Span<YdspOutputBuffer> outputs,
                                 int numSamples,
                                 const yup::MidiBuffer* midiIn,
                                 const YdspAutomationEvent* automation,
                                 int numAutomationEvents,
                                 yup::MidiBuffer* midiOut);

    /** Runs the patch for one block with per-input MIDI, sample-accurate
        parameter automation, and routed output-event collection. Realtime-safe
        (no allocation, no locking).

        Identical to the span-based overload above, with one addition:
        `midiOut` may be non-null to collect events reaching any graph-scope
        `output event` endpoint during this block, each at its
        latency-compensated sample offset.

        @param eventInputs  one MidiBuffer per graph event input, in
                            declaration order; may be shorter or contain nulls
        @param automation array of YdspAutomationEvent (may be nullptr)
        @param numAutomationEvents  number of automation events
        @param midiOut     buffer to receive routed/boundary output events for
                        this block (may be nullptr to collect nothing)

        @returns YdspProcessResult::ok when the block was processed;
                 a descriptive error otherwise (the graph was not touched).
    */
    YdspProcessResult process (yup::Span<const YdspInputBuffer> inputs,
                                 yup::Span<YdspOutputBuffer> outputs,
                                 int numSamples,
                                 yup::Span<const yup::MidiBuffer*> eventInputs,
                                 const YdspAutomationEvent* automation,
                                 int numAutomationEvents,
                                 yup::MidiBuffer* midiOut);

    //==============================================================================
    /** Returns the patch's latency in samples, for the host's delay compensation.

        This is the figure to hand to AudioProcessorBase::setLatencySamples().
        It counts only *artifact* latency - the group delay an oversampled node
        leaks, and any `[[ latency: N ]]` a processor declares - and the compiler
        has already inserted the per-edge delays that align every path inside the
        patch, so nothing here is left for the caller to correct.

        A delay the patch author wrote by hand (`-> [N] ->`, `x @ N`) is the
        effect rather than a defect, and is deliberately *not* counted: a dry/wet
        delay reports 0, because telling the host that a 500 ms echo is 500 ms of
        plugin latency would make every other track late.

        The value is a compile-time constant. A YDSP graph is a fixed DAG - no
        parameter can reroute it or change an oversampling factor - so latency
        cannot vary while a compiled patch is running, and hosts never have to be
        told it changed. A plugin that offers, say, an oversampling selector
        implements it by recompiling the patch on the control thread and calling
        setLatencySamples() again with the new figure.
    */
    int getLatencySamples() const noexcept;

    //==============================================================================
    /** Returns how many voices of a node will run on the next block.

        Counts the voices the scheduler would not skip: a voice is inactive only
        when its processor declares a `state int x [[ role: voiceActivity ]]`
        flag, that flag currently reads 0, and the voice's key is not held. A
        processor that declares no such flag never sleeps a voice, so this
        returns its full voice count. Returns 0 for an unknown node name.

        Useful as a voice meter, and the observable side of the voice-skipping
        optimisation: this shares the scheduler's predicate exactly, so the
        reported count can never disagree with what actually ran.
    */
    int getActiveVoiceCount (StringRef nodeName) const noexcept;

    /** Returns the number of MIDI/automation events dropped so far because a
        per-voice or per-node per-block queue overflowed its fixed capacity.

        The host may log this count to warn the user that the patch is being
        fed more events than it can handle. The count is reset to 0 on prepare()
        and reset(), so it is always the number of events dropped since the last
        prepare() or reset() call. The count is never decremented, so it is safe
        to read from the audio thread without locking, and it is never reset on
        process() (which would be racy) - the host must call reset() to clear it.
    */
    uint64_t getDroppedEventCount() const noexcept;

    /** Returns the number of `emit`ted output events dropped so far because a
        node's per-block output-event queue overflowed its fixed capacity
        (`maxOutputEventsPerBlock` on prepare()).

        Unlike getDroppedEventCount(), which reads one graph-wide counter,
        this sums each node's own counter, since every node owns its output-
        event queue independently. Read from the audio thread without
        locking; never decremented.
    */
    uint64_t getDroppedOutputEventCount() const noexcept;

    //==============================================================================
    /** Returns the number of input streams declared by the graph. */
    int getInputStreamCount() const noexcept;

    /** Returns the element type of a graph input stream (0-based index). */
    YdspElementType getInputStreamType (int index) const noexcept;

    /** Returns the number of output streams declared by the graph. */
    int getOutputStreamCount() const noexcept;

    /** Returns the element type of a graph output stream (0-based index). */
    YdspElementType getOutputStreamType (int index) const noexcept;

    /** Returns the number of event inputs declared by the graph. */
    int getEventInputCount() const noexcept;

    /** Returns the name of the event input at the given index (declaration
        order), or an empty string when the index is out of range. */
    String getEventInputName (int index) const noexcept;

    //==============================================================================
    /** Returns the number of parameters (graph-level and per-node). */
    int getParameterCount() const noexcept;

    /** Returns the metadata of the parameter at the given slot.

        Slots are stable for the lifetime of the graph and match the value
        returned by getParameterSlot(); iterate from 0 to getParameterCount() - 1 to
        build a host UI. Returns an empty default when the slot is invalid.
    */
    const YdspParameterInfo& getParameterInfo (int slot) const noexcept;

    /** Returns the element type of a parameter ("node.param" or "param"). */
    YdspElementType getParameterType (StringRef qualifiedName) const noexcept;

    /** Resolves a parameter's integer slot for sample-accurate automation.

        Call once on the control thread, before the audio-thread process()
        loop begins, then hand the returned slot to YdspAutomationEvent.
        Returns -1 when the parameter does not exist.
    */
    int getParameterSlot (StringRef qualifiedName) const noexcept;

    /** Returns true if a parameter with the given qualified name exists. */
    bool hasParameter (StringRef qualifiedName) const noexcept;

    /** Returns the current value of a float32 parameter ("node.param" or "param").
        Returns 0.0f when the parameter is missing or not float32-typed. */
    float getParameter (StringRef qualifiedName) const noexcept;

    /** Sets a float32 parameter value from the host thread.

        Parameters are sampled once per block by the generated kernels, so a
        value written here is picked up at the start of the next audio block.
        Must be called from the control thread, never concurrently with
        process(). No-op for non-float32 parameters (use setDoubleParameter/setIntParameter).
    */
    void setParameter (StringRef qualifiedName, float value);

    /** Returns the current value of a float64 parameter, or 0.0 when missing. */
    double getDoubleParameter (StringRef qualifiedName) const noexcept;

    /** Sets a float64 parameter value (no-op for non-float64 parameters). */
    void setDoubleParameter (StringRef qualifiedName, double value);

    /** Returns the current value of an int64 parameter, or 0 when missing. */
    int64_t getIntParameter (StringRef qualifiedName) const noexcept;

    /** Sets an int64 parameter value (no-op for non-int64 parameters). */
    void setIntParameter (StringRef qualifiedName, int64_t value);

    //==============================================================================
    /** Returns the number of `output value` endpoints (meters) the patch declares.

        Slots are stable for the lifetime of the graph; iterate from 0 to
        getOutputValueCount() - 1 with getOutputValueName() to build a meter UI
        without knowing the patch's meter names up front.
    */
    int getOutputValueCount() const noexcept;

    /** Returns the host-facing name of the meter at the given slot.

        The name is the one to pass to getOutputValue(): a node meter reads as
        "node.meter", and a meter routed to a graph-scope `output value` reads as
        that graph-level name instead. Returns an empty string for an invalid slot.
    */
    String getOutputValueName (int slot) const noexcept;

    /** Returns the current value of a float32 output value endpoint (meter). */
    float getOutputValue (StringRef qualifiedName) const noexcept;

    /** Returns the current value of a float64 meter, or 0.0 when missing. */
    double getDoubleOutputValue (StringRef qualifiedName) const noexcept;

    /** Returns the current value of an int64 meter, or 0 when missing. */
    int64_t getIntOutputValue (StringRef qualifiedName) const noexcept;

    //==============================================================================
    /** Returns the optimiser's worst-case execution report. */
    const YdspExecutionReport& getExecutionReport() const noexcept;

    /** Returns the diagnostics produced during compilation (empty on success). */
    const YdspDiagnostics& getDiagnostics() const noexcept;

private:
    friend class YdspCompiler;

    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspAudioGraph)
};

} // namespace yup
