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
/** A 1-based source location. */
struct YdspLocation
{
    int line = 0;
    int column = 0;
};

//==============================================================================
/** The primitive types of the YDSP language (float/int default to 32-bit). */
enum class YdspPrimitiveType
{
    float32Type,
    float64Type,
    int32Type,
    int64Type,
    boolType
};

/** Returns the type keyword ("float32", "float64", "int32", "int64", "bool"). */
StringRef toString (YdspPrimitiveType type) noexcept;

//==============================================================================
/** The kinds of expression nodes. */
enum class YdspExprKind
{
    intLiteral,
    floatLiteral,
    boolLiteral,
    identifier, // a named value (local, endpoint, state, builtin constant)
    unary,      // -x, !x
    binary,     // arithmetic / comparison / logical
    ternary,    // cond ? a : b
    call,       // intrinsic function call
    index,      // a[i]
    member,     // a.b (endpoint access in graph connections)
    prev,       // x'  (one-sample delay)
    delay,      // x @ n  (fixed delay)
    graphLeaf,  // graph algebra leaf: node name / graph io / '_'
    graphOp     // graph algebra composition (op is the operator)
};

//==============================================================================
/** The operators used by unary/binary/graph expression nodes. */
enum class YdspOperator
{
    none,
    add,
    sub,
    mul,
    div,
    mod,
    lt,
    le,
    gt,
    ge,
    eq,
    ne,
    andL,
    orL,
    notL,
    neg,
    bitAnd, // a & b
    bitOr,  // a | b
    bitXor, // a ^ b
    shl,    // a << b
    shr,    // a >> b
    notI,   // ~a
    seq,    // a : b
    par,    // a , b
    split,  // a <: b
    merge,  // a :> b
    recurse // a ~ b
};

//==============================================================================
/** A YDSP expression node.

    A single compact node type; the meaning of the payload fields depends on
    `kind` (see the comments on each field).
*/
struct YdspExpr
{
    YdspExprKind kind = YdspExprKind::identifier;
    YdspLocation location;

    String text; // identifier name / callee name / graph leaf name

    double number = 0; // intLiteral / floatLiteral value
    bool flag = false; // boolLiteral value

    YdspOperator op = YdspOperator::none; // unary / binary / graphOp

    // unary:      children[0]
    // binary:     children[0], children[1]
    // ternary:    children[0], children[1], children[2]
    // call:       children = arguments
    // index:      children[0] = target, children[1] = index
    // member:     children[0] = target
    // prev:       children[0]
    // delay:      children[0] = signal, children[1] = delay amount
    // graphOp:    children[0], children[1]
    std::vector<std::unique_ptr<YdspExpr>> children;

    // graphLeaf node instances: parameter overrides (name = value)
    std::vector<std::pair<String, std::unique_ptr<YdspExpr>>> overrides;
};

using YdspExprPtr = std::unique_ptr<YdspExpr>;

//==============================================================================
/** The kinds of statement nodes. */
enum class YdspStmtKind
{
    assign,     // target = value
    ifStmt,     // if (cond) thenStmt [else elseStmt]
    forStmt,    // for name in startExpr .. endExpr body
    localDecl,  // [let] [type] name [= value] ;
    returnStmt, // return [value] ;
    block,      // { children }
    emitStmt    // emit <shape> (field: expr, ...) -> <endpointName> ;
};

/** A YDSP statement node (compact single node type, see field comments). */
struct YdspStmt
{
    YdspStmtKind kind = YdspStmtKind::block;
    YdspLocation location;

    // block: children
    std::vector<std::unique_ptr<YdspStmt>> children;

    // ifStmt
    std::unique_ptr<YdspExpr> cond;
    std::unique_ptr<YdspStmt> thenStmt;
    std::unique_ptr<YdspStmt> elseStmt;

    // forStmt
    String name;
    std::unique_ptr<YdspExpr> startExpr;
    std::unique_ptr<YdspExpr> endExpr;
    std::unique_ptr<YdspStmt> body;

