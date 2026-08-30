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

} // namespace yup
