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
/** The value types of the YDSP type system (float/int default to 32-bit). */
enum class YdspValueType
{
    float32Type,
    float64Type,
    int32Type,
    int64Type,
    boolType
};

//==============================================================================
/** A compile-time constant value with its resolved type (param/state defaults). */
struct YdspConstValue
{
    YdspValueType type = YdspValueType::float32Type;
    double asDouble = 0.0; // float32/float64 payload
    int64_t asInt = 0;     // int32/int64 payload
    bool asBool = false;   // bool payload
};

//==============================================================================
/** The kind of a resolved symbol inside a process body. */
enum class YdspSymbolKind
{
    local,
    inputStream,  // audio input
    outputStream, // audio output
    inputValue,   // parameter (host-written, kernel-read)
    outputValue,  // meter (kernel-written, host-read)
    stateScalar,
    stateArray,
    builtinConstant,
    eventParam // the handler-bound event value (e.pitch / e.velocity)
};

/** Resolved information about a symbol. */
struct YdspSymbolInfo
{
    YdspSymbolKind kind = YdspSymbolKind::local;
    YdspValueType type = YdspValueType::float32Type;
    int index = -1;       // index into the owning endpoint/state list
    int arraySize = 0;    // stateArray size, or -1 for block-mode streams (0 otherwise)
    bool isArray = false; // true for state arrays and block-mode stream arrays
    bool isLet = false;   // locals only
};

//==============================================================================
/** The provenance of a statically-provable loop bound. */
enum class YdspLoopBoundKind
{
    constant,            // a non-negative integer literal
    blockSize,           // the block size itself
    blockSizeMinusConst, // blockSize - k
    blockSizePlusConst   // blockSize + k
};

/** A statically-provable loop bound. */
struct YdspLoopBound
{
    YdspLoopBoundKind kind = YdspLoopBoundKind::constant;
    int constant = 0;

    /** Returns a human-readable rendering of the bound. */
    String toString() const;
};

/** The realtime-safety analysis of one loop. */
struct YdspLoopAnalysis
{
    YdspLoopBound bound;
    int depth = 0;
};

//==============================================================================
/** The result of semantically analyzing a user-defined function. */
struct YdspAnalyzedFunc
{
    const YdspFuncDecl* decl = nullptr;
    YdspPrimitiveType returnType = YdspPrimitiveType::float32Type;
    bool hasReturnType = false;
};

/** Returns true when `name` is a built-in YDSP intrinsic (abs, sin, ...).

    Single source of truth for the intrinsic name set: semantic analysis
    resolves calls against the same table this queries, and the compiler's
    import rename pass must agree on what a top-level function may never
    shadow, so the two share one list instead of mirroring each other.
*/
bool isIntrinsicName (StringRef name) noexcept;

/** The language's function shadowing rule, shared by semantic analysis and IR
    lowering so the two can never disagree: a processor-local function shadows
    a program-level (library) function of the same name. Returns nullptr when
    neither scope defines a function named `name`. A null scope is simply
    empty, so callers with no processor context can pass nullptr. */
inline const YdspAnalyzedFunc* findFunctionInScope (StringRef name,
                                                    const std::vector<YdspAnalyzedFunc>* processorFunctions,
                                                    const std::vector<YdspAnalyzedFunc>* programFunctions) noexcept
{
    if (processorFunctions != nullptr)
        for (const auto& func : *processorFunctions)
            if (func.decl->name == name)
                return &func;

    if (programFunctions != nullptr)
        for (const auto& func : *programFunctions)
            if (func.decl->name == name)
                return &func;

    return nullptr;
}

//==============================================================================
/** The built-in event shapes of the YDSP language.

    Note that `midi` is the graph-scope shape ("there is MIDI coming in") and
    carries no payload.
*/
enum class YdspEventShape
{
    midi,          // graph-scope only
    noteOn,        // .pitch, .velocity, .bendSemitones, .isLegato
    noteOff,       // .pitch, .velocity
    pitchBend,     // .bendSemitones
    pressure,      // .pressure
    slide,         // .slide
    controlChange, // .control, .value
    programChange  // .program
};