    // assign
    std::unique_ptr<YdspExpr> target;
    std::unique_ptr<YdspExpr> value;

    // localDecl
    bool isLet = false;
    bool hasDeclType = false;
    YdspPrimitiveType declType = YdspPrimitiveType::float32Type;

    // returnStmt
    std::unique_ptr<YdspExpr> returnExpr;

    // emitStmt
    String shapeName;
    String endpointName;
    std::vector<std::pair<String, std::unique_ptr<YdspExpr>>> emitFields;
};

using YdspStmtPtr = std::unique_ptr<YdspStmt>;

//==============================================================================
/** The endpoint kinds of processors and graphs. */
enum class YdspEndpointKind
{
    inputStream,
    outputStream,
    inputValue,  // parameter (read by kernels, written by the host)
    outputValue, // meter (written by kernels, read by the host)
    inputEvent,  // a MIDI-style event input (noteOn/noteOff at processor scope, midi at graph scope)
    outputEvent   // a MIDI-style event output (noteOn/noteOff at processor scope, midi at graph scope)
};

/** A processor or graph endpoint declaration. */
struct YdspEndpointDecl
{
    YdspEndpointDecl() = default;
    YdspEndpointDecl (YdspEndpointDecl&&) = default;
    YdspEndpointDecl& operator= (YdspEndpointDecl&&) = default;

    YdspEndpointKind kind = YdspEndpointKind::inputStream;
    YdspPrimitiveType type = YdspPrimitiveType::float32Type;
    String name;
    int channelCount = 1; // >1 for multi-channel endpoints
    YdspLocation location;

    // inputValue only: the default expression (usually a literal)
    YdspExprPtr defaultValue;

    // [[ key: value, ... ]] annotations
    std::vector<std::pair<String, String>> annotations;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspEndpointDecl)
};

/** A state (history) declaration: scalar, fixed-size array, or struct instance. */
struct YdspStateDecl
{
    YdspStateDecl() = default;
    YdspStateDecl (YdspStateDecl&&) = default;
    YdspStateDecl& operator= (YdspStateDecl&&) = default;

    YdspPrimitiveType type = YdspPrimitiveType::float32Type;
    String name;
    int arraySize = 0;    // 0 = scalar, otherwise compile-time constant size
    String arraySizeName; // non-empty when the size was written as a program constant
    String structName;    // non-empty when the state is a struct instance/array
    YdspLocation location;

    // Optional initialiser: one expression for a scalar, or up to `arraySize`
    // expressions for an array (trailing elements stay zero). Lowered into the
    // processor's `init` kernel by the semantic analyzer.
    std::vector<YdspExprPtr> initialisers;

    // [[ key: value, ... ]] annotations. The only recognised key is
    // `role: voiceActivity`, which marks the per-voice activity flag the
    // runtime reads to skip an idle released voice.
    std::vector<std::pair<String, String>> annotations;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspStateDecl)
};

/** A single field of a struct declaration. */
struct YdspStructField
{
    YdspPrimitiveType type = YdspPrimitiveType::float32Type;
    String name;
    int arraySize = 0; // 0 = scalar field, otherwise fixed-size array field
    YdspLocation location;
};

/** A `struct Name { ... }` declaration inside a processor. */
struct YdspStructDecl
{
    String name;
    YdspLocation location;

    std::vector<YdspStructField> fields;
};

/** The two process body modes. */
enum class YdspProcessMode
{
    sample, // process { ... }       - implicit loop over the block
    block   // process block { ... } - explicit block processing
};

/** A processor's process body. */
struct YdspProcessDecl
{
    YdspProcessDecl() = default;
    YdspProcessDecl (YdspProcessDecl&&) = default;
    YdspProcessDecl& operator= (YdspProcessDecl&&) = default;

