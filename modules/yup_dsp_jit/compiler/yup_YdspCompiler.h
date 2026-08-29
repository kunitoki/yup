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
/** Severity of a YDSP compilation diagnostic. */
enum class YdspSeverity
{
    error,
    warning,
    info
};

//==============================================================================
/** A single YDSP compilation diagnostic (message + source location). */
struct YdspDiagnostic
{
    YdspSeverity severity = YdspSeverity::error;
    int line = 0;
    int column = 0;
    String message;
};

//==============================================================================
/** The list of diagnostics produced while compiling a YDSP program.

    Diagnostics are line/column-annotated messages: syntax errors, type
    errors, realtime-safety violations, and informational notes. A compile
    only produces a runnable YdspAudioGraph when no error diagnostics are
    present.
*/
class YdspDiagnostics
{
public:
    /** Default constructor. */
    YdspDiagnostics() = default;

    //==============================================================================
    /** Returns true if any error diagnostics are present. */
    bool hasErrors() const noexcept;

    /** Returns the number of diagnostics. */
    int getCount() const noexcept;

    /** Returns the diagnostic at the given index. */
    const YdspDiagnostic& getItem (int index) const noexcept;

    //==============================================================================
    /** Stores the source text so that toString() can render source lines with
        a caret marker at the diagnostic position. */
    void setSource (StringRef source);

    /** Adds an error diagnostic at the given 1-based line/column. */
    void addError (int line, int column, StringRef message);

    /** Adds a warning diagnostic at the given 1-based line/column. */
    void addWarning (int line, int column, StringRef message);

    /** Adds an informational diagnostic at the given 1-based line/column. */
    void addInfo (int line, int column, StringRef message);

    //==============================================================================
    /** Returns a marker for the current number of diagnostics.

        Paired with rollbackTo(), this lets a *speculative* transform - one that
        is entitled to decide not to apply - analyze something it synthesized
        and then discard whatever that reported. Without it, a bug in such a
        transform would fail the user's compile citing a construct they never
        wrote. */
    int mark() const noexcept;

    /** Discards every diagnostic added after `marker`, which must come from a
        previous mark() on this object. */
    void rollbackTo (int marker);

    /** Returns a human-readable, multi-line rendering of all diagnostics.
        When source text has been set via setSource(), each diagnostic is
        followed by the offending source line and a caret (^) marker.
    */
    String toString() const;

private:
    std::vector<YdspDiagnostic> items;
    String sourceText;
};

//==============================================================================
/** The optimisation policy applied to a YDSP compilation.

    `automatic` selects the native target available on the compiler's host.
    It is spelt out instead of `auto`, which is a C++ keyword.
*/
enum class YdspOptimizationTier
{
    baseline,
    automatic,
    aggressive
};

//==============================================================================
/** Chooses whether compilation follows the host CPU or a portable baseline. */
enum class YdspTargetPolicy
{
    host,
    baseline
};

//==============================================================================
/** Native instruction-set targets supported by the YDSP compiler. */
enum class YdspNativeTarget
{
    scalar,
    sse2,
    avx2,
    avx512,
    asimd
};

//==============================================================================
/** Options controlling one YdspCompiler::compile() call.

    The default is the host-selected `automatic` tier. `fastMath` is disabled
    by default, so a source expression keeps its strict floating-point
    evaluation order. Set it only when fused multiply-add contraction and its
    resulting rounding difference are acceptable for the patch. The current
    WebAssembly backend is scalar; its future portable f32x4 SIMD lowering is
    independent of native host target selection.
*/
struct YdspCompileOptions
{
    YdspOptimizationTier optimizationTier = YdspOptimizationTier::automatic;
    bool fastMath = false;
    YdspTargetPolicy targetPolicy = YdspTargetPolicy::host;
    YdspNativeTarget baselineTarget = YdspNativeTarget::scalar;
    bool emitOptimizationReport = false;
};

