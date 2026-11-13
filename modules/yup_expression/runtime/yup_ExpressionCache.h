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
/** Compiles expression programs once, returning the same CompiledExpression for the same text.

    Programs are compiled against a copy of the context taken on construction, so changes made to
    the original context afterwards don't affect the cache. Compile errors are cached too. A symbol
    resolver of the context must keep giving the same answers while the cache is used.

    Data-driven user interfaces often repeat the same expressions (for example `bounds.reduced (4)`)
    on many nodes: sharing the compiled programs avoids compiling them again on every load or reload.

    @code
    ExpressionContext context;
    auto bounds = context.addInput<Rectangle<float>> ("bounds");

    ExpressionCache cache (context);
    auto a = cache.compile ("bounds.reduced (4)");
    auto b = cache.compile ("bounds.reduced (4)"); // a.getValue() == b.getValue()

    cache.removeUnused(); // after a reload, drops the programs nobody uses anymore
    @endcode

    Compiling is thread safe.

    @see CompiledExpression, ExpressionContext

    @tags{Core}
*/
class YUP_API ExpressionCache
{
public:
    //==============================================================================
    /** Creates a cache compiling against a copy of the given context. */
    explicit ExpressionCache (const ExpressionContext& context);

    /** Destructor. */
    ~ExpressionCache();

    //==============================================================================
    /** Returns the program compiled from the text, compiling it the first time it is requested. */
    ResultValue<CompiledExpression> compile (StringRef text);

    /** Returns the context the programs are compiled against. */
    const ExpressionContext& getContext() const noexcept { return context; }

    //==============================================================================
    /** Returns the number of cached programs and errors. */
    int size() const;

    /** Removes the cached errors and the programs that are not used outside the cache, returning how many were removed. */
    int removeUnused();

    /** Removes everything from the cache. */
    void clear();

private:
    const ExpressionContext context;
    CriticalSection lock;
    std::unordered_map<String, ResultValue<CompiledExpression>> programs;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExpressionCache)
};

} // namespace yup
