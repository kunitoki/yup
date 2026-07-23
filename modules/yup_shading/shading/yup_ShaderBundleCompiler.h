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
/**
    Describes one pipeline stage to compile and the target languages to transpile to.

    @see ShaderBundleCompileRequest, ShaderBundleCompiler
*/
struct ShaderBundleEntry
{
    /** Pipeline stage to compile. */
    ShaderStage stage = ShaderStage::vertex;

    /** Target shading languages to transpile the compiled SPIR-V into. */
    std::vector<ShaderLanguage> targetLanguages;

    /** Options applied when compiling and transpiling this stage. */
    TranspileOptions options;
};

//==============================================================================
/**
    Describes a full shader bundle compile request: source code, source language,
    and one entry per stage to compile.

    @see ShaderBundleCompiler
*/
struct ShaderBundleCompileRequest
{
    /** The shader source code to compile. */
    String source;

    /** Language of @c source (glsl, essl, or hlsl). */
    ShaderLanguage sourceLanguage = ShaderLanguage::glsl;

    /** One entry per pipeline stage to include in the bundle. */
    std::vector<ShaderBundleEntry> entries;
};

//==============================================================================
/**
    Compiles a shader program into a ShaderBundle.

    Uses a ShaderTranspiler to:
    -# Compile source code to SPIR-V for each requested stage.
    -# Decompile the SPIR-V to each requested target language.
    -# Extract ShaderReflection data for each (SPIR-V, target language) pair.
    -# Package everything into a ShaderBundle ready for persistence.

    @code
    ShaderBundleCompileRequest req;
    req.source = myGLSLSource;
    req.sourceLanguage = ShaderLanguage::glsl;

    ShaderBundleEntry entry;
    entry.stage = ShaderStage::vertex;
    entry.targetLanguages = { ShaderLanguage::msl, ShaderLanguage::hlsl };
    req.entries.push_back (entry);

    ShaderBundleCompiler compiler;
    auto result = compiler.compile (req);
    if (result)
        result.getValue().saveToFile (File ("myShader.ysl"));
    @endcode

    @see ShaderBundle, ShaderBundleCompileRequest, ShaderTranspiler
*/
class YUP_API ShaderBundleCompiler final
{
public:
    /** Constructs a compiler with an optional existing transpiler.
        If @c transpiler is null, a new ShaderTranspiler is created internally. */
    explicit ShaderBundleCompiler (ShaderTranspiler::Ptr transpiler = nullptr);
    ~ShaderBundleCompiler();

    //==========================================================================
    /**
        Compile a shader bundle from the given request.

        For each entry in the request:
        - Compiles the source to SPIR-V using the entry's TranspileOptions.
        - For each target language, decompiles the SPIR-V and extracts reflection.
        - Adds a ShaderInfo to the bundle for each (stage, target language) pair.

        @returns A ResultValue containing the compiled ShaderBundle on success,
                 or an error message on the first compilation failure.
    */
    ResultValue<ShaderBundle> compile (const ShaderBundleCompileRequest& request);

private:
    ShaderTranspiler::Ptr transpiler;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShaderBundleCompiler)
};

} // namespace yup