//==============================================================================
/** The native-code decisions made by the most recent compilation.

    The report is populated when YdspCompileOptions::emitOptimizationReport
    is true. It records the target actually emitted after capability and
    profitability checks, rather than merely echoing the requested target.
*/
struct YdspOptimizationReport
{
    YdspOptimizationTier optimizationTier = YdspOptimizationTier::automatic;
    bool fastMath = false;
    YdspNativeTarget selectedIsa = YdspNativeTarget::scalar;
    String selectedMicroarchitecture;
    int vectorWidth = 1;
    bool vectorizationEnabled = false;
    bool unrollingEnabled = false;
    bool reductionSplittingEnabled = false;
    bool contractionEnabled = false;
    StringArray rejectedTransforms;
    size_t generatedCodeSize = 0;
    double compileTimeMilliseconds = 0.0;
    bool cacheHit = false;
    String cacheDecision;
    String benchmarkDecision;
};

//==============================================================================
/** Bounds the depth of a recursive descent so pathologically nested YDSP
    source (deeply parenthesized expressions, long unary/`~` chains, nested
    blocks, long function-call chains, deeply nested control flow) fails with
    a diagnostic instead of overflowing the native stack.

    Every mutually-recursive walk that descends into user-authored structure -
    the parser, the semantic analyzer, the IR builder's AST lowering, and the
    wasm codegen's block/loop/if emission - shares this one guard. Construct
    one at the top of each recursive entry point against a depth counter
    owned by that walker, and check exceeded() before doing any further work:

    @code
    YdspRecursionGuard guard (depth);
    if (guard.exceeded())
    {
        error ("expression nested too deeply");
        return fallbackValue;
    }
    @endcode
*/
class YdspRecursionGuard
{
public:
    /** The maximum nesting depth any guarded walk will allow. */
    static constexpr int maxDepth = 250;

    /** Increments `depthRef` for the guard's lifetime. */
    explicit YdspRecursionGuard (int& depthRef) noexcept
        : depth (depthRef)
    {
        ++depth;
    }

    ~YdspRecursionGuard() noexcept
    {
        --depth;
    }

    /** True once the guarded depth counter has passed maxDepth. */
    bool exceeded() const noexcept
    {
        return depth > maxDepth;
    }

    YUP_DECLARE_NON_COPYABLE (YdspRecursionGuard)

private:
    int& depth;
};

//==============================================================================
/** Why a bounded loop was widened, or left scalar, by the vectoriser.

    One entry per original loop, in loop order, produced only when the
    vectoriser actually ran for the kernel (the automatic/aggressive tiers on
    a target with a packed float unit). `widened` means the loop now runs at
    `laneCount` float32 lanes; any other value is the exact reason the loop
    stayed scalar - the "why is this loop scalar?" answer a host can show or
    log directly.
*/
enum class YdspVectorizationReason
{
    widened,                     //!< the loop was widened to `laneCount` lanes
    notVectorizable,             //!< malformed IR; no more specific cause is known
    unsupportedLoopBound,        //!< the bound is neither a constant nor blockSize
    multiBlockBody,              //!< the body contains an `if` or a nested loop
    nonConstantStart,            //!< the loop start is not a compile-time constant
    nonzeroStart,                //!< a blockSize loop must start at 0
    shortTripCount,              //!< the loop span is shorter than one vector
    missingHeaderCompare,        //!< the header does not hold exactly one `i < bound`
    unrecognizedInductionUpdate, //!< the body does not end with `next = i + 1; i = next`
    inductionUsedAsValue,        //!< the loop variable is used outside an array/stream index
    indirectAccess,              //!< an element is reached through a non-loop-variable index
    unrecognizedAccumulation,    //!< a carried value is not of the form `acc = acc + x`
    loopCarriedValue,            //!< the body writes a value that escapes the loop
    unsupportedWidenedOp,        //!< a select, comparison, transcendental or rounding consumes a widened value
    unsupportedWidenedType,      //!< a widened value is not float32
    stateWriteInBody,            //!< the body writes scalar state, a param or an event field
    invariantStreamStore,        //!< a stream store does not go through the loop variable
    emitInBody,                  //!< the body emits an output event
    nothingToWiden,              //!< the body has no array or stream access
    runtimeBoundWithoutStreams   //!< a blockSize loop needs a stream access at the loop variable
};

