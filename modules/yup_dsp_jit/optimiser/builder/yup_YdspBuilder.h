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
namespace detail
{

// The state layout of a processor: one slot map per element type, plus the
// region counts. Computed once per processor and shared by the kernel, the
// init kernel and every event-handler function so they all address the same
// per-voice state memory.
struct YdspStateLayout
{
    std::unordered_map<const YdspStateDecl*, int> float32ScalarSlots;
    std::unordered_map<const YdspStateDecl*, int> float64ScalarSlots;
    std::unordered_map<const YdspStateDecl*, int> int32ScalarSlots;
    std::unordered_map<const YdspStateDecl*, int> int64ScalarSlots;
    std::unordered_map<const YdspStateDecl*, int> float32ArrayBases;
    std::unordered_map<const YdspStateDecl*, int> float64ArrayBases;
    std::unordered_map<const YdspStateDecl*, int> int32ArrayBases;
    std::unordered_map<const YdspStateDecl*, int> int64ArrayBases;
    std::map<std::pair<const YdspStateDecl*, int>, int> structFieldBases;
    std::map<std::pair<const YdspStateDecl*, int>, int> structFieldStrides;

    int float32ScalarCount = 0;
    int float64ScalarCount = 0;
    int int32ScalarCount = 0;
    int int64ScalarCount = 0;
    int float32ArrayCursor = 0;
    int float64ArrayCursor = 0;
    int int32ArrayCursor = 0;
    int int64ArrayCursor = 0;
};

// Flattens a struct-typed state into the existing scalar/array slots:
// scalar fields of a scalar instance become scalar slots; every other
// combination becomes an array region (elements * instances).
void layoutStructState (const YdspStateDecl* state, const YdspStructDecl* structDecl, YdspStateLayout& layout);

// Computes the shared state layout for a processor. This covers only the
// *declared* state: hidden slots allocated by ' / @ inside the kernel's
// sample loop grow the counts *after* this base (event handlers never
// allocate hidden slots, so they only ever read this shared layout).
YdspStateLayout computeStateLayout (const YdspAnalyzedProcessor& processor);

} // namespace detail

//==============================================================================
/** Lowers one analyzed processor into an YdspIrFunction.

    Internal to the optimiser: the builder lowers each processor's AST into a
    block-structured IR with mutable virtual registers, enforcing the parameter
    sampling semantics (params loaded once per block) and lowering the delay
    primitives (' and @) into hidden state.
*/
class YdspIrBuilder
{
public:
    struct Config
    {
        bool isInit = false;
        bool isEventHandler = false;
        const YdspAnalyzedEventHandler* eventHandler = nullptr;
        const YdspIrFunction* sharedLayout = nullptr;
        const std::vector<YdspAnalyzedFunc>* programFunctions = nullptr;
    };

    YdspIrBuilder (YdspIrFunction& fn, const YdspAnalyzedProcessor& processor, YdspDiagnostics& diagnostics, const detail::YdspStateLayout& layout, Config config);

    void build();

private:
    //==============================================================================
    // Value / block management

    int newValue (YdspValueType type);
    int newBlock();
    int emitInst (YdspIrInst inst);

    /** Appends to the function's prologue (block 0) regardless of where
        lowering currently is.

        The prologue holds nothing but definitions - parameter/state loads and
        constants - so an instruction added to it later still dominates every
        use, and it is emitted before the sample loop is entered. */
    int emitInstInPrologue (YdspIrInst inst);

    void setTerminator (YdspIrTerm term, int cond = -1, int target = -1, int target2 = -1);

    int emitConstFFor (double value, YdspValueType type);
    int emitConstF (double value);
    int emitConstF64 (double value);
    int emitConstIFor (int64_t value, YdspValueType type);
    int emitConstI (int64_t value);
    int emitConstI64 (int64_t value);
    int emitConstB (bool value);

    //==============================================================================
    // State layout

    const YdspStructDecl* findStructDecl (const String& name) const
    {
        const auto it = structDecls.find (name);
        return it == structDecls.end() ? nullptr : it->second;
    }

    int newHiddenFloatScalar() { return layout.float32ScalarCount + hiddenFloat32Scalars++; }

    // A hidden ' / @ / smooth scalar slot kept in a register for the whole
    // sample loop: loaded once in the prologue, written back once in the
    // epilogue, so the per-sample body only touches the register.
    struct PromotedSlot
    {
        int slot = -1;
        int reg = -1;
        bool isInt = false;
    };

    /** Returns the value id that carries `slot` through the sample loop.

        Outside sample mode (block mode, init, event handlers) there is no loop
        to promote across, so this just emits the load where it stands and the
        matching writeHiddenSlot() stores straight to memory. */
    int promoteHiddenSlot (int slot, YdspValueType type);

    /** Updates a slot reserved by promoteHiddenSlot() with a new value. */
    void writeHiddenSlot (int slot, int reg, int value, YdspValueType type);

