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
/** Evaluates a CompiledExpression, holding its inputs, intermediate values and outputs.

    The state allocates all its storage once, on construction: evaluating a program that
    isAllocationFree() never allocates. A state must only be used by one thread at a time,
    create one state per thread to evaluate the same program concurrently.

    Rows are evaluated in blocks of up to getBlockSize() rows, every instruction processing the
    whole block at once. Inputs can be uniform (set) or read per row from memory (bindColumn,
    bindRowIndex), and outputs can be read per lane (get) or written to memory (bindOutputColumn).

    Values that don't depend on per row data are computed once, not once per row, and only computed
    again when an input they depend on is set to a different value. Computations stop propagating as
    soon as they produce the value they had before, and hasChanged() tells which outputs changed in
    the last evaluation, so callers can skip updating what depends on them. Array inputs are read
    again on every evaluation, so their elements can change in place, and functions that aren't pure
    run for every row.

    @code
    ExpressionContext context;
    auto x = context.addInput<float> ("x");
    auto program = CompiledExpression::compile ("y = x * x", context).getValue();
    auto y = program.getOutput<float> ("y").getValue();

    ExpressionState state (program);

    // One row at a time
    state.set (x, 3.0f);
    state.evaluate();
    float result = state.get (y); // 9

    // Many rows at once
    std::vector<float> xs (1000), ys (1000);
    state.bindColumn (x, xs.data());
    state.bindOutputColumn (y, ys.data());
    state.evaluateRows (1000);
    @endcode

    @see CompiledExpression, ExpressionContext

    @tags{Core}
*/
class YUP_API ExpressionState
{
public:
    //==============================================================================
    /** Creates a state for a program, able to evaluate up to blockSize rows at once. */
    explicit ExpressionState (const CompiledExpression& program, int blockSize = 64);

    /** Destructor. */
    ~ExpressionState();

    /** Returns the program evaluated by this state. */
    const CompiledExpression& getProgram() const noexcept { return program; }

    /** Returns the maximum number of rows evaluated at once. */
    int getBlockSize() const noexcept { return blockSize; }

    //==============================================================================
    /** Sets the value of an input for all the rows, removing any column bound to it.

        Inputs not used by the program are ignored, and setting the value an input already has does
        nothing, so nothing depending on it is computed again.
    */
    template <class T>
    void set (ExpressionInput<T> input, const T& value)
    {
        if (auto* lanes = prepareInput (input.index, typeid (T), &value))
            *static_cast<T*> (lanes) = value;
    }

    /** Binds the elements of an array input, which must stay valid while evaluating. */
    template <class T>
    void bindArray (ExpressionArray<T> input, const T* data, int size)
    {
        set (input, ExpressionArrayView<T> { data, jmax (0, size) });
    }

    /** Reads an input per row from memory: row r reads `*(firstRow + r * strideBytes)`.

        The memory must stay valid while evaluating. A stride different from sizeof (T) allows
        reading a field of an array of structures.
    */
    template <class T>
    void bindColumn (ExpressionInput<T> input, const T* firstRow, size_t strideBytes = sizeof (T))
    {
        bindColumn (input.index, typeid (T), firstRow, strideBytes);
    }

    /** Sets an `int` input to the index of the row being evaluated. */
    void bindRowIndex (ExpressionInput<int> input);

    /** Writes an output per row to memory: row r writes `*(firstRow + r * strideBytes)`. */
    template <class T>
    void bindOutputColumn (ExpressionOutput<T> output, T* firstRow, size_t strideBytes = sizeof (T))
    {
        bindOutputColumn (output.index, typeid (T), firstRow, strideBytes);
    }

    /** Removes all the column, row index and output column bindings. */
    void clearBindings();

    //==============================================================================
    /** Evaluates a single row, storing the outputs in lane 0. */
    void evaluate (int row = 0) noexcept;

    /** Evaluates up to getBlockSize() rows starting at startRow, returning the number of rows evaluated.

        Outputs of row (startRow + i) are available in lane i.
    */
    int evaluateBlock (int startRow, int numRows) noexcept;

