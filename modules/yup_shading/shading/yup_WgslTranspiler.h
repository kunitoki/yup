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
/** Options for the GLSL→WGSL transpiler. */
struct WgslTranspileOptions
{
    /** WGSL entry-point name in the emitted output. Default is "main". */
    String entryPoint = "main";

    /** WGSL output entry-point function name. Default is "main".
        Callers can request "vs_main"/"fs_main"/"cs_main" for pipeline builders. */
    String outputEntryPoint;

    /** Default group (descriptor set) index for resources without explicit layout(set=...).
        Default is 0 (matches glslang's default). */
    uint32_t defaultGroup = 0;

    /** Default workgroup size for compute shaders without explicit local_size_x/y/z.
        Default is (1, 1, 1). */
    std::array<uint32_t, 3> defaultWorkgroupSize { 1, 1, 1 };
};

//==============================================================================
/**
    Transpiles preprocessed GLSL source into WGSL 1.0 source code.

    This is a direct transpiler that bypasses SPIR-V completely:
    - glslang preprocesses + validates
    - The GLSL AST parser converts the preprocessed source to an AST
    - AST lowering passes transform GLSL constructs to WGSL equivalents
    - The WGSL emitter outputs WGSL 1.0 text

    The transpiler is separated from ShaderTranspiler for testability:
    golden tests can run GlslParser → WgslLowering → WgslEmitter without
    glslang, testing the pure transpilation pipeline.

    Functional bindings (@group/@binding) are assigned to match glslang's
    SPIR-V assignment 1:1, so reflection via reflectFromSPIRV() can
    populate backendSlot values that match the emitted WGSL code.

    @code
    WgslTranspileOptions opts;
    opts.entryPoint = "main";
    opts.defaultGroup = 0;

    auto result = WgslTranspiler::transpile (preprocessedGlsl,
                                              ShaderStage::vertex,
                                              opts);
    if (result)
        DBG (result.getValue());
    @endcode

    @see ShaderTranspiler, WgslLowering, WgslEmitter
*/
class YUP_API WgslTranspiler final
{
public:
    WgslTranspiler() = default;
    ~WgslTranspiler() = default;

    //==========================================================================
    /**
        Transpile preprocessed GLSL to WGSL 1.0.

        The input @p preprocessedGlsl must already have preprocessor directives
        resolved (use glslang's TShader::preprocess() before calling this).

        @param preprocessedGlsl  GLSL source with all #define/#if/#include resolved.
        @param stage             The pipeline stage (vertex, fragment, or compute).
        @param options           Options controlling binding assignment, entry-point
                                 naming, and default workgroup size.

        @returns WGSL 1.0 source code on success, or an error message on failure.
    */
    static ResultValue<String> transpile (const String& preprocessedGlsl,
                                          ShaderStage stage,
                                          const WgslTranspileOptions& options = {});

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WgslTranspiler)
};

} // namespace yup
