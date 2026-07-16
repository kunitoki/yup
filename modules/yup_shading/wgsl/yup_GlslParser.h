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
    Parses preprocessed GLSL source into an AST.

    The input must already be preprocessed (no #define, #ifdef, #include, etc.).
    Use glslang's TShader::preprocess() to resolve the preprocessor before
    feeding source to this parser.

    Grammar: GLSL 4.50 core (substantially GLSL 4.60 as used by glslang).

    @see WgslTranspiler, TranslationUnit
*/
class GlslParser
{
public:
    GlslParser() = default;
    ~GlslParser() = default;

    //==========================================================================
    /**
        Parse preprocessed GLSL source into a TranslationUnit AST.

        @param source  Preprocessed GLSL source code (no preprocessor directives
                       beyond #extension/#pragma which are quietly discarded).
        @returns       A TranslationUnit on success, or an error string formatted
                       as "line:column: message".
    */
    static ResultValue<TranslationUnit> parse (const String& source);

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlslParser)
};

} // namespace wgsl
} // namespace yup