/** The number of processor-scope event shapes (every shape but `midi`). */
inline constexpr int numProcessorEventShapes = 7;

/** The dense 0-based index of a processor-scope shape, or -1 for `midi`.

    Used to size and address the per-shape handler tables of the runtime.
*/
inline constexpr int eventShapeIndex (YdspEventShape shape) noexcept
{
    return static_cast<int> (shape) - 1;
}

//==============================================================================
/** One payload field of an event shape: the name a handler reads after `e.`,
    its byte offset inside YdspEventContext, and its value type. */
struct YdspEventFieldDesc
{
    const char* name = nullptr;
    int byteOffset = 0;
    YdspValueType type = YdspValueType::float32Type;
};

/** The declaration name and legal payload fields of one processor-scope event
    shape. Trailing unused field entries have a null `name`. */
struct YdspEventShapeDesc
{
    YdspEventShape shape = YdspEventShape::noteOn;
    const char* name = nullptr;
    std::array<YdspEventFieldDesc, 5> fields {};
};

/** Every processor-scope event shape with its legal payload fields.

    This is the single source of truth shared by the parser-facing whitelist,
    the semantic analyzer (which resolves `e.<field>`), the IR builder (which
    lowers it to an offset-carrying load) and the runtime (which fills the
    matching YdspEventContext slot). Adding a shape is a table edit.

    All floats are normalized to [0, 1] except `.pitch` (MIDI note scale) and
    `.bendSemitones` (signed semitones). `.isLegato` reads bit 0 of the flags
    word, so it is declared as a bool over the `flags` offset.
*/
inline constexpr std::array<YdspEventShapeDesc, numProcessorEventShapes> ydspEventShapes { {
    { YdspEventShape::noteOn,
      "noteOn",
      { { { "pitch", static_cast<int> (offsetof (YdspEventContext, pitch)), YdspValueType::float32Type },
          { "velocity", static_cast<int> (offsetof (YdspEventContext, velocity)), YdspValueType::float32Type },
          { "bendSemitones", static_cast<int> (offsetof (YdspEventContext, bend)), YdspValueType::float32Type },
          { "isLegato", static_cast<int> (offsetof (YdspEventContext, flags)), YdspValueType::boolType },
          { "channel", static_cast<int> (offsetof (YdspEventContext, channel)), YdspValueType::int32Type } } } },

    { YdspEventShape::noteOff,
      "noteOff",
      { { { "pitch", static_cast<int> (offsetof (YdspEventContext, pitch)), YdspValueType::float32Type },
          { "velocity", static_cast<int> (offsetof (YdspEventContext, velocity)), YdspValueType::float32Type },
          { "channel", static_cast<int> (offsetof (YdspEventContext, channel)), YdspValueType::int32Type } } } },

    { YdspEventShape::pitchBend,
      "pitchBend",
      { { { "bendSemitones", static_cast<int> (offsetof (YdspEventContext, bend)), YdspValueType::float32Type },
          { "channel", static_cast<int> (offsetof (YdspEventContext, channel)), YdspValueType::int32Type } } } },

    { YdspEventShape::pressure,
      "pressure",
      { { { "pressure", static_cast<int> (offsetof (YdspEventContext, pressure)), YdspValueType::float32Type },
          { "channel", static_cast<int> (offsetof (YdspEventContext, channel)), YdspValueType::int32Type } } } },

    { YdspEventShape::slide,
      "slide",
      { { { "slide", static_cast<int> (offsetof (YdspEventContext, slide)), YdspValueType::float32Type },
          { "channel", static_cast<int> (offsetof (YdspEventContext, channel)), YdspValueType::int32Type } } } },

    { YdspEventShape::controlChange,
      "controlChange",
      { { { "control", static_cast<int> (offsetof (YdspEventContext, index)), YdspValueType::int32Type },
          { "value", static_cast<int> (offsetof (YdspEventContext, value)), YdspValueType::float32Type },
          { "channel", static_cast<int> (offsetof (YdspEventContext, channel)), YdspValueType::int32Type } } } },

    { YdspEventShape::programChange,
      "programChange",
      { { { "program", static_cast<int> (offsetof (YdspEventContext, index)), YdspValueType::int32Type },
          { "channel", static_cast<int> (offsetof (YdspEventContext, channel)), YdspValueType::int32Type } } } },
} };