    /** Evaluates rows 0 to numRows - 1 block by block, writing the outputs to the bound output columns. */
    void evaluateRows (int numRows) noexcept;

    //==============================================================================
    /** Returns the value of an output for a lane of the last evaluated block. */
    template <class T>
    const T& get (ExpressionOutput<T> output, int lane = 0) const noexcept
    {
        if (const auto* lanes = getOutputLanes (output.index, typeid (T), lane))
            return *static_cast<const T*> (lanes);

        static const T empty {};
        return empty;
    }

    /** Returns the values of an output for the rows of the last evaluated block, indexed by lane.

        Faster than calling get for every lane when processing a block. Returns an empty span for
        handles that don't belong to this program, or before the first evaluation.
    */
    template <class T>
    Span<const T> getLanes (ExpressionOutput<T> output) const noexcept
    {
        if (! output.isValid() || numEvaluatedRows == 0)
            return {};

        if (const auto* lanes = getOutputLanes (output.index, typeid (T), 0))
            return { static_cast<const T*> (lanes), (size_t) numEvaluatedRows };

        return {};
    }

    /** Returns an output of a built-in type as a var, or a void var for custom types. */
    var getVar (int outputIndex, int lane = 0) const;

    //==============================================================================
    /** Returns true if an output may have changed in the last evaluation.

        Outputs computed per row (from bound columns, the row index or functions that aren't pure)
        always report a change. Other outputs report a change only when their value differs from the
        one of the previous evaluation (see ExpressionType::areEqual), or on the first evaluation
        after creating the state or changing the bindings.

        @code
        state.set (hovered, isMouseOver());
        state.evaluate();

        if (state.hasChanged (fill))
            repaint();
        @endcode
    */
    template <class T>
    bool hasChanged (ExpressionOutput<T> output) const noexcept
    {
        return hasOutputChanged (output.index);
    }

    /** Returns true if the output at the given index may have changed in the last evaluation. */
    bool hasOutputChanged (int outputIndex) const noexcept;

    /** Returns true if any output may have changed in the last evaluation. */
    bool hasChanged() const noexcept;

    //==============================================================================
    /** Sets an input of a built-in type from a var. */
    Result setVar (int inputIndex, const var& value);

    /** Sets an input of a built-in type from a var, looked up by name. */
    Result setVar (StringRef inputName, const var& value);

private:
    //==============================================================================
    struct Column
    {
        int reg;
        const ExpressionType* type;
        uint8* data;
        size_t stride;
    };

    struct Call
    {
        ExpressionThunk thunk;
        const void* state;
        void* output;
        void* previousOutput;
        const ExpressionType* outputType;
        const void* const* args;
        const int* inputRegisters;
        int numInputs;
        int outputRegister;
        bool isPure;
    };

    void* prepareInput (int index, std::type_index type, const void* value);
    void bindColumn (int index, std::type_index type, const void* firstRow, size_t strideBytes);
    void bindOutputColumn (int index, std::type_index type, void* firstRow, size_t strideBytes);
    const void* getOutputLanes (int index, std::type_index type, int lane) const noexcept;
    int findInputRegister (int index, std::type_index type) const;
    bool unbindRegister (int reg);
    void updateSchedule() noexcept;
    void runUniformCalls() noexcept;
    bool runUniformCall (const Call& call) noexcept;
    int runBlock (int startRow, int numRows) noexcept;

    CompiledExpression program;
    int blockSize = 0;
    std::unique_ptr<uint8[]> storage;
    std::vector<uint8*> lanes;
    std::vector<uint8> isVarying, isDirty, needsBroadcast, isChanged;
    std::vector<const void*> arguments;
    std::vector<int> argumentRegisters;
    std::vector<Call> calls;
    std::vector<int> uniformCalls, varyingCalls, broadcastRegisters, volatileRegisters;
    std::vector<Column> inputColumns, outputColumns;
    int rowIndexRegister = -1, numEvaluatedRows = 0;
    bool scheduleChanged = true, uniformsChanged = true, forceUniformCalls = true;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExpressionState)
};

} // namespace yup