    YdspProcessMode mode = YdspProcessMode::sample;
    YdspLocation location;
    std::vector<std::unique_ptr<YdspStmt>> body;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspProcessDecl)
};

/** A per-processor event handler: fires once per dequeued event for the
    named `input event` endpoint, before the block's kernel() call.

    `event midi (e: noteOn) { ... }` binds the handler to the `midi` input and
    selects the `noteOn` shape, whose legal fields `e.<field>` reads.
*/
struct YdspEventHandlerDecl
{
    YdspEventHandlerDecl() = default;
    YdspEventHandlerDecl (YdspEventHandlerDecl&&) = default;
    YdspEventHandlerDecl& operator= (YdspEventHandlerDecl&&) = default;

    String endpointName; // must match an `input event` endpoint on this processor (e.g. "midi")
    String shapeName;    // the event shape this handler processes (e.g. "noteOn")
    String paramName;    // the handler's bound parameter (e.g. "e")
    YdspLocation location;

    std::vector<std::unique_ptr<YdspStmt>> body;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspEventHandlerDecl)
};

/** A user-defined function inside a processor. */
struct YdspFuncDecl
{
    YdspFuncDecl() = default;
    YdspFuncDecl (YdspFuncDecl&&) = default;
    YdspFuncDecl& operator= (YdspFuncDecl&&) = default;

    String name;
    YdspLocation location;

    // Parameter: (name, type)
    std::vector<std::pair<String, YdspPrimitiveType>> params;

    YdspPrimitiveType returnType = YdspPrimitiveType::float32Type;
    bool hasReturnType = false;

    std::vector<std::unique_ptr<YdspStmt>> body;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspFuncDecl)
};

/** A processor (kernel) definition. */
struct YdspProcessorDecl
{
    YdspProcessorDecl() = default;
    YdspProcessorDecl (YdspProcessorDecl&&) = default;
    YdspProcessorDecl& operator= (YdspProcessorDecl&&) = default;

    String name;
    YdspLocation location;

    std::vector<YdspEndpointDecl> endpoints;
    std::vector<YdspStructDecl> structs;
    std::vector<YdspStateDecl> states;
    std::vector<YdspFuncDecl> functions;
    std::vector<YdspEventHandlerDecl> eventHandlers;
    std::unique_ptr<YdspProcessDecl> process;
    std::unique_ptr<YdspProcessDecl> init; // optional one-shot `init { ... }` block

    // [[ latency: N ]] annotation. N is the number of samples of latency the
    // processor's body introduces, expressed in the processor's *own* sample
    // domain - so an instance running at `* 4` divides it by 4. Only the
    // processor's author can know this number, which is why it is declared
    // rather than inferred, and it is treated as an artifact of the
    // implementation: the graph compensates it away.
    std::vector<std::pair<String, String>> annotations;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspProcessorDecl)
};

/** A node instantiation inside a graph. */
struct YdspNodeDecl
{
    YdspNodeDecl() = default;
    YdspNodeDecl (YdspNodeDecl&&) = default;
    YdspNodeDecl& operator= (YdspNodeDecl&&) = default;

    String instanceName;
    String processorName;
    YdspLocation location;

    int voiceCount = 1;     // >1 = a voice bank (pure runtime replication)
    int rateMultiplier = 1; // >1 = oversampling, 1 = normal
    int rateDivider = 1;    // >1 = undersampling, 1 = normal

    // parameter overrides: (paramName, valueExpr)
    std::vector<std::pair<String, YdspExprPtr>> overrides;

    // [[ mode: ..., stealing: ..., priority: ... ]] voice-behaviour annotations
    std::vector<std::pair<String, String>> annotations;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspNodeDecl)
};

/** A single edge in a connection block (src -> dst). */
struct YdspConnection
{
    YdspLocation location;

    String sourcePath;    // "sat.in", "dry", "master"
    String destPath;      // "dly.in", "wet", "sat.drive"
    int delaySamples = 0; // inline delay between source and destination
};

