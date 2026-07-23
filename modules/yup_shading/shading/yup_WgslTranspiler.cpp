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

namespace yup
{

//==============================================================================
ResultValue<String> WgslTranspiler::transpile (const String& preprocessedGlsl,
                                               ShaderStage stage,
                                               const WgslTranspileOptions& options)
{
    // Step 1: Parse GLSL → AST
    auto parseResult = wgsl::GlslParser::parse (preprocessedGlsl);

    if (parseResult.failed())
        return makeResultValueFail ("GLSL parse error: " + parseResult.getErrorMessage());

    // Step 2: Lower AST
    wgsl::WgslLoweringOptions loweringOpts;
    loweringOpts.stage = stage;
    loweringOpts.defaultGroup = options.defaultGroup;
    loweringOpts.defaultWorkgroupSize = options.defaultWorkgroupSize;

    auto lowerResult = wgsl::WgslLowering::lower (std::move (parseResult).getValue(), loweringOpts);

    if (lowerResult.failed())
        return makeResultValueFail ("WGSL lowering error: " + lowerResult.getErrorMessage());

    // Step 3: Emit WGSL
    wgsl::WgslEmitOptions emitOpts;
    emitOpts.outputEntryPoint = options.outputEntryPoint;

    auto emitResult = wgsl::WgslEmitter::emit (std::move (lowerResult).getValue(), emitOpts);

    if (emitResult.failed())
        return makeResultValueFail ("WGSL emission error: " + emitResult.getErrorMessage());

    return emitResult;
}

} // namespace yup
