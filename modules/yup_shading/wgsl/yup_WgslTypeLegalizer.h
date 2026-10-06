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

namespace wgsl
{

//==============================================================================
/**
    Makes the implicit conversions of GLSL explicit, as WGSL requires.

    Infers the type of every expression and rewrites the AST where GLSL converts
    implicitly and WGSL does not:
    - int, uint and float operands, initializers, assignments, return values and
      function arguments are converted to the type GLSL would promote them to
    - shift amounts are converted to unsigned
    - multi-argument constructor arguments are converted to the constructed type
    - scalar arguments of min, max, clamp, step and smoothstep are splatted to the
      vector type of the other arguments

    Abstract integer and float literals are left untouched, as WGSL converts them
    on its own. Expressions whose type can't be inferred are never rewritten.
*/
class WgslTypeLegalizer
{
public:
    WgslTypeLegalizer() = default;
    ~WgslTypeLegalizer() = default;

    //==========================================================================
    /**
        Rewrites the AST in place, inserting the conversions GLSL performs implicitly.

        @param ast  The translation unit to legalize.
    */
    static void legalize (TranslationUnit& ast);

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WgslTypeLegalizer)
};

} // namespace wgsl
} // namespace yup