/** Returns the descriptor of the named processor-scope shape, or nullptr. */
inline const YdspEventShapeDesc* findEventShape (StringRef name) noexcept
{
    for (const auto& shape : ydspEventShapes)
        if (name == StringRef (shape.name))
            return &shape;

    return nullptr;
}

/** Returns the descriptor of a processor-scope shape, or nullptr for `midi`. */
inline const YdspEventShapeDesc* findEventShape (YdspEventShape shape) noexcept
{
    const auto index = eventShapeIndex (shape);

    return index < 0 ? nullptr : &ydspEventShapes[static_cast<size_t> (index)];
}

/** Returns the named payload field of a shape, or nullptr when it has none. */
inline const YdspEventFieldDesc* findEventField (const YdspEventShapeDesc& shape, StringRef name) noexcept
{
    for (const auto& field : shape.fields)
        if (field.name != nullptr && name == StringRef (field.name))
            return &field;

    return nullptr;
}

/** Returns "'pitch', 'velocity', 'bendSemitones', 'isLegato', 'channel'" - a shape's legal fields, for diagnostics. */
inline String eventShapeFieldList (const YdspEventShapeDesc& shape)
{
    StringArray names;

    for (const auto& field : shape.fields)
        if (field.name != nullptr)
            names.add (String (field.name).quoted ('\''));

    return names.joinIntoString (", ");
}

/** Returns "'noteOn', 'noteOff', ..." - every processor-scope shape, for diagnostics. */
inline String eventShapeNameList()
{
    StringArray names;

    for (const auto& shape : ydspEventShapes)
        names.add (String (shape.name).quoted ('\''));

    return names.joinIntoString (", ");
}

/** The result of semantically analyzing an event handler. */
struct YdspAnalyzedEventHandler
{
    const YdspEventHandlerDecl* decl = nullptr;
    YdspEventShape shape = YdspEventShape::noteOn;
};

//==============================================================================
/** The result of semantically analyzing a processor (kernel). */
struct YdspAnalyzedProcessor
{
    const YdspProcessorDecl* decl = nullptr;
    YdspProcessMode mode = YdspProcessMode::sample;

    std::vector<const YdspEndpointDecl*> inputStreams;
    std::vector<const YdspEndpointDecl*> outputStreams;
    std::vector<const YdspEndpointDecl*> inputValues;  // parameters
    std::vector<const YdspEndpointDecl*> outputValues; // meters
    std::vector<const YdspEndpointDecl*> inputEvents;  // event endpoints (noteOn/noteOff)
    std::vector<const YdspEndpointDecl*> outputEvents; // event endpoints (noteOn/noteOff)
    std::vector<const YdspStateDecl*> states;

    // The `state int x [[ role: voiceActivity ]]` declaration, when the
    // processor has one. The runtime reads this scalar out of each voice's
    // state slice and skips a released voice whose flag reads 0.
    const YdspStateDecl* activityState = nullptr;

    // Analyzed event handlers (one per `event` declaration, in order).
    std::vector<YdspAnalyzedEventHandler> eventHandlers;

    // Every loop of the process body, in walk order, with its bound analysis.
    std::vector<YdspLoopAnalysis> loops;

