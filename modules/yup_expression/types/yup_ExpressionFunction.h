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
/** A typed native function callable from expressions.

    Functions are usually created by ExpressionContext::addFunction or by an ExpressionTypeBuilder,
    which deduce the parameter and result types from the C++ signature of the callable.

    Some names are reserved for the language:
    - `operator+`, `operator<`, ... implement binary operators, `operator-` with a single
      parameter implements negation and `operator!` the logical not.
    - `.name` implements a member read (`value.name`) or a method call (`value.name (args)`),
      the object is passed as first parameter.
    - `@convert` implements an implicit conversion from its parameter type to its result type.

    @tags{Core}
*/
struct YUP_API ExpressionFunction
{
    /** The name used to call the function. */
    String name;

    /** The parameter types, in order. */
    std::vector<const ExpressionType*> parameterTypes;

    /** The result type. */
    const ExpressionType* resultType = nullptr;

    /** The block routine evaluating the function. */
    ExpressionThunk thunk = nullptr;

    /** The state passed to the thunk, kept alive by every program using the function. */
    std::shared_ptr<const void> state;

    /** True if the result only depends on the arguments, allowing constant folding and deduplication. */
    bool isPure = true;

    /** Returns true if both functions have the same name, parameter types and result type. */
    bool hasSameSignature (const ExpressionFunction& other) const noexcept
    {
        return name == other.name && parameterTypes == other.parameterTypes && resultType == other.resultType;
    }

    /** Returns a readable signature, for example `lerp (float, float, float) -> float`. */
    String getSignature() const;
};

//==============================================================================
namespace detail
{

template <class F>
struct ExpressionCallableTraits : ExpressionCallableTraits<decltype (&F::operator())>
{
};

template <class R, class... A>
struct ExpressionCallableTraits<R (*) (A...)>
{
    using Signature = R (*) (A...);
};

template <class R, class... A>
struct ExpressionCallableTraits<R (*) (A...) noexcept> : ExpressionCallableTraits<R (*) (A...)>
{
};

template <class C, class R, class... A>
struct ExpressionCallableTraits<R (C::*) (A...)> : ExpressionCallableTraits<R (*) (A...)>
{
};

template <class C, class R, class... A>
struct ExpressionCallableTraits<R (C::*) (A...) const> : ExpressionCallableTraits<R (*) (A...)>
{
};

template <class C, class R, class... A>
struct ExpressionCallableTraits<R (C::*) (A...) noexcept> : ExpressionCallableTraits<R (*) (A...)>
{
};

template <class C, class R, class... A>
struct ExpressionCallableTraits<R (C::*) (A...) const noexcept> : ExpressionCallableTraits<R (*) (A...)>
{
};

template <class F, class R, class... A>
struct ExpressionInvoker
{
    static void thunk (const void* state, void* output, const void* const* args, int numRows)
    {
        invoke (state, output, args, numRows, std::index_sequence_for<A...>());
    }

    template <size_t... I>
    static void invoke (const void* state, void* output, const void* const* args, int numRows, std::index_sequence<I...>)
    {
        auto& function = *const_cast<F*> (static_cast<const F*> (state));
        auto* result = static_cast<R*> (output);

        for (int row = 0; row < numRows; ++row)
            result[row] = function (static_cast<const A*> (args[I])[row]...);
    }
};

template <class F, class R, class... A, class Resolver>
ExpressionFunction makeExpressionFunction (const String& name, F function, bool isPure, Resolver&& resolveType, R (*) (A...))
{
    static_assert (! std::is_void_v<R>, "Expression functions must return a value");
    static_assert (! std::is_member_pointer_v<F>, "Wrap member function pointers in a lambda");

    ExpressionFunction result;
    result.name = name;
    result.parameterTypes = { resolveType (std::type_identity<std::decay_t<A>>())... };
    result.resultType = resolveType (std::type_identity<std::decay_t<R>>());
    result.thunk = &ExpressionInvoker<F, std::decay_t<R>, std::decay_t<A>...>::thunk;
    result.state = std::make_shared<F> (std::move (function));
    result.isPure = isPure;
    return result;
}

/** Creates an ExpressionFunction from a callable, using resolveType (std::type_identity<T>) to map C++ types. */
template <class F, class Resolver>
ExpressionFunction makeExpressionFunction (const String& name, F&& function, bool isPure, Resolver&& resolveType)
{
    using Function = std::decay_t<F>;
    using Signature = typename ExpressionCallableTraits<Function>::Signature;

    return makeExpressionFunction<Function> (name, Function (std::forward<F> (function)), isPure, resolveType, Signature (nullptr));
}

} // namespace detail

} // namespace yup