/** The two mutually exclusive graph body forms. */
enum class YdspGraphBodyKind
{
    none,
    connections,
    algebra
};

/** A graph definition.

    A program may declare several graphs: the one annotated `[[ main ]]` (or the
    only one, when there is just one) is the entry point, and every other graph
    is a *subgraph* that is inlined into its user before the runtime sees it.
*/
struct YdspGraphDecl
{
    YdspGraphDecl() = default;
    YdspGraphDecl (YdspGraphDecl&&) = default;
    YdspGraphDecl& operator= (YdspGraphDecl&&) = default;

    String name;
    YdspLocation location;

    std::vector<YdspEndpointDecl> endpoints;
    std::vector<YdspNodeDecl> nodes;

    // [[ main ]] entry-point annotation.
    std::vector<std::pair<String, String>> annotations;

    // True for a graph pulled in by `import`. An imported graph is a library
    // component, never the importing program's entry point.
    bool isImported = false;

    YdspGraphBodyKind bodyKind = YdspGraphBodyKind::none;
    std::vector<YdspConnection> connections; // connections form
    YdspExprPtr algebraRoot;                 // algebra form (process = expr;)

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspGraphDecl)
};

/** A `declare key "value"` metadata statement. */
struct YdspDeclare
{
    String key;
    String value;
    YdspLocation location;
};

/** A program-scope `let name = <constant expression>;` declaration.

    Program constants are compile-time only: the semantic analyzer const-folds
    them away, so they cost nothing at runtime and may be used as an array size,
    a `for` bound or anywhere an expression is expected.
*/
struct YdspLetDecl
{
    YdspLetDecl() = default;
    YdspLetDecl (YdspLetDecl&&) = default;
    YdspLetDecl& operator= (YdspLetDecl&&) = default;

    String name;
    YdspExprPtr value;
    YdspLocation location;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspLetDecl)
};

/** An `import X.Y.Z [as alias]` directive.

    The dotted path maps to the file `X/Y/Z.ydsp`; without an alias the last
    segment (`Z`) becomes the namespace the imported names are accessed under.
*/
struct YdspImportDecl
{
    String path;  // dotted module path, e.g. "fx.Delay"
    String alias; // empty if no alias
    YdspLocation location;
};

/** A whole YDSP program: metadata + processors + one or more graphs. */
struct YdspProgram
{
    YdspProgram() = default;
    YdspProgram (YdspProgram&&) = default;
    YdspProgram& operator= (YdspProgram&&) = default;

    std::vector<YdspDeclare> declares;
    std::vector<YdspImportDecl> imports;
    std::vector<YdspLetDecl> constants;
    std::vector<YdspFuncDecl> functions;
    std::vector<YdspProcessorDecl> processors;
    std::vector<YdspGraphDecl> graphs;

    // Processors the compiler synthesized rather than the program declaring
    // them (kernel fusion builds one per fused chain). Held by pointer because
    // an analyzed node refers to its declaration by address, so growing this
    // list must not move what is already in it.
    std::vector<std::unique_ptr<YdspProcessorDecl>> synthesizedProcessors;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspProgram)
};

//==============================================================================
/** Factory helpers for expression nodes. */
namespace YdspExprFactory
{
YdspExprPtr makeInt (YdspLocation location, long long value);
YdspExprPtr makeFloat (YdspLocation location, double value);
YdspExprPtr makeBool (YdspLocation location, bool value);
YdspExprPtr makeIdentifier (YdspLocation location, String name);
YdspExprPtr makeBinary (YdspLocation location, YdspOperator op, YdspExprPtr lhs, YdspExprPtr rhs);
YdspExprPtr makeUnary (YdspLocation location, YdspOperator op, YdspExprPtr operand);
YdspExprPtr makeCall (YdspLocation location, String callee, std::vector<YdspExprPtr> args);
YdspExprPtr clone (const YdspExpr& expr);
} // namespace YdspExprFactory

} // namespace yup