    // Hidden state slots reserved for ' and @ delay primitives (sample mode).
    int hiddenStateCount = 0;

    // The processor's `[[ latency: N ]]` declaration, in its own sample domain
    // (0 when it declares none). A node instance divides this by its rate
    // multiplier to get the figure in graph-rate samples.
    int declaredLatencySamples = 0;

    // True when the processor's process body is statically proven to be realtime-safe.
    bool provenRealtimeSafe = true;

    // Analyzed user-defined functions in this processor.
    std::vector<YdspAnalyzedFunc> functions;

    /** Maximum permitted delay length in samples. */
    static constexpr int maxDelay = 65536;
};

//==============================================================================
/** Maps a YDSP primitive type to its IR storage type (bool is stored as i32). */
inline YdspValueType toStorageType (YdspPrimitiveType type) noexcept
{
    switch (type)
    {
        case YdspPrimitiveType::float32Type:
            return YdspValueType::float32Type;
        case YdspPrimitiveType::float64Type:
            return YdspValueType::float64Type;
        case YdspPrimitiveType::int32Type:
            return YdspValueType::int32Type;
        case YdspPrimitiveType::int64Type:
            return YdspValueType::int64Type;
        case YdspPrimitiveType::boolType:
            return YdspValueType::int32Type; // bool stored as i32
    }

    return YdspValueType::float32Type;
}

/** Returns true if the value type is a floating-point type. */
inline bool isFloatValueType (YdspValueType type) noexcept
{
    return type == YdspValueType::float32Type || type == YdspValueType::float64Type;
}

/** Returns true if the value type is an integer type. */
inline bool isIntValueType (YdspValueType type) noexcept
{
    return type == YdspValueType::int32Type || type == YdspValueType::int64Type;
}

/** Returns true if the value type is 64 bits wide. */
inline bool is64BitValueType (YdspValueType type) noexcept
{
    return type == YdspValueType::float64Type || type == YdspValueType::int64Type;
}

/** Returns the storage size in bytes of the value type (4 or 8). */
inline int elementSizeBytes (YdspValueType type) noexcept
{
    return is64BitValueType (type) ? 8 : 4;
}

//==============================================================================
/** The sinc half-width every oversampled node's resampler is built with.

    Named so the latency figure below is *derived* from the same constant the
    runtime instantiates its `yup::Oversampler` with, rather than restated. */
inline constexpr int ydspOversamplerSincRadius = 8;

/** The group delay of an oversampled node, in input-rate samples.

    `yup::Oversampler::getLatencyInSamples()` is `2 * SincRadius`: the resampler
    contributes SincRadius going up and SincRadius coming back down. It does not
    depend on the oversampling factor, so this is the same integer for `* 2`,
    `* 4` and `* 8` - and it is linear phase and exactly integral, which is what
    lets a plain integer delay compensate it perfectly. */
inline constexpr int ydspOversamplerLatencySamples = 2 * ydspOversamplerSincRadius;

//==============================================================================
/** How a node's voice bank responds to note events (`[[ mode: ... ]]`). */
enum class YdspVoiceMode
{
    poly, // every note takes its own voice slot
    mono  // one voice, driven by a held-note stack with legato continuations
};

/** What a poly node does with a note-on when every voice is held
    (`[[ stealing: ... ]]`). */
enum class YdspVoiceStealing
{
    oldest, // release and reuse the oldest-triggered voice
    newest, // release and reuse the newest-triggered voice
    none    // ignore the note-on
};

/** Which of a mono node's held notes actually sounds (`[[ priority: ... ]]`). */
enum class YdspMonoPriority
{
    last, // the most recently pressed key
    low,  // the lowest held pitch
    high  // the highest held pitch
};

//==============================================================================
/** A node instance resolved against its processor definition. */
struct YdspAnalyzedNode
{
    String instanceName;