//==============================================================================
/** The outcome of trying to widen one bounded loop. */
struct YdspVectorizationResult
{
    /** The loop's index into YdspIrFunction::loops (stable across the pass). */
    int loopId = -1;

    /** widened, or the exact reason the loop stayed scalar. */
    YdspVectorizationReason reason = YdspVectorizationReason::notVectorizable;

    /** The widened lane count when reason == widened, else 1. */
    int laneCount = 1;

    /** True when the loop was widened to SIMD lanes. */
    bool widened() const noexcept
    {
        return reason == YdspVectorizationReason::widened;
    }

    /** Renders "loop 2 was widened to 4 lanes" or
        "loop 2 was not vectorized: <reason text>". */
    String describe() const;
};

//==============================================================================
/** The vectoriser's outcome for one kernel: one entry per original loop. */
struct YdspVectorizationReport
{
    /** Per-loop outcomes, parallel to the kernel's original loops. */
    std::vector<YdspVectorizationResult> loops;

    /** The number of loops that were widened. */
    int countWidened() const noexcept;

    /** The distinct "stayed scalar" reasons, deduplicated, as human text. */
    StringArray rejectionReasons() const;
};

//==============================================================================
/** The optimiser's worst-case execution analysis for one generated kernel.

    After optimisation the compiler proves that every loop in the kernel is
    statically bounded (constant or blockSize-derived). boundedIterationCount
    is the product of all loop bounds - the maximum number of times the
    kernel's loop bodies can execute per block - and provenRealtimeSafe is
    true only when every bound is statically known.
*/
struct YdspKernelReport
{
    /** The name of the kernel. */
    String name;

    /** The number of instructions in the kernel's IR after optimisation. */
    int instructionCount = 0;

    /** The product of all statically-known loop bounds in the kernel. If any
        loop is unbounded, this is 0 and provenRealtimeSafe is false. */
    int boundedIterationCount = 0;

    /** True when every loop bound is statically known. */
    bool provenRealtimeSafe = true;

    /** Rendered loop bounds ("16", "blockSize", "blockSize - 1"), one per loop. */
    yup::StringArray loopBounds;

    /** True when at least one loop was widened to SIMD lanes, and the lane count
        that was used (1 when nothing was widened, and on the wasm backend, which
        stays scalar). A widened reduction reassociates, so a kernel reporting
        `vectorized` is not bit-identical to the scalar one. */
    bool vectorized = false;
    int vectorWidth = 1;

    /** Per-loop widening outcomes: `widened` or the exact reason each *original*
        loop stayed scalar - the loops that existed before the vectoriser ran,
        in their order (synthetic scalar tail loops the vectoriser appends are
        not reported). Empty when the vectoriser did not run for this kernel. */
    yup::YdspVectorizationReport loopVectorization;

    /** True when at least one loop was fully unrolled. `boundedIterationCount`
        above is unaffected - it still answers "how many iterations could this
        run", which is the same number whether they are written out or looped -
        so this is the only way to tell the two forms apart from outside. An
        unrolled loop is bit-identical to the rolled one. */
    bool unrolled = false;

    /** True when a widened accumulator was halved. Separate from `vectorized`
        because it re-associates the sum a *second* time, and separate from
        `unrolled` because it only fires on chains long enough to be worth it -
        so neither of those answers "did this kernel's reduction get shortened". */
    bool reductionSplit = false;
};

//==============================================================================
/** The worst-case execution report of a compiled YDSP patch.

    Exposed through YdspAudioGraph::getExecutionReport() so a host can verify,
    before starting audio, that the patch fits its realtime CPU budget.
*/
class YdspExecutionReport
{
public:
    /** Returns the per-kernel worst-case analyses (const). */
    const std::vector<YdspKernelReport>& getKernels() const noexcept;

    /** Returns the per-kernel worst-case analyses (mutable, for the optimizer). */
    std::vector<YdspKernelReport>& getKernels() noexcept;