    bool stateIsInt (const YdspStateDecl* state) const
    {
        return ! isFloatValueType (toStorageType (state->type));
    }

    int stateScalarSlot (const YdspStateDecl* state) const
    {
        switch (toStorageType (state->type))
        {
            case YdspValueType::float32Type:
                return layout.float32ScalarSlots.at (state);
            case YdspValueType::float64Type:
                return layout.float64ScalarSlots.at (state);
            case YdspValueType::int32Type:
                return layout.int32ScalarSlots.at (state);
            case YdspValueType::int64Type:
                return layout.int64ScalarSlots.at (state);
            default:
                return 0;
        }
    }

    int stateArrayBase (const YdspStateDecl* state) const
    {
        switch (toStorageType (state->type))
        {
            case YdspValueType::float32Type:
                return layout.float32ArrayBases.at (state);
            case YdspValueType::float64Type:
                return layout.float64ArrayBases.at (state);
            case YdspValueType::int32Type:
                return layout.int32ArrayBases.at (state);
            case YdspValueType::int64Type:
                return layout.int64ArrayBases.at (state);
            default:
                return 0;
        }
    }

    //==============================================================================
    // Sample-mode implicit loop

    void buildSampleLoop();

    //==============================================================================
    // Statements

    void lowerStatements (const std::vector<std::unique_ptr<YdspStmt>>& statements);
    void lowerStatement (const YdspStmt& stmt);
    /** Emits the loop's end bound and reports the resolved YdspLoopBound.

        The bound is returned through `outBound` rather than a member: the
        caller only records it after lowering the body, and a nested loop
        lowered in between resolves a bound of its own. */
    int emitLoopBound (const YdspStmt& stmt, YdspLoopBound& outBound);
    bool resolveLoopBoundForStmt (const YdspStmt& stmt, YdspLoopBound& out) const;
    void lowerAssignment (const YdspStmt& stmt);

    //==============================================================================
    // Expressions

    int lowerExpr (const YdspExpr& expr);
    int lowerIdentifier (const String& name, const YdspLocation& location);
    int lowerEventField (const YdspExpr& expr);
    int captureValue (const YdspExpr& expr);
    int lowerBinary (const YdspExpr& expr);
    int lowerCall (const YdspExpr& expr);

    // Returns true when the expression is a source literal (or a negated
    // literal) that may contextually adapt to another operand's type. Mirrors
    // the semantic analyzer's isAdaptableTo() - cast results like `int64(1)`
    // are NOT adaptable literals.
    bool isAdaptableLiteral (const YdspExpr& expr);

    // Unifies two operand values to a common type, widening literal constants
    // to the other operand's width (the analyzer already guarantees that at
    // least one side is an adaptable literal when the widths differ).
    void unifyOperands (const YdspExpr& a, int av, const YdspExpr& b, int bv, int& outA, int& outB, YdspValueType& outType);

    // Re-emits a literal constant at the target width; returns -1 when the value
    // is not a constant or the width class differs.
    int widenConst (int value, YdspValueType targetType);

    // Coerces a value to the target type: literal constants are re-emitted at
    // the target width, everything else gets an explicit conversion instruction
    // (the analyzer rejects non-literal mismatches, so this is defensive).
    int coerceTo (int value, YdspValueType targetType);

    int lowerCastToInt (int value, YdspValueType targetType);
    int lowerCastToFloat (int value, YdspValueType targetType);

    int emitBinary (YdspIrOp op, YdspValueType type, int a, int b)
    {
        return emitInst ({ op, newValue (type), a, b });
    }

    bool isFloatValue (int value) const
    {
        return isFloatValueType (valueTypes[static_cast<size_t> (value)]);
    }

    // Returns the float width of an argument: the first float-typed operand,
    // defaulting to float32 (adaptable literals and non-float constants).
    YdspValueType floatWidthOf (int first) const
    {
        return isFloatValueType (valueTypes[static_cast<size_t> (first)]) ? valueTypes[static_cast<size_t> (first)] : YdspValueType::float32Type;
    }

    template <typename... Rest>
    YdspValueType floatWidthOf (int first, int second, Rest... rest) const
    {
        const auto t = valueTypes[static_cast<size_t> (first)];
        return isFloatValueType (t) ? t : floatWidthOf (second, rest...);
    }

    bool isIntValue (int value) const
    {
        return isIntValueType (valueTypes[static_cast<size_t> (value)]);
    }

    template <typename... Args>
    bool anyIsIntValue (Args... args) const
    {
        return (isIntValue (args) || ...);
    }

    // Returns the int width of an argument: the first int-typed operand,
    // defaulting to int32 (adaptable literals coerce to whichever width wins).
    YdspValueType intWidthOf (int first) const
    {
        return isIntValueType (valueTypes[static_cast<size_t> (first)]) ? valueTypes[static_cast<size_t> (first)] : YdspValueType::int32Type;
    }

    template <typename... Rest>
    YdspValueType intWidthOf (int first, int second, Rest... rest) const
    {
        const auto t = valueTypes[static_cast<size_t> (first)];
        return isIntValueType (t) ? t : intWidthOf (second, rest...);
    }

