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
/** Typed handle to an output of a CompiledExpression, used to read values from an ExpressionState. */
template <class T>
struct ExpressionOutput
{
    /** The index of the output in the program. */
    int index = -1;

    /** Returns true if the handle refers to an output. */
    bool isValid() const noexcept { return index >= 0; }
};

//==============================================================================
/** An expression program compiled against an ExpressionContext.

    Compiling type checks the program, resolves overloads and conversions, folds constants,
    removes duplicated and unused computations and lowers everything to a flat list of typed
    instructions. The result is immutable, cheap to copy and independent from the context it
    was compiled with, so it can be shared between threads: each thread evaluates it with its
    own ExpressionState.

    @code
    ExpressionContext context;
    auto x = context.addInput<float> ("x");

    auto program = CompiledExpression::compile ("y = x * 2; visible = x > 0.5", context);
    if (program.failed())
        return; // program.getErrorMessage() is like "1:5: Unknown symbol 'z'"

    auto y = program.getReference().getOutput<float> ("y");
    @endcode

    @see Expression, ExpressionContext, ExpressionState

    @tags{Core}
*/
class YUP_API CompiledExpression
{
public:
    //==============================================================================
    /** Creates an empty program without outputs. */
    CompiledExpression();

    /** Parses and compiles a program. */
    static ResultValue<CompiledExpression> compile (StringRef text, const ExpressionContext& context);

    /** Compiles a parsed program. */
    static ResultValue<CompiledExpression> compile (const Expression& expression, const ExpressionContext& context);

    //==============================================================================
    /** Returns the number of outputs, in the order they are assigned in the program. */
    int getNumOutputs() const noexcept;

    /** Returns the name of an output. */
    const String& getOutputName (int index) const;

    /** Returns the type of an output. */
    const ExpressionType* getOutputType (int index) const;

    /** Returns the index of the output with the given name, or -1. */
    int findOutput (StringRef name) const;

    /** Returns a typed handle to an output, failing if it doesn't exist or has a different type. */
    template <class T>
    ResultValue<ExpressionOutput<T>> getOutput (StringRef name) const
    {
        const auto index = findOutput (name);

        if (index < 0)
            return makeResultValueFail ("Unknown output '" + String (name) + "'");

        if (! getOutputType (index)->is<T>())
            return makeResultValueFail ("Output '" + String (name) + "' has type '" + getOutputType (index)->getName() + "'");

        return makeResultValueOk (ExpressionOutput<T> { index });
    }

    //==============================================================================
    /** Returns the number of inputs: the inputs of the context followed by those declared by its symbol resolver. */
    int getNumInputs() const noexcept;

    /** Returns the name of an input. */
    const String& getInputName (int index) const;

    /** Returns the type of an input. */
    const ExpressionType* getInputType (int index) const;

    /** Returns the index of the input with the given name, or -1. */
    int findInput (StringRef name) const;

    /** Returns a typed handle to an input, failing if it doesn't exist or has a different type.

        This is how inputs declared by an ExpressionContext::SymbolResolver are set on an ExpressionState.
    */
    template <class T>
    ResultValue<ExpressionInput<T>> getInput (StringRef name) const
    {
        const auto index = findInput (name);

        if (index < 0)
            return makeResultValueFail ("Unknown input '" + String (name) + "'");

        if (! getInputType (index)->is<T>())
            return makeResultValueFail ("Input '" + String (name) + "' has type '" + getInputType (index)->getName() + "'");

        return makeResultValueOk (ExpressionInput<T> { index });
    }

    /** Returns true if the program reads the input. */
    bool usesInput (int index) const;

    //==============================================================================
    /** Returns true if every output was folded to a constant. */
    bool isConstant() const noexcept;

    /** Returns true if every value is trivially copyable, so evaluating never allocates (as long as user functions don't). */
    bool isAllocationFree() const noexcept;

    /** Returns the number of instructions executed per evaluation. */
    int getNumInstructions() const noexcept;

    //==============================================================================
    /** Returns true if both refer to the same compiled program, as copies of one program do. */
    bool operator== (const CompiledExpression& other) const noexcept { return program == other.program; }

private:
    struct Program;
    friend struct ExpressionCompiler;
    friend class ExpressionState;
    friend class ExpressionCache;

    std::shared_ptr<const Program> program;
};

} // namespace yup