    // Exactly one of these is set: a node either instantiates a processor or a
    // subgraph. A subgraph node is a placeholder that the graph inliner splices
    // away, so nothing past semantic analysis ever sees one.
    const YdspProcessorDecl* processor = nullptr;
    const YdspGraphDecl* subgraph = nullptr;
    int subgraphIndex = -1; // index into YdspProgram::graphs, or -1

    int voiceCount = 1;         // >1 = a voice bank (runtime replication)
    bool isEventDriven = false; // true when the processor declares event handlers

    bool isMidiOnly = false;

    // Voice behaviour, from the node's annotations. The defaults reproduce the
    // behaviour of an unannotated node.
    YdspVoiceMode voiceMode = YdspVoiceMode::poly;
    YdspVoiceStealing stealing = YdspVoiceStealing::oldest;
    YdspMonoPriority monoPriority = YdspMonoPriority::last;

    // Oversampling factor of this node, from its `[[ oversample: ... ]]`
    // annotation. 1 = no oversampling, 2 = 2x, 4 = 4x, 8 = 8x. The runtime instantiates a yup
    // Oversampler with the same factor, and the compiler inserts a resampler
    // at every input and output to convert between the graph's sample rate and
    // the node's own. The oversampler's group delay is added to the processor's declared
    // `[[ latency ]]` to get the node's total latency contribution.
    int rateMultiplier = 1;
    int rateDivider = 1;

    // Samples of *artifact* latency this node contributes, at the graph's rate:
    // the oversampler's group delay plus the processor's declared `[[ latency ]]`
    // converted out of its own sample domain. The sole source of truth once
    // analyzeGraph() has run - a fused node synthesises a new processor
    // declaration that would report 0, so it is set from its members' sum.
    int latencySamples = 0;

    // Initial values for the target's inputValue endpoints, in order.
    std::vector<YdspConstValue> paramDefaults;

    // Host-visible name of each inputValue endpoint, when it differs from the
    // usual `instanceName.endpointName`. Only a fused node fills this in: it
    // stands for several original nodes, so its parameters keep the names the
    // patch gave them (`filter.cutoff`) rather than acquiring the fused node's.
    // Empty for every ordinary node, which is the signal to derive the name.
    std::vector<String> paramPublicNames;

    // The same, for the outputValue endpoints: a fused node's meters stay
    // addressable as `filter.level` rather than acquiring the fused node's
    // instance name. Empty for every ordinary node.
    std::vector<String> meterPublicNames;

    /** Returns the target's endpoint list, whether it is a processor or a
        subgraph. Both declaration kinds carry the same endpoint vector, so
        every graph-scope query can go through here. */
    const std::vector<YdspEndpointDecl>& endpoints() const noexcept
    {
        if (processor != nullptr)
            return processor->endpoints;

        if (subgraph != nullptr)
            return subgraph->endpoints;

        static const std::vector<YdspEndpointDecl> empty;
        return empty;
    }

    /** Returns the name of the processor or graph this node instantiates. */
    String targetName() const
    {
        if (processor != nullptr)
            return processor->name;

        if (subgraph != nullptr)
            return subgraph->name;

        return {};
    }
};

/** A resolved audio edge (stream-to-stream). */
struct YdspAnalyzedEdge
{
    int srcNode = -1;     // -1 = graph input
    int srcStream = 0;    // graph input index, or node output stream index
    int dstNode = -1;     // -1 = graph output
    int dstStream = 0;    // node input stream index, or graph output index
    int delaySamples = 0; // inline delay on this edge

    // Samples of delay the latency-compensation pass adds to align this edge's
    // arrival with the other edges converging on the same destination. Kept
    // apart from delaySamples - which keeps meaning "what the author wrote", so
    // the fusion predicate keeps its meaning and the pass stays idempotent. The
    // compiler sums the two at the one line that consumes them.
    int compensationSamples = 0;
};