    YdspIrOp movOpFor (YdspValueType type) const
    {
        switch (type)
        {
            case YdspValueType::float32Type:
            case YdspValueType::float64Type:
                return YdspIrOp::movF;
            case YdspValueType::int32Type:
            case YdspValueType::int64Type:
                return YdspIrOp::movI;
            case YdspValueType::boolType:
                return YdspIrOp::movB;
        }

        return YdspIrOp::movF;
    }

    const YdspEndpointDecl* findEndpoint (const String& name) const
    {
        for (const auto* endpoint : processor.inputStreams)
            if (endpoint->name == name)
                return endpoint;

        for (const auto* endpoint : processor.outputStreams)
            if (endpoint->name == name)
                return endpoint;

        for (const auto* endpoint : processor.inputValues)
            if (endpoint->name == name)
                return endpoint;

        for (const auto* endpoint : processor.outputValues)
            if (endpoint->name == name)
                return endpoint;

        return nullptr;
    }

    const YdspStateDecl* findState (const String& name) const
    {
        for (const auto* state : processor.states)
            if (state->name == name)
                return state;

        return nullptr;
    }

    // Resolves a `member` expression (`base.field`) against the flattened
    // struct state layout. stride == 0 means a scalar slot (outBase = slot);
    // otherwise outBase is an array region and outInstanceIndex (value id, or
    // -1 for a scalar struct instance) is the instance element index.
    bool resolveStructFieldLayout (const YdspExpr& memberExpr, int& outBase, int& outStride, int& outInstanceIndex, YdspValueType& outType);

    int endpointStreamIndex (const YdspEndpointDecl* endpoint) const
    {
        if (endpoint->kind == YdspEndpointKind::inputStream)
        {
            for (size_t i = 0; i < processor.inputStreams.size(); ++i)
                if (processor.inputStreams[i] == endpoint)
                    return static_cast<int> (i);
        }
        else
        {
            for (size_t i = 0; i < processor.outputStreams.size(); ++i)
                if (processor.outputStreams[i] == endpoint)
                    return static_cast<int> (i);
        }

        return -1;
    }

    int endpointValueIndex (const YdspEndpointDecl* endpoint) const
    {
        if (endpoint->kind == YdspEndpointKind::inputValue)
        {
            for (size_t i = 0; i < processor.inputValues.size(); ++i)
                if (processor.inputValues[i] == endpoint)
                    return static_cast<int> (i);
        }
        else
        {
            for (size_t i = 0; i < processor.outputValues.size(); ++i)
                if (processor.outputValues[i] == endpoint)
                    return static_cast<int> (i);
        }

        return -1;
    }

    void lowerStateStore (const YdspStateDecl* state, int value);

    //==============================================================================
    // Function inlining

    int lowerFunctionCall (const YdspAnalyzedFunc& func, const YdspExpr& expr);
    void lowerFunctionBody (const std::vector<std::unique_ptr<YdspStmt>>& body);

    //==============================================================================

    int returnValue = -1;

    YdspIrFunction& fn;
    const YdspAnalyzedProcessor& processor;
    YdspDiagnostics& diagnostics;
    const detail::YdspStateLayout& layout;

    const std::vector<YdspAnalyzedFunc>* programFunctions = nullptr;

    bool isInit = false;
    bool isEventHandler = false;

    // The analyzed handler being lowered (when isEventHandler).
    const YdspAnalyzedEventHandler* eventHandler = nullptr;

    // When building the one-shot init kernel or an event-handler function,
    // adopt the main kernel's final state layout so all functions of a
    // processor address the same memory.
    const YdspIrFunction* sharedLayout = nullptr;

    // Hidden ' / @ slots allocated past the shared layout's counts (only the
    // kernel's sample loop allocates them; handlers never do).
    int hiddenFloat32Scalars = 0;
    int hiddenInt32Scalars = 0;
    int hiddenFloat32ArrayElements = 0;

    std::vector<YdspValueType> valueTypes;
    std::unordered_map<String, int> locals;
    std::unordered_map<const YdspStateDecl*, int> stateRegs;
    std::unordered_map<size_t, int> lastOutputValue;

    // Functions currently being inlined, direct or indirect caller to callee.
    std::unordered_set<String> functionsBeingInlined;

    // Struct-typed state declarations (used by resolveStructFieldLayout).
    std::unordered_map<String, const YdspStructDecl*> structDecls;

    std::unordered_map<int, int64_t> intConstPayloads;
    std::unordered_map<int, double> floatConstPayloads;

    std::vector<int> paramValues;

    std::vector<std::tuple<int, int, const YdspExpr*>> pendingPrevStores;

    std::vector<PromotedSlot> promotedHiddenSlots;

    int currentBlock = 0;
    int sampleIndex = -1;
    int blockSizeValue = -1;

    int recursionDepth = 0;
};

} // namespace yup