    /** Returns the total worst-case loop iterations across all kernels. */
    int getTotalBoundedIterations() const noexcept;

    /** Returns true if every kernel is proven realtime-safe. */
    bool isProvenRealtimeSafe() const noexcept;

private:
    friend class YdspAudioGraph;

    std::vector<YdspKernelReport> kernels;
};

//==============================================================================
/** The element type of a compiled endpoint buffer (streams, params, meters).

    Mirrors the YDSP primitive types on the host side; kernels address their
    buffers with the matching element size.
*/
enum class YdspElementType
{
    float32, // 4-byte IEEE float (the default for streams and params)
    float64, // 8-byte IEEE double
    int32,   // 4-byte signed integer
    int64,   // 8-byte signed integer
    boolean  // 4-byte integer boolean (0 or 1)
};

//==============================================================================
/** The outcome of a YdspAudioGraph::process() call.

    process() never asserts, throws or allocates: caller-side problems with the
    supplied stream buffers are reported through this value and the call is
    otherwise ignored (the graph is left untouched), so hosts may validate and
    log at their leisure.
*/
enum class YdspProcessResult
{
    ok,                 // the block was processed
    invalidGraph,       // the graph is null or failed to compile
    invalidBufferCount, // inputs/outputs span size != declared stream counts
    bufferTypeMismatch, // a buffer's variant alternative != declared stream type
    bufferTooShort      // a buffer holds fewer than numSamples elements
};

//==============================================================================
/** A typed, non-owning view of one input stream buffer.

    The buffer is a variant of fixed-element spans: the *active alternative*
    is the buffer's type, so there is no `void*` reinterpretation at the call
    site and no way to silently pass a buffer of the wrong element type.

    The active alternative must match the graph's declared stream type (see
    YdspAudioGraph::getInputStreamType()) and hold at least `numSamples`
    elements; mismatches are silently ignored rather than touching the buffer. Boolean streams (stored as
    int32 0/1) use the int32 alternative.
*/
using YdspInputBuffer = std::variant<
    yup::Span<const float>,
    yup::Span<const double>,
    yup::Span<const int32_t>,
    yup::Span<const int64_t>>;

/** A typed, non-owning view of one output stream buffer (the mutable
    counterpart of YdspInputBuffer). */
using YdspOutputBuffer = std::variant<
    yup::Span<float>,
    yup::Span<double>,
    yup::Span<int32_t>,
    yup::Span<int64_t>>;

//==============================================================================
/** A sample-accurate, stepped parameter change for the audio thread.

    Resolve the target slot once, on the control thread, via
    YdspAudioGraph::getParameterSlot() and then deliver the (slot, sampleOffset,
    value) triple to YdspAudioGraph::process(). The runtime applies the value as
    an exact-sample step change: every sub-block after `sampleOffset` sees the
    new value. Automation is deliberately *not* interpolated: de-zippering is
    the patch's job, via the `smooth (param, tau)` intrinsic or the
    `[[ smoothing: tau ]]` endpoint annotation.
*/
struct YdspAutomationEvent
{
    int parameterSlot = -1; // resolved via YdspAudioGraph::getParameterSlot()
    int sampleOffset = 0;   // sample index within the block (0 <= offset < numSamples)
    float value = 0.0f;     // the new float32 value
};

//==============================================================================
/** Metadata of one patch parameter, for building host UIs.

    Populated from the patch's `input value` endpoints: the qualified name
    (graph-level name, or "node.param"), the annotation style `[[ name: ... ]]`
    display name (falling back to the endpoint name), the value type, the
    declared default value (`input value float x = default`), and the
    `[[ min: ... ]]` / `[[ max: ... ]]` annotation bounds (falling back to
    `0` and `1` when not annotated).

    A parameter annotated `[[ values: { "a", "b", ... } ]]` is discrete: the
    annotation's labels are exposed in `discreteValues`, evenly spaced over
    [minValue, maxValue] (label i sits at minValue + i * step, with
    step = (maxValue - minValue) / (count - 1)). Hosts should snap the control
    to those steps and show the matching label instead of the raw number.

    `unit`, `stepSize` and `style` surface the optional `[[ unit: ... ]]`,
    `[[ step: ... ]]` and `[[ style: ... ]]` annotations verbatim, for a host UI
    to use as it sees fit (e.g. a unit suffix, a slider increment, or a hint
    about which control to render). All three are empty/zero when absent.
*/
struct YdspParameterInfo
{
    String name;        // qualified name ("node.param" or graph-level "param")
    String displayName; // [[ name: "..." ]] annotation, else the endpoint name
    YdspElementType type = YdspElementType::float32;
    double defaultValue = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;
    StringArray discreteValues; // [[ values: "a", "b", ... ]] labels; empty for continuous parameters

