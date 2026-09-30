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
    auto parseResult = wgsl::GlslParser::parse (preprocessedGlsl);

    if (parseResult.failed())
        return makeResultValueFail ("GLSL parse error: " + parseResult.getErrorMessage());

    wgsl::WgslLoweringOptions loweringOpts;
    loweringOpts.stage = stage;
    loweringOpts.defaultGroup = options.defaultGroup;
    loweringOpts.defaultWorkgroupSize = options.defaultWorkgroupSize;
    loweringOpts.entryPointName = options.outputEntryPoint.isNotEmpty() ? options.outputEntryPoint.toStdString() : "main";

    auto lowerResult = wgsl::WgslLowering::lower (std::move (parseResult).getValue(), loweringOpts);

    if (lowerResult.failed())
        return makeResultValueFail ("WGSL lowering error: " + lowerResult.getErrorMessage());

    if (options.warnings != nullptr)
        for (const auto& warning : lowerResult.getReference().warnings)
            options.warnings->add (String (warning));

    auto emitResult = wgsl::WgslEmitter::emit (lowerResult.getReference());

    if (emitResult.failed())
        return makeResultValueFail ("WGSL emission error: " + emitResult.getErrorMessage());

    return emitResult;
}

} // namespace yup
