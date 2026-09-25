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

    /** Returns bytes reserved for shared intermediate stream and mix buffers.
        Excludes state, delay lines and per-voice/resampler buffers. Zero before
        prepare(); query only while preparation is not running. */
    size_t getScratchMemorySizeBytes() const noexcept;

    /** Preallocates all state, buffer and parameter memory.

        Must be called before process() (once per sample rate / block size).
        Sample rate must be finite and positive, including at each node's
        processing rate after conversion to float. Block size and all event
        capacities must be positive. Invalid arguments leave the graph unchanged.
        Call only while processing and control access are stopped.

        @param maxEventsPerVoicePerBlock  fixed capacity of each voice's
            per-block pending-event queue; overflowing events are dropped and
            counted in getDroppedEventCount() (never allocated on the audio
            thread).
        @param maxAutomationPerNodePerBlock  fixed capacity of each node's
            per-block pending-automation queue (same overflow semantics).
        @param maxOutputEventsPerBlock  fixed capacity of each node's
            per-block output-event queue; overflowing `emit`s are dropped and
            counted in getDroppedOutputEventCount() (same overflow semantics).

        @returns success, or a descriptive validation failure. Existing calls
                 may ignore this result, but hosts should check it.
    */
    Result prepare (double sampleRate,
                    int maxBlockSize,
                    int maxEventsPerVoicePerBlock = 64,
                    int maxAutomationPerNodePerBlock = 32,
                    int maxOutputEventsPerBlock = 32);

    /** Re-runs the patch's init/reset sequence.

        Zeroes every node's state memory and re-executes the processors' one-shot
        init kernels (in topological order), restoring the patch to its freshly
        prepared state. Parameter values are left untouched. Safe to call between
        process() blocks (not from the realtime audio thread).
    */
    void reset();

    /** Enables or pauses recording in a graph compiled with enableTracing.
        May be called concurrently with process(). Already queued messages remain
        available. Has no effect when tracing was compiled out. */
    void setTracingEnabled (bool enabled) noexcept;

    /** Drains and formats queued trace messages on the calling control thread.
        Never call on the audio thread: this allocates strings. Exactly one
        consumer may drain or print, concurrently with one process() producer.
        Stop both before prepare(), reset(), moving or destroying the graph.
        At most 256 records are drained per call; a full queue drops new records.
        Init, event and process messages share the same queue in execution order. */
    StringArray drainTraceMessages();

    /** Drains and prints trace messages to stdout on the calling control thread.
        Call periodically from a host timer or control loop, never from process().
        Uses the same single-consumer contract as drainTraceMessages(). */
    void printTraceMessages();

    /** Returns the number of trace records dropped since prepare(). Thread-safe. */
    uint64_t getDroppedTraceCount() const noexcept;

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

    /** Processes one block described by non-owning request spans.

        Stream buffers follow declaration order and must match each endpoint's
        type and hold at least request.numSamples elements. Event inputs follow
        getEventInputName() order; missing entries or null buffers carry no MIDI.
        Each input is routed only to explicitly connected nodes. Optional
        midiOut collects latency-compensated graph output events.

        Validation completes before consuming parameter updates or changing
        state, event queues, counters or output buffers. Invalid requests return
        an error without side effects. A valid zero-length request is a no-op.
        prepare() must have succeeded, including for zero-length requests.

        Automation requires valid float32 slots and offsets within the block.
        Slots driving rate-converted nodes return unsupportedAutomation; use
        block-boundary setters for those nodes. Event queue capacity overflow
        during a valid call still drops excess events and increments counters.

        @param request storage must remain valid for the synchronous call
        @returns ok on success, or a validation error with no side effects
    */
    YdspProcessResult process (const YdspProcessRequest& request);

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

    //==============================================================================
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

    /** Returns a conservative byte capacity for a host MidiBuffer receiving one
        process() call's output. Call after prepare(), on the control thread,
        and reserve this many bytes before processing to avoid reallocations.
        The host must clear the buffer before each process() call.
    */
    size_t getMidiOutputBufferSizeBytes() const noexcept;

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

    /** Returns whether a slot accepts sample-accurate float32 automation.
        False for invalid/non-float32 slots or any slot driving a rate-converted
        node. Resolve this capability on the control thread before scheduling. */
    bool supportsSampleAccurateAutomation (int slot) const noexcept;

    /** Returns true if a parameter with the given qualified name exists. */
    bool hasParameter (StringRef qualifiedName) const noexcept;

    /** Returns the last published float32 parameter value ("node.param" or "param").

        Parameter and meter getters may run concurrently with process(). Each
        value is an independent atomic snapshot, published after processing a
        block (initially the compiled default). Reading multiple slots is not
        a coherent whole-graph snapshot. Getters never consume queued updates.
        Returns 0.0f when the parameter is missing or not float32-typed.
    */
    float getParameter (StringRef qualifiedName) const noexcept;

    /** Slot-based getParameter(), with identical snapshot and type-checking semantics.
        Invalid slots return zero. This overload performs no name lookup or allocation. */
    float getParameterBySlot (int slot) const noexcept;

    /** Sets a float32 parameter value from the host thread.

        The value is queued and applied by the audio thread at the start of the
        next process() call, so this is safe to call concurrently with
        process() from one control thread (all setters share a single-producer
        queue; callers must serialize producers). A bounded ring absorbs the
        update; an overfull ring drops the new value and increments
        getDroppedEventCount(). Parameters are
        sampled once per block by the generated kernels. No-op for non-float32
        parameters (use setDoubleParameter/setIntParameter).
    */
    void setParameter (StringRef qualifiedName, float value);

    /** Slot-based setParameter(), with the same single-producer queue contract.
        Invalid slots are ignored. This overload performs no name lookup or allocation. */
    void setParameterBySlot (int slot, float value);

    /** Returns the last published float64 parameter snapshot, or 0.0 when missing.
        Threading and publication semantics are the same as getParameter(). */
    double getDoubleParameter (StringRef qualifiedName) const noexcept;

    /** Slot-based getDoubleParameter(), with identical snapshot and type-checking semantics.
        Invalid slots return zero. This overload performs no name lookup or allocation. */
    double getDoubleParameterBySlot (int slot) const noexcept;

    /** Sets a float64 parameter value (no-op for non-float64 parameters).
        Queued and applied at the start of the next process() call, like
        setParameter(); safe to call concurrently with process(). */
    void setDoubleParameter (StringRef qualifiedName, double value);

    /** Slot-based setDoubleParameter(), with the same single-producer queue contract.
        Invalid slots are ignored. This overload performs no name lookup or allocation. */
    void setDoubleParameterBySlot (int slot, double value);

    /** Returns the last published int64 parameter snapshot, or 0 when missing.
        Threading and publication semantics are the same as getParameter(). */
    int64_t getIntParameter (StringRef qualifiedName) const noexcept;

    /** Slot-based getIntParameter(), with identical snapshot and type-checking semantics.
        Invalid slots return zero. This overload performs no name lookup or allocation. */
    int64_t getIntParameterBySlot (int slot) const noexcept;

    /** Sets an int64 parameter value (no-op for non-int64 parameters).
        Queued and applied at the start of the next process() call, like
        setParameter(); safe to call concurrently with process(). */
    void setIntParameter (StringRef qualifiedName, int64_t value);

    /** Slot-based setIntParameter(), with the same single-producer queue contract.
        Invalid slots are ignored. This overload performs no name lookup or allocation. */
    void setIntParameterBySlot (int slot, int64_t value);

    //==============================================================================
    /** Returns the number of `output parameter` endpoints (meters) the patch declares.

        Slots are stable for the lifetime of the graph; iterate from 0 to
        getOutputValueCount() - 1 with getOutputValueName() to build a meter UI
        without knowing the patch's meter names up front.
    */
    int getOutputValueCount() const noexcept;

    /** Resolves a meter name to a stable slot, or -1 when missing.
        Resolve names before processing, then use slot-based getters to avoid
        name lookup. Slots match getOutputValueName() enumeration. */
    int getOutputValueSlot (StringRef qualifiedName) const noexcept;

    /** Returns the host-facing name of the meter at the given slot.

        The name is the one to pass to getOutputValue(): a node meter reads as
        "node.meter", and a meter routed to a graph-scope `output parameter` reads as
        that graph-level name instead. Returns an empty string for an invalid slot.
    */
    String getOutputValueName (int slot) const noexcept;

    /** Returns a float32 meter snapshot published after the last processed block.
        May run concurrently with process(); slots are published independently. */
    float getOutputValue (StringRef qualifiedName) const noexcept;

    /** Slot-based getOutputValue(), returning the last published value.
        Invalid or mismatched slots return zero. No name lookup or allocation. */
    float getOutputValueBySlot (int slot) const noexcept;

    /** Returns the last published float64 meter snapshot, or 0.0 when missing. */
    double getDoubleOutputValue (StringRef qualifiedName) const noexcept;

    /** Slot-based getDoubleOutputValue(), returning the last published value.
        Invalid or mismatched slots return zero. No name lookup or allocation. */
    double getDoubleOutputValueBySlot (int slot) const noexcept;

    /** Returns the last published int64 meter snapshot, or 0 when missing. */
    int64_t getIntOutputValue (StringRef qualifiedName) const noexcept;

    /** Slot-based getIntOutputValue(), returning the last published value.
        Invalid or mismatched slots return zero. No name lookup or allocation. */
    int64_t getIntOutputValueBySlot (int slot) const noexcept;

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