    String unit;           // [[ unit: "..." ]] annotation (e.g. "Hz", "dB"), else empty
    double stepSize = 0.0; // [[ step: ... ]] annotation (UI increment), else 0 (continuous)
    String style;          // [[ style: "..." ]] annotation (host-defined control hint), else empty

    /** Returns true if this parameter carries a [[ values ]] annotation. */
    bool isDiscrete() const noexcept { return discreteValues.size() >= 2; }

    /** Returns the label for the discrete position closest to `value`, or an
        empty string when this parameter is not discrete. */
    String labelForValue (double value) const
    {
        if (! isDiscrete())
            return {};

        const auto count = discreteValues.size();
        const auto step = (maxValue - minValue) / static_cast<double> (count - 1);
        auto index = static_cast<int> (std::lround ((value - minValue) / step));

        return discreteValues[std::clamp (index, 0, static_cast<int> (count) - 1)];
    }
};

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

//==============================================================================
/** Compiles YDSP source text into a realtime-runnable YdspAudioGraph.

    Compilation is a control-thread operation (it allocates and generates
    machine code); the resulting YdspAudioGraph runs in the audio callback.

    @see YdspAudioGraph
*/
class YUP_API YdspCompiler
{
public:
    /** Constructor. */
    YdspCompiler();

    /** Destructor. */
    ~YdspCompiler();

    YdspCompiler (YdspCompiler&&) = default;
    YdspCompiler& operator= (YdspCompiler&&) = default;

    //==============================================================================
    /** Compiles YDSP source text into a runnable graph.

        `import` directives in the source resolve relative to the directory of
        the importing file: nested imports inside an imported file resolve
        against that file's own folder, and the top-level source resolves
        against `importBasePath`. Pass the directory (or the file path) of the
        patch when the source text was read from disk so relative import paths
        like `import fx.Delay` resolve to the expected folder; when
        empty, top-level imports resolve relative to the process's current
        working directory.

        When `threadPool` is non-null, reading, lexing and parsing the
        imported files runs in parallel on that pool (the merge into the
        program stays single-threaded). Pass your own `ThreadPool`; the
        compiler never adds or removes jobs it does not own. When it is null,
        imports are resolved sequentially with identical results.

        On failure the returned ResultValue is a failure and the detailed
        diagnostics are available through getDiagnostics().
    */
    ResultValue<YdspAudioGraph> compile (StringRef source, StringRef importBasePath = {}, ThreadPool* threadPool = nullptr);

    /** Compiles YDSP source text with an explicit native-code policy.

        The import and thread-pool arguments have the same meaning as the
        backward-compatible overload above. `baselineTarget` is only used
        when options.targetPolicy is YdspTargetPolicy::baseline.
    */
    ResultValue<YdspAudioGraph> compile (StringRef source, const YdspCompileOptions& options, StringRef importBasePath = {}, ThreadPool* threadPool = nullptr);

    /** Returns the diagnostics of the most recent compile. */
    const YdspDiagnostics& getDiagnostics() const noexcept;

    /** Returns the native-code report of the most recent compile.

        When the most recent options did not request a report, this returns an
        empty report whose generatedCodeSize and compileTimeMilliseconds are 0.
    */
    const YdspOptimizationReport& getOptimizationReport() const noexcept;

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspCompiler)
};

} // namespace yup