/** A resolved parameter edge (graph inputValue -> node inputValue). */
struct YdspAnalyzedValueEdge
{
    int srcParam = -1; // graph inputValue index
    int dstNode = -1;
    int dstParam = 0; // node inputValue index
};

/** A resolved meter edge (node outputValue -> graph outputValue). */
struct YdspAnalyzedMeterEdge
{
    int srcNode = -1;
    int srcMeter = 0;  // node outputValue index
    int dstMeter = -1; // graph outputValue index
};

/** A resolved event edge: node outputEvent -> node inputEvent, node outputEvent
    -> graph outputEvent, or graph inputEvent -> node inputEvent.

    An event endpoint's declared name is a channel identifier, not a shape: a
    single `output event <name>;` can carry any number of shapes across its
    lifetime (one `emit <shape> ... -> <name>;` per shape), so an edge does not
    itself have one fixed shape - the shape travels with each event payload and
    is dispatched at the destination by its own per-shape handler, exactly as a
    host-originated event already is.

    A graph's own `input event <name>;` reaches a node only through an
    explicit connection like any other endpoint kind - there is no implicit
    broadcast-by-matching-name.
*/
struct YdspAnalyzedEventEdge
{
    int srcNode = -1;    // -1 = the graph's own `input event` boundary endpoint
    int srcEndpoint = 0; // index among that node's `output event` endpoints, or the graph's `inputEvents`
    int dstNode = -1;    // -1 = the graph's own `output event` boundary endpoint
    int dstEndpoint = 0; // index among that node's `input event` endpoints, or the graph's `outputEvents`
    int compensationSamples = 0; // assigned by a later step (latency task), 0 until then
};

/** The result of semantically analyzing the main graph. */
struct YdspAnalyzedGraph
{
    std::vector<const YdspEndpointDecl*> inputStreams;
    std::vector<const YdspEndpointDecl*> outputStreams;
    std::vector<const YdspEndpointDecl*> inputValues;  // graph parameters
    std::vector<const YdspEndpointDecl*> outputValues; // graph meters

    // The graph's named event inputs, in declaration order. Each node whose
    // processor declares the same input name subscribes to it; events on a
    // graph event input reach only those nodes.
    std::vector<const YdspEndpointDecl*> inputEvents;

    // The graph's named event outputs, in declaration order.
    std::vector<const YdspEndpointDecl*> outputEvents;

    // Initial values of the graph parameters (const-evaluated defaults).
    std::vector<YdspConstValue> inputValueDefaults;

    std::vector<YdspAnalyzedNode> nodes;
    std::vector<YdspAnalyzedEdge> edges;
    std::vector<YdspAnalyzedValueEdge> valueEdges;
    std::vector<YdspAnalyzedMeterEdge> meterEdges;
    std::vector<YdspAnalyzedEventEdge> eventEdges;

    // Node indices in topological execution order, for the graph's process body.
    std::vector<int> topoOrder;

    // Artifact latency of the whole graph, in samples: the figure a plugin
    // wrapper reports to its host. One scalar, because that is all any plugin
    // format can represent - which is why the pass equalises across separate
    // graph outputs rather than leaving each with its own.
    int latencySamples = 0;
};

/** The full result of semantic analysis of a YDSP program. */
struct YdspAnalyzedProgram
{
    /** The analyzed processors in the program, in declaration order. */
    std::vector<YdspAnalyzedProcessor> processors;

    /** The analyzed functions in the program, in declaration order. */
    std::vector<YdspAnalyzedFunc> functions;

    /** The analyzed graphs in the program, in declaration order. */
    YdspAnalyzedGraph graph;

    /** Owning reference to the parsed AST - keeps all analysis pointers valid. */
    std::unique_ptr<YdspProgram> ast;

    /** Returns the analyzed processor for the given declaration, or nullptr. */
    const YdspAnalyzedProcessor* findProcessor (const YdspProcessorDecl* decl) const;
};

} // namespace yup
